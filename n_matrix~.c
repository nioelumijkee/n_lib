#include <string.h>
#include "m_pd.h"

#define MAX_IN  32 // MAX PD INLET ???
#define MAX_OUT 32 // MAX PD OUTLET ???
#define MAX_SEQ 1024 // MAX_IN * MAX_OUT
#define MAX_VEC 66 // MAX_IN + MAX_OUT + 2
#define MODE_ADD 0
#define MODE_MULT 1

#define _clip_minmax(min, max, v) v=(v>max)?max:(v<min)?min:v;
#define _clip_min(min, v) v=(v<min)?min:v;
#define _clip_max(max, v) v=(v>max)?max:v;

#define NOUSE(x) if(x){};

static t_class *n_matrix_class;

typedef struct _n_matrix
{
  t_object x_obj;
  t_int bs;                     // block size 
  t_int all_in;                 // number in's 
  t_int all_out;                // number out's 
  t_int all;                    // in's + out's 
  t_int *v_d[MAX_VEC];          // vector for dsp_addv 
  t_float l[MAX_IN][MAX_OUT];   // level's 
  t_int s_in[MAX_SEQ];          // seq in
  t_int s_out[MAX_SEQ];         // seq out
  t_int s_len;                  // seq length
  int mode;
} t_n_matrix;

t_int *n_matrix_perform_add(t_int *w)
{
  int i,j;
  t_n_matrix *x = (t_n_matrix *)(w[1]);
  t_sample *sig_in[MAX_IN];
  t_sample *sig_out[MAX_OUT]; 
  for (i = 0; i < x->all_in; i++)
    sig_in[i] = (t_sample *)(w[2 + i]);
  for (i = 0; i < x->all_out; i++)
    sig_out[i] = (t_sample *)(w[2 + i + x->all_in]);
  t_float buf[MAX_IN];
  for (i=0; i<x->bs; i++)
    {
      // copy to buffer
      for (j=0; j<x->all_in; j++)
	buf[j] = sig_in[j][i];
      // clear outs
      for (j=0; j<x->all_out; j++)
	sig_out[j][i] = 0.0;
      // seq
      for (j=0; j<x->s_len; j++)
	sig_out[x->s_out[j]][i]+=buf[x->s_in[j]];
    }
  return (w + x->all + 2);
}

t_int *n_matrix_perform_mult(t_int *w)
{
  int i,j;
  t_n_matrix *x = (t_n_matrix *)(w[1]);
  t_sample *sig_in[MAX_IN];
  t_sample *sig_out[MAX_OUT]; 
  for (i = 0; i < x->all_in; i++)
    sig_in[i] = (t_sample *)(w[2 + i]);
  for (i = 0; i < x->all_out; i++)
    sig_out[i] = (t_sample *)(w[2 + i + x->all_in]);
  t_float buf[MAX_IN];
  for (i=0; i<x->bs; i++)
    {
      // copy to buffer
      for (j=0; j<x->all_in; j++)
	buf[j] = sig_in[j][i];
      // clear outs
      for (j=0; j<x->all_out; j++)
	sig_out[j][i] = 0.0;
      // seq
      for (j=0; j<x->s_len; j++)
	sig_out[x->s_out[j]][i]+=buf[x->s_in[j]] * x->l[x->s_in[j]][x->s_out[j]];
    }
  return (w + x->all + 2);
}

static void n_matrix_dsp(t_n_matrix *x, t_signal **sp)
{
  x->bs = sp[0]->s_n;
  x->v_d[0] = (t_int *)x;
  for (int i = 0; i < x->all; i++)
    x->v_d[i + 1] = (t_int *)sp[i]->s_vec;
  if (x->mode==MODE_ADD)
    dsp_addv(n_matrix_perform_add, x->all + 1, (t_int *)x->v_d);
  else
    dsp_addv(n_matrix_perform_mult, x->all + 1, (t_int *)x->v_d);
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
          sprintf(b,"%s%-4.2f|",buf,x->l[n_in][n_out]);
          strcpy(buf,b);
        }
      post(buf);
    }
}

static void n_matrix_calc_seq(t_n_matrix *x)
{
  x->s_len = 0;
  for (int out=0; out<x->all_out; out++)
    {
      for (int in=0; in<x->all_in; in++)
        {
          if (x->l[in][out] != 0)
            {
              x->s_in[x->s_len] = in;
              x->s_out[x->s_len] = out;
              x->s_len++;
            }
        }
    }
}

static void n_matrix_set(t_n_matrix *x, t_floatarg in, t_floatarg out, t_floatarg l)
{
  if (in  >= 0  &&
      in  < x->all_in &&
      out >= 0  &&
      out < x->all_out)
    x->l[(int)in][(int)out] = l;
  n_matrix_calc_seq(x);
}

static void *n_matrix_new(t_symbol *s, int ac, t_atom *av)
{ 
  t_n_matrix *x = (t_n_matrix *)pd_new(n_matrix_class);
  if (ac >= 1)
    {
      t_symbol *mode = atom_getsymbolarg(0,ac,av);
      if (mode->s_name[0] == 'a')
	x->mode=MODE_ADD;
      else
	x->mode=MODE_MULT;
    }
  else
    x->mode=MODE_MULT;
  if (ac >= 2)
    {
      x->all_in  = atom_getfloatarg(1,ac,av);
      _clip_minmax(1, MAX_IN, x->all_in);
    }
  else
    x->all_in = 1;
  if (ac >= 3)
    {
      x->all_out = atom_getfloatarg(2,ac,av);
      _clip_minmax(1, MAX_OUT, x->all_in);
    }
  else
    x->all_out = 1;
  x->all = x->all_in + x->all_out;
  // create inlet/outlet
  for (int i = 1; i < x->all_in; i++)
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
  for (int i = 0; i < x->all_out; i++)
    outlet_new(&x->x_obj, &s_signal);
  return (x);
  NOUSE(s);
}

void n_matrix_tilde_setup(void)
{
  n_matrix_class = class_new(gensym("n_matrix~"),
			     (t_newmethod)n_matrix_new, 
                             0,
                             sizeof(t_n_matrix),
			     0, A_GIMME, 0);
  class_addmethod(n_matrix_class, nullfn, gensym("signal"), 0);
  class_addmethod(n_matrix_class, (t_method)n_matrix_dsp, gensym("dsp"), 0);
  class_addmethod(n_matrix_class, (t_method)n_matrix_set, gensym("set"), 
		  A_FLOAT, A_FLOAT, A_FLOAT, 0);
  class_addmethod(n_matrix_class, (t_method)n_matrix_debug,gensym("debug"),0);
}
