#include "m_pd.h"
#include "include/random.h"
#include "include/pd_open_array.c"

#define O_ERROR -1
#define MIN_SPEED 0.01
#define MAX_GRANULES 16384

#define W0(N) w[0][(N)].w_float
#define W1(N) w[1][(N)].w_float
#define W2(N) w[2][(N)].w_float

#define OUTLETF(S, F)					    \
  {							    \
    t_atom a[1];					    \
    SETFLOAT(a, (t_float)(F));				    \
    outlet_anything(x->out, gensym((S)), 1, a);		    \
  }

static t_class *n_stretch_class;

typedef struct _n_stretch
{
  t_object x_obj;
  t_outlet *out; /* outlet */
  t_symbol *s[3]; /* array's */
  int disp; /* dispalyed */
  int sharing_method; /* shared method granules */
  int min_grain; /* minimal size grain for detect */
  int seed; /* random seed */
  int g_start[MAX_GRANULES]; /* grain start */
  int g_len[MAX_GRANULES]; /* grain length */
  int g_all; /* all granules */
  int g_sharing[MAX_GRANULES];
  int g_do[MAX_GRANULES]; /* table with adding/remove/nothing todo granules */
  int g_add[MAX_GRANULES];
} t_n_stretch;

//----------------------------------------------------------------------------//
// timestretch algorithm
void n_stretch_doit(t_n_stretch *x, t_floatarg f0, t_floatarg f1, t_floatarg f2)
{
  int i,j,k,m,n;
  t_word *w[3];
  t_garray *g[3];
  int l[3];
  t_float speed;
  int start;
  int len;
  int end;
  int disp;
  int len_dst;
  int diff;
  int count;
  int g_st0;
  int g_len0;
  int g_st1;
  int g_len1;
  t_float f_c;
  t_float f_x;
  t_float p;
  t_float p0, p1;
  t_float b;
  t_float v0, v1;
  int len_mix;

  // open array source ---------------------------------- //
  l[0] = pd_open_array(x->s[0], &w[0], &g[0]);
  if (l[0] < 2)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      post("n_stretch: bad array source");
      return;
    }

  // validate start length ------------------------------ //
  start = f0;
  len = f1;
  if (start < 0)
    {
      start = l[0] + start;
      if (start < 0)
	{
	  start = 0;
	}
    }
  else if (start >= l[0])
    {
      start = l[0] - 1;
    }
 
 // end 0 ... la
  if (len <= 0)
    {
      end = l[0] + len;
      if (end < 0)
	{
	  end = 0;
	}
    }
  else
    {
      end = start + len;
    }

  if (end > l[0])
    {
      end = l[0];
    }

  // length
  len = end - start;
  if (len < 1)
    {
      len = 0;
    }

  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      post("n_stretch: length = 0");
      return;
    }

  // open array destination ----------------------------- //
  l[1] = pd_open_array(x->s[1], &w[1], &g[1]);
  if (l[1] < 1)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      post("n_stretch: bad array destination");
      return;
    }

  // open array disp and resize ------------------------- //
  if (x->disp)
    {
      l[2] = pd_open_array(x->s[2], &w[2], &g[2]);
      if (l[2] < 1)
	{
	  outlet_float(x->out, (t_float)O_ERROR);
	  post("n_stretch: bad array destination");
	  disp = 0;
	}
      // resize disp
      else
	{
	  garray_resize(g[2],l[0]);
	  l[2] = pd_open_array(x->s[2], &w[2], &g[2]);
	  disp = 1;
	}
    }
  else
    {
      disp = 0;
    }

  // speed ---------------------------------------------- //
  if (f2 < MIN_SPEED)
    f2 = MIN_SPEED;
  speed = f2;

  // detect grain's ------------------------------------- //
  x->g_all = 0;
  x->g_start[0] = start;
  j = 0;
  for(i = start; i < end-1; i++)
    {
      j++;
      // find cross
      if (W0(i) < 0 && W0(i+1) >= 0 && j >= x->min_grain)
	{
	  x->g_len[x->g_all] = j;
	  x->g_all++;
	  x->g_start[x->g_all] = i;
	  j = 0;
	  if (disp)
	    {
	      W2(i) = 1;
	    }
	}
      // last
      else if (i == end - 2)
	{
	  x->g_len[x->g_all] = j;
	  x->g_all++;
	}
      // nothing
      else
	{
	  if (disp)
	    {
	      W2(i) = 0;
	    }
	}
      // very large array
      if (x->g_all == MAX_GRANULES)
	{
	  post("n_stretch: very large array");
	  break;
	}
    }
  
  // disp ----------------------------------------------- //
  if (disp)
    {
      for(i=0; i<start; i++)
	{
	  W2(i) = 0;
	}
      W2(i) = 1;
      for(i=end; i<l[0]; i++)
	{
	  W2(i) = 0;
	}
      garray_redraw(g[2]);
    }

  // outlet --------------------------------------------- //
  OUTLETF("granules", x->g_all);

  // sharing -------------------------------------------- //
  // no sharing
  if (x->sharing_method == 0)
    {
      for (i=0; i<x->g_all; i++)
	{
	  x->g_sharing[i] = i;
	}
    }
  // random
  else if (x->sharing_method == 1)
    {
      for (i=0; i<x->g_all; i++)
	{
	  x->g_sharing[i] = -1;
	}
      i = 0;
      while (i < x->g_all)
        {
	  AF_RANDOM(x->seed);
	  j = x->seed % x->g_all;
	  if (j < 0)
	    j = 0 - j;
	  if (x->g_sharing[j] < 0)
            {
	      x->g_sharing[j] = i;
	      i++;
            }
        }
    }

  // length --------------------------------------------- //

  len_dst = len * (1. / speed);

  // outlet --------------------------------------------- //
  OUTLETF("orginal_length", len);
  OUTLETF("stretched_length", len_dst);

  // filling -------------------------------------------- //
  //  0        nothing
  // -1        delete granule
  //  1 ... n  add granule

  // clear
  for (i = 0; i < x->g_all; i++)
    {
      x->g_do[i] = 0;
      x->g_add[i] = 0;
    }

  // decrease length
  if (len_dst < len)
    {
      diff = len - len_dst;

      j = 0;
      for (i = 0; i < x->g_all; i++)
	{
	  k = x->g_sharing[i];
	  j += x->g_len[k];
	  if (j <= diff)
	    {
	      x->g_do[k] = -1;
	    }
	  else
	    {
	      j -= x->g_len[k];
	      len_dst = len - j;
	      break;
	    }
	}
    }
  
  // increase length
  else
    {
      diff = len_dst - len;
      j = 0;
      for (i = 0; ; i++)
	{
	  i = i % x->g_all;
	  k = x->g_sharing[i];
	  // if not last
	  if (k < x->g_all - 1)
	    {
	      m = (x->g_len[k] + x->g_len[k+1]) * (x->g_do[k] + 1) * 0.5;
	    }
	  else
	    {
	      m = x->g_len[k];
	    }

	  j -= x->g_add[k];
	  x->g_add[k] = m;
	  j += m;

	  if (j <= diff)
	    {
	      x->g_do[k]++;
	    }
	  else
	    {
	      j -= m;
	      len_dst = len + j;
	      break;
	    }
	}
    }

  /*
   *    100             200 = 0   = 0 = 0
   *    100     150     200 = 150 = 1 = 100 + 200 / 2
   *    100  133  166   200 = 200 = 2 = 100 + 200 + 100 + 200 / 2
   *    100 125 150 175 200 = 350 = 3 = 100 + 200 + 100 + 200 + 100 + 200 / 2
   */

  // outlet --------------------------------------------- //
  OUTLETF("dest_length", len_dst);

  // open array destination and resize ------------------ //
  garray_resize(g[1], len_dst);
  l[1] = pd_open_array(x->s[1], &w[1], &g[1]);
  if (l[1] != len_dst)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      post("n_stretch: array destination error resize");
      return;
    }

  // mix ------------------------------------------------ //

  // granules

  count = 0;
  for (i = 0; i < x->g_all; i++)
    {
      // copy current granule
      if (x->g_do[i] >= 0)
	{
	  for (j = 0; j < x->g_len[i]; j++)
	    {
	      W1(count) = W0(j + x->g_start[i]);
	      count++;
	      if (count >= l[1])
		count = l[1] - 1;
	    }
	}
      // mixfade 2 granules
      if (x->g_do[i] > 0)
	{
	  // last
	  if (i == x->g_all - 1)
	    {
	      g_st0  = g_st1  = x->g_start[i];
	      g_len0 = g_len1 = x->g_len[i];
	    }
	  // no last
	  else
	    {
	      j = i + 1;
	      g_st0 = x->g_start[i];
	      g_st1 = x->g_start[j];
	      g_len0 = x->g_len[i];
	      g_len1 = x->g_len[j];
	    }

	  // mix
	  f_c = 1. / ((t_float)x->g_do[i] + 1.);
	  for (j = 0; j < x->g_do[i]; j++)
	    {
	      f_x = (j + 1.) * f_c; /* coef xfade */
	      len_mix = (g_len1 - g_len0) * f_x + g_len0; /* len mixed granule */
	      for (k = 0; k < len_mix; k++)
		{
		  p = (t_float)k / (t_float)len_mix; /* 0 ... 1 */

		  p0 = (p * (t_float)g_len0) + (t_float)g_st0; /* pos 0 granule */
		  p1 = (p * (t_float)g_len1) + (t_float)g_st1; /* pos 1 granule */

		  // int fract
		  m = p0;
		  b = p0 - m;
		  n = m + 1;
		  // mix
		  v0 = (W0(n) - W0(m)) * b + W0(m); /* value 0 */

		  // int fract
		  m = p1;
		  b = p1 - m;
		  n = m + 1;
		  // mix
		  v1 = (W0(n) - W0(m)) * b + W0(m); /* value 1 */

		  W1(count) = (v1 - v0) * f_x + v0; /* mix between two granules */
		  count++; /* count inc */
		  if (count >= l[1])
		    count = l[1] - 1;
		}
	    }
	}
    }
  garray_redraw(g[1]);
}

