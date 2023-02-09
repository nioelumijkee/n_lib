#include "m_pd.h"

static t_class *n_print_class;

typedef struct _n_print
{
  t_object x_obj;
  t_float level;
} t_n_print;

static t_symbol *n_print_args2symbol(int argc, t_atom *argv)
{
  t_symbol *s;
  char *buf;
  int bufsize;
  t_binbuf *bb = binbuf_new();
  binbuf_add(bb, argc, argv);
  binbuf_gettext(bb, &buf, &bufsize);
  buf = resizebytes(buf, bufsize, bufsize + 1);
  buf[bufsize] = 0;
  s = gensym(buf);
  freebytes(buf, bufsize + 1);
  binbuf_free(bb);
  return (s);
}

static void n_print_bang(t_n_print *x)
{
  logpost(x,
	  (const int)x->level,
	  "bang");
}

static void n_print_float(t_n_print *x, t_float f)
{
  logpost(x,
	  (const int)x->level,
	  "%g",
	  f);
}

static void n_print_symbol(t_n_print *x, t_symbol *s)
{
  logpost(x,
	  (const int)x->level,
	  "%s",
	  s->s_name);
}

static void n_print_list(t_n_print *x, t_symbol *s, int argc, t_atom *argv)
{
  t_symbol *output = n_print_args2symbol(argc, argv);
  logpost(x,
	  (const int)x->level,
	  "%s %s",
	  s->s_name,
	  output->s_name);
}

static void n_print_anything(t_n_print *x, t_symbol *s, int argc, t_atom *argv)
{
  t_symbol *output = n_print_args2symbol(argc, argv);
  logpost(x,
	  (const int)x->level,
	  "%s %s",
	  s->s_name,
	  output->s_name);
}

void *n_print_new(t_floatarg f)
{
  t_n_print *x = (t_n_print *)pd_new(n_print_class);
  x->level = f;
  floatinlet_new(&x->x_obj, &x->level);
  return (void *)x;
}

void n_print_setup(void)
{
  n_print_class = class_new(gensym("n_print"),
			    (t_newmethod)n_print_new,
			    0,
			    sizeof(t_n_print),
			    0, A_DEFFLOAT, 0);
  class_addbang(n_print_class, (t_method)n_print_bang);
  class_addfloat(n_print_class, (t_method)n_print_float);
  class_addsymbol(n_print_class, (t_method)n_print_symbol);
  class_addlist(n_print_class, (t_method)n_print_list);
  class_addanything(n_print_class, (t_method)n_print_anything);
}
