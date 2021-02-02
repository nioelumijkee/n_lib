#include "m_pd.h"
#include "include/math.h"


#define A_MAX_CH 32

static t_class *n_demux_class;

typedef struct _n_demux
{
  t_object x_obj;
  t_int n; /* block size */
  t_int **v_d; /* vector */
  int in_all;
  int all;
  int sel;
} t_n_demux;

//----------------------------------------------------------------------------//
t_int *n_demux_perform(t_int *w)
{
  int i,n;
  t_n_demux *x = (t_n_demux *)(w[1]);
  t_sample *sig_in = (t_sample *)(w[2]);
  t_sample *sig_out;
  
  n = x->n;
  sig_out = (t_sample *)(w[3 + x->sel]);
  while (n--)
    {
      *(sig_out++) = *(sig_in++);
    }
  
  
  // zero
  for(i = 0; i < x->in_all; i++)
    {
      n = x->n;
      sig_out = (t_float *)(w[3 + i]);
      if (i != x->sel)
        {
	  while (n--)
            {
	      *(sig_out++) = 0.;
            }
        }
    }
  
  return (w + x->all + 2);
}

//----------------------------------------------------------------------------//
void n_demux_dsp(t_n_demux *x, t_signal **sp)
{
  int i;
  x->n = sp[0]->s_n;
  x->v_d[0] = (t_int *)x;
  for (i = 0; i < x->all; i++)
    {
      x->v_d[i + 1] = (t_int *)sp[i]->s_vec;
    }
  dsp_addv(n_demux_perform, x->all + 1, (t_int *)x->v_d);
}

//----------------------------------------------------------------------------//
void n_demux_float(t_n_demux *x, t_float f)
{
  AF_CLIP_MINMAX(0, x->in_all - 1, f);
  x->sel = f;
}

//----------------------------------------------------------------------------//
void *n_demux_new(t_floatarg f)
{
  int i;
  t_n_demux *x = (t_n_demux *)pd_new(n_demux_class);
  AF_CLIP_MINMAX(1, A_MAX_CH, f);
  x->in_all = f;
  for (i = 0; i < x->in_all; i++)
    outlet_new(&x->x_obj, &s_signal);
  x->all = x->in_all + 1;
  
  x->v_d = getbytes(sizeof(t_int *) *(x->all + 2));

  x->sel = 0;
  return (void *)x;
}

//----------------------------------------------------------------------------//
static void n_demux_free(t_n_demux *x)
{
  freebytes(x->v_d, sizeof(t_int *) * (x->all + 2));
}

//----------------------------------------------------------------------------//
void n_demux_tilde_setup(void)
{
  n_demux_class = class_new(gensym("n_demux~"), (t_newmethod)n_demux_new, (t_method)n_demux_free, sizeof(t_n_demux), 0, A_DEFFLOAT, 0);
  class_addmethod(n_demux_class, nullfn, gensym("signal"), 0);
  class_addmethod(n_demux_class, (t_method)n_demux_dsp, gensym("dsp"), 0);
  class_addfloat(n_demux_class, (t_method)n_demux_float);
}
