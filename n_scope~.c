#include "m_pd.h"
#include "include/clip.h"
#include "include/constant.h"

#define MAX_INLETS 16
#define MIN_WIDTH 8
#define MAX_WIDTH 1024
#define MIN_HEIGHT 8
#define MAX_HEIGHT 1024
#define MAX_FPS 100.
#define MAX_VGRID 16


static t_class *n_scope_class;

typedef struct _n_scope_in
{
  int on;
  t_float amp;
  int col;
  int grid_hor_center;
  int grid_hor_low;
  t_float min;
  t_float max;
  int min_z;
  int max_z;
  int ofs;
} t_n_scope_in;

typedef struct _n_scope
{
  t_object x_obj;
  t_outlet *out;               /* outlet */
  t_float sr;                  /* sample rate */
  t_int n;                     /* block size */
  t_int in_all;                /* number in's */
  t_sample **v_in;             /* vector */
  t_int **v_d;                 /* vector for dsp_addv */
  int on;                      /* parameters */
  int separate;
  int grid;
  int grid_ver;
  int sync;
  int sync_in;
  int sync_dc;
  t_float sync_dc_hp;
  t_float treshold;
  int spp;
  t_float fps;
  int recpos;
  int width;
  int height;
  int fcol;
  int bcol;
  int rcol;
  int gcol;
  int scol;
  int height_1;                /* height + 1 */
  t_n_scope_in c[MAX_INLETS];  /* in's */
  int maxel;                   /* display settings */
  int ofs_grid_hor_center;     /* offsets elements */
  int ofs_grid_hor_low;
  int ofs_grid_ver_line;
  int ofs_waves;
  int height_one;              /* height wave */
  int height_half;
  t_float grid_ver_ox;
  t_float dc_f;                /* dc filter */
  t_float dc_z;
  int frame_count;             /* frame */
  int frame_max;
  int spp_count;               /* spp */
  int spp_max;
  int disp;                    /* disp */
  int disp_count;
  int find;
  t_float in_sync_z;           /* sync z */
  int recpos_view;             /* rec pos view */
} t_n_scope;

//----------------------------------------------------------------------------//
// outlet
//----------------------------------------------------------------------------//
static void n_scope_outlet_config_canvas(t_n_scope *x)
{
  t_atom a[2];
  SETFLOAT(a,(t_float)x->width + 1);
  SETFLOAT(a+1,(t_float)x->height + 1);
  outlet_anything(x->out, gensym("size"), 2, a);
  SETFLOAT(a,(t_float)x->maxel);
  outlet_anything(x->out, gensym("maxel"), 1, a);
}

//----------------------------------------------------------------------------//
static void n_scope_outlet_create_background(t_n_scope *x)
{
  t_atom a[9];
  SETFLOAT(a,(t_float)3);
  SETFLOAT(a+1,(t_float)0); // id
  SETFLOAT(a+2,(t_float)x->fcol); // fcolor
  SETFLOAT(a+3,(t_float)x->bcol); // bcolor
  SETFLOAT(a+4,(t_float)1); // width
  SETFLOAT(a+5,(t_float)0); // x0
  SETFLOAT(a+6,(t_float)0); // y0
  SETFLOAT(a+7,(t_float)x->width + 1); // x1
  SETFLOAT(a+8,(t_float)x->height + 1); // y1
  outlet_anything(x->out, &s_list, 9, a);
}

//----------------------------------------------------------------------------//
static void n_scope_outlet_create_recpos(t_n_scope *x)
{
  t_atom a[8];
  SETFLOAT(a,(t_float)1);
  SETFLOAT(a+1,(t_float)1); // id
  SETFLOAT(a+2,(t_float)x->rcol); // color
  SETFLOAT(a+3,(t_float)1); // width
  SETFLOAT(a+4,(t_float)0); // x0
  SETFLOAT(a+5,(t_float)0); // y0
  SETFLOAT(a+6,(t_float)0); // x1
  SETFLOAT(a+7,(t_float)0); // y1
  outlet_anything(x->out, &s_list, 8, a);
}

