// only GIF
// 2021(c)Nio

#include <string.h>
#include "m_pd.h"
#include "m_imp.h"
#include "g_canvas.h"
#include "g_all_guis.h"
#include "include/pdfunc.h"

#define HOR       0
#define VER       1
#define NOINIT    0
#define INIT      1
#define NEVERST   -99999999
#define NOUSE(X)  if(X){};
#define MODE_RECT 0
#define MODE_PICT 1

static t_class *n_knob_class;
t_widgetbehavior n_knob_widgetbehavior;
t_symbol *extdir;
t_symbol *_s_empty;

typedef struct _n_knob
{
  t_object x_obj;
  t_glist * x_glist;
  t_symbol *x_bindname;
  int dollarzero;
  t_symbol *curdir;
  int sel;
  /* par */
  int r_w;
  int r_h;
  int mode;
  t_symbol *filename;
  int orientation;
  int frames;
  t_float min;
  t_float max;
  t_float resolution;
  t_float step;
  t_float default_state;
  int init;
  t_symbol *snd;
  t_symbol *rcv;
  t_symbol *lab;
  int lab_fs;
  int lab_x;
  int lab_y;
  int num_vis;
  int num_w;
  int num_fs;
  int num_x;
  int num_y;
  int lab_col;
  int num_col;
  int b_col;
  int f_col;
  t_float state;
  /* image */
  int img_w;
  int img_h;
  int frame_w;
  int frame_h;
  int frame_w_2;
  int frame_h_2;
  /* internal */
  t_float state_old;
  t_float diff;
  int minmaxdir;
  int framepos;
  int framepos_old;
  int shift;
  t_float count;
  int numint;
  int labdisp;
  /* snd rcv */
  t_symbol *snd_real;
  int snd_able;
  t_symbol *rcv_real;
  int rcv_able;
} t_n_knob;

// -------------------------------------------------------------------------- //
// sys_vgui
// -------------------------------------------------------------------------- //
// Rx - rect
// Ix - image
// Fx - frame
// Lx - label
// Nx - num
// IMGSRC - image source
// IMG - image
int visible(t_n_knob *x)
{
  return(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist));
}

// -------------------------------------------------------------------------- //
void n_knob_bind_image(t_n_knob *x)
{
  if (x->img_w == -1) return;
  sys_vgui(".x%x.c bind I%x <Double-Button-1> \
           {pdsend [concat %s _double \\;]}\n", 
           glist_getcanvas(x->x_glist), 
           x, 
           x->x_bindname->s_name);
}

