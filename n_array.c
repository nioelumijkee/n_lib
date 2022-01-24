#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "m_pd.h"
#include "include/constant.h"
#include "include/conversion.h"
#include "include/math.h"
#include "include/clip.h"
#include "include/swap.h"
#include "include/pd_open_array.c"
#include "include/array_pd.h"
#include "include/array_validate.h"
#include "include/array_analisys.h"
#include "include/array_various.h"
#include "include/array_moving.h"
#include "include/array_windowing.h"
#include "include/sinesum.h"
#include "include/array_interpolation.h"
#include "include/array_todisp.h"
#include "include/array_mix.h"
#include "include/array_clip.h"
#include "include/array_vec.h"
#include "include/array_curve.h"

#define O_DONE 1
#define O_ERROR 0
#define NOT_FOUND -1
#define MAXA 16
#define MAX_FFT 16384
#define DIS(x) if(x) {};

//----------------------------------------------------------------------------//
// declare variables
#define N_ARRAYS(N)                                   \
  t_symbol *s[N];                                     \
  t_word   *w[N];                                     \
  t_garray *g[N];                                     \
  int       l[N];                                     \
  int   start[N];                                     \
  int     end[N];                                     \
  int     len[N];

//----------------------------------------------------------------------------//
// get arguments for array in atom
// open array
// validate start and length
#define N_USE_ARRAY(N, NAME, START, LEN)                            \
  s[N] = atom_getsymbolarg(NAME, ac, av);                           \
                                                                    \
                                                                    \
  if (START == -1)                                                  \
    {                                                               \
      start[N] = 0;                                                 \
    }                                                               \
  else                                                              \
    {                                                               \
      start[N] = atom_getfloatarg(START, ac, av);                   \
    }                                                               \
                                                                    \
                                                                    \
  if (LEN == -1)                                                    \
    {                                                               \
      len[N] = 0;                                                   \
    }                                                               \
  else                                                              \
    {                                                               \
      len[N] = atom_getfloatarg(LEN, ac, av);                       \
    }                                                               \
                                                                    \
                                                                    \
  l[N] = pd_open_array(s[N], &w[N], &g[N]);                         \
  if (l[N] <= 0)                                                    \
    {                                                               \
      error("n_array: open array: %s", s[N]->s_name);               \
      outlet_float(x->out, (t_float)O_ERROR);                       \
      return;                                                       \
    }                                                               \
                                                                    \
                                                                    \
  arr_validate(l[N], &start[N], &end[N], &len[N]);                  \
  if (len[N] == 0)                                                  \
    {                                                               \
      post("n_array: length operation == 0");                       \
      outlet_float(x->out, (t_float)O_ERROR);                       \
      return;                                                       \
    }

//----------------------------------------------------------------------------//
// for fft
void mayer_init(void);
void mayer_term(void);

//----------------------------------------------------------------------------//
static t_class *n_array_class;

typedef struct _n_array
{
  t_object x_obj;
  t_outlet *out;
  int seed;
} t_n_array;