//----------------------------------------------------------------------------//
static void n_scope_outlet_create_grid(t_n_scope *x)
{
  int i,j;
  t_atom a[8];
  // center
  SETFLOAT(a,(t_float)1);
  for(j=0; j<MAX_INLETS; j++)
    {
      i = x->ofs_grid_hor_center+j;
      SETFLOAT(a+1,(t_float)i); // id
      SETFLOAT(a+2,(t_float)x->fcol); // color
      SETFLOAT(a+3,(t_float)1); // width
      SETFLOAT(a+4,(t_float)0); // x0
      SETFLOAT(a+5,(t_float)0); // y0
      SETFLOAT(a+6,(t_float)0); // x1
      SETFLOAT(a+7,(t_float)0); // y1
      outlet_anything(x->out, &s_list, 8, a);
    }
  // hor. low
  SETFLOAT(a,(t_float)1);
  for(j=0; j<MAX_INLETS; j++)
    {
      i =  x->ofs_grid_hor_low+j;
      SETFLOAT(a+1,(t_float)i); // id
      SETFLOAT(a+2,(t_float)x->fcol); // color
      SETFLOAT(a+3,(t_float)1); // width
      SETFLOAT(a+4,(t_float)0); // x0
      SETFLOAT(a+5,(t_float)0); // y0
      SETFLOAT(a+6,(t_float)0); // x1
      SETFLOAT(a+7,(t_float)0); // y1
      outlet_anything(x->out, &s_list, 8, a);
    }
  // ver. line
  SETFLOAT(a,(t_float)1);
  for(j=0; j<MAX_VGRID; j++)
    {
      i =  x->ofs_grid_ver_line+j;
      SETFLOAT(a+1,(t_float)i); // id
      SETFLOAT(a+2,(t_float)x->fcol); // color
      SETFLOAT(a+3,(t_float)1); // width
      SETFLOAT(a+4,(t_float)0); // x0
      SETFLOAT(a+5,(t_float)0); // y0
      SETFLOAT(a+6,(t_float)0); // x1
      SETFLOAT(a+7,(t_float)0); // y1
      outlet_anything(x->out, &s_list, 8, a);
    }
}

//----------------------------------------------------------------------------//
static void n_scope_outlet_create_waves(t_n_scope *x)
{
  int i,j;
  t_atom a[8];
  SETFLOAT(a,(t_float)1);
  for(j=0; j< (x->width * x->in_all); j++)
    {
      i = x->ofs_waves+j;
      SETFLOAT(a+1,(t_float)i); // id
      SETFLOAT(a+2,(t_float)x->fcol); // color
      SETFLOAT(a+3,(t_float)1); // width
      SETFLOAT(a+4,(t_float)0); // x0
      SETFLOAT(a+5,(t_float)0); // y0
      SETFLOAT(a+6,(t_float)0); // x1
      SETFLOAT(a+7,(t_float)0); // y1
      outlet_anything(x->out, &s_list, 8, a);
    }
}

//----------------------------------------------------------------------------//
static void n_scope_outlet_background(t_n_scope *x)
{
  t_atom a[4];
  // background color
  SETFLOAT(a,(t_float)10);
  SETFLOAT(a+1,(t_float)0); // id
  SETFLOAT(a+2,(t_float)x->fcol); // color
  SETFLOAT(a+3,(t_float)x->bcol); // color
  outlet_anything(x->out, &s_list, 4, a);
}

//----------------------------------------------------------------------------//
static void n_scope_outlet_recpos_color(t_n_scope *x)
{
  t_atom a[3];
  // recpos color
  SETFLOAT(a,(t_float)10);
  SETFLOAT(a+1,(t_float)1); // id
  SETFLOAT(a+2,(t_float)x->rcol); // color
  outlet_anything(x->out, &s_list, 3, a);
}

//----------------------------------------------------------------------------//
static void n_scope_outlet_recpos_view(t_n_scope *x)
{
  t_atom a[6];
  // recpos move
  SETFLOAT(a,(t_float)9);
  if (x->recpos_view)
    {
      SETFLOAT(a+1,(t_float)1); // id
      SETFLOAT(a+2,(t_float)x->disp_count+1); // x0
      SETFLOAT(a+3,(t_float)1); // y0
      SETFLOAT(a+4,(t_float)x->disp_count+1); // x1
      SETFLOAT(a+5,(t_float)x->height); // y1
      outlet_anything(x->out, &s_list, 6, a);
    }
  else
    {
      SETFLOAT(a+1,(t_float)1); // id
      SETFLOAT(a+2,(t_float)0); // x0
      SETFLOAT(a+3,(t_float)0); // y0
      SETFLOAT(a+4,(t_float)0); // x1
      SETFLOAT(a+5,(t_float)0); // y1
      outlet_anything(x->out, &s_list, 6, a);
    }
}