void n_knob_draw_image(t_n_knob *x)
{
  if (x->img_w == -1) return;
  int ofs_x0, ofs_y0, ofs_x1, ofs_y1;
  // only vertical
  ofs_x0 = 0;
  ofs_y0 = x->framepos * x->frame_h;
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

void n_knob_config_image(t_n_knob *x)
{
  if (x->img_w == -1) return;
  int ofs_x0, ofs_y0, ofs_x1, ofs_y1;
  // only vertical
  ofs_x0 = 0;
  ofs_y0 = x->framepos * x->frame_h;
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

void n_knob_coords_image(t_n_knob *x)
{
  sys_vgui(".x%x.c coords I%x %d %d\n",
           glist_getcanvas(x->x_glist), 
           x,
           text_xpix(&x->x_obj, x->x_glist) + x->frame_w_2, 
           text_ypix(&x->x_obj, x->x_glist) + x->frame_h_2);
}

void n_knob_erase_image(t_n_knob *x)
{
  if (x->img_w == -1) return;
  sys_vgui(".x%x.c delete I%x\n",
           glist_getcanvas(x->x_glist), 
           x);
}

// -------------------------------------------------------------------------- //
void n_knob_bind_rect(t_n_knob *x)
{
  sys_vgui(".x%x.c bind R%x <Double-Button-1> \
           {pdsend [concat %s _double \\;]}\n", 
           glist_getcanvas(x->x_glist), 
           x, 
           x->x_bindname->s_name);
}

void n_knob_draw_rect(t_n_knob *x)
{
  sys_vgui(".x%x.c create rectangle %d %d %d %d \
           -fill #%6.6x -outline #%6.6x -width 1 -tags R%x\n",
           glist_getcanvas(x->x_glist),
           text_xpix(&x->x_obj, x->x_glist), 
           text_ypix(&x->x_obj, x->x_glist),
           text_xpix(&x->x_obj, x->x_glist) + x->r_w, 
           text_ypix(&x->x_obj, x->x_glist) + x->r_h,
	   x->b_col,
	   x->f_col,
           x);
}

void n_knob_config_rect(t_n_knob *x)
{
  sys_vgui(".x%x.c itemconfigure R%x -fill #%6.6x -outline #%6.6x\n",
           glist_getcanvas(x->x_glist), 
           x,
	   x->b_col,
	   x->f_col);
}

void n_knob_coords_rect(t_n_knob *x)
{
  sys_vgui(".x%x.c coords R%x %d %d %d %d\n",
           glist_getcanvas(x->x_glist), 
           x,
           text_xpix(&x->x_obj, x->x_glist), 
           text_ypix(&x->x_obj, x->x_glist),
           text_xpix(&x->x_obj, x->x_glist) + x->r_w, 
           text_ypix(&x->x_obj, x->x_glist) + x->r_h);
}

void n_knob_erase_rect(t_n_knob *x)
{
  sys_vgui(".x%x.c delete R%x\n",
           glist_getcanvas(x->x_glist), 
           x);
}

// -------------------------------------------------------------------------- //
void n_knob_draw_frame(t_n_knob *x)
{
  if (x->mode == MODE_PICT)
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
  else
    {
      sys_vgui(".x%x.c create rectangle %d %d %d %d \
           -tags F%x -outline blue\n",
	       glist_getcanvas(x->x_glist),
	       text_xpix(&x->x_obj, x->x_glist) - 1, 
	       text_ypix(&x->x_obj, x->x_glist) - 1,
	       text_xpix(&x->x_obj, x->x_glist) + x->r_w, 
	       text_ypix(&x->x_obj, x->x_glist) + x->r_h,
	       x);
    }
}

void n_knob_coords_frame(t_n_knob *x)
{
  if (x->mode == MODE_PICT)
    {
      sys_vgui(".x%x.c coords F%x %d %d %d %d\n",
	       glist_getcanvas(x->x_glist), 
	       x,
	       text_xpix(&x->x_obj, x->x_glist) - 1, 
	       text_ypix(&x->x_obj, x->x_glist) - 1,
	       text_xpix(&x->x_obj, x->x_glist) + x->frame_w, 
	       text_ypix(&x->x_obj, x->x_glist) + x->frame_h);
    }
  else
    {
      sys_vgui(".x%x.c coords F%x %d %d %d %d\n",
	       glist_getcanvas(x->x_glist), 
	       x,
	       text_xpix(&x->x_obj, x->x_glist) - 1, 
	       text_ypix(&x->x_obj, x->x_glist) - 1,
	       text_xpix(&x->x_obj, x->x_glist) + x->r_w, 
	       text_ypix(&x->x_obj, x->x_glist) + x->r_h);
    }
}

void n_knob_erase_frame(t_n_knob *x)
{
  sys_vgui(".x%x.c delete F%x\n",
           glist_getcanvas(x->x_glist), 
           x);
}

// -------------------------------------------------------------------------- //
void n_knob_bind_label(t_n_knob *x)
{
  sys_vgui(".x%x.c bind L%x <Double-Button-1> \
           {pdsend [concat %s _double \\;]}\n", 
           glist_getcanvas(x->x_glist), 
           x, 
           x->x_bindname->s_name);
}

void n_knob_draw_label(t_n_knob *x)
{
  sys_vgui(".x%x.c create text %d %d -text {%s} \
       -anchor w -font {{%s} -%d %s} -fill #%6.6x -tags L%x\n",
           glist_getcanvas(x->x_glist),
           text_xpix(&x->x_obj, x->x_glist) + x->lab_x, 
           text_ypix(&x->x_obj, x->x_glist) + x->lab_y,
           x->lab->s_name,
           sys_font,
           x->lab_fs,
           sys_fontweight,
           x->lab_col,
           x);
}

void n_knob_coords_label(t_n_knob *x)
{
  sys_vgui(".x%x.c coords L%x %d %d\n",
           glist_getcanvas(x->x_glist), 
           x,
           text_xpix(&x->x_obj, x->x_glist) + x->lab_x, 
           text_ypix(&x->x_obj, x->x_glist) + x->lab_y);
}

void n_knob_erase_label(t_n_knob *x)
{
  sys_vgui(".x%x.c delete L%x\n",
           glist_getcanvas(x->x_glist), 
           x);
}

// -------------------------------------------------------------------------- //
void n_knob_bind_num(t_n_knob *x)
{
  sys_vgui(".x%x.c bind N%x <Double-Button-1> \
           {pdsend [concat %s _double \\;]}\n", 
           glist_getcanvas(x->x_glist), 
           x, 
           x->x_bindname->s_name);
}

void n_knob_draw_num(t_n_knob *x)
{
  char buf[32];
  if (x->numint)
    sprintf(buf," %-d",(int)x->state);
  else
    sprintf(buf," %-f",x->state);
  buf[x->num_w + 1] = '\0';
  sys_vgui(".x%x.c create text %d %d -text {%s} \
           -anchor w -font {{%s} -%d %s} -fill #%6.6x -tags N%x\n",
           glist_getcanvas(x->x_glist),
           text_xpix(&x->x_obj, x->x_glist) + x->num_x, 
           text_ypix(&x->x_obj, x->x_glist) + x->num_y,
           buf,
           sys_font,
           x->num_fs,
           sys_fontweight,
           x->num_col,
           x);
}

void n_knob_config_num(t_n_knob *x)
{
  char buf[32];
  if (x->numint)
    sprintf(buf," %-d",(int)x->state);
  else
    sprintf(buf," %-f",x->state);
  buf[x->num_w + 1] = '\0';
  sys_vgui(".x%x.c itemconfigure N%x -text {%s}\n",
           glist_getcanvas(x->x_glist),
	   x,
           buf);
}

void n_knob_coords_num(t_n_knob *x)
{
  sys_vgui(".x%x.c coords N%x %d %d\n",
           glist_getcanvas(x->x_glist), 
           x,
           text_xpix(&x->x_obj, x->x_glist) + x->num_x, 
           text_ypix(&x->x_obj, x->x_glist) + x->num_y);
}

void n_knob_erase_num(t_n_knob *x)
{
  sys_vgui(".x%x.c delete N%x\n",
           glist_getcanvas(x->x_glist), 
           x);
}

// -------------------------------------------------------------------------- //
// widget
// -------------------------------------------------------------------------- //
void n_knob_getrect(t_gobj *z, 
                           t_glist *glist,
                           int *xp1, 
                           int *yp1, 
                           int *xp2, 
                           int *yp2)
{
  t_n_knob* x = (t_n_knob*)z;
  *xp1 = text_xpix(&x->x_obj, glist);
  *yp1 = text_ypix(&x->x_obj, glist);
  if (x->mode == MODE_PICT)
    {
      *xp2 = text_xpix(&x->x_obj, glist) + x->frame_w;
      *yp2 = text_ypix(&x->x_obj, glist) + x->frame_h;
    }
  else
    {
      *xp2 = text_xpix(&x->x_obj, glist) + x->r_w;
      *yp2 = text_ypix(&x->x_obj, glist) + x->r_h;
    }
}

void n_knob_displace(t_gobj *z, 
                            t_glist *glist, 
                            int dx, 
                            int dy)
{
  t_n_knob *x = (t_n_knob *)z;
  x->x_obj.te_xpix += dx;
  x->x_obj.te_ypix += dy;
  if (x->mode == MODE_PICT) n_knob_coords_image(x);
  else                      n_knob_coords_rect(x);
  if (x->sel)               n_knob_coords_frame(x);
  if (x->labdisp)           n_knob_coords_label(x);
  if (x->num_vis)           n_knob_coords_num(x);
  canvas_fixlinesfor(glist,(t_text*) x);
}

void n_knob_select(t_gobj *z, t_glist *glist, int state)
{
  t_n_knob *x = (t_n_knob *)z;
  x->x_glist = glist;
  if (state)
    {
      x->sel = 1;
      n_knob_draw_frame(x);
    }
  else 
    {
      x->sel = 0;
      n_knob_erase_frame(x);
    }
}

void n_knob_delete(t_gobj *z, t_glist *glist)
{
  t_text *x = (t_text *)z;
  canvas_deletelinesfor(glist, x);
}
       
void n_knob_vis(t_gobj *z, t_glist *glist, int vis)
{
  t_n_knob* x = (t_n_knob*)z;
  x->x_glist = glist;
  if (vis)
    {
      if (x->mode == MODE_PICT)
	{
	  n_knob_draw_image(x);
	  n_knob_bind_image(x);
	}
      else
	{
	  n_knob_draw_rect(x);
	  n_knob_bind_rect(x);
	}
      if (x->sel)
	n_knob_draw_frame(x);
      if (x->labdisp)
	{
	  n_knob_draw_label(x);
	  n_knob_bind_label(x);
	}
      if (x->num_vis)
	{
	  n_knob_draw_num(x);
	  n_knob_bind_num(x);
	}
    }
  else
    {
      if (x->mode == MODE_PICT)
	{
	  n_knob_erase_image(x);
	}
      else
	{
	  n_knob_erase_rect(x);
	}
      if (x->sel)       n_knob_erase_frame(x);
      if (x->labdisp)   n_knob_erase_label(x);
      if (x->num_vis)   n_knob_erase_num(x);
    }
}

// -------------------------------------------------------------------------- //
// various
// -------------------------------------------------------------------------- //
void n_knob_out(t_n_knob *x)
{
  outlet_float(x->x_obj.ob_outlet, x->state);
  if (x->snd_able && x->snd_real->s_thing)
    {
      pd_float(x->snd_real->s_thing, x->state);
    }
}

t_float quantize(t_float state, t_float step)
{
  int i;
  if (step > 0)
    {
      if (state > 0)
        {
          i = (state / step) + 0.5;
          state = i * step;
        }
      else
        {
          i = (state / step) - 0.5;
          state = i * step;
        }
    }
  return (state);
}

void n_knob_calc_state_from_count(t_n_knob *x)
{
  t_float f = ((x->count / x->resolution) * x->diff) + x->min;
  x->state = quantize(f, x->step);
}

void n_knob_calc_state_from_float(t_n_knob *x, t_float f)
{
  if (x->minmaxdir)
    x->state = (f>x->min)?x->min:(f<x->max)?x->max:f;
  else
    x->state = (f>x->max)?x->max:(f<x->min)?x->min:f;
  x->state = quantize(x->state, x->step);
}

void n_knob_calc_count_from_state(t_n_knob *x)
{
  t_float f = x->state - x->min;
  f = f / x->diff; 
  x->count = f * x->resolution;
  x->framepos = (x->count / x->resolution) * (x->frames - 1);
}

// motion
void n_knob_motion(t_n_knob *x, t_float dx, t_float dy)
{
  if (x->orientation == VER)
    {
      dy = 0 - dy;
      if (x->shift) dy = dy * 0.05;
      x->count += dy;
    }
  else
    {
      if (x->shift) dx = dx * 0.05;
      x->count += dx;
    }
  x->count = (x->count>x->resolution)?x->resolution:(x->count<0)?0:x->count;
  n_knob_calc_state_from_count(x);
  if (x->state != x->state_old)
    {
      x->framepos = ((x->state - x->min) / x->diff) * (x->frames - 1.0);
      if(visible(x))
        {
	  if (x->mode == MODE_PICT)
	    {
	      if (x->framepos != x->framepos_old)
		{
		  n_knob_config_image(x);
		}
	    }
          if (x->num_vis)
	    {
	      n_knob_config_num(x);
	    }
        }
      x->framepos_old = x->framepos;
      x->state_old = x->state;
      n_knob_out(x);
    }
}

int n_knob_newclick(t_gobj *z, 
                           t_glist *glist, 
                           int xpix, 
                           int ypix, 
                           int shift, 
                           int alt, 
                           int c, 
                           int doit)
{
  t_n_knob *x = (t_n_knob *)z;
  if (doit)
    {
      x->shift = shift;
      glist_grab(glist,
                 &x->x_obj.te_g,
                 (t_glistmotionfn)n_knob_motion,
                 (t_glistkeyfn)NULL,
                 (t_floatarg)xpix,
                 (t_floatarg)ypix);
    }
  return (1);
  NOUSE(alt);
  NOUSE(c);
}

// -------------------------------------------------------------------------- //
// for all
void n_knob_redraw(t_n_knob* x)
{
  if(visible(x))
    {
      // image or rect
      n_knob_erase_image(x);
      n_knob_erase_rect(x);
      if (x->mode == MODE_PICT)
	{
	  n_knob_draw_image(x);
	  n_knob_bind_image(x);
	}
      else
	{
	  n_knob_draw_rect(x);
	  n_knob_bind_rect(x);
	}
      // frame
      n_knob_erase_frame(x);
      if (x->sel)
        {
          n_knob_draw_frame(x);
        }
      // label
      n_knob_erase_label(x);
      if (x->labdisp)
	{
	  n_knob_draw_label(x);
	  n_knob_bind_label(x);
	}
      // num
      n_knob_erase_num(x);
      if (x->num_vis)
	{
	  n_knob_draw_num(x);
	  n_knob_bind_num(x);
	}
    }
}

// -------------------------------------------------------------------------- //
// methods
// -------------------------------------------------------------------------- //
void n_knob_rsize_set(t_n_knob* x, t_floatarg w, t_floatarg h)
{
  x->r_w = (w<4)?4:w;
  x->r_h = (h<4)?4:h;
}

void n_knob_rsize(t_n_knob* x, t_floatarg w, t_floatarg h)
{
  n_knob_rsize_set(x, w, h);
  n_knob_redraw(x);
}

void n_knob_mode_set(t_n_knob* x, t_floatarg f)
{
  x->mode = (f > 0);
}

void n_knob_mode(t_n_knob* x, t_floatarg f)
{
  n_knob_mode_set(x, f);
  n_knob_redraw(x);
}

void n_knob_orientation(t_n_knob* x, t_floatarg f)
{
  x->orientation = (f > 0);
}

void n_knob_image(t_n_knob *x, 
		  t_symbol *fn,
		  t_floatarg frames)
{
  x->img_w = -1;
  x->img_h = -1;
  x->filename = fn;
  x->frames = (frames<1)?1:frames;
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

void n_knob_knobpar_set(t_n_knob* x, 
			       t_floatarg min, 
			       t_floatarg max,
			       t_floatarg step,
			       t_floatarg default_state,
			       t_floatarg resolution)
{
  int i;
  t_float f;
  t_float maxstep;
  x->min = min;
  x->max = max;
  if (x->min > x->max) x->minmaxdir = 1;
  else                 x->minmaxdir = 0;
  if (step < 0)        step = 0;
  if (resolution < 1)  resolution = 1;
  x->diff = x->max - x->min;
  maxstep = x->diff;
  if (maxstep < 0) maxstep = 0 - maxstep;
  if      (step < 0)       step = 0;
  else if (step > maxstep) step = maxstep;
  // int or float
  i = step;
  f = step - i;
  if (f == 0 && step > 0) x->numint = 1;
  else                    x->numint = 0;
  if (x->minmaxdir)
    x->default_state =
      (default_state>x->min)?x->min:(default_state<x->max)?x->max:default_state;
  else
    x->default_state =
      (default_state>x->max)?x->max:(default_state<x->min)?x->min:default_state;
  x->step = step;
  x->resolution = resolution;
  n_knob_calc_state_from_float(x, x->state);
  n_knob_calc_count_from_state(x);
}

void n_knob_knobpar(t_n_knob* x, 
			   t_floatarg min, 
			   t_floatarg max,
			   t_floatarg step,
			   t_floatarg default_state,
			   t_floatarg resolution)
{
  n_knob_knobpar_set(x, min, max, step, default_state, resolution);
  n_knob_redraw(x);
}

void n_knob_snd(t_n_knob* x, t_symbol *s)
{
  if ((!(strcmp(s->s_name, "empty"))) || (s->s_name[0]=='\0'))
    {
      x->snd      = _s_empty;
      x->snd_real = _s_empty;
      x->snd_able = 0;
    }
  else
    {
      x->snd      = s;
      x->snd_real = dollarzero2sym(s, x->dollarzero);
      x->snd_able = 1;
    }
}

void n_knob_rcv(t_n_knob* x, t_symbol *s)
{
  if (x->rcv_able)
    pd_unbind(&x->x_obj.ob_pd, x->rcv_real);
  if ((!(strcmp(s->s_name, "empty"))) || (s->s_name[0]=='\0'))
    {
      x->rcv      = _s_empty;
      x->rcv_real = _s_empty;
      x->rcv_able = 0;
    }
  else
    {
      x->rcv      = s;
      x->rcv_real = dollarzero2sym(s, x->dollarzero);
      x->rcv_able = 1;
    }
  if (x->rcv_able)
    pd_bind(&x->x_obj.ob_pd, x->rcv_real);
}

void n_knob_lab_set(t_n_knob* x, t_symbol *s)
{
  if ((!(strcmp(s->s_name, "empty"))) || (s->s_name[0]=='\0'))
    {
      x->lab     = _s_empty;
      x->labdisp = 0;
    }
  else
    {
      x->lab = s;
      x->labdisp = 1;
    }
}

void n_knob_lab(t_n_knob* x, t_symbol *s)
{
  n_knob_lab_set(x, s);
  n_knob_redraw(x);
}

void n_knob_labpar_set(t_n_knob* x, 
			      t_floatarg lab_fs, 
			      t_floatarg lab_x,
			      t_floatarg lab_y,
			      t_floatarg lab_col)
{
  x->lab_fs = (lab_fs>256)?256:(lab_fs<4)?4:lab_fs;
  x->lab_x = lab_x;
  x->lab_y = lab_y;
  x->lab_col = lab_col;
}

void n_knob_labpar(t_n_knob* x, 
			  t_floatarg lab_fs, 
			  t_floatarg lab_x,
			  t_floatarg lab_y,
			  t_floatarg lab_col)
{
  lab_col = pdcolor(lab_col);
  n_knob_labpar_set(x, lab_fs, lab_x, lab_y, lab_col);
  n_knob_redraw(x);
}

void n_knob_numpar_set(t_n_knob* x, 
			      t_floatarg num_w, 
			      t_floatarg num_fs, 
			      t_floatarg num_x,
			      t_floatarg num_y,
			      t_floatarg num_col)
{
  x->num_w = (num_w>12)?12:(num_w<1)?1:num_w;
  x->num_fs = (num_fs>256)?256:(num_fs<4)?4:num_fs;
  x->num_x = num_x;
  x->num_y = num_y;
  x->num_col = num_col;
}

void n_knob_numpar(t_n_knob* x, 
			  t_floatarg num_w, 
			  t_floatarg num_fs, 
			  t_floatarg num_x,
			  t_floatarg num_y,
			  t_floatarg num_col)
{
  num_col = pdcolor(num_col);
  n_knob_numpar_set(x, num_w, num_fs, num_x, num_y, num_col);
  n_knob_redraw(x);
}

void n_knob_numvis_set(t_n_knob* x, t_floatarg f)
{
  x->num_vis = (f > 0);
}

void n_knob_numvis(t_n_knob* x, t_floatarg f)
{
  n_knob_numvis_set(x, f);
  n_knob_redraw(x);
}

void n_knob_rcolor_set(t_n_knob* x, 
			      t_floatarg b_col,
			      t_floatarg f_col)
{
  x->b_col = b_col;
  x->f_col = f_col;
}

void n_knob_rcolor(t_n_knob* x, 
			  t_floatarg b_col,
			  t_floatarg f_col)
{
  b_col = pdcolor(b_col);
  f_col = pdcolor(f_col);
  n_knob_rcolor_set(x, b_col, f_col);
  n_knob_redraw(x);
}

void n_knob_set(t_n_knob* x, t_floatarg f)
{
  n_knob_calc_state_from_float(x, f);
  n_knob_calc_count_from_state(x);
  if(visible(x))
    {
      if (x->mode == MODE_PICT)
	{
	  if (x->framepos != x->framepos_old)
	    {
	      n_knob_config_image(x);
	    }
	}
      if (x->num_vis)
	{
	  n_knob_config_num(x);
	}
    }
  x->framepos_old = x->framepos;
}

void n_knob_init(t_n_knob* x, t_floatarg f)
{
  if (f > 0)  x->init = INIT;
  else        x->init = NOINIT;
}

void n_knob_bang(t_n_knob* x)
{
  n_knob_out(x);
}

void n_knob_float(t_n_knob* x, t_float f)
{
  n_knob_calc_state_from_float(x, f);
  n_knob_calc_count_from_state(x);
  if(visible(x))
    {
      if (x->mode == MODE_PICT)
	{
	  if (x->framepos != x->framepos_old)
	    {
	      n_knob_config_image(x);
	    }
	}
      if (x->num_vis)
	{
	  n_knob_config_num(x);
	}
    }
  x->framepos_old = x->framepos;
  n_knob_out(x);
}

void n_knob_default(t_n_knob* x)
{
  n_knob_float(x, x->default_state);
}

// -------------------------------------------------------------------------- //
// callback
// -------------------------------------------------------------------------- //
void n_knob_loadbang(t_n_knob *x, t_floatarg action)
{
  if(action == LB_LOAD && x->init)
    {
      n_knob_out(x);
    }
}

void n_knob_size_callback(t_n_knob *x, t_float w, t_float h)
{
  if (w == -1)
    {
      post("n_knob error: load: %s",x->filename->s_name);
      return;
    }
  x->img_w = w;
  x->img_h = h;
  // only vertical
  x->frame_w = x->img_w;
  x->frame_h = x->img_h / x->frames;
  x->frame_w_2 = x->frame_w / 2;
  x->frame_h_2 = x->frame_h / 2;
  n_knob_calc_count_from_state(x);
  n_knob_redraw(x);
}

void n_knob_double_callback(t_n_knob* x)
{ 
  if(!x->x_glist->gl_edit)
    {
      n_knob_float(x, x->default_state);
    }
}

// -------------------------------------------------------------------------- //
// setup, save and properties
// -------------------------------------------------------------------------- //
void n_knob_set_par(t_n_knob *x, t_int ac, t_atom *av)
{
  int r_w;
  int r_h;
  int mode;
  t_symbol *filename;
  int orientation;
  int frames;
  t_float min;
  t_float max;
  t_float step;
  t_float default_state;
  int resolution;
  int init;
  t_symbol *snd;
  t_symbol *rcv;
  t_symbol *lab;
  int lab_fs;
  int lab_x;
  int lab_y;
  int num_vis;
  int num_w;
  int num_fs;
  int num_x;
  int num_y;
  int lab_col;
  int num_col;
  int b_col;
  int f_col;

  // arguments
  if (ac >= 27
      && IS_A_FLOAT(av,  0)
      && IS_A_FLOAT(av,  1)
      && IS_A_FLOAT(av,  2)
      && IS_A_SYMBOL(av, 3)
      && IS_A_FLOAT(av,  4)
      && IS_A_FLOAT(av,  5)
      && IS_A_FLOAT(av,  6)
      && IS_A_FLOAT(av,  7)
      && IS_A_FLOAT(av,  8)
      && IS_A_FLOAT(av,  9)
      && IS_A_FLOAT(av,  10)
      && IS_A_FLOAT(av,  11)
      && (IS_A_FLOAT(av, 12) || IS_A_SYMBOL(av, 12))
      && (IS_A_FLOAT(av, 13) || IS_A_SYMBOL(av, 13))
      && (IS_A_FLOAT(av, 14) || IS_A_SYMBOL(av, 14))
      && IS_A_FLOAT(av,  15)
      && IS_A_FLOAT(av,  16)
      && IS_A_FLOAT(av,  17)
      && IS_A_FLOAT(av,  18)
      && IS_A_FLOAT(av,  19)
      && IS_A_FLOAT(av,  20)
      && IS_A_FLOAT(av,  21)
      && IS_A_FLOAT(av,  22)
      && (IS_A_FLOAT(av, 23) || IS_A_SYMBOL(av, 23))
      && (IS_A_FLOAT(av, 24) || IS_A_SYMBOL(av, 24))
      && (IS_A_FLOAT(av, 25) || IS_A_SYMBOL(av, 25))
      && (IS_A_FLOAT(av, 26) || IS_A_SYMBOL(av, 26)))
    {
      r_w           = atom_getfloatarg(0,ac,av);
      r_h           = atom_getfloatarg(1,ac,av);
      mode          = atom_getfloatarg(2,ac,av);
      filename      = atom_getsymbolarg(3,ac,av);
      orientation   = atom_getfloatarg(4,ac,av);
      frames        = atom_getfloatarg(5,ac,av);
      min           = atom_getfloatarg(6,ac,av);
      max           = atom_getfloatarg(7,ac,av);
      step          = atom_getfloatarg(8,ac,av);
      default_state = atom_getfloatarg(9,ac,av);
      resolution    = atom_getfloatarg(10,ac,av);
      init          = atom_getfloatarg(11,ac,av);
      snd           = mygetsymbolarg(12,ac,av);
      rcv           = mygetsymbolarg(13,ac,av);
      lab           = mygetsymbolarg(14,ac,av);
      lab_fs        = atom_getfloatarg(15,ac,av);
      lab_x         = atom_getfloatarg(16,ac,av);
      lab_y         = atom_getfloatarg(17,ac,av);
      num_vis       = atom_getfloatarg(18,ac,av);
      num_w         = atom_getfloatarg(19,ac,av);
      num_fs        = atom_getfloatarg(20,ac,av);
      num_x         = atom_getfloatarg(21,ac,av);
      num_y         = atom_getfloatarg(22,ac,av);
      lab_col       = mygetintarg(23,ac,av);
      num_col       = mygetintarg(24,ac,av);
      b_col         = mygetintarg(25,ac,av);
      f_col         = mygetintarg(26,ac,av);
    }
  else
    {
      // default state
      r_w           = 40;
      r_h           = 20;
      mode          = MODE_PICT;
      filename      = gensym("default.gif");
      orientation   = VER;
      frames        = 33;
      min           = 0.0;
      max           = 1.0;
      step          = 0.01;
      default_state = 0.5;
      resolution    = 256;
      init          = NOINIT;
      snd           = _s_empty;
      rcv           = _s_empty;
      lab           = _s_empty;
      lab_fs        = 11;
      lab_x         = 0;
      lab_y         = -8;
      num_vis       = 0;
      num_w         = 5;
      num_fs        = 11;
      num_x         = 0;
      num_y         = 42;
      lab_col       = pdcolor(22);
      num_col       = pdcolor(22);
      b_col         = pdcolor(10);
      f_col         = pdcolor(28);
    }

  // rsize
  n_knob_rsize_set(x, r_w, r_h);

  // mode
  n_knob_mode_set(x, mode);

  // orientation
  n_knob_orientation(x, orientation);
  
  // label
  n_knob_lab_set(x, lab);
  n_knob_labpar_set(x, lab_fs, lab_x, lab_y, lab_col);

  // numeric
  n_knob_numpar_set(x, num_w, num_fs, num_x, num_y, num_col);
  n_knob_numvis_set(x, num_vis);

  // snd rcv
  n_knob_snd(x, snd);
  n_knob_rcv(x, rcv);

  // knob
  n_knob_knobpar_set(x, min, max, step, default_state, resolution);

  // rcolor
  n_knob_rcolor_set(x, b_col, f_col);

  // init
  x->init = init;

  // image (there callback ... and redraw)
  n_knob_image(x, filename, frames);
  return;
}

void n_knob_save(t_gobj *z, t_binbuf *b)
{
  t_n_knob *x = (t_n_knob *)z;
  char buf[256];
  t_symbol *snd;
  t_symbol *rcv;
  t_symbol *lab;
  sprintf(buf,"%s",x->snd->s_name); dollarinstring(buf); snd = gensym(buf); 
  sprintf(buf,"%s",x->rcv->s_name); dollarinstring(buf); rcv = gensym(buf); 
  sprintf(buf,"%s",x->lab->s_name); dollarinstring(buf); lab = gensym(buf); 
  binbuf_addv(b, "ssiisiiisiiffffiisssiiiiiiiiiiiif", 
              gensym("#X"), 
              gensym("obj"),
              (int)x->x_obj.te_xpix, 
              (int)x->x_obj.te_ypix,   
              gensym("n_knob"),
              (int)x->r_w,
              (int)x->r_h,
              (int)x->mode,
              x->filename,
              (int)x->orientation,
              (int)x->frames,
              x->min,
              x->max,
              x->step,
              x->default_state,
              (int)x->resolution,
              (int)x->init,
              snd,
              rcv,
              lab,
              (int)x->lab_fs,
              (int)x->lab_x,
              (int)x->lab_y,
              (int)x->num_vis,
              (int)x->num_w,
              (int)x->num_fs,
              (int)x->num_x,
              (int)x->num_y,
              (int)x->lab_col,
              (int)x->num_col,
              (int)x->b_col,
              (int)x->f_col,
              x->state);
  binbuf_addv(b, ";");
}

void n_knob_properties(t_gobj *z, t_glist *owner)
{
  t_n_knob *x = (t_n_knob *)z;
  char buf[MAXPDSTRING];
  t_symbol *snd;
  t_symbol *rcv;
  t_symbol *lab;
  sprintf(buf,"%s",x->snd->s_name); dollarinstring(buf); snd = gensym(buf); 
  sprintf(buf,"%s",x->rcv->s_name); dollarinstring(buf); rcv = gensym(buf); 
  sprintf(buf,"%s",x->lab->s_name); dollarinstring(buf); lab = gensym(buf); 
  sprintf(buf, "pdtk_n_knob_dialog %%s %d %d %d %s %d %d %g %g %g %g %d %d %s %s %s %d %d %d %d %d %d %d %d %d %d %d %d\n",
          (int)x->r_w,
          (int)x->r_h,
          (int)x->mode,
          x->filename->s_name,
          (int)x->orientation,
          (int)x->frames,
          x->min,
          x->max,
          x->step,
          x->default_state,
          (int)x->resolution,
          (int)x->init,
          snd->s_name,
          rcv->s_name,
          lab->s_name,
          (int)x->lab_fs,
          (int)x->lab_x,
          (int)x->lab_y,
          (int)x->num_vis,
          (int)x->num_w,
          (int)x->num_fs,
          (int)x->num_x,
          (int)x->num_y,
          (int)x->lab_col,
          (int)x->num_col,
          (int)x->b_col,
          (int)x->f_col);
  gfxstub_new(&x->x_obj.ob_pd, x, buf);
  NOUSE(owner);
}

void n_knob_dialog(t_n_knob *x, t_symbol *s, int ac, t_atom *av)
{
  if (!x)
    {
      post("n_knob error: dialog error: unexisting object");
      return;
    }
  n_knob_set_par(x, ac, av);
  NOUSE(s);
}

void *n_knob_new(t_symbol *s, t_int ac, t_atom *av)
{ 
  char buf[MAXPDSTRING];
  t_n_knob *x = (t_n_knob *)pd_new(n_knob_class);
  x->x_glist = (t_glist*) canvas_getcurrent();
  outlet_new(&x->x_obj, &s_float);
  // const
  x->dollarzero = canvas_getdollarzero();
  x->curdir = canvas_getcurrentdir();
  // bind
  sprintf(buf, "#%lx", (long)x);
  pd_bind(&x->x_obj.ob_pd, x->x_bindname = gensym(buf));
  // init internal par.
  x->img_w = -1;
  x->img_h = -1;
  x->sel = 0;
  x->state_old = NEVERST;
  x->framepos_old = NEVERST;
  x->snd_able = 0;
  x->rcv_able = 0;
  // state
  n_knob_set_par(x, ac, av);
  if (ac == 28 && x->init)
    x->state = atom_getfloatarg(27,ac,av);
  else
    x->state = x->default_state;
  return (void*)x;
  NOUSE(s);
}

void n_knob_free(t_n_knob* x)
{
  pd_unbind(&x->x_obj.ob_pd, x->x_bindname);
  if (x->rcv_able)
    pd_unbind(&x->x_obj.ob_pd, x->rcv_real);
}

void n_knob_setup(void)
{
  n_knob_class = class_new(gensym("n_knob"),
			   (t_newmethod)n_knob_new,
			   (t_method)n_knob_free,
			   sizeof(t_n_knob),
			   0,A_GIMME,0);
  class_addmethod(n_knob_class,(t_method)n_knob_rsize,
		  gensym("rsize"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_knob_class,(t_method)n_knob_mode,
		  gensym("mode"),A_FLOAT,0);
  class_addmethod(n_knob_class,(t_method)n_knob_orientation,
		  gensym("orientation"),A_FLOAT,0);
  class_addmethod(n_knob_class,(t_method)n_knob_image,
		  gensym("image"),A_SYMBOL,A_FLOAT,0);
  class_addmethod(n_knob_class,(t_method)n_knob_knobpar,
		  gensym("knobpar"),A_FLOAT,A_FLOAT,A_FLOAT,A_FLOAT,A_FLOAT,0);
  class_addmethod(n_knob_class,(t_method)n_knob_snd,
		  gensym("send"),A_DEFSYMBOL,0);
  class_addmethod(n_knob_class,(t_method)n_knob_rcv,
		  gensym("receive"),A_DEFSYMBOL,0);
  class_addmethod(n_knob_class,(t_method)n_knob_lab,
		  gensym("label"),A_DEFSYMBOL,0);
  class_addmethod(n_knob_class,(t_method)n_knob_labpar,
		  gensym("labpar"),A_FLOAT,A_FLOAT,A_FLOAT,A_FLOAT,0);
  class_addmethod(n_knob_class,(t_method)n_knob_numpar,
		  gensym("numpar"),A_FLOAT,A_FLOAT,A_FLOAT,A_FLOAT,A_FLOAT,0);
  class_addmethod(n_knob_class,(t_method)n_knob_numvis,
		  gensym("numvis"),A_FLOAT,0);
  class_addmethod(n_knob_class,(t_method)n_knob_rcolor,
		  gensym("rcolor"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_knob_class,(t_method)n_knob_set,
		  gensym("set"),A_FLOAT,0);
  class_addmethod(n_knob_class,(t_method)n_knob_init,
		  gensym("init"),A_FLOAT,0);
  class_addbang(n_knob_class,(t_method)n_knob_bang);
  class_addfloat(n_knob_class,(t_method)n_knob_float);
  class_addmethod(n_knob_class,(t_method)n_knob_default,
		  gensym("default"),0);
  class_addmethod(n_knob_class,(t_method)n_knob_size_callback,
		  gensym("_size"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_knob_class,(t_method)n_knob_double_callback,
		  gensym("_double"),0);
  class_addmethod(n_knob_class,(t_method)n_knob_loadbang,
		  gensym("loadbang"),A_DEFFLOAT, 0);
  class_addmethod(n_knob_class,(t_method)n_knob_dialog,
		  gensym("dialog"),A_GIMME, 0);
  n_knob_widgetbehavior.w_getrectfn=n_knob_getrect;
  n_knob_widgetbehavior.w_displacefn=n_knob_displace;
  n_knob_widgetbehavior.w_selectfn=n_knob_select;
  n_knob_widgetbehavior.w_deletefn=n_knob_delete;
  n_knob_widgetbehavior.w_visfn=n_knob_vis;
  n_knob_widgetbehavior.w_clickfn=n_knob_newclick;
  class_setsavefn(n_knob_class,&n_knob_save);
  class_setwidget(n_knob_class,&n_knob_widgetbehavior);
  class_setpropertiesfn(n_knob_class, n_knob_properties);
  sys_vgui("eval [read [open {%s/n_knob.tcl}]]\n",n_knob_class->c_externdir->s_name);
  extdir = n_knob_class->c_externdir;
  _s_empty = gensym("empty");
}