//----------------------------------------------------------------------------//
void n_array_size(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, -1, -1);
  outlet_float(x->out, (t_float)l[0]);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_resize(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, -1, -1);
  int i = atom_getfloatarg(1, ac, av);
  garray_resize(g[0], i);
  i = pd_open_array(s[0], &w[0], &g[0]);
  outlet_float(x->out, (t_float)i);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_dump(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  int i,j;
  t_atom *a;
  a = (t_atom *)getbytes(sizeof(t_atom) * len[0]);
  for (i = start[0], j = 0; i < end[0]; i++, j++)
    {
      SETFLOAT(a + j, w[0][i].w_float);
    }
  outlet_list(x->out, &s_list, len[0], a);
  freebytes(a, sizeof(t_atom) * len[0]);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_set(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, -1, -1);
  ac--;
  av++;
  int i;
  garray_resize(g[0], ac);
  l[0] = pd_open_array(s[0], &w[0], &g[0]);
  i = 0;
  while (ac--)
    {
      w[0][i].w_float = atom_getfloat(av++);
      i++;
    }
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)l[0]);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_sum(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float res =  sum(w[0], start[0], end[0]);
  outlet_float(x->out, res);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_mean_arith(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float res =  mean_arith(w[0], start[0], end[0], len[0]);
  outlet_float(x->out, res);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_mean_geo(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float res =  mean_geometric(w[0], start[0], end[0], len[0]);
  outlet_float(x->out, res);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_mean_harm(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float res =  mean_harmonic(w[0], start[0], end[0], len[0]);
  outlet_float(x->out, res);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_centroid(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float res = centroid(w[0], start[0], end[0]);
  outlet_float(x->out, res);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_find_value(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float val       = atom_getfloatarg(3, ac, av);
  t_float tolerance = atom_getfloatarg(4, ac, av);
  t_atom a[2];
  int i = find_value(w[0], start[0], end[0], val, tolerance);
  if (i != -1)
    {
      SETFLOAT(a    , (t_float)i);
      SETFLOAT(a + 1, (t_float)w[0][i].w_float);
      outlet_list(x->out, &s_list, 2, a);
    }
  else
    {
      SETFLOAT(a    , (t_float)-1);
      SETFLOAT(a + 1, (t_float)0);
      outlet_list(x->out, &s_list, 2, a);
    }
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_find_cross_up(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float val       = atom_getfloatarg(3, ac, av);
  t_float tolerance = atom_getfloatarg(4, ac, av);
  t_atom a[2];
  int i = find_cross_up(w[0], start[0], end[0], val, tolerance);
  if (i != -1)
    {
      SETFLOAT(a    , (t_float)i);
      SETFLOAT(a + 1, (t_float)w[0][i].w_float);
      outlet_list(x->out, &s_list, 2, a);
    }
  else
    {
      SETFLOAT(a    , (t_float)-1);
      SETFLOAT(a + 1, (t_float)0);
      outlet_list(x->out, &s_list, 2, a);
    }
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_find_cross_down(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float val       = atom_getfloatarg(3, ac, av);
  t_float tolerance = atom_getfloatarg(4, ac, av);
  t_atom a[2];
  int i = find_cross_down(w[0], start[0], end[0], val, tolerance);
  if (i != -1)
    {
      SETFLOAT(a    , (t_float)i);
      SETFLOAT(a + 1, (t_float)w[0][i].w_float);
      outlet_list(x->out, &s_list, 2, a);
    }
  else
    {
      SETFLOAT(a    , (t_float)-1);
      SETFLOAT(a + 1, (t_float)0);
      outlet_list(x->out, &s_list, 2, a);
    }
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_find_periods(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float val       = atom_getfloatarg(3, ac, av);
  int     min_len   = atom_getfloatarg(4, ac, av);
  t_atom a[4];
  int pos = start[0];
  int num = 0;
  int st;
  int ln;
  end[0] -= 1;
  while (pos < end[0])
    {
      pos = find_periods(w[0], pos, end[0], val, min_len, &st, &ln);
      // found
      if (st != -1)
        {
          SETFLOAT(a    , (t_float)num);               // number
          SETFLOAT(a + 1, (t_float)w[0][st].w_float);  // value
          SETFLOAT(a + 2, (t_float)st);                // start period
          SETFLOAT(a + 3, (t_float)ln);                // length period
          outlet_list(x->out, &s_list, 4, a);
          num++;
        }
    }
  // not found
  if (num == 0)
    {
      SETFLOAT(a    , (t_float)-1);
      SETFLOAT(a + 1, (t_float)-1);
      SETFLOAT(a + 2, (t_float)-1);
      SETFLOAT(a + 3, (t_float)-1);
      outlet_list(x->out, &s_list, 4, a);
    }
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_find_minmax(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_atom a[4];
  int min_idx;
  int max_idx;
  t_float min;
  t_float max;
  find_minmax(w[0], start[0], end[0], &min_idx, &max_idx, &min, &max);
  SETFLOAT(a ,    (t_float)min_idx);
  SETFLOAT(a + 1, (t_float)min);
  SETFLOAT(a + 2, (t_float)max_idx);
  SETFLOAT(a + 3, (t_float)max);
  outlet_list(x->out, &s_list, 4, a);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_dc(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  arr_dc(w[0], start[0], end[0]);
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_normalize(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float value = atom_getfloatarg(3, ac, av);
  arr_normalize(w[0], start[0], end[0], value);
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_constant(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float value = atom_getfloatarg(3, ac, av);
  arr_constant(w[0], start[0], end[0], value);
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_random(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float value = atom_getfloatarg(3, ac, av);
  arr_random(w[0], start[0], end[0], value, &x->seed);
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_towt(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(2);
  N_USE_ARRAY(0, 0, 1, 2);
  N_USE_ARRAY(1, 3, -1, -1);
  int st = atom_getfloatarg(4, ac, av);
  int en = atom_getfloatarg(5, ac, av);
  garray_resize(g[1], len[0]+st+en);
  l[1] = pd_open_array(s[1], &w[1], &g[1]);
  arr_towt(w[0], start[0], end[0], len[0], w[1], st, en);
  garray_redraw(g[1]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_fromwt(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(2);
  N_USE_ARRAY(0, 0, -1, -1);
  N_USE_ARRAY(1, 1, -1, -1);
  int st = atom_getfloatarg(2, ac, av);
  int en = atom_getfloatarg(3, ac, av);
  garray_resize(g[1], len[0]-st-en);
  l[1] = pd_open_array(s[1], &w[1], &g[1]);
  arr_fromwt(w[0], len[0], w[1], st, en);
  garray_redraw(g[1]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_blur(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  float value = atom_getfloatarg(3, ac, av);
  arr_blur(w[0], start[0], end[0], len[0], value); 
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_unique(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  int find = arr_unique(w[0], start[0], end[0]);
  int skip = len[0] - find;
  arr_shift(w[0], end[0], l[0], l[0], 0 - skip); 
  l[0] = l[0] - skip;
  garray_resize(g[0], l[0]);
  l[0] = pd_open_array(s[0], &w[0], &g[0]);
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)find);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_count(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  float base = atom_getfloatarg(3, ac, av);
  float inc  = atom_getfloatarg(4, ac, av);
  arr_count(w[0], start[0], end[0], base, inc);
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_windowing(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_symbol *type = atom_getsymbolarg(3, ac, av);
  t_float coef = atom_getfloatarg(4, ac, av);
  arr_windowing(w[0], start[0], end[0], len[0], (char *)type->s_name, coef);
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_sinesum(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, -1, -1);
  int size_out = atom_getfloatarg(1, ac, av);
  int size_harm = ac - 1;
  if (size_out < 1)  size_out = 1;
  size_out += 3; // for interpolation
  if (size_harm < 1) size_harm = 1;
  t_float *arr_out;
  t_float *arr_harm;
  arr_out  = getbytes(sizeof(t_float) * size_out);
  arr_harm = getbytes(sizeof(t_float) * size_harm);
  int i;
  // copy from list
  for (i = 0; i < size_harm; i++)
    {
      arr_harm[i] = atom_getfloatarg(i+2, ac, av);
    }
  sinesum(arr_out, arr_harm, size_out, size_harm);
  garray_resize(g[0], size_out);
  l[0] = pd_open_array(s[0], &w[0], &g[0]);
  // copy to pd
  for (i = 0; i < size_out; i++)
    {
      w[0][i].w_float = arr_out[i];
    }
  garray_redraw(g[0]);
  freebytes(arr_out, sizeof(t_float) * size_out);  
  freebytes(arr_harm, sizeof(t_float) * size_harm);  
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_interpolation(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(2);
  N_USE_ARRAY(0, 0, -1, -1);
  N_USE_ARRAY(1, 1, -1, -1);
  t_symbol *type_s = atom_getsymbolarg(2, ac, av);
  int type;
  if      (strcmp("2p",type_s->s_name) == 0) type = 1;
  else if (strcmp("4p",type_s->s_name) == 0) type = 2;
  else                                       type = 0;
  if (l[1] < 3)
    {
      post("n_array: interpolation: size < 3");
      return;
    }
  arr_interpolation(w[0], w[1], l[0], l[1], type);
  garray_redraw(g[1]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_sort(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(MAXA);
  N_USE_ARRAY(0, 0, 1, 2);
  int type = atom_getfloatarg(3, ac, av);
  int i;
  int arrays = ac - 4;
  AF_CLIP_MAX(MAXA-1, arrays);
  // other arrays
  for (i=0; i<arrays; i++)
    {
      N_USE_ARRAY(i+1, i+4, 1, 2);
    }
  for (i=0; i<arrays; i++)
    {
      if (len[0] != len[i+1])
        {
          post("n_array: len");    
          outlet_float(x->out, (t_float)O_ERROR);
          return;
        }
    }

  arrays += 1;
  
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
              if (w[0][pos].w_float < w[0][pos2].w_float)
                {
                  // swap
                  for (i=0; i<arrays; i++)
                    {
                      SWAP(buf, w[i][pos].w_float, w[i][pos2].w_float);
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
              if (w[0][pos].w_float > w[0][pos2].w_float)
                {
                  // swap
                  for (i=0; i<arrays; i++)
                    {
                      SWAP(buf, w[i][pos].w_float, w[i][pos2].w_float);
                    }
                }
              pos--;
            }
        }
    }
  
  // smaller first
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
              if (w[0][pos].w_float > w[0][pos2].w_float)
                {
                  // swap
                  for (i=0; i<arrays; i++)
                    {
                      SWAP(buf, w[i][pos].w_float, w[i][pos2].w_float);
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
              if (w[0][pos].w_float < w[0][pos2].w_float)
                {
                  // swap
                  for (i=0; i<arrays; i++)
                    {
                      SWAP(buf, w[i][pos].w_float, w[i][pos2].w_float);
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
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_todisp(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(3);
  N_USE_ARRAY(0, 0, 1, 2);
  N_USE_ARRAY(1, 3, -1, -1);
  N_USE_ARRAY(2, 4, -1, -1);
  if (len[1] != len[2])
    {
      post("n_array: len");    
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }
  arr_todisp(w[0], w[1], w[2], start[0], len[0], start[1], end[1], len[1]);
  garray_redraw(g[1]);
  garray_redraw(g[2]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_mix(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(4);
  N_USE_ARRAY(0, 0, 1, 2);   //in0
  N_USE_ARRAY(1, 3, 4, 5);   //in1
  N_USE_ARRAY(2, 6, 7, 8);   //x
  N_USE_ARRAY(3, 9, 10, 11); //out
  if (len[0] != len[1] ||
      len[0] != len[2] ||
      len[0] != len[3])
    {
      post("n_array: len");    
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }
  arr_mix(w[0], w[1], w[2], w[3], start[0], start[1], start[2], start[3], len[0]);
  garray_redraw(g[3]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_mix_s(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(3);
  N_USE_ARRAY(0, 0, 1, 2);   //in0
  N_USE_ARRAY(1, 3, 4, 5);   //in1
  N_USE_ARRAY(2, 6, 7, 8);   //out
  t_float mix = atom_getfloatarg(9, ac, av);
  if (len[0] != len[1] ||
      len[0] != len[2])
    {
      post("n_array: len");    
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }
  arr_mix_scalar(w[0], w[1], w[2], start[0], start[1], start[2], len[0], mix);
  garray_redraw(g[2]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_shift(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float value = atom_getfloatarg(3, ac, av);
  arr_shift(w[0], start[0], end[0], l[0], value); 
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_shift_rotate(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float value = atom_getfloatarg(3, ac, av);
  arr_shift_rotate(w[0], start[0], len[0], l[0], value); 
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_reverse(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  arr_reverse(w[0], start[0], end[0], len[0]); 
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_delete(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  arr_shift(w[0], end[0], l[0], l[0], 0 - len[0]); 
  garray_resize(g[0],l[0]-len[0]);
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_concat(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(2);
  N_USE_ARRAY(0, 0, 1, 2);
  N_USE_ARRAY(1, 3, -1, -1);
  garray_resize(g[1], l[1]+len[0]);
  pd_open_array(s[1], &w[1], &g[1]);
  int i;
  for (i = 0; i < len[0]; i++)
    {
      w[1][l[1] + i].w_float = w[0][start[0] + i].w_float;
    }
  garray_redraw(g[1]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_copy(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(2);
  N_USE_ARRAY(0, 0, 1, 2);
  N_USE_ARRAY(1, 3, -1, -1);
  garray_resize(g[1], len[0]);
  pd_open_array(s[1], &w[1], &g[1]);
  int i;
  for (i = 0; i < len[0]; i++)
    {
      w[1][i].w_float = w[0][start[0] + i].w_float;
    }
  garray_redraw(g[1]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_insert(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(2);
  N_USE_ARRAY(0, 0, 1, 2);
  N_USE_ARRAY(1, 3, 4, -1);
  garray_resize(g[1], l[1]+len[0]);
  l[1] = pd_open_array(s[1], &w[1], &g[1]);
  arr_shift(w[1], start[1], end[1], l[1], len[0]); 
  int i;
  for (i = 0; i < len[0]; i++)
    {
      w[1][start[1] + i].w_float = w[0][start[0] + i].w_float;
    }
  garray_redraw(g[1]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_replace(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(2);
  N_USE_ARRAY(0, 0, 1, 2);
  N_USE_ARRAY(1, 3, 4, -1);
  int i,j;
  for (i = 0; i < len[0]; i++)
    {
      j = start[1] + i;
      if (j < l[1])
        {
          w[1][j].w_float = w[0][start[0] + i].w_float;
        }
    }
  garray_redraw(g[1]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_clip_min(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(2);
  N_USE_ARRAY(0, 0, 1, 2);
  N_USE_ARRAY(1, 3, 4, -1);
  if (len[0] > len[1])
    {
      post("n_array: len");    
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }
  arr_clip_min(w[0], w[1], start[0], start[1], len[0]);
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_clip_min_s(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float value = atom_getfloatarg(3, ac, av);
  arr_clip_min_scalar(w[0], start[0], end[0], value);
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_clip_max(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(2);
  N_USE_ARRAY(0, 0, 1, 2);
  N_USE_ARRAY(1, 3, 4, -1);
  if (len[0] > len[1])
    {
      post("n_array: len");    
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }
  arr_clip_max(w[0], w[1], start[0], start[1], len[0]);
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_clip_max_s(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float value = atom_getfloatarg(3, ac, av);
  arr_clip_max_scalar(w[0], start[0], end[0], value);
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_clip_minmax(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(3);
  N_USE_ARRAY(0, 0, 1, 2);
  N_USE_ARRAY(1, 3, 4, -1);
  N_USE_ARRAY(2, 5, 6, -1);
  if (len[0] > len[1] ||
      len[0] > len[2])
    {
      post("n_array: len");    
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }
  arr_clip_minmax(w[0], w[1], w[2], start[0], start[1], start[2], len[0]);
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_clip_minmax_s(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float min = atom_getfloatarg(3, ac, av);
  t_float max = atom_getfloatarg(4, ac, av);
  arr_clip_minmax_scalar(w[0], start[0], end[0], min, max);
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_math(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  int i,  buf_i;
  t_float buf_f;
  if      (strcmp("abs",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          AF_ABS(w[0][i].w_float, w[0][i].w_float);
        }
    }
  else if (strcmp("floor",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          AF_FLOOR(w[0][i].w_float, buf_i);
          w[0][i].w_float = buf_i;
        }
    }
  else if (strcmp("ceil",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          AF_CEIL(w[0][i].w_float, buf_i);
          w[0][i].w_float = buf_i;
        }
    }
  else if (strcmp("wrap1",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          AF_WRAP(w[0][i].w_float, buf_i, w[0][i].w_float);
        }
    }
  else if (strcmp("wrap2",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          buf_f = w[0][i].w_float * 0.5;
          AF_WRAP(buf_f, buf_i, buf_f);
          w[0][i].w_float = buf_f + buf_f;
        }
    }
  else if (strcmp("sqrt",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          w[0][i].w_float = sqrt(w[0][i].w_float);
        }
    }
  else if (strcmp("log",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          w[0][i].w_float = log(w[0][i].w_float);
        }
    }
  else if (strcmp("log2",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          w[0][i].w_float = log2(w[0][i].w_float);
        }
    }
  else if (strcmp("log10",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          w[0][i].w_float = log10(w[0][i].w_float);
        }
    }
  else if (strcmp("exp",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          w[0][i].w_float = exp(w[0][i].w_float);
        }
    }
  else if (strcmp("exp2",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          w[0][i].w_float = exp2(w[0][i].w_float);
        }
    }
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_arith(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(2);
  N_USE_ARRAY(0, 0, 1, 2);
  N_USE_ARRAY(1, 3, 4, 5);
  if (len[0] != len[1] )
    {
      post("n_array: len");
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }
  int i;
  int buf_i;
  int buf_s;
  if (strcmp("add",sym->s_name) == 0)
    {                
      for (i = 0; i < len[0]; i++)
        {
          w[0][i + start[0]].w_float = 
            w[0][i + start[0]].w_float + w[1][i + start[1]].w_float;
        }
    }
  else if (strcmp("sub",sym->s_name) == 0)
    {
      for (i = 0; i < len[0]; i++)
        {
          w[0][i + start[0]].w_float = 
            w[0][i + start[0]].w_float - w[1][i + start[1]].w_float;
        }
    }
  else if (strcmp("mult",sym->s_name) == 0)
    {
      for (i = 0; i < len[0]; i++)
        {
          w[0][i + start[0]].w_float = 
            w[0][i + start[0]].w_float * w[1][i + start[1]].w_float;
        }
    }
  // div zero ?
  else if (strcmp("div",sym->s_name) == 0)
    {
      for (i = 0; i < len[0]; i++)
        {
          w[0][i + start[0]].w_float = 
            w[0][i + start[0]].w_float / w[1][i + start[1]].w_float;
        }
    }
  else if (strcmp("mod",sym->s_name) == 0)
    {
      for (i = 0; i < len[0]; i++)
        {
          buf_i = w[1][i + start[1]].w_float;
          if (buf_i < 0)  buf_i = 0 - buf_i;
          if (buf_i == 0) buf_i = 1;
          buf_s = w[0][i + start[0]].w_float;
          w[0][i + start[0]].w_float = buf_s % buf_i;
        }
    }
  else if (strcmp("pow",sym->s_name) == 0)
    {
      for (i = 0; i < len[0]; i++)
        {
          w[0][i + start[0]].w_float = 
            pow(w[0][i + start[0]].w_float, w[1][i + start[1]].w_float);
        }
    }
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_arith_s(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float value = atom_getfloatarg(3, ac, av);
  int i;
  int buf_i;
  int buf_s;
  if (strcmp("add_s",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          w[0][i].w_float = w[0][i].w_float + value;
        }
    }
  else if (strcmp("sub_s",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          w[0][i].w_float = w[0][i].w_float - value;
        }
    }
  else if (strcmp("mult_s",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          w[0][i].w_float = w[0][i].w_float * value;
        }
    }
  else if (strcmp("div_s",sym->s_name) == 0)
    {
      if (value == 0) return; /* divzero ??? */
      for (i = start[0]; i < end[0]; i++)
        {
          w[0][i].w_float = w[0][i].w_float / value;
        }
    }
  else if (strcmp("mod_s",sym->s_name) == 0)
    {
      buf_i = value;
      if (buf_i < 0)  buf_i = 0 - buf_i;
      if (buf_i == 0) buf_i = 1;
      for (i = start[0]; i < end[0]; i++)
        {
          buf_s = w[0][i].w_float;
          w[0][i].w_float = buf_s % buf_i;
        }
    }
  else if (strcmp("pow_s",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          w[0][i].w_float = pow(w[0][i].w_float, value);
        }
    }
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_trigonometry(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  int i;
  if (strcmp("sin",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          w[0][i].w_float = sin(w[0][i].w_float);
        }
    }
  else if (strcmp("asin",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          w[0][i].w_float = asin(w[0][i].w_float);
        }
    }
  else if (strcmp("sinh",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          w[0][i].w_float = sinh(w[0][i].w_float);
        }
    }
  else if (strcmp("asinh",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          w[0][i].w_float = asinh(w[0][i].w_float);
        }
    }
  else if (strcmp("cos",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          w[0][i].w_float = cos(w[0][i].w_float);
        }
    }
  else if (strcmp("acos",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          w[0][i].w_float = acos(w[0][i].w_float);
        }
    }
  else if (strcmp("cosh",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          w[0][i].w_float = cosh(w[0][i].w_float);
        }
    }
  else if (strcmp("acosh",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          w[0][i].w_float = acosh(w[0][i].w_float);
        }
    }
  else if (strcmp("tan",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          w[0][i].w_float = tan(w[0][i].w_float);
        }
    }
  else if (strcmp("atan",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          w[0][i].w_float = atan(w[0][i].w_float);
        }
    }
  else if (strcmp("tanh",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          w[0][i].w_float = tanh(w[0][i].w_float);
        }
    }
  else if (strcmp("atanh",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          w[0][i].w_float = atanh(w[0][i].w_float);
        }
    }
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_comparison(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(3);
  N_USE_ARRAY(0, 0, 1, 2);
  N_USE_ARRAY(1, 3, 4, 5);
  N_USE_ARRAY(2, 6, 7, 8);
  if (len[0] != len[1] || len[0] != len[2])
    {
      post("n_array: len");
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }
  int i;
  if (strcmp("eq",sym->s_name) == 0)
    {
      for (i = 0; i < len[0]; i++)
        {
          w[2][i + start[2]].w_float = 
            w[0][i + start[0]].w_float == w[1][i + start[1]].w_float;
        }
    }
  else if (strcmp("ne",sym->s_name) == 0)
    {
      for (i = 0; i < len[0]; i++)
        {
          w[2][i + start[2]].w_float = 
            w[0][i + start[0]].w_float != w[1][i + start[1]].w_float;
        }
    }
  else if (strcmp("gt",sym->s_name) == 0)
    {
      for (i = 0; i < len[0]; i++)
        {
          w[2][i + start[2]].w_float = 
            w[0][i + start[0]].w_float > w[1][i + start[1]].w_float;
        }
    }
  else if (strcmp("ge",sym->s_name) == 0)
    {
      for (i = 0; i < len[0]; i++)
        {
          w[2][i + start[2]].w_float = 
            w[0][i + start[0]].w_float >= w[1][i + start[1]].w_float;
        }
    }
  else if (strcmp("lt",sym->s_name) == 0)
    {
      for (i = 0; i < len[0]; i++)
        {
          w[2][i + start[2]].w_float = 
            w[0][i + start[0]].w_float < w[1][i + start[1]].w_float;
        }
    }
  else if (strcmp("le",sym->s_name) == 0)
    {
      for (i = 0; i < len[0]; i++)
        {
          w[2][i + start[2]].w_float = 
            w[0][i + start[0]].w_float <= w[1][i + start[1]].w_float;
        }
    }
  garray_redraw(g[2]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_comparison_s(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float value = atom_getfloatarg(3, ac, av);
  int i;
  if      (strcmp("eq_s",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          w[0][i].w_float = (w[0][i].w_float == value);
        }
    }
  else if (strcmp("ne_s",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          w[0][i].w_float = (w[0][i].w_float != value);
        }
    }
  else if (strcmp("gt_s",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          w[0][i].w_float = (w[0][i].w_float > value);
        }
    }
  else if (strcmp("ge_s",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          w[0][i].w_float = (w[0][i].w_float >= value);
        }
    }
  else if (strcmp("lt_s",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          w[0][i].w_float = (w[0][i].w_float < value);
        }
    }
  else if (strcmp("le_s",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          w[0][i].w_float = (w[0][i].w_float <= value);
        }
    }
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_conversion(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  int i;
  t_float buf;
  if      (strcmp("m2f",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          buf = w[0][i].w_float;
          AF_CLIP_MINMAX(-1500., 1499., buf);
          AF_M2F(buf, buf);
          w[0][i].w_float = buf;
        }
    }
  else if (strcmp("f2m",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          buf = w[0][i].w_float;
          AF_CLIP_MIN(0., buf);
          AF_F2M(buf, buf);
          w[0][i].w_float = buf;
        }
    }
  else if (strcmp("db2pow",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          buf = w[0][i].w_float;
          AF_CLIP_MINMAX(0., 870., buf);
          AF_DB2POW(buf, buf);
          w[0][i].w_float = buf;
        }
    }
  else if (strcmp("pow2db",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          buf = w[0][i].w_float;
          AF_CLIP_MIN(0., buf);
          AF_POW2DB(buf, buf);
          w[0][i].w_float = buf;
        }
    }
  else if (strcmp("db2rms",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          buf = w[0][i].w_float;
          AF_CLIP_MINMAX(0., 485., buf);
          AF_DB2RMS(buf, buf);
          w[0][i].w_float = buf;
        }
    }
  else if (strcmp("rms2db",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          buf = w[0][i].w_float;
          AF_CLIP_MIN(0., buf);
          AF_RMS2DB(buf, buf);
          w[0][i].w_float = buf;
        }
    }
  else if (strcmp("logt2sec",sym->s_name) == 0)
    {
      for (i = start[0]; i < end[0]; i++)
        {
          buf = w[0][i].w_float;
          AF_LOGT2SEC(buf, buf);
          w[0][i].w_float = buf;
        }
    }
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
}

//----------------------------------------------------------------------------//
void n_array_fft(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(3);
  N_USE_ARRAY(0, 0, -1, -1);
  N_USE_ARRAY(1, 1, -1, -1);
  N_USE_ARRAY(2, 2, -1, -1);
  if (!(l[0] == 4      ||
        l[0] == 8      ||
        l[0] == 16     ||
        l[0] == 32     ||
        l[0] == 64     ||
        l[0] == 128    ||
        l[0] == 256    ||
        l[0] == 512    ||
        l[0] == 1024   ||
        l[0] == 2048   ||
        l[0] == 4096   ||
        l[0] == 8192   ||
        l[0] == 16384) ||
      (l[0] != l[1] || l[0] != l[2]))
    {
      post("n_array: len");
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }
  int i;
  t_float buf_r[MAX_FFT];
  t_float buf_i[MAX_FFT];
  for (i = 0; i < l[0]; i++)
    {
      buf_r[i] = buf_i[i] = w[0][i].w_float;
    }
  mayer_fft(l[0], buf_r, buf_i);
  for (i = 0; i < l[0]; i++)
    {
      w[1][i].w_float = buf_r[i];
      w[2][i].w_float = buf_i[i];
    }
  garray_redraw(g[1]);
  garray_redraw(g[2]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_ifft(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(3);
  N_USE_ARRAY(0, 0, -1, -1);
  N_USE_ARRAY(1, 1, -1, -1);
  N_USE_ARRAY(2, 2, -1, -1);
  if (!(l[0] == 4      ||
        l[0] == 8      ||
        l[0] == 16     ||
        l[0] == 32     ||
        l[0] == 64     ||
        l[0] == 128    ||
        l[0] == 256    ||
        l[0] == 512    ||
        l[0] == 1024   ||
        l[0] == 2048   ||
        l[0] == 4096   ||
        l[0] == 8192   ||
        l[0] == 16384) ||
      (l[0] != l[1] || l[0] != l[2]))
    {
      post("n_array: len");
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }
  int i;
  t_float buf_r[MAX_FFT];
  t_float buf_i[MAX_FFT];
  for (i = 0; i < l[1]; i++)
    {
      buf_r[i] = w[1][i].w_float;
      buf_i[i] = w[2][i].w_float;
    }
  mayer_ifft(l[1], buf_r, buf_i);
  t_float f = 1. / l[1];
  for (i = 0; i < l[1]; i++)
    {
      w[0][i].w_float = (buf_r[i] * f);
    }
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_vec2pol(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(4);
  N_USE_ARRAY(0, 0, -1, -1);
  N_USE_ARRAY(1, 1, -1, -1);
  N_USE_ARRAY(2, 2, -1, -1);
  N_USE_ARRAY(3, 3, -1, -1);
  if (!(l[0] == 4      ||
        l[0] == 8      ||
        l[0] == 16     ||
        l[0] == 32     ||
        l[0] == 64     ||
        l[0] == 128    ||
        l[0] == 256    ||
        l[0] == 512    ||
        l[0] == 1024   ||
        l[0] == 2048   ||
        l[0] == 4096   ||
        l[0] == 8192   ||
        l[0] == 16384) ||
      (l[0] != l[1] || l[0] != l[2] || l[0] != l[3]))
    {
      post("n_array: len");
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }
  arr_vec2pol(w[0], w[1], w[2], w[3], len[0]);
  garray_redraw(g[2]);
  garray_redraw(g[3]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_pol2vec(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(4);
  N_USE_ARRAY(0, 0, -1, -1);
  N_USE_ARRAY(1, 1, -1, -1);
  N_USE_ARRAY(2, 2, -1, -1);
  N_USE_ARRAY(3, 3, -1, -1);
  if (!(l[0] == 4      ||
        l[0] == 8      ||
        l[0] == 16     ||
        l[0] == 32     ||
        l[0] == 64     ||
        l[0] == 128    ||
        l[0] == 256    ||
        l[0] == 512    ||
        l[0] == 1024   ||
        l[0] == 2048   ||
        l[0] == 4096   ||
        l[0] == 8192   ||
        l[0] == 16384) ||
      (l[0] != l[1] || l[0] != l[2] || l[0] != l[3]))
    {
      post("n_array: len");
      outlet_float(x->out, (t_float)O_ERROR);
      return;
    }
  arr_pol2vec(w[0], w[1], w[2], w[3], len[0]);
  garray_redraw(g[2]);
  garray_redraw(g[3]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_curve_breakpoint
(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float c1 = atom_getfloatarg(3, ac, av);
  t_float c2 = atom_getfloatarg(4, ac, av);
  AF_CLIP_MINMAX(0., 1., c1);
  AF_CLIP_MINMAX(0., 1., c2);
  arr_curve_breakpoint(w[0], start[0], end[0], len[0], c1, c2);
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_curve_breakpoint_smooth
(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float c1 = atom_getfloatarg(3, ac, av);
  t_float c2 = atom_getfloatarg(4, ac, av);
  AF_CLIP_MINMAX(0., 1., c1);
  AF_CLIP_MINMAX(0., 1., c2);
  arr_curve_breakpoint_smooth(w[0], start[0], end[0], len[0], c1, c2);
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_curve_circular
(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float c1 = atom_getfloatarg(3, ac, av);
  AF_CLIP_MINMAX(0., 1., c1);
  arr_curve_circular(w[0], start[0], end[0], len[0], c1);
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_curve_circular_sigmoid
(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float c1 = atom_getfloatarg(3, ac, av);
  AF_CLIP_MINMAX(0., 1., c1);
  arr_curve_circular_sigmoid(w[0], start[0], end[0], len[0], c1);
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_curve_cubic
(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float c1 = atom_getfloatarg(3, ac, av);
  t_float c2 = atom_getfloatarg(4, ac, av);
  AF_CLIP_MINMAX(0., 1., c1);
  AF_CLIP_MINMAX(0., 1., c2);
  arr_curve_cubic(w[0], start[0], end[0], len[0], c1, c2);
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_curve_exp
(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  arr_curve_exp(w[0], start[0], end[0], len[0]);
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_curve_log
(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  arr_curve_log(w[0], start[0], end[0], len[0]);
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_curve_elleptic
(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float c1 = atom_getfloatarg(3, ac, av);
  t_float c2 = atom_getfloatarg(4, ac, av);
  AF_CLIP_MINMAX(0., 1., c1);
  AF_CLIP_MINMAX(0., 1., c2);
  arr_curve_elleptic(w[0], start[0], end[0], len[0], c1, c2);
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_curve_elleptic_seat
(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float c1 = atom_getfloatarg(3, ac, av);
  t_float c2 = atom_getfloatarg(4, ac, av);
  AF_CLIP_MINMAX(0., 1., c1);
  AF_CLIP_MINMAX(0., 1., c2);
  arr_curve_elleptic_seat(w[0], start[0], end[0], len[0], c1, c2);
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_curve_elleptic_sigmoid
(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float c1 = atom_getfloatarg(3, ac, av);
  t_float c2 = atom_getfloatarg(4, ac, av);
  AF_CLIP_MINMAX(0., 1., c1);
  AF_CLIP_MINMAX(0., 1., c2);
  arr_curve_elleptic_sigmoid(w[0], start[0], end[0], len[0], c1, c2);
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_curve_exponential
(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float c1 = atom_getfloatarg(3, ac, av);
  AF_CLIP_MINMAX(0., 1., c1);
  arr_curve_expotential(w[0], start[0], end[0], len[0], c1);
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_curve_exponential_seat
(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float c1 = atom_getfloatarg(3, ac, av);
  AF_CLIP_MINMAX(0., 1., c1);
  arr_curve_expotential_seat(w[0], start[0], end[0], len[0], c1);
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_curve_exponential_sigmoid
(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float c1 = atom_getfloatarg(3, ac, av);
  AF_CLIP_MINMAX(0., 1., c1);
  arr_curve_expotential_sigmoid(w[0], start[0], end[0], len[0], c1);
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_curve_logistic_sigmoid
(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float c1 = atom_getfloatarg(3, ac, av);
  AF_CLIP_MINMAX(0., 1., c1);
  arr_curve_logistic_sigmoid(w[0], start[0], end[0], len[0], c1);
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_curve_quadratic
(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float c1 = atom_getfloatarg(3, ac, av);
  t_float c2 = atom_getfloatarg(4, ac, av);
  AF_CLIP_MINMAX(0., 1., c1);
  AF_CLIP_MINMAX(0., 1., c2);
  arr_curve_quadratic(w[0], start[0], end[0], len[0], c1, c2);
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_curve_quartic
(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float c1 = atom_getfloatarg(3, ac, av);
  t_float c2 = atom_getfloatarg(4, ac, av);
  AF_CLIP_MINMAX(0., 1., c1);
  AF_CLIP_MINMAX(0., 1., c2);
  arr_curve_quartic(w[0], start[0], end[0], len[0], c1, c2);
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_curve_simplified_cubic_seat
(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float c1 = atom_getfloatarg(3, ac, av);
  t_float c2 = atom_getfloatarg(4, ac, av);
  AF_CLIP_MINMAX(0., 1., c1);
  AF_CLIP_MINMAX(0., 1., c2);
  arr_curve_simplified_cubic_seat(w[0], start[0], end[0], len[0], c1, c2);
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
}

//----------------------------------------------------------------------------//
void n_array_curve_simplified_quadratic
(t_n_array *x, t_symbol *sym, int ac, t_atom *av)
{
  N_ARRAYS(1);
  N_USE_ARRAY(0, 0, 1, 2);
  t_float c1 = atom_getfloatarg(3, ac, av);
  AF_CLIP_MINMAX(0., 1., c1);
  arr_curve_simplified_quadratic(w[0], start[0], end[0], len[0], c1);
  garray_redraw(g[0]);
  outlet_float(x->out, (t_float)O_DONE);
  DIS(sym);
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
#define METHOD(F,S) \
class_addmethod(n_array_class,(t_method)(F),gensym((S)),A_GIMME, 0);

void n_array_setup(void)
{
   n_array_class = class_new(gensym("n_array"), 
                             (t_newmethod)n_array_new, 0,
                             sizeof(t_n_array), 0, A_GIMME, 0);
   /* size */
   METHOD(n_array_size,                        "size");              
   METHOD(n_array_resize,                      "resize");            
   /* list */
   METHOD(n_array_dump,                        "dump");              
   METHOD(n_array_set,                         "set");               
   /* analysis */
   METHOD(n_array_sum,                         "sum");               
   METHOD(n_array_mean_arith,                  "mean_arith");        
   METHOD(n_array_mean_geo,                    "mean_geo");          
   METHOD(n_array_mean_harm,                   "mean_harm");         
   METHOD(n_array_centroid,                    "centroid");          
   METHOD(n_array_find_value,                  "find_value");        
   METHOD(n_array_find_cross_up,               "find_cross_up");     
   METHOD(n_array_find_cross_down,             "find_cross_down");   
   METHOD(n_array_find_periods,                "find_periods");      
   METHOD(n_array_find_minmax,                 "find_minmax");       
   /* various */
   METHOD(n_array_dc,                          "dc");                
   METHOD(n_array_normalize,                   "normalize");         
   METHOD(n_array_constant,                    "constant");          
   METHOD(n_array_random,                      "random");            
   METHOD(n_array_towt,                        "towt");              
   METHOD(n_array_fromwt,                      "fromwt");            
   METHOD(n_array_blur,                        "blur");              
   METHOD(n_array_unique,                      "unique");            
   METHOD(n_array_count,                       "count");             
   METHOD(n_array_windowing,                   "window");            
   METHOD(n_array_sinesum,                     "sinesum");           
   METHOD(n_array_interpolation,               "interpolation");     
   METHOD(n_array_sort,                        "sort");              
   METHOD(n_array_todisp,                      "todisp");            
   /* mix */
   METHOD(n_array_mix,                         "mix");               
   METHOD(n_array_mix_s,                       "mix_s");             
   /* moving */
   METHOD(n_array_shift,                       "shift");             
   METHOD(n_array_shift_rotate,                "shift_rotate");      
   METHOD(n_array_reverse,                     "reverse");           
   METHOD(n_array_delete,                      "delete");            
   METHOD(n_array_concat,                      "concat");            
   METHOD(n_array_copy,                        "copy");              
   METHOD(n_array_insert,                      "insert");            
   METHOD(n_array_replace,                     "replace");           
   /* clip */
   METHOD(n_array_clip_min,                    "clip_min");           
   METHOD(n_array_clip_max,                    "clip_max");           
   METHOD(n_array_clip_minmax,                 "clip_minmax");        
   METHOD(n_array_clip_min_s,                  "clip_min_s");         
   METHOD(n_array_clip_max_s,                  "clip_max_s");         
   METHOD(n_array_clip_minmax_s,               "clip_minmax_s");      
   /* math */
   METHOD(n_array_math,                        "abs");               
   METHOD(n_array_math,                        "floor");             
   METHOD(n_array_math,                        "ceil");              
   METHOD(n_array_math,                        "wrap1");             
   METHOD(n_array_math,                        "wrap2");             
   METHOD(n_array_math,                        "sqrt");              
   METHOD(n_array_math,                        "log");               
   METHOD(n_array_math,                        "log2");              
   METHOD(n_array_math,                        "log10");             
   METHOD(n_array_math,                        "exp");               
   METHOD(n_array_math,                        "exp2");              
   /* arith */
   METHOD(n_array_arith,                       "add");               
   METHOD(n_array_arith,                       "sub");               
   METHOD(n_array_arith,                       "mult");              
   METHOD(n_array_arith,                       "div");               
   METHOD(n_array_arith,                       "mod");               
   METHOD(n_array_arith,                       "pow");               
   /* arith scalar */
   METHOD(n_array_arith_s,                     "add_s");             
   METHOD(n_array_arith_s,                     "sub_s");             
   METHOD(n_array_arith_s,                     "mult_s");            
   METHOD(n_array_arith_s,                     "div_s");             
   METHOD(n_array_arith_s,                     "mod_s");             
   METHOD(n_array_arith_s,                     "pow_s");             
   /* trigonometry */
   METHOD(n_array_trigonometry,                "sin");               
   METHOD(n_array_trigonometry,                "sinh");              
   METHOD(n_array_trigonometry,                "asin");              
   METHOD(n_array_trigonometry,                "asinh");             
   METHOD(n_array_trigonometry,                "cos");               
   METHOD(n_array_trigonometry,                "cosh");              
   METHOD(n_array_trigonometry,                "acos");              
   METHOD(n_array_trigonometry,                "acosh");             
   METHOD(n_array_trigonometry,                "tan");               
   METHOD(n_array_trigonometry,                "tanh");              
   METHOD(n_array_trigonometry,                "atan");              
   METHOD(n_array_trigonometry,                "atanh");             
   /* comparison */
   METHOD(n_array_comparison,                  "eq");                
   METHOD(n_array_comparison,                  "ne");                
   METHOD(n_array_comparison,                  "ge");                
   METHOD(n_array_comparison,                  "gt");                
   METHOD(n_array_comparison,                  "le");                
   METHOD(n_array_comparison,                  "lt");                
   /* comparison scalar */
   METHOD(n_array_comparison_s,                "eq_s");              
   METHOD(n_array_comparison_s,                "ne_s");              
   METHOD(n_array_comparison_s,                "ge_s");              
   METHOD(n_array_comparison_s,                "gt_s");              
   METHOD(n_array_comparison_s,                "le_s");              
   METHOD(n_array_comparison_s,                "lt_s");              
   /* conversion */
   METHOD(n_array_conversion,                  "m2f");               
   METHOD(n_array_conversion,                  "f2m");               
   METHOD(n_array_conversion,                  "db2pow");            
   METHOD(n_array_conversion,                  "pow2db");            
   METHOD(n_array_conversion,                  "db2rms");            
   METHOD(n_array_conversion,                  "rms2db");            
   METHOD(n_array_conversion,                  "logt2sec");          
   /* fft */
   METHOD(n_array_fft,                         "fft");               
   METHOD(n_array_ifft,                        "ifft");              
   METHOD(n_array_vec2pol,                     "vec2pol");           
   METHOD(n_array_pol2vec,                     "pol2vec");           
   /* curves */
   METHOD(n_array_curve_breakpoint,            "c_breakpoint");            
   METHOD(n_array_curve_breakpoint_smooth,     "c_breakpoint_smooth");     
   METHOD(n_array_curve_circular,              "c_circular");              
   METHOD(n_array_curve_circular_sigmoid,      "c_circular_sigmoid");      
   METHOD(n_array_curve_cubic,                 "c_cubic");                 
   METHOD(n_array_curve_exp,                   "c_exp");                   
   METHOD(n_array_curve_log,                   "c_log");                   
   METHOD(n_array_curve_elleptic,              "c_elleptic");              
   METHOD(n_array_curve_elleptic_seat,         "c_elleptic_seat");         
   METHOD(n_array_curve_elleptic_sigmoid,      "c_elleptic_sigmoid");      
   METHOD(n_array_curve_exponential,           "c_exponential");           
   METHOD(n_array_curve_exponential_seat,      "c_exponential_seat");      
   METHOD(n_array_curve_exponential_sigmoid,   "c_exponential_sigmoid");   
   METHOD(n_array_curve_logistic_sigmoid,      "c_logistic_sigmoid");      
   METHOD(n_array_curve_quadratic,             "c_quadratic");             
   METHOD(n_array_curve_quartic,               "c_quartic");               
   METHOD(n_array_curve_simplified_cubic_seat, "c_simplified_cubic_seat"); 
   METHOD(n_array_curve_simplified_quadratic,  "c_simplified_quadratic");  
   /* for fft */
   mayer_init();
}