//----------------------------------------------------------------------------//
static void n_scope_outlet_grid_hor(t_n_scope *x)
{
  int i,j;
  t_atom a[6];
  // center move
  SETFLOAT(a,(t_float)9);
  for(j=0; j<x->in_all; j++)
    {
      if (x->c[j].on && x->grid)
	{
	  i = x->ofs_grid_hor_center+j;
	  SETFLOAT(a+1,(t_float)i); // id
	  SETFLOAT(a+2,(t_float)1); // x0
	  SETFLOAT(a+3,(t_float)x->c[j].grid_hor_center); // y0
	  SETFLOAT(a+4,(t_float)x->width); // x1
	  SETFLOAT(a+5,(t_float)x->c[j].grid_hor_center); // y1
	  outlet_anything(x->out, &s_list, 6, a);
	}
      else
	{
	  i = x->ofs_grid_hor_center+j;
	  SETFLOAT(a+1,(t_float)i); // id
	  SETFLOAT(a+2,(t_float)0); // x0
	  SETFLOAT(a+3,(t_float)0); // y0
	  SETFLOAT(a+4,(t_float)0); // x1
	  SETFLOAT(a+5,(t_float)0); // y1
	  outlet_anything(x->out, &s_list, 6, a);
	}
    }
  // center color
  SETFLOAT(a,(t_float)10);
  for(j=0; j<x->in_all; j++)
    {
      if (x->c[j].on && x->grid)
	{
	  i = x->ofs_grid_hor_center+j;
	  SETFLOAT(a+1,(t_float)i); // id
	  SETFLOAT(a+2,(t_float)x->gcol); // color
	  outlet_anything(x->out, &s_list, 3, a);
	}
      else
	{
	  i = x->ofs_grid_hor_center+j;
	  SETFLOAT(a+1,(t_float)i); // id
	  SETFLOAT(a+2,(t_float)x->fcol); // color
	  outlet_anything(x->out, &s_list, 3, a);
	}
    }
  // low move
  SETFLOAT(a,(t_float)9);
  for(j=0; j<x->in_all; j++)
    {
      if (x->c[j].on && x->grid && x->c[j].grid_hor_low != -1)
	{
	  i = x->ofs_grid_hor_low+j;
	  SETFLOAT(a+1,(t_float)i); // id
	  SETFLOAT(a+2,(t_float)1); // x0
	  SETFLOAT(a+3,(t_float)x->c[j].grid_hor_low); // y0
	  SETFLOAT(a+4,(t_float)x->width); // x1
	  SETFLOAT(a+5,(t_float)x->c[j].grid_hor_low); // y1
	  outlet_anything(x->out, &s_list, 6, a);
	}
      else
	{
	  i = x->ofs_grid_hor_low+j;
	  SETFLOAT(a+1,(t_float)i); // id
	  SETFLOAT(a+2,(t_float)0); // x0
	  SETFLOAT(a+3,(t_float)0); // y0
	  SETFLOAT(a+4,(t_float)0); // x1
	  SETFLOAT(a+5,(t_float)0); // y1
	  outlet_anything(x->out, &s_list, 6, a);
	}
    }
  // low color
  SETFLOAT(a,(t_float)10);
  for(j=0; j<x->in_all; j++)
    {
      if (x->c[j].on && x->grid && x->c[j].grid_hor_low != -1)
	{
	  i = x->ofs_grid_hor_low+j;
	  SETFLOAT(a+1,(t_float)i); // id
	  SETFLOAT(a+2,(t_float)x->scol); // color
	  outlet_anything(x->out, &s_list, 3, a);
	}
      else
	{
	  i = x->ofs_grid_hor_low+j;
	  SETFLOAT(a+1,(t_float)i); // id
	  SETFLOAT(a+2,(t_float)x->fcol); // color
	  outlet_anything(x->out, &s_list, 3, a);
	}
    }
}