//----------------------------------------------------------------------------//
void n_stretch_set_array(t_n_stretch *x, t_symbol *s0, t_symbol *s1, t_symbol *s2)
{
  x->s[0] = s0;
  x->s[1] = s1;
  x->s[2] = s2;
}

//----------------------------------------------------------------------------//
void n_stretch_disp(t_n_stretch *x, t_floatarg f)
{
  x->disp = f;
}

//----------------------------------------------------------------------------//
void n_stretch_sharing_method(t_n_stretch *x, t_floatarg f)
{
  x->sharing_method = f;
}

//----------------------------------------------------------------------------//
void n_stretch_min_grain(t_n_stretch *x, t_floatarg f)
{
  if (f < 1)
    f = 1;
  x->min_grain = f;
}

//----------------------------------------------------------------------------//
void n_stretch_seed(t_n_stretch *x, t_floatarg f)
{
  x->seed = f;
}

//----------------------------------------------------------------------------//
void n_stretch_dumpinfo(t_n_stretch *x)
{
  int i;
  for(i=0; i<x->g_all; i++)
    {
      post("[%7d]:[%7d][%7d]:[%7d]:[%7d]",
	   i,
	   x->g_start[i],
	   x->g_len[i],
	   x->g_sharing[i],
	   x->g_do[i]);
    }
}

