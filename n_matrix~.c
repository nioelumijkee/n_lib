#include <string.h>
#include "m_pd.h"
#include "include/parsearg.h"

#define MAX_IN  64 // MAX PD INLET ???
#define MAX_OUT 64 // MAX PD OUTLET ???
#define NOUSE(X) if(X){};

static t_class *n_matrix_class;

typedef struct _n_matrix
{
  t_object x_obj;
  t_int n;          /* block size */
  t_int all_in;     /* number in's */
  t_int all_out;    /* number out's */
  t_int all;        /* in's + out's */
  t_int **v_d;      /* vector for dsp_addv */
  t_float **l; /* level's */
  int *seq_in;
  int *seq_out;
  /* t_float l[MAX_IN][MAX_OUT]; /\* level's *\/ */
  /* int seq_in[MAX_SEQ]; */
  /* int seq_out[MAX_SEQ]; */
  int seq_len;
} t_n_matrix;

t_int *n_matrix_perform(t_int *w)
{
  int i;
  t_n_matrix *x = (t_n_matrix *)(w[1]);
  t_sample *sig_in[MAX_IN];  /* vector in's */
  t_sample *sig_out[MAX_OUT]; /* vector out's */
  for (i = 0; i < x->all_in; i++)
    {
      sig_in[i] = (t_sample *)(w[2 + i]);
    }
  for (i = 0; i < x->all_out; i++)
    {
      sig_out[i] = (t_sample *)(w[2 + i + x->all_in]);
    }
  int n;
  int n_in, n_out;
  t_float buf[MAX_IN];
  n = x->n;
  while (n--)
    {
      // copy to buffer
      for (n_in=0; n_in<x->all_in; n_in++)
	buf[n_in] = *(sig_in[n_in]++);
      // clear outs
      for (n_out=0; n_out<x->all_out; n_out++)
	*(sig_out[n_out]) = 0.0;
      // seq
      for (i=0; i<x->seq_len; i++)
        {
          *(sig_out[x->seq_out[i]]) += buf[x->seq_in[i]] *
            x->l[x->seq_in[i]][x->seq_out[i]];
        }
      // out
      for (n_out=0; n_out<x->all_out; n_out++)
	sig_out[n_out]++;
    }
  return (w + x->all + 2);
}

static void n_matrix_dsp(t_n_matrix *x, t_signal **sp)
{
  int i;
  x->n = sp[0]->s_n;
  x->v_d[0] = (t_int *)x;
  for (i = 0; i < x->all; i++)
    x->v_d[i + 1] = (t_int *)sp[i]->s_vec;
  dsp_addv(n_matrix_perform, x->all + 1, (t_int *)x->v_d);
}

static void n_matrix_debug(t_n_matrix *x)
{
  char buf[MAXPDSTRING];
  char b[MAXPDSTRING];
  int n_in, n_out;
  post("");
  for (n_in=0; n_in<x->all_in; n_in++)
    {
      b[0] = '\0';
      buf[0] = '\0';
      for (n_out=0; n_out<x->all_out; n_out++)
        {
          sprintf(b,"%s%5g | ",buf,x->l[n_in][n_out]);
          strcpy(buf,b);
        }
      post(buf);
    }
}

static void n_matrix_calc_seq(t_n_matrix *x)
{
  int n_in, n_out;
  x->seq_len = 0;
  for (n_out=0; n_out<x->all_out; n_out++)
    {
      for (n_in=0; n_in<x->all_in; n_in++)
        {
          if (x->l[n_in][n_out] != 0)
            {
              x->seq_in[x->seq_len] = n_in;
              x->seq_out[x->seq_len] = n_out;
              x->seq_len++;
            }
        }
    }
}

static void n_matrix_list(t_n_matrix *x, t_symbol *s, int ac, t_atom *av)
{
  int n_in  = atom_getfloatarg(0,ac,av);
  int n_out = atom_getfloatarg(1,ac,av);
  t_float l = atom_getfloatarg(2,ac,av);
  if (n_in  >= 0  &&  n_in  < x->all_in &&
      n_out >= 0  &&  n_out < x->all_out)
    x->l[n_in][n_out] = l;
  n_matrix_calc_seq(x);
  if (s) {};
}

static void *n_matrix_new(t_symbol *s, int ac, t_atom *av)
{ 
  int i,j;
  t_n_matrix *x = (t_n_matrix *)pd_new(n_matrix_class);
  x->all_in  = atom_getfloatarg(0,ac,av);
  x->all_out = atom_getfloatarg(1,ac,av);
  x->all_in  = (x->all_in>MAX_IN)?MAX_IN:(x->all_in<1)?1:x->all_in;
  x->all_out = (x->all_out>MAX_OUT)?MAX_OUT:(x->all_out<1)?1:x->all_out;
  x->all = x->all_in + x->all_out;
  for (i = 1; i < x->all_in; i++)
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
  for (i = 0; i < x->all_out; i++)
    outlet_new(&x->x_obj, &s_signal);
  // seq
  x->seq_in = getbytes(sizeof(t_int) * x->all_in);
  x->seq_out = getbytes(sizeof(t_int) * x->all_out);
  x->l = getbytes(sizeof(t_float *) * x->all_in);
  for (i=0; i<x->all_in; i++)
    {
      x->l[i] = getbytes(sizeof(t_float) * x->all_out);
      for (j=0; j<x->all_out; j++)
	{
	  x->l[i][j] = 0.0;
	}
    }
  x->v_d = getbytes(sizeof(t_int *) *(x->all + 2));
  return (x);
  NOUSE(s);
}

static void n_matrix_free(t_n_matrix *x)
{
  int i;
  freebytes(x->v_d, sizeof(t_int *) * (x->all + 2));
  freebytes(x->seq_in, sizeof(t_int) * x->all_in);
  freebytes(x->seq_out, sizeof(t_int) * x->all_out);
  for (i=0; i<x->all_in; i++)
    {
      freebytes(x->l[i], sizeof(t_float) * x->all_out);
    }
  freebytes(x->l, sizeof(t_float *) * x->all_in);
}

void n_matrix_tilde_setup(void)
{
  n_matrix_class = class_new(gensym("n_matrix~"),
			     (t_newmethod)n_matrix_new, 
                             (t_method)n_matrix_free, 
                             sizeof(t_n_matrix),
			     0, A_GIMME, 0);
  class_addmethod(n_matrix_class, nullfn, gensym("signal"), 0);
  class_addmethod(n_matrix_class, (t_method)n_matrix_dsp, gensym("dsp"), 0);
  class_addlist(n_matrix_class, (t_method)n_matrix_list);
  class_addmethod(n_matrix_class, (t_method)n_matrix_debug,gensym("debug"),0);
}
