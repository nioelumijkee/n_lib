#include "m_pd.h"
#include "g_all_guis.h"
#include "include/parsearg.h"

#define MAXSTEP 64

static t_class *n_euklid_class;
typedef struct _n_euklid
{
  t_object x_obj;
  t_outlet *out;
  int step;
  int hit;
  int acc;
  int hit_rot;
  int acc_rot;
} t_n_euklid;

// make Euklid rhythm's
void n_euklid_bang(t_n_euklid *x)
{
  int hit, acc, n, i, hi, ph, ac;
  t_atom a[MAXSTEP];
  hit = (x->hit > x->step) ? x->step : x->hit;
  acc = (x->acc > hit)     ? hit     : x->acc;
  n=0;
  for (i=0; i<x->step; i++)
    {
      hi = i * hit;
      hi = hi % x->step;
      ph = (i + x->hit_rot) % x->step;
      if (hi < hit)
	{
	  ac = n + x->acc_rot;
	  ac = ac % x->step;
	  ac = ac * acc;
	  ac = ac % hit;
	  if (ac < acc)
	    SETFLOAT(a+ph, (t_float)2);
	  else
	    SETFLOAT(a+ph, (t_float)1);
	  n++;
	}
      else
	SETFLOAT(a+ph, (t_float)0);
    }
  outlet_list(x->out, gensym("list"), x->step, a);
}

void n_euklid_step(t_n_euklid *x, t_floatarg f)
{
  x->step = (f<0)?0:(f>MAXSTEP)?MAXSTEP:f;
}

void n_euklid_hit(t_n_euklid *x, t_floatarg f)
{
  x->hit = (f<0)?0:(f>MAXSTEP)?MAXSTEP:f;
}

void n_euklid_acc(t_n_euklid *x, t_floatarg f)
{
  x->acc = (f<0)?0:(f>MAXSTEP)?MAXSTEP:f;
}

void n_euklid_hit_rot(t_n_euklid *x, t_floatarg f)
{
  x->hit_rot = (f<0)?0:(f>MAXSTEP)?MAXSTEP:f;
}

void n_euklid_acc_rot(t_n_euklid *x, t_floatarg f)
{
  x->acc_rot = (f<0)?0:(f>MAXSTEP)?MAXSTEP:f;
}

void *n_euklid_new(t_symbol *s, int ac, t_atom *av)
{
  t_n_euklid *x = (t_n_euklid *)pd_new(n_euklid_class);
  IFARGF(1, n_euklid_step, 16);
  IFARGF(2, n_euklid_hit, 4);
  IFARGF(3, n_euklid_acc, 1);
  IFARGF(4, n_euklid_hit_rot, 0);
  IFARGF(5, n_euklid_acc_rot, 0);
  x->out = outlet_new(&x->x_obj, 0);
  return (void *)x;
  if(s){};
}

void n_euklid_setup(void)
{
  n_euklid_class = class_new(gensym("n_euklid"), (t_newmethod)n_euklid_new, 0,
			     sizeof(t_n_euklid), CLASS_DEFAULT, A_GIMME, 0);
  class_addbang(n_euklid_class, (t_method)n_euklid_bang);
  class_addmethod(n_euklid_class,(t_method)n_euklid_step,gensym("step"),A_FLOAT,0);
  class_addmethod(n_euklid_class,(t_method)n_euklid_hit,gensym("hit"),A_FLOAT,0);
  class_addmethod(n_euklid_class,(t_method)n_euklid_acc,gensym("acc"),A_FLOAT,0);
  class_addmethod(n_euklid_class,(t_method)n_euklid_hit_rot,gensym("hit_rot"),A_FLOAT,0);
  class_addmethod(n_euklid_class,(t_method)n_euklid_acc_rot,gensym("acc_rot"),A_FLOAT,0);
}
