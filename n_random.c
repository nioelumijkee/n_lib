#include "m_pd.h"
#include <time.h>
#include "include/random.h"

static t_class *n_random_class;
typedef struct _n_random
{
  t_object x_obj;
  unsigned int seed;
  unsigned int range;
  unsigned int z;
} t_n_random;

//----------------------------------------------------------------------------//
void n_random_bang(t_n_random *x)
{
  unsigned int i;
  AF_RANDOM(x->seed);
  i = x->seed % x->range;
  /* not repeat */
  if (i == x->z)
    {
      i += x->z;
      i = i % x->range;
    }
  x->z = i;
  if (x->z == 0)
    {
      AF_RANDOM(x->seed);
      x->z = x->seed % x->range;
    }
  outlet_float(x->x_obj.ob_outlet,(t_float)i);
}

//----------------------------------------------------------------------------//
void n_random_float(t_n_random *x, t_floatarg f)
{
  x->range = f;
  if (x->range == 0) 
    {    
      x->range = 1;
    }
}

//----------------------------------------------------------------------------//
void *n_random_new(t_floatarg f)
{
  t_n_random *x = (t_n_random *)pd_new(n_random_class);
  time_t lt = time(NULL);
  x->seed = (long)x; /* random seed for varioius object in path */
  x->seed += lt;
  n_random_float(x, f);
  outlet_new(&x->x_obj, 0);
  return (void *)x;
}

//----------------------------------------------------------------------------//
void n_random_setup(void)
{
  n_random_class = class_new(gensym("n_random"), (t_newmethod)n_random_new, 0, sizeof(t_n_random), CLASS_DEFAULT, A_DEFFLOAT, 0);
  class_addbang(n_random_class, (t_method)n_random_bang);
  class_addfloat(n_random_class, (t_method)n_random_float);
}
