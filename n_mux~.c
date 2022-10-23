/* from zexy */

#include "m_pd.h"
#include "include/clip.h"

#define A_MAX_CH 32

static t_class *n_mux_class;

typedef struct _n_mux
{
  t_object x_obj;
  int input;
  int n_in;
  t_sample **in;
} t_n_mux;

//----------------------------------------------------------------------------//
static t_int *n_mux_perform(t_int *w)
{
  t_n_mux *x = (t_n_mux *)(w[1]);
  t_sample *out = (t_sample *)(w[2]);
  int n = (int)(w[3]);
  t_sample *in = x->in[x->input];
  while(n--)*out++=*in++;
  return (w+4);
}

//----------------------------------------------------------------------------//
static void n_mux_dsp(t_n_mux *x, t_signal **sp)
{
  int n = 0;
  t_sample **dummy = x->in;
  for(n=0;n<x->n_in;n++)
    {    
      *dummy++ = sp[n]->s_vec;
    }
  dsp_add(n_mux_perform, 3, x, sp[n]->s_vec, sp[0]->s_n);
}

//----------------------------------------------------------------------------//
static void n_mux_float(t_n_mux *x, t_float f)
{
  CLIP_MINMAX(0, x->n_in - 1, f);
  x->input = f;
}

//----------------------------------------------------------------------------//
static void *n_mux_new(t_floatarg f)
{
  int i;
  t_n_mux *x = (t_n_mux *)pd_new(n_mux_class);
  CLIP_MINMAX(1, A_MAX_CH, f);
  x->n_in = f;
  for (i = 1; i < x->n_in; i++)
    {
      inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    }
  outlet_new(&x->x_obj, &s_signal);
  x->in = (t_sample **)getbytes(x->n_in * sizeof(t_sample *));
  i=x->n_in;
  while(i--)x->in[i]=0;
  x->input = 0;
  return (void *)x;
}

//----------------------------------------------------------------------------//
static void n_mux_free(t_n_mux *x)
{
  freebytes(x->in, x->n_in * sizeof(t_sample *));
}

//----------------------------------------------------------------------------//
void n_mux_tilde_setup(void)
{
  n_mux_class=class_new(gensym("n_mux~"),(t_newmethod)n_mux_new,
			(t_method)n_mux_free,sizeof(t_n_mux),0,A_DEFFLOAT,0);
  class_addmethod(n_mux_class,nullfn,gensym("signal"),0);
  class_addmethod(n_mux_class,(t_method)n_mux_dsp,gensym("dsp"),0);
  class_addfloat(n_mux_class,(t_method)n_mux_float);
}