//----------------------------------------------------------------------------//
static void n_scope_outlet_grid_ver(t_n_scope *x)
{
  int i,j;
  t_float k;
  t_atom a[6];
  // lines move
  SETFLOAT(a,(t_float)9);
  for(j=0; j<MAX_VGRID; j++)
    {
      if (j<x->grid_ver && x->grid)
	{
	  k = x->grid_ver_ox * (j + 1);
	  i = x->ofs_grid_ver_line+j;
	  SETFLOAT(a+1,(t_float)i); // id
	  SETFLOAT(a+2,(t_float)k); // x0
	  SETFLOAT(a+3,(t_float)1); // y0
	  SETFLOAT(a+4,(t_float)k); // x1
	  SETFLOAT(a+5,(t_float)x->height); // y1
	  outlet_anything(x->out, &s_list, 6, a);
	}
      else
	{
	  i = x->ofs_grid_ver_line+j;
	  SETFLOAT(a+1,(t_float)i); // id
	  SETFLOAT(a+2,(t_float)0); // x0
	  SETFLOAT(a+3,(t_float)0); // y0
	  SETFLOAT(a+4,(t_float)0); // x1
	  SETFLOAT(a+5,(t_float)0); // y1
	  outlet_anything(x->out, &s_list, 6, a);
	}
    }
  // lines color
  SETFLOAT(a,(t_float)10);
  for(j=0; j<MAX_VGRID; j++)
    {
      if (j<x->grid_ver && x->grid)
	{
	  i = x->ofs_grid_ver_line+j;
	  SETFLOAT(a+1,(t_float)i); // id
	  SETFLOAT(a+2,(t_float)x->gcol); // color
	  outlet_anything(x->out, &s_list, 3, a);
	}
      else
	{
	  i = x->ofs_grid_ver_line+j;
	  SETFLOAT(a+1,(t_float)i); // id
	  SETFLOAT(a+2,(t_float)x->fcol); // color
	  outlet_anything(x->out, &s_list, 3, a);
	}
    }
}

//----------------------------------------------------------------------------//
static void n_scope_outlet_waves(t_n_scope *x)
{
  int i,j,k;
  t_atom a[6];
  // waves move
  SETFLOAT(a,(t_float)9);
  for(j=0; j<x->in_all; j++)
    {
      k = x->ofs_waves + (j * x->width);
      if (x->c[j].on)
	{
	  for(i=0; i<x->width; i++)
	    {
	      SETFLOAT(a+1,(t_float)k+i); // id
	      SETFLOAT(a+2,(t_float)i); // x0
	      SETFLOAT(a+3,(t_float)x->c[j].grid_hor_center); // y0
	      SETFLOAT(a+4,(t_float)i); // x1
	      SETFLOAT(a+5,(t_float)x->c[j].grid_hor_center); // y1
	      outlet_anything(x->out, &s_list, 6, a);
	    }
	}
      else
	{
	  for(i=0; i<x->width; i++)
	    {
	      SETFLOAT(a+1,(t_float)k+i); // id
	      SETFLOAT(a+2,(t_float)0); // x0
	      SETFLOAT(a+3,(t_float)0); // y0
	      SETFLOAT(a+4,(t_float)0); // x1
	      SETFLOAT(a+5,(t_float)0); // y1
	      outlet_anything(x->out, &s_list, 6, a);
	    }
	}
    }
  // waves color
  SETFLOAT(a,(t_float)10);
  for(j=0; j<x->in_all; j++)
    {
      k = x->ofs_waves + (j * x->width);
      if (x->c[j].on)
	{
	  for(i=0; i<x->width; i++)
	    {
	      SETFLOAT(a+1,(t_float)k+i); // id
	      SETFLOAT(a+2,(t_float)x->c[j].col); // color
	      outlet_anything(x->out, &s_list, 3, a);
	    }
	}
      else
	{
	  for(i=0; i<x->width; i++)
	    {
	      SETFLOAT(a+1,(t_float)k+i); // id
	      SETFLOAT(a+2,(t_float)x->fcol); // color
	      outlet_anything(x->out, &s_list, 3, a);
	    }
	}
    }
}

//----------------------------------------------------------------------------//
// calculations
//----------------------------------------------------------------------------//
static void n_scope_calc_h(t_n_scope *x)
{
  int i, j;
  int last;

  // all in's
  j = 0;
  for (i = 0; i < x->in_all; i++)
    {
      if (x->c[i].on)
	j++;
    }

  // no separate
  if (x->separate == 0 || j <= 1)
    {
      x->height_one  = x->height;
      x->height_half = x->height / 2;
      for (i = 0; i < x->in_all; i++)
	{
	  x->c[i].grid_hor_center  = x->height_half;
	  x->c[i].grid_hor_low     = -1;
	} 
    }

  // separate
  else
    {
      x->height_one = x->height / j;
      x->height_half = x->height_one / 2;
      j = 0;
      last = 0;
      for (i = 0; i < x->in_all; i++)
	{
	  if (x->c[i].on)
	    {
	      x->c[i].grid_hor_center = x->height_one * (j + 0.5);
	      x->c[i].grid_hor_low    = x->height_one * (j + 1);
	      last = i;
	      j++;
	    }
	}
      x->c[last].grid_hor_low    = -1;
    }
}

