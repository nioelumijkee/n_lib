/* convolver */

#include <immintrin.h>

#include <math.h>
#include "m_pd.h"
#include "include/parsearg.h"
#include "include/pd_open_array.c"

#define MAXSIZE   8192
#define MAXSIZE_M 1024

static t_class *n_conv_class;

typedef struct _n_conv
{
  t_object x_obj;
  int on;
  int bs;

  __m256 z[MAXSIZE_M];
  __m256 a[MAXSIZE_M];
  __m256 s[MAXSIZE_M];

  int size_i;
  int size;
  int size_m;

  t_symbol *s_a; /* array */
  t_word   *w_a;
  t_garray *g_a;
  int       l_a;

  int count_z;
} t_n_conv;

//----------------------------------------------------------------------------//
void n_conv_postinfo(t_n_conv *x)
{
  post("----------------------------");
  post("array      : %s", x->s_a->s_name);
  post("array size : %d", x->l_a);
  post("size       : %d", x->size);
  post("size m     : %d", x->size_m);
}

//----------------------------------------------------------------------------//
void calc_size(t_n_conv *x)
{
  // size
  int i = x->size_i;

  i = (i > MAXSIZE)  ? MAXSIZE  : i;
  i = (i > x->l_a-1) ? x->l_a-1 : i;
  i = (i < 8)        ? 8        : i;

  i = i / x->bs;
  i = i * x->bs;

  x->size   = i;
  x->size_m = x->size / 8;

  // copy
  t_float *f_a = (t_float *)x->a;
  for (i=0; i<x->size; i++) 
    {
      f_a[i] = x->w_a[i].w_float;
    }

  for (i=0; i<x->size_m; i++)
    {
      int j = i*8;
      post("%f %f %f %f %f %f %f %f",
	   f_a[j+0],
	   f_a[j+1],
	   f_a[j+2],
	   f_a[j+3],
	   f_a[j+4],
	   f_a[j+5],
	   f_a[j+6],
	   f_a[j+7]);
    }

  // bs
  if (x->bs < 8 || x->bs%8 > 1)
    {
      x->on = 0;
      post("error: n_conv~: bad block size: %d", x->bs);
      post("error: n_conv~: block size must be greatly 8 and div 8 without rest");
    }
  else
    {
      x->on = 1;
    }

  // counts
  x->count_z = x->size - 1;
}

//----------------------------------------------------------------------------//
void n_conv_open(t_n_conv *x, t_symbol *s)
{
  // array
  x->s_a = s;
  x->l_a = pd_open_array(x->s_a, &x->w_a, &x->g_a);
  if (x->l_a < 8)
    {
      post("error: n_conv~: bad array or array size: %d", x->l_a);
      post("error: n_conv~: array size must be greatly 8");
    }

  // size
  calc_size(x);
}

//----------------------------------------------------------------------------//
void n_conv_size(t_n_conv *x, t_floatarg f)
{
  x->size_i = f;
  calc_size(x);
}

//----------------------------------------------------------------------------//
t_int *n_conv_perform(t_int *w)
{
  t_n_conv *x = (t_n_conv *)(w[1]);
  t_float *in = (t_float *)(w[2]);
  t_float *out = (t_float *)(w[3]);

  int i,j,k,h,m;
  float f, sum;  

/* __m256 _mm256_mul_ps (__m256 a, __m256 b) */
/* __m256 _mm256_add_ps (__m256 a, __m256 b) */
/* __m256 _mm256_load_ps (float const * mem_addr) */
/* void _mm256_store_ps (float * mem_addr, __m256 a) */

  if (x->on && x->l_a >= 8)
    {
      __m256 z;
      __m256 msum, msum2;
      float e[8];

      // z mult a 
      t_float *f_z = (t_float *)x->z;
      for (i=0; i<x->bs; i++)
	{

	  // write sample to z
	  f_z[x->count_z] = (t_float)*(in++);

	  // mult
	  for (k=0; k<x->size_m; k++)
	    {

	      h = (k*8);

	      for (j=0; j<8; j++)
		{
		  m = h + j + x->count_z;
		  if (m >= x->size)
		    m -= x->size;
		  e[j] = f_z[m];
		}

	      z = _mm256_set_ps(e[0], e[1], e[2], e[3],  e[4], e[5], e[6], e[7]);




	      x->s[k] = _mm256_mul_ps(x->a[k], z);
	    }

	  // sum
	  /* _m256_setzero_ps */
	  msum = _mm256_set_ps(0.0, 0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 0.0);
	  for (k=0; k<x->size_m; k++)
	    {
	      msum2 = _mm256_add_ps(msum, x->s[k]);
	      msum = msum2;
	    }

	  // to float
	  _mm256_store_ps(e, msum);
	  sum = e[0] + e[1] + e[2] + e[3] + e[4] + e[5] + e[6] + e[7];
	  *(out++) = sum;

	  // count
	  x->count_z--;
	  if (x->count_z < 0) 
	    x->count_z += x->size;
	}

    }

  return (w + 4);
}

//----------------------------------------------------------------------------//
void n_conv_dsp(t_n_conv *x, t_signal **sp)
{
  dsp_add(n_conv_perform,
          3,
          x,
          sp[0]->s_vec,
          sp[1]->s_vec);
  if (x->bs != sp[0]->s_n)
    {
      x->bs = sp[0]->s_n;
      calc_size(x);
    }
}

//----------------------------------------------------------------------------//
void *n_conv_new(t_symbol *s, int ac, t_atom *av)
{
  t_n_conv *x = (t_n_conv *)pd_new(n_conv_class);

  x->bs = 64;

  if (ac >= 1 && IS_A_SYMBOL(av, 0)) 
    {
      n_conv_open(x, atom_getsymbolarg(0,ac,av));
    }
  else
    {
      x->s_a = gensym("");
      x->l_a = 0;
    }
  IFARG(2, n_conv_size, 0);

  outlet_new(&x->x_obj, &s_signal);
  return (void *)x;
  if (s) {}
}

//----------------------------------------------------------------------------//
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
