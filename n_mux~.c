#include "m_pd.h"
#include "include/clip.h"

#define A_MAX_CH 32

static t_class *n_mux_class;

typedef struct _n_mux
{
  t_object x_obj;
  t_int n; /* block size */
  t_int **v_d; /* vector */
  int in_all;
  int all;
  int sel;
} t_n_mux;

//----------------------------------------------------------------------------//
t_int *n_mux_perform(t_int *w)
{
  t_n_mux *x = (t_n_mux *)(w[1]);
  t_sample *sig_in = (t_sample *)(w[x->sel + 2]);
  t_sample *sig_out = (t_sample *)(w[x->in_all + 2]);
  int n = x->n;
  
  while (n--)
    {
      *(sig_out++) = *(sig_in++);
    }
  
  return (w + x->all + 2);
}

//----------------------------------------------------------------------------//
void n_mux_dsp(t_n_mux *x, t_signal **sp)
{
  int i;
  x->n = sp[0]->s_n;
  x->v_d[0] = (t_int *)x;
  for (i = 0; i < x->all; i++)
    {
      x->v_d[i + 1] = (t_int *)sp[i]->s_vec;
    }
  dsp_addv(n_mux_perform, x->all + 1, (t_int *)x->v_d);
}

//----------------------------------------------------------------------------//
void n_mux_float(t_n_mux *x, t_float f)
{
  AF_CLIP_MINMAX(0, x->in_all - 1, f);
  x->sel = f;
}

//----------------------------------------------------------------------------//
void *n_mux_new(t_floatarg f)
{
  int i;
  t_n_mux *x = (t_n_mux *)pd_new(n_mux_class);
  AF_CLIP_MINMAX(1, A_MAX_CH, f);
  x->in_all = f;
  for (i = 1; i < x->in_all; i++)
    {
      inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    }
  outlet_new(&x->x_obj, &s_signal);

  x->all = x->in_all + 1;

  x->v_d = getbytes(sizeof(t_int *) *(x->all + 2));

  x->sel = 0;
  return (void *)x;
}

//----------------------------------------------------------------------------//
static void n_mux_free(t_n_mux *x)
{
  freebytes(x->v_d, sizeof(t_int *) * (x->all + 2));
}

//----------------------------------------------------------------------------//
void n_mux_tilde_setup(void)
{
  n_mux_class = class_new(gensym("n_mux~"), (t_newmethod)n_mux_new, (t_method)n_mux_free, sizeof(t_n_mux), 0, A_DEFFLOAT, 0);
  class_addmethod(n_mux_class, nullfn, gensym("signal"), 0);
  class_addmethod(n_mux_class, (t_method)n_mux_dsp, gensym("dsp"), 0);
  class_addfloat(n_mux_class, (t_method)n_mux_float);
}
