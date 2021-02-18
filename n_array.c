#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "m_pd.h"
#include "include/constant.h"
#include "include/conversion.h"
#include "include/math.h"
#include "include/random.h"
#include "include/windowing.h"
#include "include/pd_open_array.c"

#define O_DONE 1
#define O_ERROR 0
#define NOT_FOUND -1
#define MAXA 16
#define MAX_FFT 16384

//----------------------------------------------------------------------------//
#define SWAP(BUF, E0, E1) \
  (BUF) = (E0);		  \
  (E0) = (E1);		  \
  (E1) = (BUF);

#define ARRAYS								\
  t_symbol *s[MAXA];							\
  t_word *w[MAXA];							\
  t_garray *g[MAXA];							\
  int l[MAXA];								\

// open array
#define USE_ARRAY(N)							\
  l[(N)] = pd_open_array(s[(N)], &w[(N)], &g[(N)]);			\
  if (l[(N)] <= 0)							\
    {									\
      error("n_array: open array: %s", s[(N)]->s_name);			\
      outlet_float(x->out, (t_float)O_ERROR);				\
      return;								\
    }

#define W(N,I) w[(N)][(I)].w_float

int debug = 0;

void mayer_init( void);
void mayer_term( void);

//----------------------------------------------------------------------------//
static t_class *n_array_class;

typedef struct _n_array
{
  t_object x_obj;
  t_outlet *out;
  int seed;
} t_n_array;
  

//----------------------------------------------------------------------------//
// internal
//----------------------------------------------------------------------------//
void n_array_validate_sl(int la, int *start, int *end, int *len)
{
  // start 0 ... la-1
  if (*start < 0)
    {
      *start = la + *start;
      if (*start < 0)
	{
	  *start = 0;
	}
    }
  else if (*start >= la)
    {
      *start = la - 1;
    }
 
 // end 0 ... la
  if (*len <= 0)
    {
      *end = la + *len;
      if (*end < 0)
	{
	  *end = 0;
	}
    }
  else
    {
      *end = *start + *len;
    }

  if (*end > la)
    {
      *end = la;
    }

  // length
  *len = *end - *start;
  if (*len < 1)
    {
      *len = 0;
    }
  if (debug)
    {
      post("start: %d, end: %d, len: %d", *start, *end, *len);
    }
}

//----------------------------------------------------------------------------//
void n_array_validate_s(int la, int *start)
{
  // start 0 ... la-1
  if (*start < 0)
    {
      *start = la + *start;
      if (*start < 0)
	{
	  *start = 0;
	}
    }
  else if (*start >= la)
    {
      *start = la - 1;
    }
  if (debug)
    {
      post("start: %d", start);
    }
}

//----------------------------------------------------------------------------//
void n_array_debug(t_n_array *x, t_floatarg f)
{
  debug = f;
  if (x) {};
}

//----------------------------------------------------------------------------//
// size operations
//----------------------------------------------------------------------------//
void n_array_size(t_n_array *x, t_symbol *s0)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);

  outlet_float(x->out, (t_float)l[0]);
}

//----------------------------------------------------------------------------//
void n_array_resize(t_n_array *x, t_symbol *s0, t_floatarg f)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);

  garray_resize(g[0], f);
  l[0] = pd_open_array(s[0], &w[0], &g[0]);
  outlet_float(x->out, (t_float)l[0]);
}

//----------------------------------------------------------------------------//
// list operations
//----------------------------------------------------------------------------//
void n_array_dump(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body
  int i,j;
  t_atom *a;
  a = (t_atom *)getbytes(sizeof(t_atom) * len);
  for (i = start, j = 0; i < end; i++, j++)
    {
      SETFLOAT(a + j, W(0,i));
    }
  outlet_list(x->out, &s_list, len, a);
  freebytes(a, sizeof(t_atom) * len);
}

//----------------------------------------------------------------------------//
void n_array_set(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  ARRAYS;
  s[0] = atom_getsymbolarg(0, ac, av);
  USE_ARRAY(0);

  ac--;
  av++;

  // body
  int i;
  garray_resize(g[0], ac);
  l[0] = pd_open_array(s[0], &w[0], &g[0]);
  i = 0;
  while (ac--)
    {
      W(0,i) = atom_getfloat(av++);
      i++;
    }
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)l[0]);
  if (sym) {}
}

//----------------------------------------------------------------------------//
// analysis operations
//----------------------------------------------------------------------------//
void n_array_minmax(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body
  int i;
  t_atom a[4];
  int min_idx = start;
  int max_idx = start;
  t_float min = W(0,start);
  t_float max = W(0,start);
  for (i = start+1; i < end; i++)
    {
      if (W(0,i) < min)
	{
	  min_idx = i;
	  min = W(0,i);
	}
      if (W(0,i) > max)
	{
	  max_idx = i;
	  max = W(0,i);
	}
    }
  SETFLOAT(a ,    (t_float)min_idx);
  SETFLOAT(a + 1, (t_float)min);
  SETFLOAT(a + 2, (t_float)max_idx);
  SETFLOAT(a + 3, (t_float)max);
  outlet_list(x->out, &s_list, 4, a);
}

//----------------------------------------------------------------------------//
void n_array_sum(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body
  int i;
  t_float sum = 0;
  for (i = start; i < end; i++)
    {
      sum += W(0,i);
    }
  outlet_float(x->out, sum);
}

//----------------------------------------------------------------------------//
void n_array_mean_arith(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body
  int i;
  t_float sum = 0;
  for (i = start; i < end; i++)
    {
      sum += W(0,i);
    }
  outlet_float(x->out, sum / len);
}

//----------------------------------------------------------------------------//
void n_array_mean_geo(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }


  // body
  int i;
  t_float sum = 1.;
  for (i = start; i < end; i++)
    {
      sum *= W(0,i);
    }

  if (sum < 0)
    {
      sum = 0 - sum;
    }
  sum = pow(sum, (1. / len));
  outlet_float(x->out, sum);
}

//----------------------------------------------------------------------------//
void n_array_mean_harm(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body
  int i;
  t_float sum = 0;
  for (i = start; i < end; i++)
    {
      sum += 1. / W(0,i);
    }
  outlet_float(x->out, len / sum);
}

//----------------------------------------------------------------------------//
void n_array_centroid(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body
  int i,j;
  t_float sum_mul = 0;
  t_float sum = 0;
  j = 0;
  for (i = start; i < end; i++)
    {
      sum_mul += W(0,i) * j;
      sum += W(0,i);
      j++;
    }
  if (sum == 0)
    {
      post("n_array: centoid: div zero");
      outlet_float(x->out, 0);
    }
  else
    {
      outlet_float(x->out, sum_mul / sum);
    }
}

//----------------------------------------------------------------------------//
void n_array_find(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1, t_floatarg val, t_floatarg tol)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body
  int i,j;
  t_atom a[2];
  j = NOT_FOUND;
  float min = val - tol;
  float max = val + tol;
  for (i = start; i < end; i++)
    {
      if (W(0,i) >= min && W(0,i) <= max)
	{
	  j = i;
	  i = end;
	}
    }
  SETFLOAT(a    , (t_float)j);
  SETFLOAT(a + 1, (t_float)W(0,j));
  outlet_list(x->out, &s_list, 2, a);
}

//----------------------------------------------------------------------------//
void n_array_find_cross(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1, t_floatarg val)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body
  int i,j;
  t_atom a[2];
  j = NOT_FOUND;
  for (i = start; i < end-1; i++)
    {
      if (W(0,i) < val && W(0,i+1) >= val)
	{
	  j = i;
	  i = end;
	}
    }

  if (j != NOT_FOUND)
    {
      SETFLOAT(a    , (t_float)j);
      SETFLOAT(a + 1, (t_float)W(0,j));
      outlet_list(x->out, &s_list, 2, a);
    }
  else
    {
      SETFLOAT(a    , (t_float)NOT_FOUND);
      SETFLOAT(a + 1, (t_float)NOT_FOUND);
      outlet_list(x->out, &s_list, 2, a);
    }
}

//----------------------------------------------------------------------------//
void n_array_find_periods(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1, t_floatarg val, t_floatarg min)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body
  int i,j;
  t_atom a[4];
  int st;
  int ln;
  t_float v;

  i = start;
  st = NOT_FOUND;
  ln = NOT_FOUND;
  j = 0;
  while (i < end-1)
    {
      if (W(0,i) < val && W(0,i+1) >= val)
	{
	  if (st == NOT_FOUND)
	    {
	      st = i;
	      v = W(0, i);
	    }
	  else if (ln == NOT_FOUND)
	    {
	      ln = i - st;
	      if (ln < min)
		{
		  ln = NOT_FOUND;
		}
	    }
	  else
	    {
	      SETFLOAT(a    , (t_float)j);
	      SETFLOAT(a + 1, (t_float)v);
	      SETFLOAT(a + 2, (t_float)st);
	      SETFLOAT(a + 3, (t_float)ln);
	      outlet_list(x->out, &s_list, 4, a);
	      st = NOT_FOUND;
	      ln = NOT_FOUND;
	      j++;
	    }
	}
      i++;
    }
  if (j == 0)
    {
      SETFLOAT(a    , (t_float)NOT_FOUND);
      SETFLOAT(a + 1, (t_float)NOT_FOUND);
      SETFLOAT(a + 2, (t_float)NOT_FOUND);
      SETFLOAT(a + 3, (t_float)NOT_FOUND);
      outlet_list(x->out, &s_list, 4, a);
    }
}

//----------------------------------------------------------------------------//
// various
//----------------------------------------------------------------------------//
void n_array_dc(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body
  int i;
  t_float min = W(0,start);
  t_float max = W(0,start);
  for (i = start+1; i < end; i++)
    {
      if (W(0,i) < min)
	{
	  min = W(0,i);
	}
      if (W(0,i) > max)
	{
	  max = W(0,i);
	}
    }
  t_float offset = ((max - min) * 0.5) - max;
  for (i = start; i < end; i++)
    {
      W(0,i) = W(0,i) + offset;
    } 
  garray_redraw(g[0]);
  outlet_float(x->out, offset);
}

