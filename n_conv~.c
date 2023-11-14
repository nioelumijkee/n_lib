/* convolver */
#include <immintrin.h>
#include <math.h>
#include "m_pd.h"
#include "g_all_guis.h"
#include "include/pdfunc.h"

#define MAX_SIZE_IMP    8192
#define MAX_SIZE_IMP_1  8191
#define MAX_SIZE_IMP2   16384
#define MAX_SIZE_M      1024 // IMP/8
#define MAX_SIZE_MD     2048 // (IMP/8)*2

static t_class *n_conv_class;
typedef struct _n_conv
{
  t_object x_obj;
  int on;
  int bs;
  float z[MAX_SIZE_IMP2];
  __m256 imp[MAX_SIZE_M];
  int size_i;
  int size;
  int size_m;
  t_symbol *s_a; /* array */
  t_word   *w_a;
  t_garray *g_a;
  int       l_a;
  int count;
} t_n_conv;

//----------------------------------------------------------------------------//
// UTILS                                                                      //
//----------------------------------------------------------------------------//
void n_conv_postinfo(t_n_conv *x)
{
  post("================================");
  post("on         : %d", x->on);
  post("size block : %d", x->bs);
  post("array      : %s", x->s_a->s_name);
  post("array size : %d", x->l_a);
  post("size       : %d", x->size);
  post("size m     : %d", x->size_m);
}

void calc_size(t_n_conv *x)
{
  x->on = 0;

  // open pd array
  x->l_a = pd_open_array(x->s_a, &x->w_a, &x->g_a);

  // bs
  if ((x->bs < 8) || (x->bs%8 != 0))
    {
      post("error: n_conv~: block size must be greatly 8 and div 8 without rest: %d",
	   x->bs);
      return;
    }

  // len array
  if (x->l_a < x->bs)
    {
      post("error: n_conv~: bad array or array size: %d", x->l_a);
      return;
    }

  x->on = 1;

  // size imp
  int i = x->size_i;
  i = (i > MAX_SIZE_IMP)  ? MAX_SIZE_IMP  : i;
  i = (i > x->l_a)        ? x->l_a        : i;
  i = (i < x->bs)         ? x->bs         : i;
  i = i / x->bs;
  i = i * x->bs;
  x->size   = i;
  x->size_m = x->size / 8;

  // copy imp
  float *f = (float*)x->imp;
  for (i=0; i<x->size; i++)
    {
      f[i] = x->w_a[i].w_float;
    }

  // clear mem
  for (i=0; i<MAX_SIZE_IMP2; i++)
    {
      x->z[i] = 0.0;
    }

  // count
  x->count = MAX_SIZE_IMP_1;
}

void n_conv_open(t_n_conv *x, t_symbol *s)
{
  x->s_a = s;
  calc_size(x);
}

void n_conv_size(t_n_conv *x, t_floatarg f)
{
  x->size_i = f;
  calc_size(x);
}

//----------------------------------------------------------------------------//
// DSP                                                                        //
//----------------------------------------------------------------------------//
t_int *n_conv_perform(t_int *w)
{
  t_n_conv *x  = (t_n_conv *)(w[1]);
  t_float *in  = (t_float *)(w[2]);
  t_float *out = (t_float *)(w[3]);
  if (x->on)
    {
      int i,j,k;
      int size_m = x->size_m;
      __m256 sa_m[MAX_SIZE_M];
      __m256 s1_m;
      __m256 s2_m;
      float e[8];

      // write to z
      int count  = x->count;
      for (i = 0; i < x->bs; i++)
	{
	  x->z[count] = x->z[count+MAX_SIZE_IMP] = (float)*(in++);
	  count--;
	}

      // count
      if (count < 0)
	count = MAX_SIZE_IMP_1;
      x->count = count;
      count += x->bs;

      // conv
      for (i=0; i<x->bs; i++)
	{
	  // mult
	  for (j=0; j<size_m; j++)
	    {
	      __m256 _z   = _mm256_loadu_ps(x->z   + count + (j<<3));
	      sa_m[j] = _mm256_mul_ps(_z, x->imp[j]);
	    }
	  // sum
	  s1_m = _mm256_setzero_ps();
	  for (k=0; k<size_m; k++) // why k ?
	    {
	      s2_m = _mm256_add_ps(s1_m, sa_m[k]);
	      s1_m = s2_m;
	    }
	  // to float and out
	  _mm256_store_ps(e, s1_m);
	  *(out++) = e[0] + e[1] + e[2] + e[3] + e[4] + e[5] + e[6] + e[7];
	  // dec
	  count--;
	}
    }
  else
    {
      int n = x->bs;
      while(n--)
	*(out++) = *(in++);
    }
  return (w + 4);
}

void n_conv_dsp(t_n_conv *x, t_signal **sp)
{
  x->bs = sp[0]->s_n;
  calc_size(x);
  dsp_add(n_conv_perform,
          3,
          x,
          sp[0]->s_vec,
          sp[1]->s_vec);
}

//----------------------------------------------------------------------------//
// SETUP                                                                      //
//----------------------------------------------------------------------------//
void *n_conv_new(t_symbol *s, int ac, t_atom *av)
{
  t_n_conv *x = (t_n_conv *)pd_new(n_conv_class);
  x->bs = 64;
  x->size_i = 64;
  if (ac >= 1 && IS_A_SYMBOL(av, 0)) 
    {
      n_conv_open(x, atom_getsymbolarg(0,ac,av));
    }
  else
    {
      x->s_a = gensym("");
      x->l_a = 0;
      x->on = 0;
    }
  if (ac >= 2 && IS_A_FLOAT(av, 1))
    {
      n_conv_size(x, atom_getfloatarg(1,ac,av));
    }
  outlet_new(&x->x_obj, &s_signal);
  return (void *)x;
  if (s) {}
}

void n_conv_tilde_setup(void)
{
  n_conv_class=class_new(gensym("n_conv~"),(t_newmethod)n_conv_new,
			 0,
			 sizeof(t_n_conv),0,A_GIMME,0);
  class_addmethod(n_conv_class,(t_method)n_conv_dsp,gensym("dsp"),0);
  class_addmethod(n_conv_class,nullfn,gensym("signal"),0);
  class_addmethod(n_conv_class,(t_method)n_conv_open,gensym("open"),A_SYMBOL,0);
  class_addmethod(n_conv_class,(t_method)n_conv_size,gensym("size"),A_FLOAT,0);
  class_addmethod(n_conv_class,(t_method)n_conv_postinfo,gensym("info"),0);
}
