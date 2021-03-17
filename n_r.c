#include <m_pd.h>

static t_class *n_r_class;

typedef struct _n_r
{
  t_object x_obj;
  t_outlet *out;
  t_symbol *receive_name;
} t_n_r;

//----------------------------------------------------------------------------//
static void n_r_receive(t_n_r *x, t_symbol *s)
{
  // unbind
  if (x->receive_name->s_name[0] != '\0')
    pd_unbind(&x->x_obj.ob_pd, x->receive_name);
  
  // bind
  x->receive_name = s;
  if (x->receive_name->s_name[0] != '\0')
    pd_bind(&x->x_obj.ob_pd, x->receive_name);
}

//----------------------------------------------------------------------------//
static void n_r_bang(t_n_r *x)
{
  outlet_bang(x->out);
}

//----------------------------------------------------------------------------//
static void n_r_float(t_n_r *x, t_float f)
{
  outlet_float(x->out, f);
}

//----------------------------------------------------------------------------//
static void n_r_symbol(t_n_r *x, t_symbol *s)
{
  outlet_symbol(x->out, s);
}

//----------------------------------------------------------------------------//
static void n_r_pointer(t_n_r *x, t_gpointer *p)
{
  outlet_pointer(x->out, p);
}

//----------------------------------------------------------------------------//
static void n_r_list(t_n_r *x, t_symbol *s, int ac, t_atom *av)
{
  outlet_list(x->out, s, ac, av);
}

//----------------------------------------------------------------------------//
static void n_r_anything(t_n_r *x, t_symbol *s, int ac, t_atom *av)
{
  outlet_anything(x->out, s, ac, av);
}

//----------------------------------------------------------------------------//
static void *n_r_new(void)
{
  t_n_r *x = (t_n_r *)pd_new(n_r_class);
  x->out = outlet_new(&x->x_obj, 0);
  x->receive_name = gensym("");
  return (x);
}

//----------------------------------------------------------------------------//
static void n_r_ff(t_n_r *x)
{
  // unbind
  if (x->receive_name->s_name[0] != '\0')
    pd_unbind(&x->x_obj.ob_pd, x->receive_name);
}

//----------------------------------------------------------------------------//
void n_r_setup(void)
{
  n_r_class = class_new(gensym("n_r"), (t_newmethod)n_r_new, (t_method)n_r_ff, sizeof(t_n_r), 0, A_GIMME, 0);
  class_addbang(n_r_class, (t_method)n_r_bang);
  class_addfloat(n_r_class, (t_method)n_r_float);
  class_addsymbol(n_r_class, (t_method)n_r_symbol);
  class_addpointer(n_r_class, (t_method)n_r_pointer);
  class_addlist(n_r_class, (t_method)n_r_list);
  class_addanything(n_r_class, (t_method)n_r_anything);
  class_addmethod(n_r_class, (t_method)n_r_receive, gensym("receive"), A_SYMBOL, 0);
}