//----------------------------------------------------------------------------//
static void n_scope_calc_maxel(t_n_scope *x)
{
  x->maxel = 2 +             // background and recpos
    MAX_INLETS +             // grid hor. center
    MAX_INLETS +             // grid hor. low
    MAX_VGRID +              // grid ver. line
    (x->in_all * x->width);  // waves
  x->ofs_grid_hor_center    = 2;
  x->ofs_grid_hor_low       = x->ofs_grid_hor_center + MAX_INLETS;
  x->ofs_grid_ver_line      = x->ofs_grid_hor_low    + MAX_INLETS;
  x->ofs_waves              = x->ofs_grid_ver_line   + MAX_VGRID;
  int i;
  for (i=0; i<x->in_all; i++)
    {
      x->c[i].ofs = x->ofs_waves + (i * x->width);
    }
}

//----------------------------------------------------------------------------//
static void n_scope_calc_grid_ver(t_n_scope *x)
{
  if (x->grid_ver > 0)
    {  
      x->grid_ver_ox = (t_float)x->width / (t_float)(x->grid_ver+1);
    }
  else
    {
      x->grid_ver_ox = 0.;
    }
}

//----------------------------------------------------------------------------//
static void n_scope_calc_dc_hp(t_n_scope *x)
{
  x->dc_f = x->sync_dc_hp * (AC_2PI / x->sr);
  x->dc_z = 0;
}

//----------------------------------------------------------------------------//
static void n_scope_calc_frame(t_n_scope *x)
{
  x->frame_max = x->sr / x->fps;
}

//----------------------------------------------------------------------------//
static void n_scope_calc_recpos_view(t_n_scope *x)
{
  if ((x->frame_max * 2) < (x->spp * x->width) && x->recpos)
    {
      x->recpos_view = 1;
    }
  else
    {
      x->recpos_view = 0;
    }
}

//----------------------------------------------------------------------------//
static void n_scope_reset_sr(t_n_scope *x)
{
  n_scope_calc_dc_hp(x);
  n_scope_calc_frame(x);
}

//----------------------------------------------------------------------------//
static void n_scope_reset(t_n_scope *x)
{
  x->dc_z = 0.;
  x->frame_count = 0;
  x->spp_count = 0;
  x->disp = 0;
  x->disp_count = 0;
  x->find = 0;
  x->in_sync_z = 0;
}

//----------------------------------------------------------------------------//
// par.
//----------------------------------------------------------------------------//
static void n_scope_on(t_n_scope *x, t_floatarg f)
{
  x->on = f;
}

//----------------------------------------------------------------------------//
static void n_scope_separate(t_n_scope *x, t_floatarg f)
{
  x->separate = f;
  n_scope_calc_h(x);
  n_scope_outlet_grid_hor(x);
  n_scope_outlet_waves(x);
}

//----------------------------------------------------------------------------//
static void n_scope_grid(t_n_scope *x, t_floatarg f)
{
  x->grid = f;
  n_scope_outlet_grid_hor(x);
  n_scope_outlet_grid_ver(x);
}

//----------------------------------------------------------------------------//
static void n_scope_grid_ver(t_n_scope *x, t_floatarg f)
{
  AF_CLIP_MINMAX(0, MAX_VGRID, f);
  x->grid_ver = f;
  n_scope_calc_grid_ver(x);
  n_scope_outlet_grid_ver(x);
}

//----------------------------------------------------------------------------//
static void n_scope_sync(t_n_scope *x, t_floatarg f)
{
  x->sync = f;
}

//----------------------------------------------------------------------------//
static void n_scope_sync_in(t_n_scope *x, t_floatarg f)
{
  AF_CLIP_MINMAX(0, x->in_all - 1, f);
  x->sync_in = f;
}

//----------------------------------------------------------------------------//
static void n_scope_sync_dc(t_n_scope *x, t_floatarg f)
{
  x->sync_dc = f;
}

//----------------------------------------------------------------------------//
static void n_scope_sync_dc_hp(t_n_scope *x, t_floatarg f)
{
  AF_CLIP_MIN(1, f);
  x->sync_dc_hp = f;
  n_scope_calc_dc_hp(x);
}

//----------------------------------------------------------------------------//
static void n_scope_treshold(t_n_scope *x, t_floatarg f)
{
  x->treshold = f;
}

//----------------------------------------------------------------------------//
static void n_scope_spp(t_n_scope *x, t_floatarg f)
{
  AF_CLIP_MIN(1, f);
  x->spp = f;
  x->spp_max = x->spp - 1;
  n_scope_calc_recpos_view(x);
  n_scope_outlet_recpos_view(x);
}

