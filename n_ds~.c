/* simply drum sampler */

#include "m_pd.h"
#include "include/parsearg.h"
#include "include/pdfunc.h"

#define MIN_INC 0.001

static t_class *n_ds_class;
typedef struct _n_ds
{
  t_object  x_obj;
  int       on;
  t_float   len_f; // len first in samples
  t_float   inc_f; // inc first
  t_float   inc_l; // inc last
  t_float   start; // start
  t_float   ph;
  int       a_l;
  int       a_l_2;
  t_word   *a_w;
  t_garray *a_g;
} t_n_ds;

void n_ds_reset(t_n_ds *x)
{
  x->on=0;
  x->ph=0;
}

void n_ds_bang(t_n_ds *x)
{
  x->on = 1;
  x->ph = x->start;
}

void n_ds_start(t_n_ds *x, t_floatarg f)
{
  x->start = (f<0)?0:f;
}

void n_ds_len_f(t_n_ds *x, t_floatarg f)
{
  x->len_f = (f<0)?0:f;
}

void n_ds_inc_f(t_n_ds *x, t_floatarg f)
{
  x->inc_f = (f<MIN_INC)?MIN_INC:f;
}

void n_ds_inc_l(t_n_ds *x, t_floatarg f)
{
  x->inc_l = (f<MIN_INC)?MIN_INC:f;
}

void n_ds_array(t_n_ds *x, t_symbol *s)
{
  x->a_l = pd_open_array(s, &x->a_w, &x->a_g);
  x->a_l_2 = x->a_l - 2;
  n_ds_reset(x);
}

t_int *n_ds_perform(t_int *w)
{
  t_n_ds *x = (t_n_ds *)(w[1]);
  t_float *out = (t_float *)(w[2]);
  int n = (int)(w[3]);
  int i;
  t_float f;
  if (x->on)
    {
      while (n--)
        {
	  // sampler
	  if (x->ph > x->a_l_2) 
	    {
	      x->ph =  x->a_l_2;
	      x->on = 0;
	    }
	  else if (x->ph > x->len_f) x->ph += x->inc_l;
	  else                       x->ph += x->inc_f;
	  // read
	  i = x->ph;
	  f = x->ph - i;
	  *(out++) = (x->a_w[i + 1].w_float - x->a_w[i].w_float) * f + x->a_w[i].w_float;
        }
    }
  else { while (n--) { *(out++) = 0.; } }
  return (w + 4);
}

void n_ds_dsp(t_n_ds *x, t_signal **sp)
{
  dsp_add(n_ds_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}

void *n_ds_new(t_symbol *s, int ac, t_atom *av)
{
  t_n_ds *x = (t_n_ds *)pd_new(n_ds_class);
  IFARGS(1, n_ds_array, gensym(""));
  IFARGF(2, n_ds_start, 0);
  IFARGF(3, n_ds_len_f, 0);
  IFARGF(4, n_ds_inc_f, 1);
  IFARGF(5, n_ds_inc_l, 0);
  outlet_new(&x->x_obj, &s_signal);
  return (void *)x;
  if(s){};
}

void n_ds_tilde_setup(void)
{
  n_ds_class = class_new(gensym("n_ds~"),
			 (t_newmethod)n_ds_new,
			 0,
			 sizeof(t_n_ds),
			 0, A_GIMME, 0);
  class_addmethod(n_ds_class,(t_method)n_ds_dsp,gensym("dsp"),0); 
  class_addbang(n_ds_class, (t_method)n_ds_bang);
  class_addmethod(n_ds_class,(t_method)n_ds_array,gensym("array"),A_SYMBOL,0);
  class_addmethod(n_ds_class,(t_method)n_ds_start,gensym("start"),A_FLOAT,0);
  class_addmethod(n_ds_class,(t_method)n_ds_len_f,gensym("len_f"),A_FLOAT,0);
  class_addmethod(n_ds_class,(t_method)n_ds_inc_f,gensym("inc_f"),A_FLOAT,0);
  class_addmethod(n_ds_class,(t_method)n_ds_inc_l,gensym("inc_l"),A_FLOAT,0);
}
