/* delay in samples */

#include "m_pd.h"
#include "include/parsearg.h"

static t_class *n_d_class;

typedef struct _n_d
{
  t_object x_obj;
  int bs;
  int count;
  int del;
  t_float *buf;
} t_n_d;

//----------------------------------------------------------------------------//
t_int *n_d_perform(t_int *w)
{
  t_n_d *x = (t_n_d *)(w[1]);
  t_float *in = (t_float *)(w[2]);
  t_float *out = (t_float *)(w[3]);
  int n = (int)(w[4]);

  int i;
  
  while (n--)
    {
      // write
      x->buf[x->count] = *(in++);

      // read
      i = x->count - x->del;
      if (i < 0) i += x->bs;
      *(out++) = x->buf[i];

      // count
      x->count++;
      if (x->count >= x->bs) x->count = 0;
    }

  return (w + 5);
}

//----------------------------------------------------------------------------//
void n_d_dsp(t_n_d *x, t_signal **sp)
{
  dsp_add(n_d_perform,
          4,
          x,
          sp[0]->s_vec,
          sp[1]->s_vec,
          sp[0]->s_n);
}

//----------------------------------------------------------------------------//
void n_d_float(t_n_d *x, t_float f)
{
  if      (f < 0)       f = 0;
  else if (f > x->bs-1) f = x->bs-1;
  x->del = f;
}

//----------------------------------------------------------------------------//
void n_d_bufsize(t_n_d *x, t_floatarg f)
{
  if (f < 1) f = 1;
  if (f != x->bs)
    {
      if (x->buf != NULL) freebytes(x->buf, sizeof(t_float) * x->bs);
      x->bs = f;
      x->count = 0;
      x->buf = getbytes(sizeof(t_float) * x->bs);
    }
}

//----------------------------------------------------------------------------//
void *n_d_new(t_symbol *s, int ac, t_atom *av)
{
  t_n_d *x = (t_n_d *)pd_new(n_d_class);
  x->buf = NULL;
  IFARG(1, n_d_bufsize, 512);
  IFARG(2, n_d_float, 0);
  outlet_new(&x->x_obj, &s_signal);
  return (void *)x;
  if (s) {};
}

//----------------------------------------------------------------------------//
static void n_d_free(t_n_d *x)
{
  freebytes(x->buf, sizeof(t_float) * x->bs);
}

//----------------------------------------------------------------------------//
void n_d_tilde_setup(void)
{
  n_d_class=class_new(gensym("n_d~"),(t_newmethod)n_d_new,
		      (t_method)n_d_free,
		      sizeof(t_n_d),0,A_GIMME,0);
  class_addmethod(n_d_class,nullfn,gensym("signal"),0);
  class_addmethod(n_d_class,(t_method)n_d_dsp,gensym("dsp"),0);
  class_addfloat(n_d_class,(t_method)n_d_float);
  class_addmethod(n_d_class,(t_method)n_d_bufsize,gensym("bs"),A_FLOAT,0);
}
