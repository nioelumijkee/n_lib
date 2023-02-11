#include "m_pd.h"
#include "include/parsearg.h"
#include "include/pdfunc.h"

#define MINLENREC 8
#define MAXLENREC 8192
#define MAXLENREC_1 8191
#define NOUSE(X)if(X){};

static t_class *n_rec1p_class;

typedef struct _n_rec1p
{
  t_object x_obj;
  t_outlet *out;
  t_float buf[MAXLENREC];
  int mode;
  int min_len;
  int rec;
  int rec_count;
  int find;
  int play_buf;
  int play_len;
  int play_count;
  t_float tr;
  t_float z;
  t_symbol *a_s;
} t_n_rec1p;


void n_rec1p_reset(t_n_rec1p *x)
{
  x->rec_count = 0;
  x->find = 1;
  if (x->mode) x->z = -99999;
  else         x->z = 99999;
  x->play_buf = 0;
  x->play_len = 0;
  x->play_count = 0;
}

void n_rec1p_record(t_n_rec1p *x)
{
  x->rec = 1;
  n_rec1p_reset(x);
  outlet_float(x->out, 1);
}

void n_rec1p_min_len(t_n_rec1p *x, t_floatarg f)
{
  x->min_len = (f>MAXLENREC_1)?MAXLENREC_1:(f<MINLENREC)?MINLENREC:f;
  n_rec1p_reset(x);
}

void n_rec1p_treshold(t_n_rec1p *x, t_floatarg f)
{
  x->tr = f;
  n_rec1p_reset(x);
}

// 0 rise
// 1 fall
void n_rec1p_mode(t_n_rec1p *x, t_floatarg f)
{
  x->mode = (f>0);
  n_rec1p_reset(x);
}

void n_rec1p_array(t_n_rec1p *x, t_symbol *s)
{
  x->a_s = s;
}

void n_rec1p_copy_to_array(t_n_rec1p *x)
{
  if (x->a_s->s_name[0] == '\0') return;
  int i;
  int       a_l;
  t_word   *a_w;
  t_garray *a_g;
  a_l = pd_open_array(x->a_s, &a_w, &a_g);
  if (a_l<1) return;
  if (a_l != x->rec_count)
    garray_resize(a_g, x->rec_count);
  a_l = pd_open_array(x->a_s, &a_w, &a_g);
  if (a_l != x->rec_count) {post("n_rec1p: error resize: %s", x->a_s->s_name); return;}
  for(i=0; i<x->rec_count; i++)
    a_w[i].w_float = x->buf[i];
  garray_redraw(a_g);
}

t_int *n_rec1p_perform(t_int *w)
{
  t_n_rec1p *x = (t_n_rec1p *)(w[1]);
  t_float *in = (t_float *)(w[2]);
  t_float *out = (t_float *)(w[3]);
  int n = (int)(w[4]);
  t_float a;
  while (n--)
    {
      a = *(in++);
      // record
      if (x->rec)
	{
	  if (x->find) // find
	    {
	      if (((x->mode == 1) && a <= x->tr && x->z >  x->tr) || // fall
		  ((x->mode == 0) && a >  x->tr && x->z <= x->tr))   // rise
		x->find = 0;
	    }
	  if (x->find == 0) // find complete
	    {
	      if (x->rec_count < x->min_len)
		{
		  x->buf[x->rec_count] = a;
		  x->rec_count++;
		}
	      // find
	      else
		{
		  if (((x->mode == 1) && a <= x->tr && x->z >  x->tr) || // fall
		      ((x->mode == 0) && a >  x->tr && x->z <= x->tr))   // rise
		    {
		      x->rec = 0;
		      x->play_buf = 1;
		      x->play_count = 0;
		      x->play_len = x->rec_count;
		      n_rec1p_copy_to_array(x);
		      outlet_float(x->out, 0);
		    }
		  else
		    {
		      x->buf[x->rec_count] = a;
		      x->rec_count++;
		    }
		}
	      if (x->rec_count >= MAXLENREC_1) x->rec_count = MAXLENREC_1;
	    }
	  x->z = a;
	}
      // play
      if (x->play_buf)
	{
	  *(out)++ = x->buf[x->play_count];
	  x->play_count++;
	  if (x->play_count >= x->play_len)
	    x->play_count = 0;
	}
      else
	{
	  *(out)++ = a;
	}
    }

  return (w+5);
}

void n_rec1p_dsp(t_n_rec1p *x, t_signal **sp)
{
  dsp_add(n_rec1p_perform,
	  4,
	  x,
	  sp[0]->s_vec,
	  sp[1]->s_vec,
	  sp[0]->s_n);
}

void *n_rec1p_new(t_symbol *s, int ac, t_atom *av)
{
  t_n_rec1p *x = (t_n_rec1p *)pd_new(n_rec1p_class);
  IFARGS(1, n_rec1p_array, gensym(""));
  IFARGF(2, n_rec1p_min_len, 128);
  IFARGF(3, n_rec1p_treshold, 0);
  IFARGF(4, n_rec1p_mode, 0);
  outlet_new(&x->x_obj, &s_signal);
  x->out = outlet_new(&x->x_obj, 0);
  return (void *)x;
  NOUSE(s);
}

void n_rec1p_tilde_setup(void)
{
  n_rec1p_class = class_new(gensym("n_rec1p~"),
			      (t_newmethod)n_rec1p_new,
			      0,
			      sizeof(t_n_rec1p),
			      0, A_GIMME, 0);
  class_addmethod(n_rec1p_class,nullfn,gensym("signal"),0);
  class_addmethod(n_rec1p_class,(t_method)n_rec1p_dsp,gensym("dsp"),0);
  class_addbang(n_rec1p_class, (t_method)n_rec1p_record);
  class_addmethod(n_rec1p_class,(t_method)n_rec1p_min_len,gensym("min_len"),A_FLOAT,0);
  class_addmethod(n_rec1p_class,(t_method)n_rec1p_treshold,gensym("treshold"),A_FLOAT,0);
  class_addmethod(n_rec1p_class,(t_method)n_rec1p_mode,gensym("mode"),A_FLOAT,0);
  class_addmethod(n_rec1p_class,(t_method)n_rec1p_record,gensym("array"),A_SYMBOL,0);
}
