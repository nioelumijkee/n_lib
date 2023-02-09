#include "m_pd.h"

static t_class *n_peak_class;

typedef struct _n_peak {
  t_object x_obj;
  t_outlet *x_outlet;
  t_float max;
} t_n_peak;


void n_peak_out(t_n_peak *x)
{
  outlet_float(x->x_outlet, x->max);
}

void n_peak_reset(t_n_peak *x)
{
  x->max = 0.;
}

void n_peak_bang(t_n_peak *x)
{
  n_peak_out(x);
  n_peak_reset(x);
}

t_int *n_peak_perform(t_int *w)
{
  t_n_peak *x = (t_n_peak *)(w[1]);
  t_float *in     = (t_float *)(w[2]);
  int n           = (int)(w[3]);
  t_float a;
  t_float max = x->max;
  while (n--)
    {
      a = *(in++);
      if (a < 0)   a = 0.0 - a;
      if (a > max) max = a;
    }
  x->max = max;
  return (w+4);
}

void n_peak_dsp(t_n_peak *x, t_signal **sp)
{
  dsp_add(n_peak_perform,
	  3,
	  x,
	  sp[0]->s_vec,
	  sp[0]->s_n);
  n_peak_reset(x);
}

void *n_peak_new(void)
{
  t_n_peak *x = (t_n_peak *)pd_new(n_peak_class);
  n_peak_reset(x);
  x->x_outlet = outlet_new(&x->x_obj, &s_float);
  return (void *)x;
}


void n_peak_tilde_setup(void)
{
  n_peak_class = class_new(gensym("n_peak~"),
			   (t_newmethod)n_peak_new,
			   0,
			   sizeof(t_n_peak),
			   0, 0, 0);
  class_addmethod(n_peak_class, nullfn, gensym("signal"), 0);
  class_addmethod(n_peak_class,(t_method)n_peak_dsp, gensym("dsp"), 0);
  class_addbang(n_peak_class,(t_method)n_peak_bang);
  class_addmethod(n_peak_class, (t_method)n_peak_out,gensym("out"),0);
  class_addmethod(n_peak_class, (t_method)n_peak_reset,gensym("reset"),0);
}