//----------------------------------------------------------------------------//
static void *n_stretch_new(void)
{
  t_n_stretch *x = (t_n_stretch *)pd_new(n_stretch_class);
  x->out = outlet_new(&x->x_obj, 0);
  x->s[0] = gensym("");
  x->s[1] = gensym("");
  x->s[2] = gensym("");
  x->seed = (long)x;
  return (x);
}

//----------------------------------------------------------------------------//
void n_stretch_setup(void)
{
  n_stretch_class = class_new(gensym("n_stretch"), (t_newmethod)n_stretch_new, NULL, sizeof(t_n_stretch), 0, A_GIMME, 0);
  class_addmethod(n_stretch_class, (t_method)n_stretch_doit, gensym("doit"), A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
  class_addmethod(n_stretch_class, (t_method)n_stretch_set_array, gensym("set_array"), A_DEFSYMBOL, A_DEFSYMBOL, A_DEFSYMBOL, 0);
  class_addmethod(n_stretch_class, (t_method)n_stretch_disp, gensym("disp"), A_DEFFLOAT, 0);
  class_addmethod(n_stretch_class, (t_method)n_stretch_sharing_method, gensym("sharing_method"), A_DEFFLOAT, 0);
  class_addmethod(n_stretch_class, (t_method)n_stretch_min_grain, gensym("min_grain"), A_DEFFLOAT, 0);
  class_addmethod(n_stretch_class, (t_method)n_stretch_seed, gensym("seed"), A_DEFFLOAT, 0);
  class_addmethod(n_stretch_class, (t_method)n_stretch_dumpinfo, gensym("dumpinfo"), 0);
}