//----------------------------------------------------------------------------//
static void n_scope_fps(t_n_scope *x, t_floatarg f)
{
  AF_CLIP_MINMAX(0.01, MAX_FPS, f);
  if (x->fps != f)
    {
      x->fps = f;
      n_scope_calc_frame(x);
      n_scope_calc_recpos_view(x);
      n_scope_outlet_recpos_view(x);
    }
}

//----------------------------------------------------------------------------//
static void n_scope_recpos(t_n_scope *x, t_floatarg f)
{
  x->recpos = f;
  n_scope_calc_recpos_view(x);
  n_scope_outlet_recpos_view(x);
}

//----------------------------------------------------------------------------//
static void n_scope_size(t_n_scope *x, t_floatarg w, t_floatarg h)
{
  AF_CLIP_MINMAX(MIN_WIDTH, MAX_WIDTH, w);
  AF_CLIP_MINMAX(MIN_HEIGHT, MAX_HEIGHT, h);
  if (x->width != w || x->height != h)
    {
      x->width = w;
      x->height = h;
      x->height_1 = x->height + 1;
      n_scope_calc_h(x);
      n_scope_calc_maxel(x);
      n_scope_calc_grid_ver(x);
      n_scope_calc_recpos_view(x);
      n_scope_outlet_config_canvas(x);
      n_scope_outlet_create_background(x);
      n_scope_outlet_create_recpos(x);
      n_scope_outlet_create_grid(x);
      n_scope_outlet_create_waves(x);
      n_scope_outlet_background(x);
      n_scope_outlet_recpos_color(x);
      n_scope_outlet_recpos_view(x);
      n_scope_outlet_grid_hor(x);
      n_scope_outlet_grid_ver(x);
      n_scope_outlet_waves(x);
    }
}

//----------------------------------------------------------------------------//
static void n_scope_color(t_n_scope *x, t_floatarg fcol, t_floatarg bcol, t_floatarg rcol, t_floatarg gcol, t_floatarg scol)
{
  if (x->fcol != fcol)
    {
      x->fcol = fcol;
      n_scope_outlet_background(x);
    }
  if (x->bcol != bcol)
    {
      x->bcol = bcol;
      n_scope_outlet_background(x);
    }
  if (x->rcol != rcol)
    {
      x->rcol = rcol;
      n_scope_outlet_recpos_color(x);
    }
  if (x->gcol != gcol)
    {
      x->gcol = gcol;
      n_scope_outlet_grid_hor(x);
      n_scope_outlet_grid_ver(x);
    }
  if (x->scol != scol)
    {
      x->scol = scol;
      n_scope_outlet_grid_hor(x); 
   }
}

//----------------------------------------------------------------------------//
static void n_scope_in_on(t_n_scope *x, t_floatarg in, t_floatarg f)
{
  AF_CLIP_MINMAX(0, x->in_all - 1, in);
  if (x->c[(int)in].on != f)
    {
      x->c[(int)in].on = f;
      n_scope_calc_h(x);
      n_scope_outlet_grid_hor(x);
      n_scope_outlet_waves(x);
     }
}

//----------------------------------------------------------------------------//
static void n_scope_in_amp(t_n_scope *x, t_floatarg in, t_floatarg f)
{
  AF_CLIP_MINMAX(0, x->in_all - 1, in);
  if (x->c[(int)in].amp != f)
    {
      x->c[(int)in].amp = f;
    }
}

//----------------------------------------------------------------------------//
static void n_scope_in_color(t_n_scope *x, t_floatarg in, t_floatarg f)
{
  AF_CLIP_MINMAX(0, x->in_all - 1, in);
  if (x->c[(int)in].col != f)
    {
      x->c[(int)in].col = f;
      n_scope_outlet_waves(x);
    }
}