//----------------------------------------------------------------------------//
void n_array_norm(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1, t_floatarg val)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body
  int i;
  t_float min = W(0,start);
  t_float max = W(0,start);
  for (i = start+1; i < end; i++)
    {
      if (W(0,i) < min)
	{
	  min = W(0,i);
	}
      if (W(0,i) > max)
	{
	  max = W(0,i);
	}
    }
  t_float diff = (max - min) * 0.5;
  if (diff == 0)
    {
      outlet_float(x->out, (t_float)0);
    }
  else
    {
      t_float mult = (1. / diff) * val;
      for (i = start; i < end; i++)
	{
	  W(0,i) = W(0,i) * mult;
	} 
      garray_redraw(g[0]);
      outlet_float(x->out, mult);
    }
}

//----------------------------------------------------------------------------//
void n_array_constant(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1, t_floatarg val)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body
  int i;
  for (i = start; i < end; i++)
    {
      W(0,i) = val;
    }
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)val);
}

//----------------------------------------------------------------------------//
void n_array_random(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1, t_floatarg val)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body
  int i;
  for (i = start; i < end; i++)
    {
      AF_RANDOM(x->seed);
      W(0,i) = x->seed * AC_RND_NORM * val;
    }
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)val);
}

//----------------------------------------------------------------------------//
void n_array_towt(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int end = f1;
  if (start < 0 || end < 0 || (start + end) == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }


  int i,j;
  t_float *buf_start = NULL;
  t_float *buf_end = NULL;

  // mem start
  if (start > 0)
    {
      buf_start = malloc(sizeof(t_float) * start);
      if (buf_start == NULL)
	{
	  error("n_array: allocating memory");
	  return;
	}
      // copy from end
      for (i=0; i<start; i++)
	{
	  j = l[0] - start + i;
	  j = j % l[0];
	  if (j < 0)      j = 0 - j;
	  buf_start[i] = W(0,j);
	}
    }

  // mem end
  if (end > 0)
    {
      buf_end = malloc(sizeof(t_float) * end);
      if (buf_end == NULL)
	{
	  error("n_array: allocating memory");
	  return;
	}
      // copy from start
      for (i=0; i<end; i++)
	{
	  j = i;
	  j = j % l[0];
	  buf_end[i] = W(0,j);
	}
    }

  // resize
  l[0] = l[0] + start + end;
  garray_resize(g[0], l[0]);
  l[0] = pd_open_array(s[0], &w[0], &g[0]);

  // shift 
  for (i = l[0]-1; i >= start; i--)
    {
      W(0,i) = W(0,i - start);
    }

  // copy start
  for (i = 0; i < start; i++)
    {
      W(0,i) = buf_start[i];
    }

  // copy end
  for (i = 0; i < end; i++)
    {
      W(0,l[0] - end + i) = buf_end[i];
    }

  if (buf_start != NULL) { free(buf_start);}
  if (buf_end != NULL)   { free(buf_end);  }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_fromwt(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int end = f1;
  if (start < 0 || end < 0 || (start + end) == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }


  int i;

  // shift 
  for (i = 0; i < (l[0] - start - end); i++)
    {
      W(0,i) = W(0,i + start);
    }

  // resize
  l[0] = l[0] - start - end;
  garray_resize(g[0], l[0]);
  l[0] = pd_open_array(s[0], &w[0], &g[0]);

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_blur(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1, t_floatarg val)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body
  int i;
  t_float p_s = W(0,start);
  t_float p_e = W(0,end -1);
  t_float inc = (p_e - p_s) / (len-1);
  t_float base = p_s;
  for (i = start; i < end-1; i++)
    {
      AF_INTERPOL_2P(W(0,i), base, val, W(0,i));
      base = base + inc;
    }
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_unique(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body 
  int i,j,k,find;
  // find duplicates
  j = start;
  for (i = start; i < end; i++)
    {
      find = 0;
      for(k = start; k<j; k++)
	{
	  if (W(0,i) == W(0,k))
	    {
	      find = 1;
	      k = j;
	    }
	}
      if (find == 0)
	{
	  W(0,j) = W(0,i);
	  j++;
	}
    }
  // remove
  find = end - j;

  // shift 
  for (i = end; i < l[0]; i++, j++)
    {
      W(0,j) = W(0,i);
    }

  // resize
  l[0] = l[0] - find;
  garray_resize(g[0], l[0]);

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)find);
}

//----------------------------------------------------------------------------//
void n_array_window(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1, t_symbol *type,t_floatarg coef)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  int i;
  t_float bf0, bf1, bf2;

  // type
  if ((strcmp("none",type->s_name) == 0) || type->s_name[0] == '\0')
    {
      AF_WINDOWING_NONE(start, end, w[0], .w_float, i); 
    }
  else if (strcmp("bartlett",type->s_name) == 0)
    {
      AF_WINDOWING_BARTLETT(start, len, w[0], .w_float, i, bf0, bf1, coef); 
    }
  else if (strcmp("blackman",type->s_name) == 0)
    {
      AF_WINDOWING_BLACKMAN(start, len, w[0], .w_float, i, bf0, bf1, bf2, coef); 
    }
  else if (strcmp("connes",type->s_name) == 0)
    {
      AF_WINDOWING_CONNES(start, len, w[0], .w_float, i, bf0, bf1, coef); 
    }
  else if (strcmp("gaussian",type->s_name) == 0)
    {
      AF_WINDOWING_GAUSSIAN(start, len, w[0], .w_float, i, bf0, bf1, coef); 
    }
  else if (strcmp("hanning",type->s_name) == 0)
    {
      AF_WINDOWING_HANNING(start, len, w[0], .w_float, i, bf0, bf1, coef); 
    }
  else if (strcmp("hamming",type->s_name) == 0)
    {
      AF_WINDOWING_HAMMING(start, len, w[0], .w_float, i, bf0, bf1, coef); 
    }
  else if (strcmp("lanczos",type->s_name) == 0)
    {
      AF_WINDOWING_LANCZOS(start, len, w[0], .w_float, i, bf0, bf1, coef); 
    }
  else if (strcmp("sin",type->s_name) == 0)
    {
      AF_WINDOWING_SIN(start, len, w[0], .w_float, i, bf0, coef); 
    }
  else if (strcmp("welch",type->s_name) == 0)
    {
      AF_WINDOWING_WELCH(start, len, w[0], .w_float, i, bf0, bf1, coef); 
    }
  else
    {
      post("n_array: unknown type");
      return;
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_sinesum(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  ARRAYS;
  s[0] = atom_getsymbolarg(0, ac, av);
  USE_ARRAY(0);
  int size = atom_getfloatarg(1, ac, av);
  int size_b = size + 3;
  int n = ac - 1;
  int i, j, k, m;
  t_float f, ph, sin_f;

  // buf
  float buf_a[n];
  float buf_sum[size];

  // body
  // copy from list
  for (i = 0; i < n; i++)
    {
      j = i + 2;
      buf_a[i] = atom_getfloatarg(j, ac, av);
    }
  
  // exit
  if (n == 0)
    return;
  
  // clear buf
  for (i = 0; i < size_b; i++)
    {
      buf_sum[i] = 0.;
    }
  
  // calc
  for (i = 0; i < n; i++)
    {
      k = i + 1;
      for (j = 0; j < size; j++)
	{
	  ph = (float)j / size; // 0 ... 1
	  ph *= k;
	  ph += 0.5; // sin
	  m = ph;	// wrap 0 ... 1
	  ph = ph - m;
	  ph = (ph + ph - 1.); // -1 ... 1
	  ph *= AC_PI;		 // -pi ... pi
	  sin_f = sin(ph);
	  f = sin_f * buf_a[i]; // * a
	  buf_sum[j] += f;
	}
    }
  
  // resize src
  garray_resize(g[0], size_b);
  pd_open_array(s[0], &w[0], &g[0]);
  
  // copy to array
  for (i = 1; i <= size; i++)
    {
      W(0,i) = buf_sum[i - 1];
    }
  W(0,0) = buf_sum[size - 1];
  W(0,size_b - 2) = buf_sum[0];
  W(0,size_b - 1) = buf_sum[1];
  
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);

  if (sym) {}
}

