#include <string.h>
#include "m_pd.h"
#include "g_canvas.h"

#define OUTLETF(S, F)					    \
  {							    \
    SETFLOAT(a, (t_float)(F));				    \
    outlet_anything(x->out, gensym((S)), 1, a);		    \
  }

#define OUTLETS(S, STR)					    \
  {							    \
    SETSYMBOL(a, gensym((STR)));			    \
    outlet_anything(x->out, gensym((S)), 1, a);		    \
  }

static t_class *n_cnvinfo_class;
typedef struct _n_cnvinfo
{
  t_object x_obj;
  t_outlet *out;
  t_canvas *canvas;
} t_n_cnvinfo;

static void n_cnvinfo_canvas(t_n_cnvinfo *x, t_floatarg f)
{
  t_canvas *canvas = x->canvas;
  int d = f;
  t_atom a[1];
  if (d < 0) d = 0;
  while (d && canvas)
    {
      canvas = canvas->gl_owner;
      d--;
    }
  char buf[MAXPDSTRING];
  snprintf(buf, MAXPDSTRING, ".x%lx", (long unsigned int)canvas);
  OUTLETS("id", buf);
  if (canvas)
    {
      OUTLETF("screenx1",   canvas->gl_screenx1);
      OUTLETF("screeny1",   canvas->gl_screeny1);
      OUTLETF("screenx2",   canvas->gl_screenx2);
      OUTLETF("screeny2",   canvas->gl_screeny2);
      OUTLETS("name",       canvas->gl_name->s_name);
      OUTLETF("font",       canvas->gl_font);
      OUTLETF("havewindow", canvas->gl_havewindow);
      OUTLETF("mapped",     canvas->gl_mapped);
      OUTLETF("dirty",      canvas->gl_dirty);
      OUTLETF("edit",       canvas->gl_edit);
      OUTLETF("goprect",    canvas->gl_goprect);
      OUTLETF("hidetext",   canvas->gl_hidetext);
      OUTLETF("zoom",       canvas->gl_zoom);
    }
  else
    {
      OUTLETF("screenx1",   0);
      OUTLETF("screeny1",   0);
      OUTLETF("screenx2",   0);
      OUTLETF("screeny2",   0);
      OUTLETS("name",       "none");
      OUTLETF("font",       0);
      OUTLETF("havewindow", 0);
      OUTLETF("mapped",     0);
      OUTLETF("dirty",      0);
      OUTLETF("edit",       0);
      OUTLETF("goprect",    0);
      OUTLETF("hidetext",   0);
      OUTLETF("zoom",       0);
    }
}

static void n_cnvinfo_canvas_0(t_n_cnvinfo *x)
{
  n_cnvinfo_canvas(x,0);
}

static void *n_cnvinfo_new()
{
  t_n_cnvinfo *x = (t_n_cnvinfo *)pd_new(n_cnvinfo_class);
  t_glist *glist = (t_glist *)canvas_getcurrent();
  x->canvas = (t_canvas *)glist_getcanvas(glist);
  x->out = outlet_new(&x->x_obj, 0);
  return (void*)x;
}

void n_cnvinfo_setup(void)
{
  n_cnvinfo_class = class_new(gensym("n_cnvinfo"),
			      (t_newmethod)n_cnvinfo_new,
			      0, 
			      sizeof(t_n_cnvinfo),
			      0, 0, 0);
  class_addbang(n_cnvinfo_class, (t_method)n_cnvinfo_canvas_0);
  class_addmethod(n_cnvinfo_class, (t_method)n_cnvinfo_canvas, gensym("canvas"),
		  A_DEFFLOAT, 0);
}
