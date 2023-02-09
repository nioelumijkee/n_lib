// only GIF
#include <string.h>
#include "m_pd.h"
#include "m_imp.h"
#include "g_canvas.h"
#include "g_all_guis.h"

#define NOUSE(X) if(X){};

static t_class *n_img_class;
t_widgetbehavior n_img_widgetbehavior;
t_symbol *extdir;

typedef struct _n_img
{
  t_object x_obj;
  t_glist * x_glist;
  t_symbol *x_bindname;
  t_symbol *curdir;
  int sel;
  /* par */
  t_symbol *filename;
  int frames;
  int pos;
  /* image */
  int img_w;
  int img_h;
  int frame_w;
  int frame_h;
  int frame_w_2;
  int frame_h_2;
} t_n_img;

// -------------------------------------------------------------------------- //
// sys_vgui
// -------------------------------------------------------------------------- //
// Ix - image
// Fx - frame
// IMGSRC - image source
// IMG - image
int visible(t_n_img *x)
{
  return(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist));
}

static void n_img_draw_image(t_n_img *x)
{
  if (x->img_w == -1) return;
  int ofs_x0, ofs_y0, ofs_x1, ofs_y1;
  // only vertical
  ofs_x0 = 0;
  ofs_y0 = x->pos * x->frame_h;
  ofs_x1 = x->frame_w;
  ofs_y1 = ofs_y0 + x->frame_h;
  sys_vgui("IMG%x copy IMGSRC%x -from %d %d %d %d\n",
           x,
           x,
           ofs_x0,
           ofs_y0,
           ofs_x1,
           ofs_y1);
  sys_vgui(".x%x.c create image %d %d -image IMG%x -tags I%x\n",
	   glist_getcanvas(x->x_glist),
	   text_xpix(&x->x_obj, x->x_glist) + x->frame_w_2,
	   text_ypix(&x->x_obj, x->x_glist) + x->frame_h_2,
	   x,
	   x);
}

static void n_img_config_image(t_n_img *x)
{
  if (x->img_w == -1) return;
  int ofs_x0, ofs_y0, ofs_x1, ofs_y1;
  // only vertical
  ofs_x0 = 0;
  ofs_y0 = x->pos * x->frame_h;
  ofs_x1 = x->frame_w;
  ofs_y1 = ofs_y0 + x->frame_h;
  sys_vgui("IMG%x copy IMGSRC%x -from %d %d %d %d\n",
           x,
           x,
           ofs_x0,
           ofs_y0,
           ofs_x1,
           ofs_y1);
}

static void n_img_draw_frame(t_n_img *x)
{
  sys_vgui(".x%x.c create rectangle %d %d %d %d \
           -tags F%x -outline blue\n",
           glist_getcanvas(x->x_glist),
           text_xpix(&x->x_obj, x->x_glist) - 1, 
           text_ypix(&x->x_obj, x->x_glist) - 1,
           text_xpix(&x->x_obj, x->x_glist) + x->frame_w, 
           text_ypix(&x->x_obj, x->x_glist) + x->frame_h,
           x);
}

static void n_img_coords_image(t_n_img *x)
{
  sys_vgui(".x%x.c coords I%x %d %d\n",
           glist_getcanvas(x->x_glist), 
           x,
           text_xpix(&x->x_obj, x->x_glist) + x->frame_w_2, 
           text_ypix(&x->x_obj, x->x_glist) + x->frame_h_2);
}

static void n_img_coords_frame(t_n_img *x)
{
  sys_vgui(".x%x.c coords F%x %d %d %d %d\n",
           glist_getcanvas(x->x_glist), 
           x,
           text_xpix(&x->x_obj, x->x_glist) - 1, 
           text_ypix(&x->x_obj, x->x_glist) - 1,
           text_xpix(&x->x_obj, x->x_glist) + x->frame_w, 
           text_ypix(&x->x_obj, x->x_glist) + x->frame_h);
}

static void n_img_erase_image(t_n_img *x)
{
  if (x->img_w == -1) return;
  sys_vgui(".x%x.c delete I%x\n",
           glist_getcanvas(x->x_glist), 
           x);
}

static void n_img_erase_frame(t_n_img *x)
{
  sys_vgui(".x%x.c delete F%x\n",
           glist_getcanvas(x->x_glist), 
           x);
}

