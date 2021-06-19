#include "m_pd.h"
#include "g_canvas.h"

static t_class *n_cnvrcv_class;

typedef struct _n_cnvrcv
{
  t_object x_obj;
  t_symbol *pd_rcv;
} t_n_cnvrcv;

//----------------------------------------------------------------------------//
static void n_cnvrcv_anything(t_n_cnvrcv *x, t_symbol *s, int ac, t_atom *av)
{
  outlet_anything(x->x_obj.ob_outlet, s, ac, av);
}

//----------------------------------------------------------------------------//
static void n_cnvrcv_cnv(t_n_cnvrcv *x, t_symbol *s)
{
  pd_unbind(&x->x_obj.ob_pd, x->pd_rcv);
  x->pd_rcv = s;
  pd_bind(&x->x_obj.ob_pd, x->pd_rcv);
}

//----------------------------------------------------------------------------//
static void *n_cnvrcv_new()
{
  t_n_cnvrcv *x = (t_n_cnvrcv *)pd_new(n_cnvrcv_class);
  t_glist *glist;
  t_canvas *canvas;
  glist = (t_glist *)canvas_getcurrent();
  canvas = glist_getcanvas(glist);
  char buf[MAXPDSTRING];
  if (canvas)
    {
      sprintf(buf, ".x%lx", (t_int)canvas);
      x->pd_rcv = gensym(buf);
    }
  pd_bind(&x->x_obj.ob_pd, x->pd_rcv);
  outlet_new(&x->x_obj, 0);
  return (x);
}

//----------------------------------------------------------------------------//
static void n_cnvrcv_ff(t_n_cnvrcv *x)
{
  pd_unbind(&x->x_obj.ob_pd, x->pd_rcv);
}

//----------------------------------------------------------------------------//
void n_cnvrcv_setup(void)
{
  n_cnvrcv_class = class_new(gensym("n_cnvrcv"), (t_newmethod)n_cnvrcv_new, (t_method)n_cnvrcv_ff, sizeof(t_n_cnvrcv), CLASS_DEFAULT, A_GIMME, 0);
  class_addanything(n_cnvrcv_class, n_cnvrcv_anything);
  class_addmethod(n_cnvrcv_class, (t_method)n_cnvrcv_cnv, gensym("cnv"), A_SYMBOL, 0);
}
