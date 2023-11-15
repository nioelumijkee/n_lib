/*
  GrainCloude - granular synthesis.
*/

#include <math.h>
#include "m_pd.h"
#include "include/pdfunc.h"

#define PI 3.1415927

#define GRAINS 64
#define ENVTSIZE 512

#define _clip_minmax(min, max, v) v=(v>max)?max:(v<min)?min:v;
#define _clip_min(min, v) v=(v<min)?min:v;
#define _clip_max(max, v) v=(v>max)?max:v;

#define _read_ar(x, A, out) {					    \
    int k = x;							    \
    out = x - k;						    \
    out = (A[k + 1] - A[k]) * out + A[k];			    \
  }

static t_class *n_graincloud_class;

typedef struct _n_grain
{
  int on;
  t_float phase;
  t_float length;
  t_float div_1_length;
  t_float start;
  t_float speed;
  t_float a_l;
  t_float a_r;
  t_float shape;
} t_n_grain;

typedef struct _n_graincloud
{
  t_object x_obj;
  t_symbol *s_a; /* sample array */
  t_word   *w_a;
  t_garray *g_a;
  int       l_a;
  int       l_a_1;
  t_n_grain g[GRAINS]; /* grains */
} t_n_graincloud;

t_float env_table[ENVTSIZE];

//----------------------------------------------------------------------------//
void n_graincloud_calc_constant(t_n_graincloud *x)
{
  int i;
  for (i=0; i<GRAINS; i++)
    {
      x->g[i].on = 0;
    }
}

void n_graincloud_array(t_n_graincloud *x, t_symbol *s)
{
  x->s_a = s;
  x->l_a = pd_open_array(x->s_a, &x->w_a, &x->g_a);
  x->l_a_1 = x->l_a - 1;
  if (x->l_a <= 0)
    post("error: n_graincloud: array: len = %d", x->l_a);
}

// start grain
void n_graincloud_s(t_n_graincloud *x, t_symbol *s, int ac, t_atom *av)
{
  t_float f;

  // number
  int i  = atom_getfloatarg(0, ac, av);
  _clip_minmax(0, GRAINS - 1, i);
  x->g[i].on = 1;
  
  // phase
  x->g[i].phase = 0.0;

  // level
  t_float level = atom_getfloatarg(1, ac, av);

  // pos
  x->g[i].start = atom_getfloatarg(2, ac, av);

  // len
  x->g[i].length = atom_getfloatarg(3, ac, av);
  _clip_min(1., x->g[i].length);
  x->g[i].div_1_length = 1. / x->g[i].length;

  // speed
  x->g[i].speed = atom_getfloatarg(4, ac, av);
  _clip_min(0.01, x->g[i].speed);

  // pan
  t_float pan = atom_getfloatarg(5, ac, av);
  _clip_minmax(-1., 1., pan);

  // pan
  f = 1. - pan;
  x->g[i].a_l = (4.0 - f) * f * 0.33333 * level;
  f = 1. + pan;
  x->g[i].a_r = (4.0 - f) * f * 0.33333 * level;

  // shape
  f = atom_getfloatarg(6, ac, av);
  _clip_minmax(0., 1.,  f);
  x->g[i].shape = ((f*f) * 16.) + 1.;

  //
  return;
  if(s){};
}

// calc env
void envtable(void)
{
  int i;
  t_float f;
  for (i=0; i<ENVTSIZE; i++)
    {
      f = (t_float)i / (ENVTSIZE-1.0);
      f = sin(f * PI);
      env_table[i] = f;
    }
}

//----------------------------------------------------------------------------//
#define W(I) x->w_a[(I)].w_float
t_int *n_graincloud_perform(t_int *w)
{
  t_n_graincloud *x = (t_n_graincloud *)(w[1]);
  t_float *out_l = (t_float *)(w[2]);
  t_float *out_r = (t_float *)(w[3]);
  int n = (int)(w[4]);
  int i,j;
  t_float pos;
  t_float sum_l;
  t_float sum_r;
  t_float f;
  t_float env;
  while (n--)
    {
      sum_l=0.0; // reset
      sum_r=0.0;
      for (i=0; i<GRAINS; i++)
        {
          if (x->g[i].on)
            {
              // read
              pos = x->g[i].phase + x->g[i].start;
              if      (pos < 0)        pos = 0;
              else if (pos > x->l_a_1) pos = x->l_a_1;
              
              // env
              f = x->g[i].phase * x->g[i].div_1_length;
              f = f * ENVTSIZE;
              _read_ar(f, env_table, env);

	      // shape
	      env = env * x->g[i].shape;
	      if      (env > 1.0) env = 1.0;
	      else if (env < 0.0) env = 0.0;
              
              // grain
              j = pos;
              f = pos - j;
              f = (W(j+1) - W(j)) * f + W(j);
              f = f * env;
              
              // pan
              sum_l += f * x->g[i].a_l;
              sum_r += f * x->g[i].a_r;
              
              // phase
              x->g[i].phase += x->g[i].speed;
              if (x->g[i].phase >= x->g[i].length)
                {
                  x->g[i].phase = x->g[i].length;
                  x->g[i].on = 0;
                }
            }
        }
      // out
      *(out_l++) = sum_l;
      *(out_r++) = sum_r;
    }
  return (w + 5);
}

void n_graincloud_dsp(t_n_graincloud *x, t_signal **sp)
{
  dsp_add(n_graincloud_perform,
          4,
          x,
          sp[0]->s_vec,
          sp[1]->s_vec,
          sp[0]->s_n);
}

//----------------------------------------------------------------------------//
void *n_graincloud_new(void)
{
  t_n_graincloud *x = (t_n_graincloud *)pd_new(n_graincloud_class);
  n_graincloud_calc_constant(x);
  outlet_new(&x->x_obj, &s_signal);
  outlet_new(&x->x_obj, &s_signal);
  x->s_a = gensym("");
  x->l_a = 0;
  x->l_a_1 = 0;
  return (void *)x;
}

void n_graincloud_tilde_setup(void)
{
  n_graincloud_class = class_new(gensym("n_graincloud~"),
				 (t_newmethod)n_graincloud_new,
				 0,
				 sizeof(t_n_graincloud),
				 0, A_GIMME,0);
  class_addmethod(n_graincloud_class,(t_method)n_graincloud_dsp,gensym("dsp"),0);
  class_addmethod(n_graincloud_class,(t_method)n_graincloud_array,gensym("array"),
		  A_SYMBOL,0);
  class_addmethod(n_graincloud_class,(t_method)n_graincloud_s,gensym("s"),A_GIMME,0);
  envtable();
}