// -------------------------------------------------------------------------- //
// widget
// -------------------------------------------------------------------------- //
static void n_img_getrect(t_gobj *z, 
                           t_glist *glist,
                           int *xp1, 
                           int *yp1, 
                           int *xp2, 
                           int *yp2)
{
  t_n_img* x = (t_n_img*)z;
  *xp1 = text_xpix(&x->x_obj, glist);
  *yp1 = text_ypix(&x->x_obj, glist);
  *xp2 = text_xpix(&x->x_obj, glist) + x->frame_w;
  *yp2 = text_ypix(&x->x_obj, glist) + x->frame_h;
}

static void n_img_displace(t_gobj *z, 
                            t_glist *glist, 
                            int dx, 
                            int dy)
{
  t_n_img *x = (t_n_img *)z;
  x->x_obj.te_xpix += dx;
  x->x_obj.te_ypix += dy;
  n_img_coords_image(x);
  if (x->sel)
    {
      n_img_coords_frame(x);
    }
  canvas_fixlinesfor(glist,(t_text*) x);
}

static void n_img_select(t_gobj *z, t_glist *glist, int state)
{
  t_n_img *x = (t_n_img *)z;
  x->x_glist = glist;
  if (state)
    {
      x->sel = 1;
      n_img_draw_frame(x);
    }
  else 
    {
      x->sel = 0;
      n_img_erase_frame(x);
    }
}

static void n_img_delete(t_gobj *z, t_glist *glist)
{
  t_text *x = (t_text *)z;
  canvas_deletelinesfor(glist, x);
}

static void n_img_vis(t_gobj *z, t_glist *glist, int vis)
{
  t_n_img* x = (t_n_img*)z;
  x->x_glist = glist;
  if (vis)
    {
      n_img_draw_image(x);
      if (x->sel)
        {
          n_img_draw_frame(x);
        }
    }
  else
    {
      n_img_erase_image(x);
      if (x->sel)
        {
          n_img_erase_frame(x);
        }
    }
}

// -------------------------------------------------------------------------- //
// methods
// -------------------------------------------------------------------------- //
static void n_img_image(t_n_img *x, 
			t_symbol *fn,
			t_floatarg frames,
			int pos)
{
  x->img_w = -1;
  x->img_h = -1;
  x->filename = fn;
  x->frames=(frames<1)?1:frames;
  x->pos = (pos<0)?0:(pos>x->frames-1)?x->frames-1:pos;
  sys_vgui("image create photo IMG%x\n",x);
  sys_vgui("if { [file exists {%s/%s}] == 1 } {			 \
               image create photo IMGSRC%x -file {%s/%s};		\
               pdsend [concat %s _size [image width IMGSRC%x] [image height IMGSRC%x]]; \
            } elseif { [file exists {%s/images/%s}] == 1 } {		\
               image create photo IMGSRC%x -file {%s/images/%s};	\
               pdsend [concat %s _size [image width IMGSRC%x] [image height IMGSRC%x]]; \
            } else { pdsend [concat %s _size -1 -1] }\n",
           x->curdir->s_name, x->filename->s_name,
           x, x->curdir->s_name, x->filename->s_name,
           x->x_bindname->s_name, x, x,
           extdir->s_name, x->filename->s_name,
           x, extdir->s_name, x->filename->s_name,
           x->x_bindname->s_name, x, x,
           x->x_bindname->s_name);
}

static void n_img_float(t_n_img* x, t_float pos)
{
  x->pos = (pos<0)?0:(pos>x->frames-1)?x->frames-1:pos;
  if(visible(x))
    {
      n_img_config_image(x);
    }
}

// -------------------------------------------------------------------------- //
// callback
// -------------------------------------------------------------------------- //
static void n_img_size_callback(t_n_img *x, t_float w, t_float h)
{
  if (w == -1)
    {
      post("n_img error: load: %s",x->filename->s_name);
      return;
    }
  x->img_w = w;
  x->img_h = h;
  // only vertical
  x->frame_w = x->img_w;
  x->frame_h = x->img_h / x->frames;
  x->frame_w_2 = x->frame_w / 2;
  x->frame_h_2 = x->frame_h / 2;
  if(visible(x))
    {
      n_img_erase_image(x);
      n_img_draw_image(x);
      n_img_erase_frame(x);
      if (x->sel)
        {
          n_img_draw_frame(x);
        }
    }
}

