#include <math.h>
#include "m_pd.h"
#include "include/p2f.h"

static t_class *n_p2f_class;
typedef struct _n_p2f
{
  t_object x_obj;
} t_n_p2f;

void n_p2f_float(t_n_p2f *x, t_floatarg p)
{
  p = p2f_clip(p);
  outlet_float(x->x_obj.ob_outlet, (t_float)p2f(p));
}

void n_p2f_calc(t_n_p2f *x, t_floatarg f)
{
  f = (exp2(((f / 4.) - 100. - 69.) / 12.)) * 440.;
  outlet_float(x->x_obj.ob_outlet, f);
}

void *n_p2f_new(void)
{
  t_n_p2f *x = (t_n_p2f *)pd_new(n_p2f_class);
  outlet_new(&x->x_obj, 0);
  return (void *)x;
}

void n_p2f_setup(void)
{
  n_p2f_class = class_new(gensym("n_p2f"),
			  (t_newmethod)n_p2f_new,
			  0,
			  sizeof(t_n_p2f),
			  0, 0, 0);
  class_addfloat(n_p2f_class, (t_method)n_p2f_float);
  class_addmethod(n_p2f_class, (t_method)n_p2f_calc, gensym("calc"), A_DEFFLOAT, 0);
}