//----------------------------------------------------------------------------//
// dsp
//----------------------------------------------------------------------------//
t_int *n_scope_perform(t_int *w)
{
  int i;

  t_n_scope *x = (t_n_scope *)(w[1]);
  for (i = 0; i < x->in_all; i++)
    {
      x->v_in[i] = (t_sample *)(w[i + 2]);
    }
  t_int n = x->n;
  t_float in_sync;

  t_atom a[6];
  SETFLOAT(a,(t_float)9);

  int min;
  int max;
  int min_o;
  int max_o;
  t_float in_s;

  if (x->on)
    {
      // dsp
      while (n--)
	{
	  // record sync in
	  in_sync = *(x->v_in[x->sync_in]);

	  // dc filter
	  if (x->sync_dc)
	    {
	      x->dc_z = (in_sync - x->dc_z) * x->dc_f + x->dc_z;
	      in_sync = in_sync - x->dc_z;
	    }

	  // frame count
	  x->frame_count++;
	  if (x->frame_count >= x->frame_max)
	    {	    
	      x->frame_count = 0;
	      x->disp = 1;
	      n_scope_outlet_recpos_view(x);
	    }

	  // disp
	  if (x->disp)
	    {
	      // find
	      if (x->find == 0)
		{
		  // no sync
		  if (x->sync == 0)  x->find = 1;
		  // sync up
		  else if (x->sync == 1)
		    {
		      if (in_sync > x->treshold && x->in_sync_z <= x->treshold)
			{
			  x->find = 1;
			  x->disp_count = 0;
			  x->spp_count = 0;
			}
		    }
		  // sync down
		  else if (x->sync == 2)
		    {
		      if (in_sync <= x->treshold && x->in_sync_z > x->treshold)
			{
			  x->find = 1;
			  x->disp_count = 0;
			  x->spp_count = 0;
			}
		    }
		}
	      // find complete
	      if (x->find)
		{
		  // find min max //
		  for (i = 0; i < x->in_all; i++)
		    {
		      if (x->c[i].on)
			{
			  in_s = *(x->v_in[i]) * x->c[i].amp;
			  if (x->spp_count == 0)
			    {
			      x->c[i].min =  in_s;
			      x->c[i].max =  in_s;
			    }
			  else
			    {
			      if (x->c[i].min >  in_s)
				x->c[i].min = in_s;
			      else if (x->c[i].max <  in_s)
				x->c[i].max = in_s;
			    }
			}
		    }
		  // count litle //
		  x->spp_count++;
		  if (x->spp_count >= x->spp_max)
		    {
		      x->spp_count = 0;
		      // display function there //
		      for (i = 0; i < x->in_all; i++)
			{
			  if (x->c[i].on)
			    {
			      min = x->c[i].min * x->height_half;
			      max = x->c[i].max * x->height_half;

			      if (min > x->c[i].max_z)
				min = x->c[i].max_z;
			      else if (max < x->c[i].min_z)
				max = x->c[i].min_z;

			      min_o = x->c[i].grid_hor_center - min;
			      max_o = x->c[i].grid_hor_center - max;

			      if (min_o < 0)
				min_o = 0;
			      else if (min_o > x->height_1)
				min_o = x->height_1;

			      if (max_o < 0)
				max_o = 0;
			      else if (max_o > x->height_1)
				max_o = x->height_1;

			      SETFLOAT(a+1,(t_float) x->c[i].ofs + x->disp_count); // id
			      SETFLOAT(a+2,(t_float) x->disp_count+1); // x0
			      SETFLOAT(a+3,(t_float) min_o); // y0
			      SETFLOAT(a+4,(t_float) x->disp_count+1); // x1
			      SETFLOAT(a+5,(t_float) max_o); // y1
			      outlet_anything(x->out, &s_list, 6, a);

			      // z
			      x->c[i].min_z = min;
			      x->c[i].max_z = max;
			    }
			}
		      // count
		      x->disp_count++;
		    }
		  // disp count
		  if (x->disp_count >= x->width)
		    {
		      x->disp_count = 0;
		      x->find = 0;
		      x->disp = 0;
		      x->spp_count = 0;
		      for (i = 0; i < x->in_all; i++)
			{
			  if (x->c[i].on)
			    {     
			      x->c[i].min_z = -999999;
			      x->c[i].max_z = 999999;
			    }
			}
		    }
		}
	    }
	  // store
	  x->in_sync_z = in_sync;
	  // next
	  for (i = 0; i < x->in_all; i++)
	    {
	      x->v_in[i]++;
	    }
	}
    }
  
  return (w + x->in_all + 2);
}

//----------------------------------------------------------------------------//
static void n_scope_dsp(t_n_scope *x, t_signal **sp)
{
  int i;

  if (x->sr != sp[0]->s_sr)
    {
      x->sr = sp[0]->s_sr;
      n_scope_reset_sr(x);
    }
  x->n = sp[0]->s_n;
  x->v_d[0] = (t_int *)x;
  for (i = 0; i < x->in_all; i++)
    {
      x->v_d[i + 1] = (t_int *)sp[i]->s_vec;
    }
  dsp_addv(n_scope_perform, x->in_all + 1, (t_int *)x->v_d);
}