// -------------------------------------------------------------------------- //
// save and properties
// -------------------------------------------------------------------------- //
static void n_img_save(t_gobj *z, t_binbuf *b)
{
  t_n_img *x = (t_n_img *)z;
  binbuf_addv(b, "ssiissii",
              gensym("#X"), 
              gensym("obj"),
              (int)x->x_obj.te_xpix, 
              (int)x->x_obj.te_ypix,   
              gensym("n_img"),
              x->filename,
              (int)x->frames,
              (int)x->pos);
  binbuf_addv(b, ";");
}

static void n_img_properties(t_gobj *z, t_glist *owner)
{
  t_n_img *x = (t_n_img *)z;
  char buf[MAXPDSTRING];
  sprintf(buf, "pdtk_n_img_dialog %%s %s %d %d\n",
          x->filename->s_name,
          (int)x->frames,
          (int)x->pos);
  gfxstub_new(&x->x_obj.ob_pd, x, buf);
  NOUSE(owner);
}

static void n_img_dialog(t_n_img *x, t_symbol *s, int ac, t_atom *av)
{
  if (!x)
    {
      post("n_img error: dialog: unexisting object");
      return;
    }
  if (ac != 3)
    {
      post("n_img error: dialog: number arguments");
      return;
    }
  t_symbol *filename    = atom_getsymbolarg(0,ac,av);
  int frames            = atom_getfloatarg(1,ac,av);
  int pos               = atom_getfloatarg(2,ac,av);

  if ((strcmp(x->filename->s_name, filename->s_name))
      || x->frames != frames
      || x->pos != pos)
    {
      n_img_image(x, filename, frames, pos);
    }
  NOUSE(s);
}

// -------------------------------------------------------------------------- //
// setup
// -------------------------------------------------------------------------- //
static void *n_img_new(t_symbol *s, t_int ac, t_atom *av)
{
  char buf[MAXPDSTRING];

  t_symbol *filename;
  int frames;
  int pos;

  t_n_img *x = (t_n_img *)pd_new(n_img_class);
  x->x_glist = (t_glist*) canvas_getcurrent();
  
  // const
  x->curdir = canvas_getcurrentdir();
  
  // bind
  sprintf(buf, "#%lx", (long)x);
  pd_bind(&x->x_obj.ob_pd, x->x_bindname = gensym(buf));

  // image and frame
  x->img_w = -1;
  x->img_h = -1;
  x->sel = 0;
  
  // arguments
  if (ac == 3
      && IS_A_SYMBOL(av,0)
      && IS_A_FLOAT(av,1)
      && IS_A_FLOAT(av,2))
    {
      filename      = atom_getsymbolarg(0,ac,av);
      frames        = atom_getfloatarg(1,ac,av);
      pos           = atom_getfloatarg(2,ac,av);
    }
  else
    {
      filename      = gensym("default.gif");
      frames        = 33;
      pos           = 0;
    }
  
  // image
  n_img_image(x, filename, frames, pos);

  return (void*)x;
  NOUSE(s);
}


static void n_img_free(t_n_img* x)
{
  pd_unbind(&x->x_obj.ob_pd, x->x_bindname);
}

void n_img_setup(void)
{
  n_img_class = class_new(gensym("n_img"),
			  (t_newmethod)n_img_new,
			  (t_method)n_img_free,
			  sizeof(t_n_img),
			  0,A_GIMME,0);
  class_addmethod(n_img_class,(t_method)n_img_image,
		  gensym("image"),A_SYMBOL,A_FLOAT,A_FLOAT,0);
  class_addfloat(n_img_class,(t_method)n_img_float);
  class_addmethod(n_img_class,(t_method)n_img_size_callback,
		  gensym("_size"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_img_class,(t_method)n_img_dialog,
		  gensym("dialog"),A_GIMME, 0);
  n_img_widgetbehavior.w_getrectfn=n_img_getrect;
  n_img_widgetbehavior.w_displacefn=n_img_displace;
  n_img_widgetbehavior.w_selectfn=n_img_select;
  n_img_widgetbehavior.w_deletefn=n_img_delete;
  n_img_widgetbehavior.w_visfn=n_img_vis;
  class_setsavefn(n_img_class,&n_img_save);
  class_setwidget(n_img_class,&n_img_widgetbehavior);
  class_setpropertiesfn(n_img_class, n_img_properties);
  sys_vgui("eval [read [open {%s/n_img.tcl}]]\n",n_img_class->c_externdir->s_name);
  extdir = n_img_class->c_externdir;
}