//----------------------------------------------------------------------------//
void n_array_interpolation(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  ARRAYS;
  s[0] = atom_getsymbolarg(0, ac, av);
  s[1] = atom_getsymbolarg(1, ac, av);
  USE_ARRAY(0);
  USE_ARRAY(1);

  int start_s = atom_getfloatarg(2, ac, av);
  int len_s =  atom_getfloatarg(3, ac, av);
  int end_s;
  n_array_validate_sl(l[0], &start_s, &end_s, &len_s);
  if (len_s == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  int start_d = atom_getfloatarg(4, ac, av);
  int len_d =  atom_getfloatarg(5, ac, av);
  int end_d;
  n_array_validate_sl(l[1], &start_d, &end_d, &len_d);
  if (len_d == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  t_symbol *type = atom_getsymbolarg(6, ac, av);

  // body

  int i, i0, i1, i2, i3;
  float f;
  float p0, p1, p2, p3, c1, c2, c3, c4;
  
  // no interpolation
  if ((strcmp("1p",type->s_name) == 0) || type->s_name[0] == '\0')
    {
      for (i = 0; i < len_d; i++)
	{
	  f = ((i / (float)l[1]) * len_s) + start_s;
	  i0 = f;
	  W(1,i + start_d) = W(0,i0);
	}
      garray_redraw(g[1]);
      outlet_float(x->out, (t_float)O_DONE);
    }
  
  // lin(2-p) interpolation
  else if (strcmp("2p",type->s_name) == 0)
    {
      for (i = 0; i < len_d; i++)
	{
	  f = ((i / (float)l[1]) * (len_s - 1)) + start_s;
	  i0 = f;
	  f = f - i0;
	  i1 = i0 + 1;
	  AF_INTERPOL_2P(W(0,i0), W(0,i1), f, W(1,i + start_d)); 
	}
      garray_redraw(g[1]);
      outlet_float(x->out, (t_float)O_DONE);
    }
  
  // (4-p) interpolation
  else if (strcmp("4p",type->s_name) == 0)
    {
      for (i = 0; i < len_d; i++)
	{
	  f = ((i / (float)l[1]) * (len_s - 1)) + start_s;
	  
	  i1 = f;
	  f = f - i1;
	  
	  i0 = i1 - 1;
	  if (i0 < 0)
	    i0 += l[0];
	  
	  i2 = i1 + 1;
	  if (i2 >= l[0])
	    i2 -= l[0];
	  
	  i3 = i1 + 2;
	  if (i3 >= l[0])
	    i3 -= l[0];
	  
	  p0 = W(0,i0);
	  p1 = W(0,i1);
	  p2 = W(0,i2);
	  p3 = W(0,i3);
	  
	  AF_INTERPOL_4P(p0, p1, p2, p3, f, c1, c2, c3, c4, W(1,i + start_d));
	}
      garray_redraw(g[1]);
      outlet_float(x->out, (t_float)O_DONE);
    }
  
  // exit
  else
    {
      post("n_array: unknown type");
      outlet_float(x->out, (t_float)O_ERROR);
    }

  if (sym) {}
}

//----------------------------------------------------------------------------//
// [0] [start] [len] [type] [1] [2] ... [n]
void n_array_sort(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  ARRAYS;
  int i;
  int start[MAXA];
  int len[MAXA];
  int end[MAXA];

  int arrays = ac - 3;
  AF_CLIP_MAX(16, arrays);

  // first array
  s[0] = atom_getsymbolarg(0, ac, av);
  USE_ARRAY(0);
  start[0] = atom_getfloatarg(1, ac, av);
  len[0] =  atom_getfloatarg(2, ac, av);
  int type = atom_getfloatarg(3, ac, av);
  n_array_validate_sl(l[0], &start[0], &end[0], &len[0]);
  if (len[0] == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // other arrays
  for (i=1; i<arrays; i++)
    {
      s[i] = atom_getsymbolarg(i + 3, ac, av);
      USE_ARRAY(i);
      start[i] = start[0]; 
      len[i] = len[0];
      n_array_validate_sl(l[i], &start[i], &end[i], &len[i]);
      if (len[i] == 0)
	{
	  outlet_float(x->out, (t_float)O_ERROR);
	  return;
	}
    }

  for (i=0; i<arrays; i++)
    {
      if (len[0] != len[i])
	{
	  post("n_array: sort: len_src != len[%d]",i);    
	  outlet_float(x->out, (t_float)O_ERROR);
	  return;
	}
    }


  
  // greaten first
  if (type == 0)
    {
      // sort
      int st = start[0];
      int en = end[0];
      int pos = st;
      int pos2;
      t_float buf;
      while(st < en)
	{
	  en--;
	  // down (small)
	  while(pos < en)
	    {
	      // compare
	      pos2 = pos + 1;
	      if (W(0,pos) < W(0,pos2))
		{
		  // swap
		  for (i=0; i<arrays; i++)
		    {
		      SWAP(buf, W(i,pos), W(i,pos2));
		    }
		}
	      pos++;
	    }
	  st++;
	  pos--;
	  // up (big)
	  while(pos >= st)
	    {
	      // compare
	      pos2 = pos - 1;
	      if (W(0,pos) > W(0,pos2))
		{
		  // swap
		  for (i=0; i<arrays; i++)
		    {
		      SWAP(buf, W(i,pos), W(i,pos2));
		    }
		}
	      pos--;
	    }
	}
    }
  else
    {
      // sort
      int st = start[0];
      int en = end[0];
      int pos = st;
      int pos2;
      t_float buf;
      while(st < en)
	{
	  en--;
	  // down (small)
	  while(pos < en)
	    {
	      // compare
	      pos2 = pos + 1;
	      if (W(0,pos) > W(0,pos2))
		{
		  // swap
		  for (i=0; i<arrays; i++)
		    {
		      SWAP(buf, W(i,pos), W(i,pos2));
		    }
		}
	      pos++;
	    }
	  st++;
	  pos--;
	  // up (big)
	  while(pos >= st)
	    {
	      // compare
	      pos2 = pos - 1;
	      if (W(0,pos) < W(0,pos2))
		{
		  // swap
		  for (i=0; i<arrays; i++)
		    {
		      SWAP(buf, W(i,pos), W(i,pos2));
		    }
		}
	      pos--;
	    }
	}
    }

  for (i=0; i<arrays; i++)
    {
      garray_redraw(g[i]);
    }

  outlet_float(x->out, (t_float)O_DONE);

  if (sym) {}
}

//----------------------------------------------------------------------------//
void n_array_count(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1, t_floatarg st, t_floatarg inc)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body 
  int i;
  t_float count = st;
  for (i = start; i < end; i++)
    {
      W(0,i) = count;
      count += inc;
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)W(0,i-1));
}

//----------------------------------------------------------------------------//
void n_array_todisp(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  ARRAYS;
  s[0] = atom_getsymbolarg(0, ac, av);
  s[1] = atom_getsymbolarg(1, ac, av);
  s[2] = atom_getsymbolarg(2, ac, av);
  USE_ARRAY(0);
  USE_ARRAY(1);
  USE_ARRAY(2);

  int start_s = atom_getfloatarg(3, ac, av);
  int len_s =  atom_getfloatarg(4, ac, av);
  int end_s;
  n_array_validate_sl(l[0], &start_s, &end_s, &len_s);
  if (len_s == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  int start_dmax = atom_getfloatarg(5, ac, av);
  int len_dmax =  atom_getfloatarg(6, ac, av);
  int end_dmax;
  n_array_validate_sl(l[1], &start_dmax, &end_dmax, &len_dmax);
  if (len_dmax == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  int start_dmin = start_dmax;
  int len_dmin = len_dmax;
  int end_dmin;
  n_array_validate_sl(l[2], &start_dmin, &end_dmin, &len_dmin);
  if (len_dmin == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  if (len_dmax != len_dmin)
    {
      post("n_array: todisp: len_max != len_min");    
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }
  
  // body
  // very fast and simple alghorhytm:)
  int i, j, x0, x1, k;
  t_float min, max;
  t_float count;
  t_float t_part;
  
  count = start_s;
  t_part = (t_float)len_s / (t_float)len_dmax;
  
  for (i = start_dmax; i < end_dmax; i++)
    {
      x0 = k = count;
      count += t_part;
      x1 = count - 1;
      if (k >= len_s)
	k -= len_s;
      // find min max
      max = min = W(0,k);
      for (j = x0 + 1; j < x1; j++)
	{
	  k = j;
	  if (k >= len_s)
	    k -= len_s;
	  if (max < W(0,k))
	    max = W(0,k);
	  if (min > W(0,k))
	    min = W(0,k);
	}
      W(1,i) = max;
      W(2,i) = min;
    }

  garray_redraw(g[1]);
  garray_redraw(g[2]);
  outlet_float(x->out, (t_float)O_DONE);

  if (sym) {}
}

//----------------------------------------------------------------------------//
void n_array_mix(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  ARRAYS;
  s[0] = atom_getsymbolarg(0, ac, av);
  s[1] = atom_getsymbolarg(1, ac, av);
  s[2] = atom_getsymbolarg(2, ac, av);
  s[3] = atom_getsymbolarg(3, ac, av);
  USE_ARRAY(0);
  USE_ARRAY(1);
  USE_ARRAY(2);
  USE_ARRAY(3);

  int start_s0 = atom_getfloatarg(4, ac, av);
  int len_s0 =  atom_getfloatarg(5, ac, av);
  int end_s0;
  n_array_validate_sl(l[0], &start_s0, &end_s0, &len_s0);
  if (len_s0 == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  int start_s1 = atom_getfloatarg(6, ac, av);
  int len_s1 =  len_s0;
  int end_s1;
  n_array_validate_sl(l[1], &start_s1, &end_s1, &len_s1);
  if (len_s1 == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  int start_m = atom_getfloatarg(7, ac, av);
  int len_m =  len_s0;
  int end_m;
  n_array_validate_sl(l[2], &start_m, &end_m, &len_m);
  if (len_m == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  int start_d = atom_getfloatarg(8, ac, av);
  int len_d =  len_s0;
  int end_d;
  n_array_validate_sl(l[3], &start_d, &end_d, &len_d);
  if (len_d == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  if (len_s1 != len_s0 ||
      len_m  != len_s0 ||
      len_d  != len_s0)
    {
      post("n_array: mix: length array");    
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }
  
  // body
  int i;
  for (i = 0; i < len_s0; i++)
    {
      AF_INTERPOL_2P(W(0,i+start_s0), W(1,i+start_s1), W(2,i+start_m), W(3,i+start_d));
    }

  garray_redraw(g[3]);
  outlet_float(x->out, (t_float)O_DONE);

  if (sym) {}
}

//----------------------------------------------------------------------------//
void n_array_mix_s(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  ARRAYS;
  s[0] = atom_getsymbolarg(0, ac, av);
  s[1] = atom_getsymbolarg(1, ac, av);
  s[2] = atom_getsymbolarg(2, ac, av);
  USE_ARRAY(0);
  USE_ARRAY(1);
  USE_ARRAY(2);

  int start_s0 = atom_getfloatarg(3, ac, av);
  int len_s0 =  atom_getfloatarg(4, ac, av);
  int end_s0;
  n_array_validate_sl(l[0], &start_s0, &end_s0, &len_s0);
  if (len_s0 == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  int start_s1 = atom_getfloatarg(5, ac, av);
  int len_s1 =  len_s0;
  int end_s1;
  n_array_validate_sl(l[1], &start_s1, &end_s1, &len_s1);
  if (len_s1 == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  int start_d = atom_getfloatarg(6, ac, av);
  int len_d =  len_s0;
  int end_d;
  n_array_validate_sl(l[2], &start_d, &end_d, &len_d);
  if (len_d == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  t_float mix =  atom_getfloatarg(7, ac, av);

  if (len_s1 != len_s0 ||
      len_d  != len_s0)
    {
      post("n_array: mix: length array");    
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }
  
  // body
  int i;
  for (i = 0; i < len_s0; i++)
    {
      AF_INTERPOL_2P(W(0,i+start_s0), W(1,i+start_s1), mix, W(2,i+start_d));
    }

  garray_redraw(g[2]);
  outlet_float(x->out, (t_float)O_DONE);

  if (sym) {}
}

//----------------------------------------------------------------------------//
// remove operations
//----------------------------------------------------------------------------//
void n_array_reverse(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body 
  int i, m, pos, pos2;
  float buf;
  
  m = len / 2;
  for (i = 0; i < m; i++)
    {
      pos = i + start;
      pos2 = end - i - 1;
      SWAP(buf, W(0,pos), W(0,pos2));
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_shift(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1, t_floatarg shift)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  int sh = shift;
  sh = sh % len;
  if (sh < 0)
    {
      sh = len + sh;
    }


  // body 
  int i, pos, pos2, m;
  int a_s = start;
  int a_l = sh;
  int b_s = start + sh;
  int b_l = len - sh;
  t_float buf;


  
  // reverse a
  if (a_l > 1)
    {
      m = a_l / 2;
      for (i = 0; i < m; i++)
	{
	  pos = i + a_s;
	  pos2 = a_l - i - 1 + a_s;
	  SWAP(buf, W(0,pos), W(0,pos2));
	}
    }
  
  // reverse b
  if (b_l > 1)
    {
      m = b_l /2;
      for (i = 0; i < m; i++)
	{
	  pos = i + b_s;
	  pos2 = b_l - i - 1 + b_s;
	  SWAP(buf, W(0,pos), W(0,pos2));
	}
    }
  
  // reverse ab
  for (i = 0; i < (len / 2); i++)
    {
      pos = i + a_s;
      pos2 = len - i - 1 + a_s;
      SWAP(buf, W(0,pos), W(0,pos2));
    }
  
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_delete(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body 
  int i;

  // copy
  for (i = end; i < l[0]; i++)
    {
      W(0,i-len) = W(0,i);
    }

  garray_resize(g[0],l[0]-len);
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_concat(t_n_array *x, t_symbol *s0, t_symbol *s1, t_floatarg f0, t_floatarg f1)
{
  ARRAYS;
  s[0] = s0;
  s[1] = s1;
  USE_ARRAY(0);
  USE_ARRAY(1);

  int start = f0;
  int len =  f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body 
  int i;

  garray_resize(g[1], l[1]+len);
  pd_open_array(s[1], &w[1], &g[1]);

  // copy
  for (i = 0; i < len; i++)
    {
      W(1,l[1] + i) = W(0,start + i);
    }

  garray_redraw(g[1]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_copy(t_n_array *x, t_symbol *s0, t_symbol *s1, t_floatarg f0, t_floatarg f1)
{
  ARRAYS;
  s[0] = s0;
  s[1] = s1;
  USE_ARRAY(0);
  USE_ARRAY(1);

  int start = f0;
  int len =  f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body 
  int i;

  garray_resize(g[1], len);
  pd_open_array(s[1], &w[1], &g[1]);

  // copy
  for (i = 0; i < len; i++)
    {
      W(1,i) = W(0,start + i);
    }

  garray_redraw(g[1]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_insert(t_n_array *x, t_symbol *s0, t_symbol *s1, t_floatarg f0, t_floatarg f1, t_floatarg f2)
{
  ARRAYS;
  s[0] = s0;
  s[1] = s1;
  USE_ARRAY(0);
  USE_ARRAY(1);

  int start_s = f0;
  int len_s =  f1;
  int end_s;
  n_array_validate_sl(l[0], &start_s, &end_s, &len_s);
  if (len_s == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  int start_d = f2;
  int len_d = l[1];
  int end_d;
  n_array_validate_sl(l[1], &start_d, &end_d, &len_d);

  // body 
  int i;

  garray_resize(g[1], l[1]+len_s);
  pd_open_array(s[1], &w[1], &g[1]);

  // shift
  for (i = l[1]-1; i >= start_d; i--)
    {
      W(1,i+len_s) = W(1,i);
    }

  // copy
  for (i = 0; i < len_s; i++)
    {
      W(1,start_d + i) = W(0,start_s + i);
    }


  garray_redraw(g[1]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
// clip
//----------------------------------------------------------------------------//
void n_array_clipmin(t_n_array *x, t_symbol *s0, t_symbol *s1, t_floatarg f0, t_floatarg f1, t_floatarg f2)
{
  ARRAYS;
  s[0] = s0;
  s[1] = s1;
  USE_ARRAY(0);
  USE_ARRAY(1);
  int start_s = f0;
  int len_s = f1;
  int end_s;
  n_array_validate_sl(l[0], &start_s, &end_s, &len_s);
  if (len_s == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  int start_min = f2;
  int len_min = f1;
  int end_min;
  n_array_validate_sl(l[1], &start_min, &end_min, &len_min);
  if (len_min == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  if (len_s != len_min )
    {
      post("n_array: clipmin: length");
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body 
  int i;
  for (i = 0; i < len_s; i++)
    {
      AF_CLIP_MIN(W(1, i+start_min), W(0, i+start_s));
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_clipmin_s(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1, t_floatarg min)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body 
  int i;
  for (i = start; i < end; i++)
    {
      AF_CLIP_MIN(min, W(0, i));
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_clipmax(t_n_array *x, t_symbol *s0, t_symbol *s1, t_floatarg f0, t_floatarg f1, t_floatarg f2)
{
  ARRAYS;
  s[0] = s0;
  s[1] = s1;
  USE_ARRAY(0);
  USE_ARRAY(1);
  int start_s = f0;
  int len_s = f1;
  int end_s;
  n_array_validate_sl(l[0], &start_s, &end_s, &len_s);
  if (len_s == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  int start_max = f2;
  int len_max = f1;
  int end_max;
  n_array_validate_sl(l[1], &start_max, &end_max, &len_max);
  if (len_max == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  if (len_s != len_max )
    {
      post("n_array: clipmax: length");
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body 
  int i;
  for (i = 0; i < len_s; i++)
    {
      AF_CLIP_MAX(W(1, i+start_max), W(0, i+start_s));
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_clipmax_s(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1, t_floatarg max)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body 
  int i;
  for (i = start; i < end; i++)
    {
      AF_CLIP_MAX(max, W(0, i));
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_clipminmax(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  ARRAYS;
  s[0] = atom_getsymbolarg(0, ac, av);
  s[1] = atom_getsymbolarg(1, ac, av);
  s[2] = atom_getsymbolarg(2, ac, av);
  USE_ARRAY(0);
  USE_ARRAY(1);
  USE_ARRAY(2);

  int start_s = atom_getfloatarg(3, ac, av);
  int len_s =  atom_getfloatarg(4, ac, av);
  int end_s;
  n_array_validate_sl(l[0], &start_s, &end_s, &len_s);
  if (len_s == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  int start_min = atom_getfloatarg(5, ac, av);
  int len_min =  len_s;
  int end_min;
  n_array_validate_sl(l[1], &start_min, &end_min, &len_min);
  if (len_min == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  int start_max = atom_getfloatarg(6, ac, av);
  int len_max =  len_s;
  int end_max;
  n_array_validate_sl(l[2], &start_max, &end_max, &len_max);
  if (len_max == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  if (len_min != len_s ||
      len_max  != len_s)
    {
      post("n_array: clipminmax: length");    
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }
  
  // body
  int i;
  for (i = 0; i < len_s; i++)
    {
      AF_CLIP_MINMAX(W(1, i+start_min), W(2, i+start_max), W(0, i+start_s));
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);

  if (sym) {}
}

//----------------------------------------------------------------------------//
void n_array_clipminmax_s(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1, t_floatarg min, t_floatarg max)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body 
  int i;
  for (i = start; i < end; i++)
    {
      AF_CLIP_MINMAX(min, max, W(0, i));
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
// arithmetic
//----------------------------------------------------------------------------//
void n_array_arithmetic(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  ARRAYS;
  s[0] = atom_getsymbolarg(0, ac, av);
  USE_ARRAY(0);
  int start = atom_getfloatarg(1, ac, av);
  int len = atom_getfloatarg(2, ac, av);
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body
  int i,buf_i;
  t_float buf_f;
  if (strcmp("abs",sym->s_name) == 0)
    {
      for (i = start; i < end; i++)
	{
	  AF_ABS(W(0,i), W(0,i));
	}
    }
  else if (strcmp("floor",sym->s_name) == 0)
    {
      for (i = start; i < end; i++)
	{
	  AF_FLOOR(W(0,i), buf_i);
	  W(0,i) = buf_i;
 	}
    }
  else if (strcmp("ceil",sym->s_name) == 0)
    {
      for (i = start; i < end; i++)
	{
	  AF_CEIL(W(0,i), buf_i);
	  W(0,i) = buf_i;
	}
    }
  else if (strcmp("wrap1",sym->s_name) == 0)
    {
      for (i = start; i < end; i++)
	{
	  AF_WRAP(W(0,i), buf_i, W(0,i));
	}
    }
  else if (strcmp("wrap2",sym->s_name) == 0)
    {
      for (i = start; i < end; i++)
	{
	  buf_f = W(0,i) * 0.5;
	  AF_WRAP(buf_f, buf_i, buf_f);
	  W(0,i) = buf_f + buf_f;
	}
    }
  else if (strcmp("sqrt",sym->s_name) == 0)
    {
      for (i = start; i < end; i++)
	{
	  W(0, i) = sqrt(W(0, i));
	}
    }
  else if (strcmp("log",sym->s_name) == 0)
    {
      for (i = start; i < end; i++)
	{
	  W(0, i) = log(W(0, i));
	}
    }
  else if (strcmp("log2",sym->s_name) == 0)
    {
      for (i = start; i < end; i++)
	{
	  W(0, i) = log2(W(0, i));
	}
    }
  else if (strcmp("log10",sym->s_name) == 0)
    {
      for (i = start; i < end; i++)
	{
	  W(0, i) = log10(W(0, i));
	}
    }
  else if (strcmp("exp",sym->s_name) == 0)
    {
      for (i = start; i < end; i++)
	{
	  W(0, i) = exp(W(0, i));
	}
    }
  else if (strcmp("exp2",sym->s_name) == 0)
    {
      for (i = start; i < end; i++)
	{
	  W(0, i) = exp2(W(0, i));
	}
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
// math
//----------------------------------------------------------------------------//
void n_array_math(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  ARRAYS;
  s[0] = atom_getsymbolarg(0, ac, av);
  s[1] = atom_getsymbolarg(1, ac, av);
  USE_ARRAY(0);
  USE_ARRAY(1);
  int start_s = atom_getfloatarg(2, ac, av);
  int len_s = atom_getfloatarg(3, ac, av);
  int end_s;
  n_array_validate_sl(l[0], &start_s, &end_s, &len_s);
  if (len_s == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  int start_a = atom_getfloatarg(4, ac, av);
  int len_a = len_s;
  int end_a;
  n_array_validate_sl(l[1], &start_a, &end_a, &len_a);
  if (len_a == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  if (len_s != len_a )
    {
      post("n_array: math: length");
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body 
  int i;
  int buf_i;
  int buf_s;
  if (strcmp("add",sym->s_name) == 0)
    {
      for (i = 0; i < len_s; i++)
	{
	  W(0, i+start_s) = W(0, i+start_s) + W(1, i+start_a);
	}
    }
  else if (strcmp("sub",sym->s_name) == 0)
    {
      for (i = 0; i < len_s; i++)
	{
	  W(0, i+start_s) = W(0, i+start_s) - W(1, i+start_a);
	}
    }
  else if (strcmp("mult",sym->s_name) == 0)
    {
      for (i = 0; i < len_s; i++)
	{
	  W(0, i+start_s) = W(0, i+start_s) * W(1, i+start_a);
	}
    }
  else if (strcmp("div",sym->s_name) == 0)
    {
      for (i = 0; i < len_s; i++)
	{
	  W(0, i+start_s) = W(0, i+start_s) / W(1, i+start_a);
	}
    }
  else if (strcmp("mod",sym->s_name) == 0)
    {
      for (i = 0; i < len_s; i++)
	{
	  buf_i = W(1, i+start_a);
	  if (buf_i < 0)  buf_i = 0 - buf_i;
	  if (buf_i == 0) buf_i = 1;
	  buf_s = W(0, i+start_s);
	  W(0, i+start_s) = buf_s % buf_i;
	}
    }
  else if (strcmp("pow",sym->s_name) == 0)
    {
      for (i = 0; i < len_s; i++)
	{
	  W(0, i+start_s) = pow(W(0, i+start_s), W(1, i+start_a));
	}
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_math_s(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  ARRAYS;
  s[0] = atom_getsymbolarg(0, ac, av);
  USE_ARRAY(0);
  int start = atom_getfloatarg(1, ac, av);
  int len = atom_getfloatarg(2, ac, av);
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  t_float val = atom_getfloatarg(3, ac, av);

  // body 
  int i;
  int buf_i;
  int buf_s;
  if (strcmp("add_s",sym->s_name) == 0)
    {
      for (i = start; i < end; i++)
	{
	  W(0, i) = W(0, i) + val;
	}
    }
  else if (strcmp("sub_s",sym->s_name) == 0)
    {
      for (i = start; i < end; i++)
	{
	  W(0, i) = W(0, i) - val;
	}
    }
  else if (strcmp("mult_s",sym->s_name) == 0)
    {
      for (i = start; i < end; i++)
	{
	  W(0, i) = W(0, i) * val;
	}
    }
  else if (strcmp("div_s",sym->s_name) == 0)
    {
      if (val == 0) return; /* divzero ??? */
      for (i = start; i < end; i++)
	{
	  W(0, i) = W(0, i) / val;
	}
    }
  else if (strcmp("mod_s",sym->s_name) == 0)
    {
      buf_i = val;
      if (buf_i < 0)  buf_i = 0 - buf_i;
      if (buf_i == 0) buf_i = 1;
      for (i = start; i < end; i++)
	{
	  buf_s = W(0, i);
	  W(0, i) = buf_s % buf_i;
	}
    }
  else if (strcmp("pow",sym->s_name) == 0)
    {
      for (i = start; i < end; i++)
	{
	  W(0, i) = pow(W(0, i), val);
	}
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
// trigonometry
//----------------------------------------------------------------------------//
void n_array_trigonometry(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  ARRAYS;
  s[0] = atom_getsymbolarg(0, ac, av);
  USE_ARRAY(0);
  int start = atom_getfloatarg(1, ac, av);
  int len = atom_getfloatarg(2, ac, av);
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body
  int i;
  if (strcmp("sin",sym->s_name) == 0)
    {
      for (i = start; i < end; i++)
	{
	  W(0, i) = sin(W(0, i));
	}
    }
  else if (strcmp("asin",sym->s_name) == 0)
    {
      for (i = start; i < end; i++)
	{
	  W(0, i) = asin(W(0, i));
	}
    }
  else if (strcmp("sinh",sym->s_name) == 0)
    {
      for (i = start; i < end; i++)
	{
	  W(0, i) = sinh(W(0, i));
	}
    }
  else if (strcmp("asinh",sym->s_name) == 0)
    {
      for (i = start; i < end; i++)
	{
	  W(0, i) = asinh(W(0, i));
	}
    }
  else if (strcmp("cos",sym->s_name) == 0)
    {
      for (i = start; i < end; i++)
	{
	  W(0, i) = cos(W(0, i));
	}
    }
  else if (strcmp("acos",sym->s_name) == 0)
    {
      for (i = start; i < end; i++)
	{
	  W(0, i) = acos(W(0, i));
	}
    }
  else if (strcmp("cosh",sym->s_name) == 0)
    {
      for (i = start; i < end; i++)
	{
	  W(0, i) = cosh(W(0, i));
	}
    }
  else if (strcmp("acosh",sym->s_name) == 0)
    {
      for (i = start; i < end; i++)
	{
	  W(0, i) = acosh(W(0, i));
	}
    }
  else if (strcmp("tan",sym->s_name) == 0)
    {
      for (i = start; i < end; i++)
	{
	  W(0, i) = tan(W(0, i));
	}
    }
  else if (strcmp("atan",sym->s_name) == 0)
    {
      for (i = start; i < end; i++)
	{
	  W(0, i) = atan(W(0, i));
	}
    }
  else if (strcmp("tanh",sym->s_name) == 0)
    {
      for (i = start; i < end; i++)
	{
	  W(0, i) = tanh(W(0, i));
	}
    }
  else if (strcmp("atanh",sym->s_name) == 0)
    {
      for (i = start; i < end; i++)
	{
	  W(0, i) = atanh(W(0, i));
	}
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_atan2(t_n_array *x, t_symbol *s0, t_symbol *s1, t_floatarg f0, t_floatarg f1, t_floatarg f2)
{
  ARRAYS;
  s[0] = s0;
  s[1] = s1;
  USE_ARRAY(0);
  USE_ARRAY(1);
  int start_s = f0;
  int len_s = f1;
  int end_s;
  n_array_validate_sl(l[0], &start_s, &end_s, &len_s);
  if (len_s == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  int start_a = f2;
  int len_a = f1;
  int end_a;
  n_array_validate_sl(l[1], &start_a, &end_a, &len_a);
  if (len_a == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  if (len_s != len_a )
    {
      post("n_array: atan2: length");
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body 
  int i;
  for (i = 0; i < len_s; i++)
    {
      W(0, i+start_s) = atan2(W(0, i+start_s), W(1, i+start_a));
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_atan2_s(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1, t_floatarg p)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body 
  int i;
  for (i = start; i < end; i++)
    {
      W(0, i) = atan2(W(0, i), p);
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
// comparison
//----------------------------------------------------------------------------//
void n_array_comparison(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  ARRAYS;
  s[0] = atom_getsymbolarg(0, ac, av);
  s[1] = atom_getsymbolarg(1, ac, av);
  s[2] = atom_getsymbolarg(2, ac, av);
  USE_ARRAY(0);
  USE_ARRAY(1);
  USE_ARRAY(2);

  int start_0 = atom_getfloatarg(3, ac, av);
  int len_0 =  atom_getfloatarg(4, ac, av);
  int end_0;
  n_array_validate_sl(l[0], &start_0, &end_0, &len_0);
  if (len_0 == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  int start_1 = atom_getfloatarg(5, ac, av);
  int len_1 =  len_0;
  int end_1;
  n_array_validate_sl(l[1], &start_1, &end_1, &len_1);
  if (len_1 == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  int start_d = atom_getfloatarg(6, ac, av);
  int len_d =  len_0;
  int end_d;
  n_array_validate_sl(l[2], &start_d, &end_d, &len_d);
  if (len_d == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  if ((len_1 != len_0) || (len_d != len_0))
    {
      post("n_array: comparison: length");    
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }
  
  // body
  int i;
  if (strcmp("eq",sym->s_name) == 0)
    {
      for (i = 0; i < len_0; i++)
	{
	  W(2, i + start_d) = (W(0, i + start_0) == (W(1, i + start_1)));
	}
    }
  else if (strcmp("ne",sym->s_name) == 0)
    {
      for (i = 0; i < len_0; i++)
	{
	  W(2, i + start_d) = (W(0, i + start_0) != (W(1, i + start_1)));
	}
    }
  else if (strcmp("gt",sym->s_name) == 0)
    {
      for (i = 0; i < len_0; i++)
	{
	  W(2, i + start_d) = (W(0, i + start_0) > (W(1, i + start_1)));
	}
    }
  else if (strcmp("ge",sym->s_name) == 0)
    {
      for (i = 0; i < len_0; i++)
	{
	  W(2, i + start_d) = (W(0, i + start_0) >= (W(1, i + start_1)));
	}
    }
  else if (strcmp("lt",sym->s_name) == 0)
    {
      for (i = 0; i < len_0; i++)
	{
	  W(2, i + start_d) = (W(0, i + start_0) < (W(1, i + start_1)));
	}
    }
  else if (strcmp("le",sym->s_name) == 0)
    {
      for (i = 0; i < len_0; i++)
	{
	  W(2, i + start_d) = (W(0, i + start_0) <= (W(1, i + start_1)));
	}
    }

  garray_redraw(g[2]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_comparison_s(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  ARRAYS;
  s[0] = atom_getsymbolarg(0, ac, av);
  USE_ARRAY(0);
  int start = atom_getfloatarg(1, ac, av);
  int len = atom_getfloatarg(2, ac, av);
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  t_float val = atom_getfloatarg(3, ac, av);

  // body
  int i;
  if (strcmp("eq_s",sym->s_name) == 0)
    {
      for (i = start; i < end; i++)
	{
	  W(0, i) = (W(0, i) == val);
	}
    }
  else if (strcmp("ne_s",sym->s_name) == 0)
    {
      for (i = start; i < end; i++)
	{
	  W(0, i) = (W(0, i) != val);
	}
    }
  else if (strcmp("gt_s",sym->s_name) == 0)
    {
      for (i = start; i < end; i++)
	{
	  W(0, i) = (W(0, i) > val);
	}
    }
  else if (strcmp("ge_s",sym->s_name) == 0)
    {
      for (i = start; i < end; i++)
	{
	  W(0, i) = (W(0, i) >= val);
	}
    }
  else if (strcmp("lt_s",sym->s_name) == 0)
    {
      for (i = start; i < end; i++)
	{
	  W(0, i) = (W(0, i) < val);
	}
    }
  else if (strcmp("le_s",sym->s_name) == 0)
    {
      for (i = start; i < end; i++)
	{
	  W(0, i) = (W(0, i) <= val);
	}
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
// conversion
//----------------------------------------------------------------------------//
void n_array_m2f(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body 
  int i;
  t_float buf_f;
  for (i = start; i < end; i++)
    {
      buf_f = W(0,i);
      AF_CLIP_MINMAX(-1500., 1499., buf_f);
      AF_M2F(buf_f, buf_f);
      W(0,i) = buf_f;
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_f2m(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body 
  int i;
  t_float buf_f;
  for (i = start; i < end; i++)
    {
      buf_f = W(0,i);
      AF_CLIP_MIN(0., buf_f);
      AF_F2M(buf_f, buf_f);
      W(0,i) = buf_f;
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_db2pow(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body 
  int i;
  t_float buf_f;
  for (i = start; i < end; i++)
    {
      buf_f = W(0,i);
      AF_CLIP_MINMAX(0., 870., buf_f);
      AF_DB2POW(buf_f, buf_f);
      W(0,i) = buf_f;
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_pow2db(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body 
  int i;
  t_float buf_f;
  for (i = start; i < end; i++)
    {
      buf_f = W(0,i);
      AF_CLIP_MIN(0., buf_f);
      AF_POW2DB(buf_f, buf_f);
      W(0,i) = buf_f;
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_db2rms(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body 
  int i;
  t_float buf_f;
  for (i = start; i < end; i++)
    {
      buf_f = W(0,i);
      AF_CLIP_MINMAX(0., 485., buf_f);
      AF_DB2RMS(buf_f, buf_f);
      W(0,i) = buf_f;
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_rms2db(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body 
  int i;
  t_float buf_f;
  for (i = start; i < end; i++)
    {
      buf_f = W(0,i);
      AF_CLIP_MIN(0., buf_f);
      AF_RMS2DB(buf_f, buf_f);
      W(0,i) = buf_f;
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_logt2sec(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body 
  int i;
  t_float buf_f;
  for (i = start; i < end; i++)
    {
      buf_f = W(0,i);
      AF_LOGT2SEC(buf_f, buf_f);
      W(0,i) = buf_f;
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
// fft
//----------------------------------------------------------------------------//
void n_array_fft(t_n_array *x, t_symbol *s0, t_symbol *s1, t_symbol *s2)
{
  ARRAYS;
  s[0] = s0;
  s[1] = s1;
  s[2] = s2;
  USE_ARRAY(0);
  USE_ARRAY(1);
  USE_ARRAY(2);

  if (!(l[1] == 4  ||
      l[1] == 8    ||
      l[1] == 16   ||
      l[1] == 32   ||
      l[1] == 64   ||
      l[1] == 128  ||
      l[1] == 256  ||
      l[1] == 512  ||
      l[1] == 1024 ||
      l[1] == 2048 ||
      l[1] == 4096 ||
      l[1] == 8192 ||
      l[1] == 16384))
    {
      post("n_array: fft: length");
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  if (l[1] != l[0] || l[2] != l[0])
    {
      post("n_array: fft: length r i");
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }


  // body
  int i;

  t_float buf_r[MAX_FFT];
  t_float buf_i[MAX_FFT];

  // copy
  for (i = 0; i < l[0]; i++)
    {
      buf_r[i] = buf_i[i] = W(0,i);
    }

  // fft
  mayer_fft(l[0], buf_r, buf_i);

  // copy
  for (i = 0; i < l[0]; i++)
    {
      W(1, i) = buf_r[i];
      W(2, i) = buf_i[i];
    }

  garray_redraw(g[1]);
  garray_redraw(g[2]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_ifft(t_n_array *x, t_symbol *s0, t_symbol *s1, t_symbol *s2)
{
  ARRAYS;
  s[0] = s0;
  s[1] = s1;
  s[2] = s2;
  USE_ARRAY(0);
  USE_ARRAY(1);
  USE_ARRAY(2);

  if (!(l[1] == 4  ||
      l[1] == 8    ||
      l[1] == 16   ||
      l[1] == 32   ||
      l[1] == 64   ||
      l[1] == 128  ||
      l[1] == 256  ||
      l[1] == 512  ||
      l[1] == 1024 ||
      l[1] == 2048 ||
      l[1] == 4096 ||
      l[1] == 8192 ||
      l[1] == 16384))
    {
      post("n_array: ifft: length");
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  if (l[0] != l[1] || l[2] != l[1])
    {
      post("n_array: ifft: length dst i");
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }


  // body
  int i;

  t_float buf_r[MAX_FFT];
  t_float buf_i[MAX_FFT];

  // copy
  for (i = 0; i < l[1]; i++)
    {
      buf_r[i] = W(1,i);
      buf_i[i] = W(2,i);
    }

  // fft
  mayer_ifft(l[1], buf_r, buf_i);

  // copy
  t_float f = 1. / l[1];
  for (i = 0; i < l[1]; i++)
    {
      W(0, i) = (buf_r[i] * f);
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_vec2pol(t_n_array *x, t_symbol *s0, t_symbol *s1, t_symbol *s2, t_symbol *s3)
{
  ARRAYS;
  s[0] = s0;
  s[1] = s1;
  s[2] = s2;
  s[3] = s3;
  USE_ARRAY(0);
  USE_ARRAY(1);
  USE_ARRAY(2);
  USE_ARRAY(3);

  if (!(l[0] == 4  ||
      l[0] == 8    ||
      l[0] == 16   ||
      l[0] == 32   ||
      l[0] == 64   ||
      l[0] == 128  ||
      l[0] == 256  ||
      l[0] == 512  ||
      l[0] == 1024 ||
      l[0] == 2048 ||
      l[0] == 4096 ||
      l[0] == 8192 ||
      l[0] == 16384))
    {
      post("n_array: vec2pol: length");
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  if (l[1] != l[0] || l[2] != l[0] || l[3] != l[0])
    {
      post("n_array: vec2pol: length r a ph");
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }


  // body
  int i;


  t_float bsc = 1. / ((t_float)l[0] / 2.);
  t_float re, im, a, b;
  t_float div_im_re;
  t_float ab;
  t_float f;
  for (i = 0; i < l[0]; i++)
    {
      re = W(0, i) * bsc;
      im = W(1, i) * bsc;
      // a
      a = re * re;
      b = im * im;
      a = a + b + 1e-12;
      W(2, i) = sqrt(a);

      // FAtan2

      // safe
      if (re > 0)
	{
	  div_im_re = im / (re + 1e-12);
	}
      else
	{
	  div_im_re = im / (re - 1e-12);
	}
      
      // abs
      AF_ABS(div_im_re, ab);

      if (ab < 1)
	{
	  f = (div_im_re * div_im_re * 0.28) + 1.;
	  f = div_im_re / f;
	  if (re < 0)
	    {
	      if (im > 0)
		{
		  f += AC_PI;
		}
	      else
		{
		  f -= AC_PI;
		}
	    }
	}
      else
	{
	  f = div_im_re / ((div_im_re * div_im_re) + 0.28);
	  f = 1.5708 - f;
	  if (im < 0)
	    {
	      f -= AC_PI;
	    }
	}
      W(3, i) = f;
    }

  garray_redraw(g[2]);
  garray_redraw(g[3]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_pol2vec(t_n_array *x, t_symbol *s0, t_symbol *s1, t_symbol *s2, t_symbol *s3)
{
  ARRAYS;
  s[0] = s0;
  s[1] = s1;
  s[2] = s2;
  s[3] = s3;
  USE_ARRAY(0);
  USE_ARRAY(1);
  USE_ARRAY(2);
  USE_ARRAY(3);

  if (!(l[0] == 4  ||
      l[0] == 8    ||
      l[0] == 16   ||
      l[0] == 32   ||
      l[0] == 64   ||
      l[0] == 128  ||
      l[0] == 256  ||
      l[0] == 512  ||
      l[0] == 1024 ||
      l[0] == 2048 ||
      l[0] == 4096 ||
      l[0] == 8192 ||
      l[0] == 16384))
    {
      post("n_array: pol2vec: length");
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  if (l[1] != l[0] || l[2] != l[0] || l[3] != l[0])
    {
      post("n_array: pol2vec: length ph r i");
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }


  // body
  int i;


  t_float a, ph;
  t_float cos_f;
  t_float sin_f;
  t_float f;
  t_float m = l[0] * 0.5;
  for (i = 0; i < l[0]; i++)
    {
      a = W(0,i); /* amp */
      ph = W(1,i); /* -pi ... pi */
      
      f = ph * ph;
      // cos -pi ... pi
      cos_f = (((((((((f * -2.605e-7) + 2.47609e-5) * f) - 0.00138884) * f) + 0.0416666) * f) - 0.499923) * f) + 1.;
      // sin -pi ... pi
      sin_f = ((((((((((f * -2.39E-008) + 2.7526E-006) * f) - 0.000198409) * f) + 0.00833333) * f) - 0.166667) * f) + 1.) * ph;
      
      W(2,i) = a * cos_f * m;
      W(3,i) = a * sin_f * m;
    }

  garray_redraw(g[2]);
  garray_redraw(g[3]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
// curves
//----------------------------------------------------------------------------//
void n_array_c_breakpoint(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1, t_floatarg f2, t_floatarg f3)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // clip
  AF_CLIP_MINMAX(0., 1., f2);
  AF_CLIP_MINMAX(0., 1., f3);

  // body 
  int i, j;
  t_float f,e;
  t_float h = f2 + 1e-12;
  t_float m = (1 - f3) / (1 - h);
  t_float n = f3 / h;
  // cycle
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (t_float)len;
      if (f > f2)
	{
	  e = f3 + (m * (f - h));
	}
      else
	{
	  e = n * f; 
	}
      W(0, i) = e;
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_c_breakpoint_smooth(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1, t_floatarg f2, t_floatarg f3)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // clip
  AF_CLIP_MINMAX(0., 1., f2);
  AF_CLIP_MINMAX(0., 1., f3);

  // body 
  int i, j;
  t_float f, e;
  t_float m = f2 + 1e-12;
  t_float n = f3 + 1e-12;
  t_float a = f2 / n;
  t_float b = 1. - f2;
  t_float c = 1. - f3 + 1e-12;
  t_float d = b / c;
  // cycle
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (t_float)len;
      if (f < f2)
	{
	  e = f / m;
	  e = pow(e, a);
	  e = e * f3;
	}
      else
	{
	  e = f - 1.;
	  e = e * -1.;
	  e = e / b;
	  e = pow(e, d);
	  e = e * c;
	  e = e * -1.;
	  e = e + 1.;
	}
      W(0, i) = e;
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_c_circular(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1, t_floatarg f2)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // clip
  AF_CLIP_MINMAX(0., 1., f2);

  // body 
  int i, j;
  t_float f, e;
  t_float a = pow(f2, 2.);
  t_float b = pow(1. - f2, 2.);
  // cycle
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (t_float)len;
      if (f < f2)
	{
	  e = a - pow(f - f2, 2.);
	  e = sqrt(e);
	}
      else
	{
	  e = b - pow(f - f2, 2.);
	  e = 1. - sqrt(e);
	}
      W(0, i) = e;
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_c_circular_sigmoid(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1, t_floatarg f2)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // clip
  AF_CLIP_MINMAX(0., 1., f2);

  // body 
  int i, j;
  t_float f, e;
  t_float a = f2 * f2;
  t_float b = pow(1. - f2, 2.);
  // cycle
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (t_float)len;
      if (f < f2)
	{
	  e = f2 - sqrt(a - (f*f));
	}
      else
	{
	  e = b - pow(f - 1., 2.);
	  e = f2 + sqrt(e);
	}
      W(0, i) = e;
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_c_cubic(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1, t_floatarg f2, t_floatarg f3)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // clip
  AF_CLIP_MINMAX(0., 1., f2);
  AF_CLIP_MINMAX(0., 1., f3);

  // body 
  int i, j;
  t_float f, e;
  t_float m = f2 + 1e-6;
  t_float a = 1. - m;
  t_float b = 1. - f3;
  // cycle
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (t_float)len;
      if (f < f2)
	{
	  e = 1. - (f / m);
	  e = pow(e, 3.);
	  e = f3 - (f3 * e);
	}
      else
	{
	  e = (f - m) / a;
	  e = pow(e, 3.);
	  e = f3 + (b * e);
	}
      W(0, i) = e;
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_c_exp(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body 
  int i, j;
  t_float f, e;
  // cycle
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (t_float)len;
      e = f * 100.;
      AF_DB2RMS(e, e);
      W(0, i) = e;
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_c_log(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // body 
  int i, j;
  t_float f, e;
  t_float a = 2 * log(10);
  // cycle
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (t_float)len;
      if (f > 0.01)
	{
	  e = log(f) + a;
	  e = e / a;
	  W(0, i) = e;
	}
      else
	{
	  W(0, i) = f;
	}
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_c_elleptic(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1, t_floatarg f2, t_floatarg f3)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // clip
  AF_CLIP_MINMAX(0., 1., f2);
  AF_CLIP_MINMAX(0., 1., f3);

  // body 
  int i, j;
  t_float f, e;
  t_float m = f2 + 1e-12;
  t_float n = f3 + 1e-12;
  t_float a = 1. - m;
  t_float b = 1. - f3 + 1e-12;
  t_float c = a / b;
  t_float d = m / n;
  // cycle
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (t_float)len;
      if (f < f2)
	{
	  e = f / m;
	  e = pow(e, d);
	  e = e * n;
	}
      else
	{
	  e = f - 1;
	  e = e * -1;
	  e = e / a;
	  e = pow(e, c);
	  e = e * b;
	  e = e * -1;
	  e = e + 1;
	}
      W(0, i) = e;
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_c_elleptic_seat(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1, t_floatarg f2, t_floatarg f3)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // clip
  AF_CLIP_MINMAX(0., 1., f2);
  AF_CLIP_MINMAX(0., 1., f3);

  // body 
  int i, j;
  t_float f, e;
  t_float m = f2 + 1e-12;
  t_float n = f3 + 1e-12;
  t_float a = n / m;
  t_float b = pow(m, 2);
  t_float c = (1 - n) / (1 - m);
  t_float d = pow(1- m, 2);
  // cycle
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (t_float)len;
      if (f < f2)
	{
	  e = a * sqrt(b - pow(f - m, 2));
	}
      else
	{
	  e = 1 - (c * sqrt(d - pow(f - m, 2)));
	}
      W(0, i) = e;
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_c_elleptic_sigmoid(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1, t_floatarg f2, t_floatarg f3)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // clip
  AF_CLIP_MINMAX(0., 1., f2);
  AF_CLIP_MINMAX(0., 1., f3);

  // body 
  int i, j;
  t_float f, e;
  t_float m = f2 + 1e-12;
  t_float n = f3 + 1e-12;
  t_float a = pow(m, 2);
  t_float b = (1 - n) / (1 - m);
  t_float c = pow(1 - m, 2);
  // cycle
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (t_float)len;
      if (f < f2)
	{
	  e = sqrt(a - pow(f, 2));
	  e = n * (1 - (e / m));
	}
      else
	{
	  e = sqrt(c - pow(f - 1, 2));
	  e = n + (b * e);
	}
      W(0, i) = e;
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_c_exponential(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1, t_floatarg f2)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // clip
  AF_CLIP_MINMAX(0., 1., f2);
  f2 = 0.01 + (f2 * 0.98);

  // body 
  int i, j;
  t_float f, e;
  t_float a = f2 * 2;
  t_float b = 1 / (1 - ((f2 - 0.5) * 2));
  // cycle
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (t_float)len;
      if (f2 < 0.5)
	{
	  e = pow(f,a);
	}
      else
	{
	  e = pow(f,b);
	}
      W(0, i) = e;
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_c_exponential_seat(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1, t_floatarg f2)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // clip
  AF_CLIP_MINMAX(0., 1., f2);

  // body 
  int i, j;
  t_float f, e;
  t_float a = 1 - f2;
  // cycle
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (t_float)len;
      if (f < 0.5)
	{
	  e = pow(f * 2,a) / 2;
	}
      else
	{
	  e = 1 - (pow(2 * (1 - f), a) / 2);
	}
      W(0, i) = e;
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_c_exponential_sigmoid(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1, t_floatarg f2)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // clip
  AF_CLIP_MINMAX(0., 1., f2);
  f2 += 1e-6;

  // body 
  int i, j;
  t_float f, e;
  t_float a = 1 / f2;
  // cycle
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (t_float)len;
      if (f < 0.5)
	{
	  e = pow(f * 2,a) / 2;
	}
      else
	{
	  e = 1 - (pow(2 * (1 - f), a) / 2);
	}
      W(0, i) = e;
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_c_logistic_sigmoid(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1, t_floatarg f2)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // clip
  AF_CLIP_MINMAX(0., 1., f2);
  f2 = 0.001 + (f2 * 0.998);
  f2 = (1 / (1 - f2)) - 1;

  // body 
  int i, j;
  t_float f, e;
  t_float a = 1 / (1 + exp(f2));
  t_float b = 1 / (1 + exp(0 - f2));
  // cycle
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (t_float)len;
      e = 1 / (1 + exp(0 -((f - 0.5) * f2 * 2)));
      e = (e - a) / (b - a);
      W(0, i) = e;
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_c_quadratic(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1, t_floatarg f2, t_floatarg f3)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // clip
  AF_CLIP_MINMAX(0., 1., f2);
  AF_CLIP_MINMAX(0., 1., f3);

  if (f2 == 0.5)
    {
      f2 = 0.50001;
    }

  // body 
  int i, j;
  t_float f, e;
  t_float a = 1 - (2 * f2);
  t_float b = f2 * f2;
  t_float c = 2 * f3;
  t_float d = 1 - (2 * f3);
  // cycle
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (t_float)len;
      e = (sqrt(b + (f * a)) - f2) / a;
      e = (d * e * e) + (c * e);
      W(0, i) = e;
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_c_quartic(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1, t_floatarg f2, t_floatarg f3)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // clip
  AF_CLIP_MINMAX(0., 1., f2);
  AF_CLIP_MINMAX(0., 1., f3);

  f2 = 1 - f2;
 
  // body 
  int i, j;
  t_float f, e;
  t_float a = 1 - (2 * f2);
  t_float b = 2 * f2;
  t_float c = 1 - (2 * f3);
  t_float d = 2 * f3;
  // cycle
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (t_float)len;
      e = (a * pow(f, 2)) + (b * f);
      e = (c * pow(e, 2)) + (d * e);
      W(0, i) = e;
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_c_simplified_cubic_seat(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1, t_floatarg f2, t_floatarg f3)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // clip
  AF_CLIP_MINMAX(0., 1., f2);
  AF_CLIP_MINMAX(0., 1., f3);

  f2 += 1e-6;
  f3 = 1 - f3;

  // body 
  int i, j;
  t_float f, e;
  t_float a = (1 - f3) * f2;
  t_float b = 1 - f2;
  t_float c = 1 - f3;
  // cycle
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (t_float)len;
      if (f < f2)
	{
	  e = (f * f3) + (a * (1 - pow(1 - (f / f2), 3)));
	}
      else
	{
	  e = (f * f3) + (c * (f2  + (b * pow((f - f2) / b, 3))));
	}
      W(0, i) = e;
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_c_simplified_quadratic(t_n_array *x, t_symbol *s0, t_floatarg f0, t_floatarg f1, t_floatarg f2)
{
  ARRAYS;
  s[0] = s0;
  USE_ARRAY(0);
  int start = f0;
  int len = f1;
  int end;
  n_array_validate_sl(l[0], &start, &end, &len);
  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }

  // clip
  AF_CLIP_MINMAX(0., 1., f2);

  f2 = f2 + f2;

  // body 
  int i, j;
  t_float f, e;
  t_float a = f2;
  t_float b = f2;

  AF_CLIP_MAX(1., a);
  AF_CLIP_MIN(1., b);

  if (a == 0.5)
    {
      a = 0.50001;
    }

  b = b - 1;

  t_float c = 1 - (2 * a);
  t_float d = a * a;
  t_float m = 2 * b;
  // cycle
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (t_float)len;
      e = (sqrt(d + (f * c)) - a) / c;
      e = (1 - m) * (e * e) + (m * e);
      W(0, i) = e;
    }

  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
// setup
//----------------------------------------------------------------------------//
void *n_array_new(void)
{
  t_n_array *x = (t_n_array *)pd_new(n_array_class);
  x->out = outlet_new(&x->x_obj, 0);
  x->seed = (long)x;
  return (void *)x;
}

//----------------------------------------------------------------------------//
void n_array_setup(void)
{
   n_array_class = class_new(gensym("n_array"), (t_newmethod)n_array_new, 0, sizeof(t_n_array), 0, A_GIMME, 0);
   /*  */
   class_addmethod(n_array_class, (t_method)n_array_debug, gensym("debug"), A_DEFFLOAT, 0);
   /* size */
   class_addmethod(n_array_class, (t_method)n_array_size, gensym("size"), A_SYMBOL, 0);
   class_addmethod(n_array_class, (t_method)n_array_resize, gensym("resize"), A_SYMBOL, A_DEFFLOAT, 0);
   /* list */
   class_addmethod(n_array_class, (t_method)n_array_dump, gensym("dump"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_set, gensym("set"), A_GIMME, 0);
   /* analysis */
   class_addmethod(n_array_class, (t_method)n_array_minmax, gensym("minmax"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_sum, gensym("sum"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_mean_arith, gensym("mean_arith"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_mean_geo, gensym("mean_geo"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_mean_harm, gensym("mean_harm"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_centroid, gensym("centroid"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_find, gensym("find"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_find_cross, gensym("find_cross"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_find_periods, gensym("find_periods"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
   /* various */
   class_addmethod(n_array_class, (t_method)n_array_dc, gensym("dc"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_norm, gensym("norm"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_constant, gensym("constant"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_random, gensym("random"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_towt, gensym("towt"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_fromwt, gensym("fromwt"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_blur, gensym("blur"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_unique, gensym("unique"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_window, gensym("window"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_DEFSYMBOL, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_sinesum, gensym("sinesum"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_interpolation, gensym("interpolation"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_sort, gensym("sort"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_count, gensym("count"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_todisp, gensym("todisp"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_mix, gensym("mix"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_mix_s, gensym("mix_s"), A_GIMME, 0);
   /* remove */
   class_addmethod(n_array_class, (t_method)n_array_reverse, gensym("reverse"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_shift, gensym("shift"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_delete, gensym("delete"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_concat, gensym("concat"), A_SYMBOL, A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_copy, gensym("copy"), A_SYMBOL, A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_insert, gensym("insert"), A_SYMBOL, A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
   /* clip */
   class_addmethod(n_array_class, (t_method)n_array_clipmin, gensym("clipmin"), A_SYMBOL, A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_clipmin_s, gensym("clipmin_s"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_clipmax, gensym("clipmax"), A_SYMBOL, A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_clipmax_s, gensym("clipmax_s"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_clipminmax, gensym("clipminmax"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_clipminmax_s, gensym("clipminmax_s"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
   /* arithmetic */
   class_addmethod(n_array_class, (t_method)n_array_arithmetic, gensym("abs"),A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_arithmetic, gensym("floor"),A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_arithmetic, gensym("ceil"),A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_arithmetic, gensym("wrap1"),A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_arithmetic, gensym("wrap2"),A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_arithmetic, gensym("sqrt"),A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_arithmetic, gensym("log"),A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_arithmetic, gensym("log2"),A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_arithmetic, gensym("log10"),A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_arithmetic, gensym("exp"),A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_arithmetic, gensym("exp2"),A_GIMME, 0);
   /* math */
   class_addmethod(n_array_class, (t_method)n_array_math, gensym("add"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_math, gensym("sub"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_math, gensym("mult"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_math, gensym("div"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_math, gensym("mod"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_math, gensym("pow"), A_GIMME, 0);
   /* math scalar */
   class_addmethod(n_array_class, (t_method)n_array_math_s, gensym("add_s"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_math_s, gensym("sub_s"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_math_s, gensym("mult_s"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_math_s, gensym("div_s"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_math_s, gensym("mod_s"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_math_s, gensym("pow_s"), A_GIMME, 0);
   /* trigonometry */
   class_addmethod(n_array_class, (t_method)n_array_trigonometry, gensym("sin"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_trigonometry, gensym("sinh"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_trigonometry, gensym("asin"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_trigonometry, gensym("asinh"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_trigonometry, gensym("cos"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_trigonometry, gensym("cosh"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_trigonometry, gensym("acos"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_trigonometry, gensym("acosh"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_trigonometry, gensym("tan"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_trigonometry, gensym("tanh"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_trigonometry, gensym("atan"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_trigonometry, gensym("atanh"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_atan2, gensym("atan2"), A_SYMBOL, A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_atan2_s, gensym("atan2_s"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
   /* comparison */
   class_addmethod(n_array_class, (t_method)n_array_comparison, gensym("eq"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_comparison, gensym("ne"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_comparison, gensym("ge"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_comparison, gensym("gt"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_comparison, gensym("le"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_comparison, gensym("lt"), A_GIMME, 0);
   /* comparison scalar */
   class_addmethod(n_array_class, (t_method)n_array_comparison_s, gensym("eq_s"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_comparison_s, gensym("ne_s"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_comparison_s, gensym("ge_s"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_comparison_s, gensym("gt_s"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_comparison_s, gensym("le_s"), A_GIMME, 0);
   class_addmethod(n_array_class, (t_method)n_array_comparison_s, gensym("lt_s"), A_GIMME, 0);
   /* conversion */
   class_addmethod(n_array_class, (t_method)n_array_m2f, gensym("m2f"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_f2m, gensym("f2m"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_db2pow, gensym("db2pow"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_pow2db, gensym("pow2db"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_db2rms, gensym("db2rms"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_rms2db, gensym("rms2db"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_logt2sec, gensym("logt2sec"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, 0);
   /* fft */
   class_addmethod(n_array_class, (t_method)n_array_fft, gensym("fft"), A_SYMBOL, A_SYMBOL, A_SYMBOL, 0);
   class_addmethod(n_array_class, (t_method)n_array_ifft, gensym("ifft"), A_SYMBOL, A_SYMBOL, A_SYMBOL, 0);
   class_addmethod(n_array_class, (t_method)n_array_vec2pol, gensym("vec2pol"), A_SYMBOL, A_SYMBOL, A_SYMBOL, A_SYMBOL, 0);
   class_addmethod(n_array_class, (t_method)n_array_pol2vec, gensym("pol2vec"), A_SYMBOL, A_SYMBOL, A_SYMBOL, A_SYMBOL, 0);
   /* curves */
   class_addmethod(n_array_class, (t_method)n_array_c_breakpoint, gensym("c_breakpoint"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_c_breakpoint_smooth, gensym("c_breakpoint_smooth"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_c_circular, gensym("c_circular"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_c_circular_sigmoid, gensym("c_circular_sigmoid"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_c_cubic, gensym("c_cubic"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_c_exp, gensym("c_exp"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_c_log, gensym("c_log"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_c_elleptic, gensym("c_elleptic"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_c_elleptic_seat, gensym("c_elleptic_seat"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_c_elleptic_sigmoid, gensym("c_elleptic_sigmoid"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_c_exponential, gensym("c_exponential"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_c_exponential_seat, gensym("c_exponential_seat"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_c_exponential_sigmoid, gensym("c_exponential_sigmoid"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_c_logistic_sigmoid, gensym("c_logistic_sigmoid"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_c_quadratic, gensym("c_quadratic"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_c_quartic, gensym("c_quartic"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_c_simplified_cubic_seat, gensym("c_simplified_cubic_seat"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
   class_addmethod(n_array_class, (t_method)n_array_c_simplified_quadratic, gensym("c_simplified_quadratic"), A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
   /* */
   mayer_init();
}