//----------------------------------------------------------------------------//
// setup
//----------------------------------------------------------------------------//
static void *n_scope_new(t_floatarg f)
{ 
  int i;
  t_n_scope *x = (t_n_scope *)pd_new(n_scope_class);
  AF_CLIP_MINMAX(1, MAX_INLETS, f);
  x->in_all = f;
  x->v_in = getbytes(sizeof(t_sample *) * x->in_all);
  x->v_d  = getbytes(sizeof(t_int *) * (x->in_all + 2));
  for (i = 1; i < x->in_all; i++)
    {
      inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    }
  x->out = outlet_new(&x->x_obj, 0);

  // init
  x->on = 0;
  x->separate = 1;
  x->grid = 1;
  x->grid_ver = 1;
  x->sync = 0;
  x->sync_in = 0;
  x->sync_dc = 0;
  x->sync_dc_hp = 10.;
  x->treshold = 0.;
  x->spp = 10;
  x->fps = 25;
  x->recpos = 0;
  x->width = 0; /* for set size */
  x->height = 0; /* for set size */
  x->fcol = -65793;
  x->bcol = -69377;
  x->rcol = -100000;
  x->gcol = -468737;
  x->scol = -70913;
  x->height_1 = x->height + 1;

  x->sr = 44100;
  n_scope_reset_sr(x);

  for (i = 0; i < x->in_all; i++)
    {
      x->c[i].on = 0;
      x->c[i].amp = 1;
      x->c[i].col = 13;
    }

  n_scope_reset(x);

  return (x);
}

//----------------------------------------------------------------------------//
static void n_scope_free(t_n_scope *x)
{
  freebytes(x->v_in, sizeof(t_sample *) * x->in_all);
  freebytes(x->v_d, sizeof(t_int *) * (x->in_all + 2));
}

//----------------------------------------------------------------------------//
void n_scope_tilde_setup(void)
{
  n_scope_class = class_new(gensym("n_scope~"), (t_newmethod)n_scope_new, (t_method)n_scope_free, sizeof(t_n_scope), 0, A_DEFFLOAT, 0);
  class_addmethod(n_scope_class, nullfn, gensym("signal"), 0);
  class_addmethod(n_scope_class, (t_method)n_scope_dsp, gensym("dsp"), 0);
  class_addmethod(n_scope_class, (t_method)n_scope_on, gensym("on"), A_DEFFLOAT, 0);
  class_addmethod(n_scope_class, (t_method)n_scope_separate, gensym("separate"), A_DEFFLOAT, 0);
  class_addmethod(n_scope_class, (t_method)n_scope_grid, gensym("grid"), A_DEFFLOAT, 0);
  class_addmethod(n_scope_class, (t_method)n_scope_grid_ver, gensym("grid_ver"), A_DEFFLOAT, 0);
  class_addmethod(n_scope_class, (t_method)n_scope_sync, gensym("sync"), A_DEFFLOAT, 0);
  class_addmethod(n_scope_class, (t_method)n_scope_sync_in, gensym("sync_in"), A_DEFFLOAT, 0);
  class_addmethod(n_scope_class, (t_method)n_scope_sync_dc, gensym("sync_dc"), A_DEFFLOAT, 0);
  class_addmethod(n_scope_class, (t_method)n_scope_sync_dc_hp, gensym("sync_dc_hp"), A_DEFFLOAT, 0);
  class_addmethod(n_scope_class, (t_method)n_scope_treshold, gensym("treshold"), A_DEFFLOAT, 0);
  class_addmethod(n_scope_class, (t_method)n_scope_spp, gensym("spp"), A_DEFFLOAT, 0);
  class_addmethod(n_scope_class, (t_method)n_scope_fps, gensym("fps"), A_DEFFLOAT, 0);
  class_addmethod(n_scope_class, (t_method)n_scope_recpos, gensym("recpos"), A_DEFFLOAT, 0);
  class_addmethod(n_scope_class, (t_method)n_scope_size, gensym("size"), A_DEFFLOAT, A_DEFFLOAT, 0);
  class_addmethod(n_scope_class, (t_method)n_scope_color, gensym("color"), A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
  class_addmethod(n_scope_class, (t_method)n_scope_in_on, gensym("in_on"), A_DEFFLOAT, A_DEFFLOAT, 0);
  class_addmethod(n_scope_class, (t_method)n_scope_in_amp, gensym("in_amp"), A_DEFFLOAT, A_DEFFLOAT, 0);
  class_addmethod(n_scope_class, (t_method)n_scope_in_color, gensym("in_color"), A_DEFFLOAT, A_DEFFLOAT, 0);
}
