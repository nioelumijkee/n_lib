// no reapet random number (stupid method)
#include "m_pd.h"
#include "include/parsearg.h"

#define NOUSE(X) if(X){};

static t_class *n_random_class;
typedef struct _n_random
{
  t_object x_obj;
  unsigned int range;
  unsigned int seed;
  unsigned int z;
} t_n_random;

inline void rnd(unsigned int *seed)
{
  *seed *= 1103515245;
  *seed += 12345;
}

void n_random_bang(t_n_random *x)
{
  unsigned int i;
  rnd(&x->seed);
  i = x->seed % x->range;
  if (i == x->z)
    {
      i += 1;
      i = i % x->range;
    }
  x->z = i;
  outlet_float(x->x_obj.ob_outlet,(t_float)i);
}

void n_random_float(t_n_random *x, t_floatarg f)
{
  x->range = (f<1)?1:f;
}

void n_random_seed(t_n_random *x, t_floatarg f)
{
  x->seed = f;
}

void *n_random_new(t_symbol *s, int ac, t_atom *av)
{
  t_n_random *x = (t_n_random *)pd_new(n_random_class);
  IFARGF(1, n_random_seed, 0);
  IFARGF(2, n_random_float, 1);
  outlet_new(&x->x_obj, 0);
  return (void *)x;
  NOUSE(s);
}

void n_random_setup(void)
{
  n_random_class=class_new(gensym("n_random"),
			   (t_newmethod)n_random_new,0,
			   sizeof(t_n_random),
			   0,A_GIMME,0);
  class_addbang(n_random_class,(t_method)n_random_bang);
  class_addfloat(n_random_class,(t_method)n_random_float);
  class_addmethod(n_random_class,(t_method)n_random_seed,gensym("seed"),A_FLOAT,0);
}
