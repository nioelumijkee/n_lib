#include "string.h"
#include "math.h"
#include "m_pd.h"
#include "include/constant.h"
#include "include/conversion.h"
#include "include/math.h"
#include "include/windowing.h"

#define MAX_INLETS 16
#define MIN_WIDTH 8
#define MAX_WIDTH 1024
#define MIN_HEIGHT 8
#define MAX_HEIGHT 1024
#define MAX_FPS 100.
#define MAX_BS 16384
#define MAX_HGRID 32
#define MAX_VGRID 32
#define MAX_P 1024

#define DEBUG(X)

void mayer_init( void);
void mayer_term( void);

static t_class *n_spectr_class;

typedef struct _n_spectr_in
{
  int on;
  int col;
  t_float *buf_r;
  t_float *buf_i;
  t_float *buf_e;
  int ofs;
} t_n_spectr_in;

typedef struct _n_spectr
{
  t_object x_obj;
  t_outlet *out;               /* outlet */
  t_outlet *out_vic;           /* outlet */
  t_float sr;                  /* sample rate */
  t_int n;                     /* block size */
  t_int in_all;                /* number in's */
  t_sample **v_in;             /* vector */
  t_int **v_d;                 /* vector for dsp_addv */
  int on;                      /* parameters */
  int grid;                    /* grid: off, on */
  int grid_hor;                /* lin, log */
  int grid_ver;                /* lin, log */
  int vic;                     /* on, off */
  int vic_x;                   /* vic coords */
  int vic_y;
  int bs;                      /* size fft */
  int window;                  /* windowing */
  t_float wincoef;
  t_float hor_lin_min;         /* minmax */
  t_float hor_lin_max;
  t_float hor_log_min;
  t_float hor_log_max;
  t_float env;                 /* env */
  t_float fps;                 /* fps */
  int width;                   /* size */
  int height;
  int fcol;                    /* colors */
  int bcol;
  int gcol;
  int vcol;           
  int tcol;   
  int text_view;               /* text settings */
  int fs;
  int grid_hor_dx;
  int grid_hor_dy;
  int grid_ver_dx;
  int grid_ver_dy;
  int height_1;                /* height + 1 */
  t_n_spectr_in c[MAX_INLETS]; /* in's */
  int maxel;                   /* display settings */
  int ofs_grid_hor_line;       /* offsets elements */
  int ofs_grid_hor_text;
  int ofs_grid_ver_line;
  int ofs_grid_ver_text;
  int ofs_waves;
  int ofs_vic; 
  int frame_count;             /* frame */
  int frame_max;
  int dsp_count;               /* dsp count */
  int     grid_hor_p[MAX_HGRID];
  t_float grid_hor_v[MAX_HGRID];
  int     grid_hor_all;
  int     grid_ver_p[MAX_VGRID];
  t_float grid_ver_v[MAX_VGRID];
  int     grid_ver_all;
  t_float env_i;
  t_float *win;
  t_float rng_mul;
  t_float rng_add;
  int p_all;
  int p_type[MAX_P];            /* ??? */
  int p_start[MAX_P];
  int p_end[MAX_P];
  int p_x0[MAX_P];
  int p_x1[MAX_P];
  int p_max[MAX_P];
  int p_min[MAX_P];
} t_n_spectr;

//----------------------------------------------------------------------------//
// outlet
//----------------------------------------------------------------------------//
static void n_spectr_outlet_config_canvas(t_n_spectr *x)
{
  t_atom a[2];
  SETFLOAT(a,(t_float)x->width + 1);
  SETFLOAT(a+1,(t_float)x->height + 1);
  outlet_anything(x->out, gensym("size"), 2, a);
  SETFLOAT(a,(t_float)x->maxel);
  outlet_anything(x->out, gensym("maxel"), 1, a);
}

//----------------------------------------------------------------------------//
static void n_spectr_outlet_create_background(t_n_spectr *x)
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
static void n_spectr_outlet_create_grid(t_n_spectr *x)
{
  int i,j;
  t_atom a[8];
  // hor line
  SETFLOAT(a,(t_float)1);
  for(j=0; j<MAX_HGRID; j++)
    {
      i = x->ofs_grid_hor_line+j;
      SETFLOAT(a+1,(t_float)i); // id
      SETFLOAT(a+2,(t_float)x->fcol); // color
      SETFLOAT(a+3,(t_float)1); // width
      SETFLOAT(a+4,(t_float)0); // x0
      SETFLOAT(a+5,(t_float)0); // y0
      SETFLOAT(a+6,(t_float)0); // x1
      SETFLOAT(a+7,(t_float)0); // y1
      outlet_anything(x->out, &s_list, 8, a);
    }
  // hor text
  SETFLOAT(a,(t_float)8);
  for(j=0; j<MAX_HGRID; j++)
    {
      i = x->ofs_grid_hor_text+j;
      SETFLOAT(a+1,(t_float)i); // id
      SETFLOAT(a+2,(t_float)x->fcol); // color
      SETFLOAT(a+3,(t_float)x->fs); // fs
      SETFLOAT(a+4,(t_float)0); // x0
      SETFLOAT(a+5,(t_float)0); // y0
      SETSYMBOL(a+6,gensym("")); // text
      outlet_anything(x->out, &s_list, 7, a);
    }
  // ver line
  SETFLOAT(a,(t_float)1);
  for(j=0; j<MAX_VGRID; j++)
    {
      i = x->ofs_grid_ver_line+j;
      SETFLOAT(a+1,(t_float)i); // id
      SETFLOAT(a+2,(t_float)x->fcol); // color
      SETFLOAT(a+3,(t_float)1); // width
      SETFLOAT(a+4,(t_float)0); // x0
      SETFLOAT(a+5,(t_float)0); // y0
      SETFLOAT(a+6,(t_float)0); // x1
      SETFLOAT(a+7,(t_float)0); // y1
      outlet_anything(x->out, &s_list, 8, a);
    }
  // ver text
  SETFLOAT(a,(t_float)8);
  for(j=0; j<MAX_VGRID; j++)
    {
      i = x->ofs_grid_ver_text+j;
      SETFLOAT(a+1,(t_float)i); // id
      SETFLOAT(a+2,(t_float)x->fcol); // color
      SETFLOAT(a+3,(t_float)x->fs); // fs
      SETFLOAT(a+4,(t_float)0); // x0
      SETFLOAT(a+5,(t_float)0); // y0
      SETSYMBOL(a+6,gensym("")); // text
      outlet_anything(x->out, &s_list, 7, a);
    }
}

//----------------------------------------------------------------------------//
static void n_spectr_outlet_create_waves(t_n_spectr *x)
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
static void n_spectr_outlet_create_vic(t_n_spectr *x)
{
  int i,j;
  t_atom a[8];
  // lines
  SETFLOAT(a,(t_float)1);
  for(j=0; j< 4; j++)
    {
      i = x->ofs_vic+j;
      SETFLOAT(a+1,(t_float)i); // id
      SETFLOAT(a+2,(t_float)x->fcol); // color
      SETFLOAT(a+3,(t_float)1); // width
      SETFLOAT(a+4,(t_float)0); // x0
      SETFLOAT(a+5,(t_float)0); // y0
      SETFLOAT(a+6,(t_float)0); // x1
      SETFLOAT(a+7,(t_float)0); // y1
      outlet_anything(x->out, &s_list, 8, a);
    }
  // sq
  SETFLOAT(a,(t_float)2);
  i = x->ofs_vic+j;
  SETFLOAT(a+1,(t_float)i); // id
  SETFLOAT(a+2,(t_float)x->fcol); // color
  SETFLOAT(a+3,(t_float)1); // width
  SETFLOAT(a+4,(t_float)0); // x0
  SETFLOAT(a+5,(t_float)0); // y0
  SETFLOAT(a+6,(t_float)0); // x1
  SETFLOAT(a+7,(t_float)0); // y1
  outlet_anything(x->out, &s_list, 8, a);
}

//----------------------------------------------------------------------------//
static void n_spectr_outlet_background(t_n_spectr *x)
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
static void n_spectr_outlet_grid_hor(t_n_spectr *x)
{
  int i,j;
  t_atom a[6];
  char str[32];
  // move line
  SETFLOAT(a,(t_float)9);
  for(j=0; j<MAX_HGRID; j++)
    {
      if (j < x->grid_hor_all && x->grid)
	{
	  i = x->ofs_grid_hor_line + j;
	  SETFLOAT(a+1,(t_float)i); // id
	  SETFLOAT(a+2,(t_float)1); // x0
	  SETFLOAT(a+3,(t_float)x->grid_hor_p[j]); // y0
	  SETFLOAT(a+4,(t_float)x->width); // x1
	  SETFLOAT(a+5,(t_float)x->grid_hor_p[j]); // y1
	  outlet_anything(x->out, &s_list, 6, a);
	}
      else
	{
	  i = x->ofs_grid_hor_line + j;
	  SETFLOAT(a+1,(t_float)i); // id
	  SETFLOAT(a+2,(t_float)0); // x0
	  SETFLOAT(a+3,(t_float)0); // y0
	  SETFLOAT(a+4,(t_float)0); // x1
	  SETFLOAT(a+5,(t_float)0); // y1
	  outlet_anything(x->out, &s_list, 6, a);
	}
    }
  // color line
  SETFLOAT(a,(t_float)10);
  for(j=0; j<MAX_HGRID; j++)
    {
      if (j < x->grid_hor_all && x->grid)
	{
	  i = x->ofs_grid_hor_line + j;
	  SETFLOAT(a+1,(t_float)i); // id
	  SETFLOAT(a+2,(t_float)x->gcol); // color
	  outlet_anything(x->out, &s_list, 3, a);
	}
      else
	{
	  i = x->ofs_grid_hor_line + j;
	  SETFLOAT(a+1,(t_float)i); // id
	  SETFLOAT(a+2,(t_float)x->fcol); // color
	  outlet_anything(x->out, &s_list, 3, a);
	}
    }
  // move text
  SETFLOAT(a,(t_float)9);
  for(j=0; j<MAX_HGRID; j++)
    {
      if (j < x->grid_hor_all && x->grid && x->text_view)
	{
	  i = x->ofs_grid_hor_text + j;
	  SETFLOAT(a+1,(t_float)i); // id
	  SETFLOAT(a+2,(t_float)x->width         + x->grid_hor_dx); // x0
	  SETFLOAT(a+3,(t_float)x->grid_hor_p[j] + x->grid_hor_dy); // y0
	  outlet_anything(x->out, &s_list, 4, a);
	}
      else
	{
	  i = x->ofs_grid_hor_text + j;
	  SETFLOAT(a+1,(t_float)i); // id
	  SETFLOAT(a+2,(t_float)0); // x0
	  SETFLOAT(a+3,(t_float)0); // y0
	  outlet_anything(x->out, &s_list, 4, a);
	}
    }
  // color text
  SETFLOAT(a,(t_float)10);
  for(j=0; j<MAX_HGRID; j++)
    {
      if (j < x->grid_hor_all && x->grid && x->text_view)
	{
	  i = x->ofs_grid_hor_text + j;
	  SETFLOAT(a+1,(t_float)i); // id
	  SETFLOAT(a+2,(t_float)x->tcol); // color
	  outlet_anything(x->out, &s_list, 3, a);
	}
      else
	{
	  i = x->ofs_grid_hor_text + j;
	  SETFLOAT(a+1,(t_float)i); // id
	  SETFLOAT(a+2,(t_float)x->fcol); // color
	  outlet_anything(x->out, &s_list, 3, a);
	}
    }
  // fs text
  SETFLOAT(a,(t_float)11);
  for(j=0; j<MAX_HGRID; j++)
    {
      if (j < x->grid_hor_all && x->grid && x->text_view)
	{
	  i = x->ofs_grid_hor_text + j;
	  SETFLOAT(a+1,(t_float)i); // id
	  SETFLOAT(a+2,(t_float)x->fs); // fsr
	  outlet_anything(x->out, &s_list, 3, a);
	}
      else
	{
	  i = x->ofs_grid_hor_text + j;
	  SETFLOAT(a+1,(t_float)i); // id
	  SETFLOAT(a+2,(t_float)x->fs); // fs
	  outlet_anything(x->out, &s_list, 3, a);
	}
    }
  // text text
  SETFLOAT(a,(t_float)13);
  for(j=0; j<MAX_HGRID; j++)
    {
      if (j < x->grid_hor_all && x->grid && x->text_view)
	{
	  i = x->ofs_grid_hor_text + j;
	  sprintf(str, "%g",x->grid_hor_v[j]);
	  SETFLOAT(a+1,(t_float)i); // id
	  SETSYMBOL(a+2, gensym(str)); // text
	  outlet_anything(x->out, &s_list, 3, a);
	}
      else
	{
	  i = x->ofs_grid_hor_text + j;
	  SETFLOAT(a+1,(t_float)i); // id
	  SETSYMBOL(a+2, gensym("")); // text
	  outlet_anything(x->out, &s_list, 3, a);
	}
    }
}

//----------------------------------------------------------------------------//
static void n_spectr_outlet_grid_ver(t_n_spectr *x)
{
  int i,j;
  t_atom a[6];
  char str[32];
  // move line
  SETFLOAT(a,(t_float)9);
  for(j=0; j<MAX_VGRID; j++)
    {
      if (j < x->grid_ver_all && x->grid)
	{
	  i = x->ofs_grid_ver_line + j;
	  SETFLOAT(a+1,(t_float)i); // id
	  SETFLOAT(a+2,(t_float)x->grid_ver_p[j]); // x0
	  SETFLOAT(a+3,(t_float)1); // y0
	  SETFLOAT(a+4,(t_float)x->grid_ver_p[j]); // x1
	  SETFLOAT(a+5,(t_float)x->height); // y1
	  outlet_anything(x->out, &s_list, 6, a);
	}
      else
	{
	  i = x->ofs_grid_ver_line + j;
	  SETFLOAT(a+1,(t_float)i); // id
	  SETFLOAT(a+2,(t_float)0); // x0
	  SETFLOAT(a+3,(t_float)0); // y0
	  SETFLOAT(a+4,(t_float)0); // x1
	  SETFLOAT(a+5,(t_float)0); // y1
	  outlet_anything(x->out, &s_list, 6, a);
	}
    }
  // color line
  SETFLOAT(a,(t_float)10);
  for(j=0; j<MAX_VGRID; j++)
    {
      if (j < x->grid_ver_all && x->grid)
	{
	  i = x->ofs_grid_ver_line + j;
	  SETFLOAT(a+1,(t_float)i); // id
	  SETFLOAT(a+2,(t_float)x->gcol); // color
	  outlet_anything(x->out, &s_list, 3, a);
	}
      else
	{
	  i = x->ofs_grid_ver_line + j;
	  SETFLOAT(a+1,(t_float)i); // id
	  SETFLOAT(a+2,(t_float)x->fcol); // color
	  outlet_anything(x->out, &s_list, 3, a);
	}
    }
  // move text
  SETFLOAT(a,(t_float)9);
  for(j=0; j<MAX_VGRID; j++)
    {
      if (j < x->grid_ver_all && x->grid && x->text_view)
	{
	  i = x->ofs_grid_ver_text + j;
	  SETFLOAT(a+1,(t_float)i); // id
	  SETFLOAT(a+2,(t_float)x->grid_ver_p[j] + x->grid_ver_dx); // x0
	  SETFLOAT(a+3,(t_float)x->height        + x->grid_ver_dy); // y0
	  outlet_anything(x->out, &s_list, 4, a);
	}
      else
	{
	  i = x->ofs_grid_ver_text + j;
	  SETFLOAT(a+1,(t_float)i); // id
	  SETFLOAT(a+2,(t_float)0); // x0
	  SETFLOAT(a+3,(t_float)0); // y0
	  outlet_anything(x->out, &s_list, 4, a);
	}
    }
  // color text
  SETFLOAT(a,(t_float)10);
  for(j=0; j<MAX_VGRID; j++)
    {
      if (j < x->grid_ver_all && x->grid && x->text_view)
	{
	  i = x->ofs_grid_ver_text + j;
	  SETFLOAT(a+1,(t_float)i); // id
	  SETFLOAT(a+2,(t_float)x->tcol); // color
	  outlet_anything(x->out, &s_list, 3, a);
	}
      else
	{
	  i = x->ofs_grid_ver_text + j;
	  SETFLOAT(a+1,(t_float)i); // id
	  SETFLOAT(a+2,(t_float)x->fcol); // color
	  outlet_anything(x->out, &s_list, 3, a);
	}
    }
  // fs text
  SETFLOAT(a,(t_float)11);
  for(j=0; j<MAX_VGRID; j++)
    {
      if (j < x->grid_ver_all && x->grid && x->text_view)
	{
	  i = x->ofs_grid_ver_text + j;
	  SETFLOAT(a+1,(t_float)i); // id
	  SETFLOAT(a+2,(t_float)x->fs); // fsr
	  outlet_anything(x->out, &s_list, 3, a);
	}
      else
	{
	  i = x->ofs_grid_ver_text + j;
	  SETFLOAT(a+1,(t_float)i); // id
	  SETFLOAT(a+2,(t_float)x->fs); // fs
	  outlet_anything(x->out, &s_list, 3, a);
	}
    }
  // text text
  SETFLOAT(a,(t_float)13);
  for(j=0; j<MAX_VGRID; j++)
    {
      if (j < x->grid_ver_all && x->grid && x->text_view)
	{
	  i = x->ofs_grid_ver_text + j;
	  sprintf(str, "%g",x->grid_ver_v[j]);
	  SETFLOAT(a+1,(t_float)i); // id
	  SETSYMBOL(a+2, gensym(str)); // text
	  outlet_anything(x->out, &s_list, 3, a);
	}
      else
	{
	  i = x->ofs_grid_ver_text + j;
	  SETFLOAT(a+1,(t_float)i); // id
	  SETSYMBOL(a+2, gensym("")); // text
	  outlet_anything(x->out, &s_list, 3, a);
	}
    }
}

//----------------------------------------------------------------------------//
static void n_spectr_outlet_waves(t_n_spectr *x)
{
  int i,j,k;
  t_atom a[6];
  // move line
  SETFLOAT(a,(t_float)9);
  for(i=0; i<x->in_all; i++)
    {
      if (x->c[i].on)
	{
	  for (j=0; j<x->p_all; j++)
	    {
	      k = x->c[i].ofs + j;
	      SETFLOAT(a+1,(t_float)k); // id
	      SETFLOAT(a+2,(t_float)j); // x0
	      SETFLOAT(a+3,(t_float)((i + 1) * 10)); // y0
	      SETFLOAT(a+4,(t_float)j); // x1
	      SETFLOAT(a+5,(t_float)((i + 1) * 10)); // y1
	      outlet_anything(x->out, &s_list, 6, a);
	    }
	}
      else
	{
	  for (j=0; j<x->p_all; j++)
	    {
	      k = x->c[i].ofs + j;
	      SETFLOAT(a+1,(t_float)k); // id
	      SETFLOAT(a+2,(t_float)0); // x0
	      SETFLOAT(a+3,(t_float)0); // y0
	      SETFLOAT(a+4,(t_float)0); // x1
	      SETFLOAT(a+5,(t_float)0); // y1
	      outlet_anything(x->out, &s_list, 6, a);
	    }
	}
    } 
  // color line
  SETFLOAT(a,(t_float)10);
  for(i=0; i<x->in_all; i++)
    {
      if (x->c[i].on)
	{
	  for (j=0; j<x->p_all; j++)
	    {
	      k = x->c[i].ofs + j;
	      SETFLOAT(a+1,(t_float)k); // id
	      SETFLOAT(a+2,(t_float)x->c[i].col); // color
	      outlet_anything(x->out, &s_list, 3, a);
	    }
	}
      else
	{
	  for (j=0; j<x->p_all; j++)
	    {
	      k = x->c[i].ofs + j;
	      SETFLOAT(a+1,(t_float)k); // id
	      SETFLOAT(a+2,(t_float)x->fcol); // color
	      outlet_anything(x->out, &s_list, 3, a);
	    }
	}
    } 
}

//----------------------------------------------------------------------------//
static void n_spectr_outlet_vic(t_n_spectr *x)
{
  t_atom a[6];
  // move line
  SETFLOAT(a,(t_float)9);
  if (x->vic)
    {
      // -
      SETFLOAT(a+1,(t_float)x->ofs_vic); // id
      SETFLOAT(a+2,(t_float)0); // x0
      SETFLOAT(a+3,(t_float)x->vic_y); // y0
      SETFLOAT(a+4,(t_float)x->vic_x-2); // x1
      SETFLOAT(a+5,(t_float)x->vic_y); // y1
      outlet_anything(x->out, &s_list, 6, a);
      // -
      SETFLOAT(a+1,(t_float)x->ofs_vic+1); // id
      SETFLOAT(a+2,(t_float)x->vic_x+2); // x0
      SETFLOAT(a+3,(t_float)x->vic_y); // y0
      SETFLOAT(a+4,(t_float)x->width); // x1
      SETFLOAT(a+5,(t_float)x->vic_y); // y1
      outlet_anything(x->out, &s_list, 6, a);
      // |
      SETFLOAT(a+1,(t_float)x->ofs_vic+2); // id
      SETFLOAT(a+2,(t_float)x->vic_x); // x0
      SETFLOAT(a+3,(t_float)0); // y0
      SETFLOAT(a+4,(t_float)x->vic_x); // x1
      SETFLOAT(a+5,(t_float)x->vic_y-2); // y1
      outlet_anything(x->out, &s_list, 6, a);
      // |
      SETFLOAT(a+1,(t_float)x->ofs_vic+3); // id
      SETFLOAT(a+2,(t_float)x->vic_x); // x0
      SETFLOAT(a+3,(t_float)x->vic_y+2); // y0
      SETFLOAT(a+4,(t_float)x->vic_x); // x1
      SETFLOAT(a+5,(t_float)x->height); // y1
      outlet_anything(x->out, &s_list, 6, a);
    }
  else
    {
      // -
      SETFLOAT(a+1,(t_float)x->ofs_vic); // id
      SETFLOAT(a+2,(t_float)0); // x0
      SETFLOAT(a+3,(t_float)0); // y0
      SETFLOAT(a+4,(t_float)0); // x1
      SETFLOAT(a+5,(t_float)0); // y1
      outlet_anything(x->out, &s_list, 6, a);
      // -
      SETFLOAT(a+1,(t_float)x->ofs_vic+1); // id
      SETFLOAT(a+2,(t_float)0); // x0
      SETFLOAT(a+3,(t_float)0); // y0
      SETFLOAT(a+4,(t_float)0); // x1
      SETFLOAT(a+5,(t_float)0); // y1
      outlet_anything(x->out, &s_list, 6, a);
      // |
      SETFLOAT(a+1,(t_float)x->ofs_vic+2); // id
      SETFLOAT(a+2,(t_float)0); // x0
      SETFLOAT(a+3,(t_float)0); // y0
      SETFLOAT(a+4,(t_float)0); // x1
      SETFLOAT(a+5,(t_float)0); // y1
      outlet_anything(x->out, &s_list, 6, a);
      // |
      SETFLOAT(a+1,(t_float)x->ofs_vic+3); // id
      SETFLOAT(a+2,(t_float)0); // x0
      SETFLOAT(a+3,(t_float)0); // y0
      SETFLOAT(a+4,(t_float)0); // x1
      SETFLOAT(a+5,(t_float)0); // y1
      outlet_anything(x->out, &s_list, 6, a);
    }
 
  // color line
  SETFLOAT(a,(t_float)10);
  if (x->vic)
    {
      // -
      SETFLOAT(a+1,(t_float)x->ofs_vic); // id
      SETFLOAT(a+2,(t_float)x->vcol); // color
      outlet_anything(x->out, &s_list, 3, a);
      // -
      SETFLOAT(a+1,(t_float)x->ofs_vic+1); // id
      SETFLOAT(a+2,(t_float)x->vcol); // color
      outlet_anything(x->out, &s_list, 3, a);
      // |
      SETFLOAT(a+1,(t_float)x->ofs_vic+2); // id
      SETFLOAT(a+2,(t_float)x->vcol); // color
      outlet_anything(x->out, &s_list, 3, a);
      // |
      SETFLOAT(a+1,(t_float)x->ofs_vic+3); // id
      SETFLOAT(a+2,(t_float)x->vcol); // color
      outlet_anything(x->out, &s_list, 3, a);
    }
  else
    {
      // -
      SETFLOAT(a+1,(t_float)x->ofs_vic); // id
      SETFLOAT(a+2,(t_float)x->fcol); // color
      outlet_anything(x->out, &s_list, 3, a);
      // -
      SETFLOAT(a+1,(t_float)x->ofs_vic+1); // id
      SETFLOAT(a+2,(t_float)x->fcol); // color
      outlet_anything(x->out, &s_list, 3, a);
      // |
      SETFLOAT(a+1,(t_float)x->ofs_vic+2); // id
      SETFLOAT(a+2,(t_float)x->fcol); // color
      outlet_anything(x->out, &s_list, 3, a);
      // |
      SETFLOAT(a+1,(t_float)x->ofs_vic+3); // id
      SETFLOAT(a+2,(t_float)x->fcol); // color
      outlet_anything(x->out, &s_list, 3, a);
    } 
}

//----------------------------------------------------------------------------//
static void n_spectr_outlet_all(t_n_spectr *x)
{
  n_spectr_outlet_config_canvas(x);
  n_spectr_outlet_create_background(x);
  n_spectr_outlet_create_grid(x);
  n_spectr_outlet_create_waves(x);
  n_spectr_outlet_create_vic(x);
  n_spectr_outlet_background(x);
  n_spectr_outlet_grid_hor(x);
  n_spectr_outlet_grid_ver(x);
  n_spectr_outlet_waves(x);
  n_spectr_outlet_vic(x);
}

//----------------------------------------------------------------------------//
// calculations
//----------------------------------------------------------------------------//
static void n_spectr_calc_points(t_n_spectr *x)
{
  int i;
  // lin
  if (x->grid_ver == 0)
    {
      int j;
      t_float bs2 = x->bs / 2.;
      int b_start  = 0;
      int b_end    = bs2;
      int b_range  = bs2;
      t_float f;
      t_float b_inc = bs2 / x->width;
      t_float d_mul = (t_float)x->width / (t_float)b_range;
      t_float d_add = 0. - b_start;

      // init first / start end
      x->p_all = 0;
      x->p_start[x->p_all] = b_start;
      for (i = 0; i < x->width; i++)
	{
	  f = (i * b_inc) + b_start;
	  j = f;
	  
	  if (j > x->p_start[x->p_all])
	    {
	      x->p_end[x->p_all] = j;
	      x->p_all++;
	      x->p_start[x->p_all] = j;
	    }
	}
      x->p_end[x->p_all] = b_end;
      x->p_all++;


      // type
      for (i=0; i<x->p_all; i++)
	{
	  x->p_x0[i] = ((t_float)x->p_start[i] + d_add) * d_mul;
	  x->p_x1[i] = ((t_float)x->p_end[i]   + d_add) * d_mul;

	  if (x->p_x0[i] == x->p_x1[i] - 1)
	    {
	      // type 1 |
	      if (x->p_start[i] == x->p_end[i] - 1)
		{
		  x->p_type[i] = 1;
		}
	      // type 2 |||
	      else
		{
		  x->p_type[i] = 2;
		}
	    }
	  // type 0 /
	  else
	    {
	      x->p_type[i] = 0;
	    }
	}
    }
  // log
  else
    {
      int j;
      t_float bs2 = x->bs / 2.;
      t_float sr2 = x->sr / 2.;
      int b_start  = 1;
      int b_end    = bs2;
      t_float f_inc = sr2 / bs2;
      t_float f_start = b_start * f_inc;
      t_float f_end   = b_end   * f_inc;
      t_float m_start;
      t_float m_end;
      t_float m_range;
      t_float f;
      AF_F2M(f_start, m_start);
      AF_F2M(f_end, m_end);
      m_range = m_end - m_start;
      t_float m_inc = m_range / x->width;

      // init first / start end
      x->p_all = 0;
      x->p_start[x->p_all] = b_start;
      x->p_x0[x->p_all] = 0;
      for (i = 0; i < x->width; i++)
	{
	  // pitch
	  f = (i * m_inc) + m_start;
	  // freq
	  AF_M2F(f,f);
	  // block
	  j = (f / sr2) * bs2;

	  if (j > x->p_start[x->p_all])
	    {
	      x->p_end[x->p_all] = j;
	      x->p_x1[x->p_all] = i;
	      x->p_all++;
	      x->p_start[x->p_all] = j;
	      x->p_x0[x->p_all] = i;
	    }
	}
      x->p_end[x->p_all] = b_end;
      x->p_x1[x->p_all] = x->width;
      x->p_all++;

      // type
      for (i=0; i<x->p_all; i++)
	{
	  if (x->p_x0[i] == x->p_x1[i] - 1)
	    {
	      // type 1 |
	      if (x->p_start[i] == x->p_end[i] - 1)
		{
		  x->p_type[i] = 1;
		}
	      // type 2 |||
	      else
		{
		  x->p_type[i] = 2;
		}
	    }
	  // type 0 /
	  else
	    {
	      x->p_type[i] = 0;
	    }
	}
    }
  // debug
  DEBUG(for (i=0; i<x->p_all; i++)
	  {
	    post("[%3d] [%4d]-[%4d] [%4d]-[%4d] [%d]",
		 i,
		 x->p_start[i],
		 x->p_end[i],
		 x->p_x0[i],
		 x->p_x1[i],
		 x->p_type[i]);
	  });
}

//----------------------------------------------------------------------------//
static void n_spectr_calc_maxel(t_n_spectr *x)
{
  x->maxel = 1 +             // background
    MAX_HGRID +              // grid hor line
    MAX_HGRID +              // grid hor text
    MAX_VGRID +              // grid ver line
    MAX_VGRID +              // grid ver text
    (x->in_all * x->p_all) + // waves
    5;                       // vic 
  x->ofs_grid_hor_line      = 1; 
  x->ofs_grid_hor_text      = x->ofs_grid_hor_line + MAX_HGRID;
  x->ofs_grid_ver_line      = x->ofs_grid_hor_text + MAX_HGRID;
  x->ofs_grid_ver_text      = x->ofs_grid_ver_line + MAX_VGRID;
  x->ofs_waves              = x->ofs_grid_ver_text + MAX_VGRID;
  x->ofs_vic                = x->ofs_waves + (x->in_all * x->p_all);
  int i;
  for (i=0; i<x->in_all; i++)
    {
      x->c[i].ofs = x->ofs_waves + (i * x->p_all);
    }
}

//----------------------------------------------------------------------------//
static void n_spectr_calc_frame(t_n_spectr *x)
{
  x->frame_max = (t_float)x->sr / (t_float)x->fps;
}

//----------------------------------------------------------------------------//
static void n_spectr_calc_win(t_n_spectr *x)
{
  int i;
  t_float a,b,c;
  if (x->window == 0)
    {
      AF_WINDOWING_NONE(0, x->bs, x->win, ,i);
    }
  else if (x->window == 1)
    {
      AF_WINDOWING_BARTLETT(0, x->bs, x->win, ,i, a, b, x->wincoef);
    }
  else if (x->window == 2)
    {
      AF_WINDOWING_BLACKMAN(0, x->bs, x->win, ,i, a, b, c, x->wincoef);
    }
  else if (x->window == 3)
    {
      AF_WINDOWING_CONNES(0, x->bs, x->win, ,i, a, b, x->wincoef);
    }
  else if (x->window == 4)
    {
      AF_WINDOWING_GAUSSIAN(0, x->bs, x->win, ,i, a, b, x->wincoef);
    }
  else if (x->window == 5)
    {
      AF_WINDOWING_HANNING(0, x->bs, x->win, ,i, a, b, x->wincoef);
    }
  else if (x->window == 6)
    {
      AF_WINDOWING_HAMMING(0, x->bs, x->win, ,i, a, b, x->wincoef);
    }
  else if (x->window == 7)
    {
      AF_WINDOWING_LANCZOS(0, x->bs, x->win, ,i, a, b, x->wincoef);
    }
  else if (x->window == 8)
    {
      AF_WINDOWING_SIN(0, x->bs, x->win, ,i, a, x->wincoef);
    }
  else if (x->window == 9)
    {
      AF_WINDOWING_WELCH(0, x->bs, x->win, ,i, a, b, x->wincoef);
    }
}

//----------------------------------------------------------------------------//
static void n_spectr_calc_env(t_n_spectr *x)
{
  t_float f = x->env - 60.;
  f = pow(1.122, f);
  f = f * (x->sr / (x->bs * 0.5));
  if (f < 1.)
    f = 1.;
  x->env_i = 0.6931471824645996 / f;
}

//----------------------------------------------------------------------------//
static void n_spectr_calc_hor_minmax(t_n_spectr *x)
{
  t_float max;
  t_float diff;

  // calc lin
  if (x->hor_lin_min > x->hor_lin_max)
    {
      max = x->hor_lin_min;
      x->hor_lin_min = x->hor_lin_max;
      x->hor_lin_max = max;
    }

  // calc log
  if (x->hor_log_min > x->hor_log_max)
    {
      max = x->hor_log_min;
      x->hor_log_min = x->hor_log_max;
      x->hor_log_max = max;
    }

  // there calc mult and add
  // lin
  if (x->grid_hor == 0)
    {
      diff = x->hor_lin_max - x->hor_lin_min;
      x->rng_add = 0. - x->hor_lin_min;
      x->rng_mul = 1. / diff;
    }
  // log
  else
    {
      diff = x->hor_log_max - x->hor_log_min;
      x->rng_add = 0. - x->hor_log_min - 100.;
      x->rng_mul = 1. / diff;
    }
}

//----------------------------------------------------------------------------//
static void n_spectr_calc_grid_hor(t_n_spectr *x)
{
  int i;
  t_float diff;
  t_float step;
  t_float f,b;

  // log
  if (x->grid_hor == 1)
    {
      diff = x->hor_log_max - x->hor_log_min;
      
      if (diff > 120)
	step = 20;
      else if (diff > 60)
	step = 10;
      else if (diff > 30)
	step = 5;
      else if (diff > 15)
	step = 2.5;
      else if (diff > 6)
	step = 1;
      else if (diff > 3)
	step = 0.5;
      else if (diff > 1.5)
	step = 0.25;
      else if (diff > 0.75)
	step = 0.125;
      else
	step = 0.125;
      
      i = x->hor_log_max / step;
      if (x->hor_log_max >= 0)
	f = step * i;
      else
	f = step * (i - 1);
      x->grid_hor_all = 0;

      while (f >= x->hor_log_min)
	{
	  x->grid_hor_v[x->grid_hor_all] = f;
	  b = x->hor_log_max - f;
	  b = b / diff;
	  x->grid_hor_p[x->grid_hor_all] = b * (x->height + 1); /* add 1 */
	  x->grid_hor_all++;
	  f -= step;
	}
    }
  // lin
  else
    {
      diff = x->hor_lin_max - x->hor_lin_min;
      
      if (diff > 120)
	step = 20;
      else if (diff > 60)
	step = 10;
      else if (diff > 30)
	step = 5;
      else if (diff > 15)
	step = 2.5;
      else if (diff > 6)
	step = 1;
      else if (diff > 3)
	step = 0.5;
      else if (diff > 1.5)
	step = 0.25;
      else if (diff > 0.75)
	step = 0.125;
      else if (diff > 0.375)
	step = 0.06125;
      else if (diff > 0.1875)
	step = 0.03125;
      else if (diff > 0.09375)
	step = 0.015625;
      else
	step = 0.015625;
      
      i = x->hor_lin_max / step;
      if (x->hor_lin_max > 0)
	f = step * i;
      else
	f = step * (i - 1);
      x->grid_hor_all = 0;

      while (f >= x->hor_lin_min)
	{
	  x->grid_hor_v[x->grid_hor_all] = f;
	  b = x->hor_lin_max - f;
	  b = b / diff;
	  x->grid_hor_p[x->grid_hor_all] = b * (x->height + 1); // add 1
	  x->grid_hor_all++;
	  f -= step;
	}
    }
}

//----------------------------------------------------------------------------//
static void n_spectr_calc_grid_ver(t_n_spectr *x)
{
  int i,j;
  t_float diff;
  t_float step;
  t_float f,b;
  t_float min, max;

  // log
  if (x->grid_ver == 1)
    {
      t_float min_freq = (x->sr / 2.)/ (x->bs / 2.);
      t_float max_freq =  x->sr / 2.;

      AF_F2M(min_freq, min);
      AF_F2M(max_freq, max);

      diff = max - min;
      step = 12.;
            
      i = max / step;
      f = step * i;
      x->grid_ver_all = 0;

      while (f >= min)
	{
	  x->grid_ver_v[x->grid_ver_all] = f;
	  b = max - f;
	  b = b / diff;
	  x->grid_ver_p[x->grid_ver_all] = (x->width + 1) - (b * (x->width + 1)); // add 1 
	  x->grid_ver_all++;
	  f -= step;
	}
    }
  // lin
  else
    {
      j = (x->sr / 2.) / 2000.;
      for(i=0; i<j; i++)
       	{
       	  x->grid_ver_v[i] = i * 2000;
       	  x->grid_ver_p[i] = (x->grid_ver_v[i] /  (x->sr * 0.5)) * (x->width + 1); // add 1
       	}
      x->grid_ver_all = j;
    }
}

//----------------------------------------------------------------------------//
static void n_spectr_reset_sr(t_n_spectr *x)
{
  n_spectr_calc_env(x);
  n_spectr_calc_frame(x);
}

//----------------------------------------------------------------------------//
static void n_spectr_reset(t_n_spectr *x)
{
  x->dsp_count = 0;
}

//----------------------------------------------------------------------------//
// par.
//----------------------------------------------------------------------------//
static void n_spectr_on(t_n_spectr *x, t_floatarg f)
{
  x->on = f;
}

//----------------------------------------------------------------------------//
static void n_spectr_grid(t_n_spectr *x, t_floatarg f)
{
  x->grid = f;
  n_spectr_outlet_grid_hor(x);
  n_spectr_outlet_grid_ver(x);
}

//----------------------------------------------------------------------------//
static void n_spectr_grid_hor(t_n_spectr *x, t_floatarg f)
{
  x->grid_hor = f;
  n_spectr_calc_hor_minmax(x);
  n_spectr_calc_grid_hor(x);
  n_spectr_outlet_grid_hor(x);
}

//----------------------------------------------------------------------------//
static void n_spectr_grid_ver(t_n_spectr *x, t_floatarg f)
{
  x->grid_ver = f;
  n_spectr_calc_grid_ver(x);
  n_spectr_calc_points(x);
  n_spectr_calc_maxel(x);
  n_spectr_reset(x);
  n_spectr_outlet_all(x);
}

//----------------------------------------------------------------------------//
static void n_spectr_vic(t_n_spectr *x, t_floatarg f)
{
  x->vic = f;
  n_spectr_outlet_vic(x);
}

//----------------------------------------------------------------------------//
static void n_spectr_vic_coords(t_n_spectr *x, t_floatarg cx, t_floatarg cy)
{
  AF_CLIP_MINMAX(1, x->width, cx);
  AF_CLIP_MINMAX(1, x->height, cy);
  x->vic_x = cx;
  x->vic_y = cy;
  n_spectr_outlet_vic(x);
  t_atom a[2];
  t_float fr,amp;

  // lin
  if (x->grid_ver == 0)
    {
      fr = ((t_float)x->vic_x / (t_float)x->width) * (x->sr / 2.);
    }
  // log
  else
    {
      t_float min = (x->sr / 2.) / (x->bs / 2.);
      t_float max =  x->sr / 2.;
      AF_F2M(min, min);
      AF_F2M(max, max);
      t_float diff = max - min;
      fr = ((t_float)x->vic_x / (t_float)x->width) * diff;
      fr = fr + min;
      AF_M2F(fr, fr);
    }

  // lin
  if (x->grid_hor == 0)
    {
      t_float diff = x->hor_lin_max - x->hor_lin_min;
      amp = (1. - ((t_float)x->vic_y / (t_float)x->height)) * diff;
      amp = amp + x->hor_lin_min;
    }
  // log
  else
    {
      t_float diff = x->hor_log_max - x->hor_log_min;
      amp = (1. - ((t_float)x->vic_y / (t_float)x->height)) * diff;
      amp = amp + x->hor_log_min + 100.;
      AF_DB2RMS(amp, amp);
    }


  SETFLOAT(a,fr);
  SETFLOAT(a+1,amp);
  outlet_anything(x->out_vic, &s_list, 2, a);
}

//----------------------------------------------------------------------------//
static void n_spectr_bs(t_n_spectr *x, t_floatarg f)
{
  if (f == 16     ||
      f == 32     ||
      f == 64     ||
      f == 128    ||
      f == 256    ||
      f == 512    ||
      f == 1024   ||
      f == 2048   ||
      f == 4096   ||
      f == 8192   ||
      f == 16384)
    {
      if (f != x->bs)
	{
	  x->bs = f;
	  n_spectr_calc_env(x);
	  n_spectr_calc_win(x);
	  n_spectr_calc_grid_ver(x);
	  n_spectr_calc_points(x);
	  n_spectr_calc_maxel(x);
	  n_spectr_reset(x);
	  n_spectr_outlet_all(x);
	}
    }
  else 
    {
      post("n_spectr~: bad size");
    }
}

//----------------------------------------------------------------------------//
static void n_spectr_window(t_n_spectr *x, t_floatarg f1, t_floatarg f2)
{
  x->window = f1;
  x->wincoef = f2;
  n_spectr_calc_win(x);
}

//----------------------------------------------------------------------------//
static void n_spectr_hor_minmax_lin(t_n_spectr *x, t_floatarg f1, t_floatarg f2)
{
  AF_CLIP_MINMAX(-100., 100., f1);
  AF_CLIP_MINMAX(-100., 100., f2);
  x->hor_lin_min = f1;
  x->hor_lin_max = f2;
  n_spectr_calc_hor_minmax(x);
  n_spectr_calc_grid_hor(x);
  n_spectr_outlet_grid_hor(x);
}

//----------------------------------------------------------------------------//
static void n_spectr_hor_minmax_log(t_n_spectr *x, t_floatarg f1, t_floatarg f2)
{
  AF_CLIP_MINMAX(-100., 100., f1);
  AF_CLIP_MINMAX(-100., 100., f2);
  x->hor_log_min = f1;
  x->hor_log_max = f2;
  n_spectr_calc_hor_minmax(x);
  n_spectr_calc_grid_hor(x);
  n_spectr_outlet_grid_hor(x);
}

//----------------------------------------------------------------------------//
static void n_spectr_env(t_n_spectr *x, t_floatarg f)
{
  AF_CLIP_MINMAX(-10., 100., f);
  x->env = f;
  n_spectr_calc_env(x);
}

//----------------------------------------------------------------------------//
static void n_spectr_fps(t_n_spectr *x, t_floatarg f)
{
  AF_CLIP_MINMAX(0.01, MAX_FPS, f);
  x->fps = f;
  n_spectr_calc_frame(x);
}

//----------------------------------------------------------------------------//
static void n_spectr_size(t_n_spectr *x, t_floatarg w, t_floatarg h)
{
  AF_CLIP_MINMAX(MIN_WIDTH, MAX_WIDTH, w);
  AF_CLIP_MINMAX(MIN_HEIGHT, MAX_HEIGHT, h);
  if (x->width != w || x->height != h)
    {
      x->width = w;
      x->height = h;
      x->height_1 = x->height + 1;
      n_spectr_calc_points(x);
      n_spectr_calc_maxel(x);
      n_spectr_calc_grid_hor(x);
      n_spectr_calc_grid_ver(x);
      n_spectr_outlet_all(x);
    }
}

//----------------------------------------------------------------------------//
static void n_spectr_color(t_n_spectr *x, t_symbol *s, int ac, t_atom *av)
{
  int fcol  = atom_getfloatarg(0, ac, av);
  int bcol  = atom_getfloatarg(1, ac, av);
  int gcol = atom_getfloatarg(2, ac, av);
  int vcol = atom_getfloatarg(3, ac, av);
  int tcol  = atom_getfloatarg(4, ac, av);
  if ((x->fcol != fcol) || (x->bcol != bcol)) 
    {
      x->fcol = fcol;
      x->bcol = bcol;
      n_spectr_outlet_background(x);
    }
  if (x->gcol != gcol)
    {
      x->gcol = gcol;
      n_spectr_outlet_grid_hor(x);
      n_spectr_outlet_grid_ver(x);
    }
  if (x->vcol != vcol)
    {
      x->vcol = vcol;
      n_spectr_outlet_vic(x);
    }
  if (x->tcol != tcol)
    {
      x->tcol = tcol;
      n_spectr_outlet_grid_hor(x);
      n_spectr_outlet_grid_ver(x);
    }
  if (s) {}
}

//----------------------------------------------------------------------------//
static void n_spectr_text(t_n_spectr *x, t_symbol *s, int ac, t_atom *av)
{
  int text_view    = atom_getfloatarg(0, ac, av);
  int fs           = atom_getfloatarg(1, ac, av);
  int grid_hor_dx  = atom_getfloatarg(2, ac, av);
  int grid_hor_dy  = atom_getfloatarg(3, ac, av);
  int grid_ver_dx  = atom_getfloatarg(4, ac, av);
  int grid_ver_dy  = atom_getfloatarg(5, ac, av);
  if ((x->text_view   != text_view)   || 
      (x->fs          != fs)          ||
      (x->grid_hor_dx != grid_hor_dx) ||
      (x->grid_hor_dy != grid_hor_dy) ||
      (x->grid_ver_dx != grid_ver_dx) ||
      (x->grid_ver_dy != grid_ver_dy))
    {
      x->text_view   = text_view;
      x->fs          = fs;
      x->grid_hor_dx = grid_hor_dx;
      x->grid_hor_dy = grid_hor_dy;
      x->grid_ver_dx = grid_ver_dx;
      x->grid_ver_dy = grid_ver_dy;
      n_spectr_outlet_grid_hor(x);
      n_spectr_outlet_grid_ver(x);
    }
  if (s) {}
}

//----------------------------------------------------------------------------//
static void n_spectr_in_on(t_n_spectr *x, t_floatarg in, t_floatarg f)
{
  AF_CLIP_MINMAX(0, x->in_all - 1, in);
  if (x->c[(int)in].on != f)
    {
      x->c[(int)in].on = f;
      n_spectr_outlet_waves(x);
    }
}

//----------------------------------------------------------------------------//
static void n_spectr_in_color(t_n_spectr *x, t_floatarg in, t_floatarg f)
{
  AF_CLIP_MINMAX(0, x->in_all - 1, in);
  if (x->c[(int)in].col != f)
    {
      x->c[(int)in].col = f;
      n_spectr_outlet_waves(x);
    }
}

//----------------------------------------------------------------------------//
// dsp
//----------------------------------------------------------------------------//
t_int *n_spectr_perform(t_int *w)
{
  int i,j,k;

  t_n_spectr *x = (t_n_spectr *)(w[1]);
  for (i = 0; i < x->in_all; i++)
    {
      x->v_in[i] = (t_sample *)(w[i + 2]);
    }
  t_int n = x->n;
  t_float bsc = 1. / ((t_float)x->bs / 2.);
  t_float a, b;
  t_float min, max;
  t_atom t[6];
  SETFLOAT(t,(t_float)9);

  if (x->on)
    {
      // dsp
      while (n--)
	{
	  // dsp count
	  for(i=0; i<x->in_all; i++)
	    {
	      x->c[i].buf_r[x->dsp_count] = *(x->v_in[i]++);
	    }

	  // procces
	  x->dsp_count++;
	  if (x->dsp_count >= x->bs)
	    {
	      x->dsp_count = 0;

	      for(i=0; i<x->in_all; i++)
		{
		  if (x->c[i].on)
		    {
		      // windowing
		      for (j=0; j<x->bs; j++)
			{
			  x->c[i].buf_i[j] = x->c[i].buf_r[j] = x->c[i].buf_r[j] * x->win[j]; 
			}

		      // fft
		      mayer_fft(x->bs, x->c[i].buf_r, x->c[i].buf_i);
		      
		      for (j=0; j<x->bs / 2; j++)
			{
			  // vec2a
			  a = x->c[i].buf_r[j] * bsc;
			  b = x->c[i].buf_i[j] * bsc;
			  a = a * a;
			  b = b * b;
			  a = a + b + 1e-12;
			  a = sqrt(a);

			  // rms2db
			  if (x->grid_hor)
			    {
			      AF_RMS2DB(a, a);
			    }

			  // mul add clip
			  a = (a + x->rng_add) * x->rng_mul;
			  AF_CLIP_MINMAX(0., 1., a);

			  // env
			  b = a - x->c[i].buf_e[j];
			  if (b < 0)
			    {
			      b = b * x->env_i;
			      a = b + x->c[i].buf_e[j];
			    }
			  x->c[i].buf_e[j] = a;
			}
		    }
		}
	    }


	  // frame count
	  x->frame_count++;
	  if (x->frame_count >= x->frame_max)
	    {	    
	      x->frame_count = 0;
	      // display waves function there
	      for (i=0; i<x->in_all; i++)
		{
		  if (x->c[i].on)
		    {
		      for(j=0; j<x->p_all; j++)
			{
			  // find min max
			  k = x->p_start[j];
			  max = x->c[i].buf_e[k];
			  min = x->c[i].buf_e[k];
			  k++;
			  for ( ; k < x->p_end[j]; k++)
			    {
			      if (x->c[i].buf_e[k] > max)
				{
				  max = x->c[i].buf_e[k];
				}
			      if (x->c[i].buf_e[k] < min)
				{
				  min = x->c[i].buf_e[k];
				}
			    }
			  // coords
			  x->p_max[j] = (1. - max) * (x->height + 1.); // add 1
			  x->p_min[j] = (1. - min) * (x->height + 1.); // add 1
			}


		      // type
		      for(j=0; j<x->p_all-1; j++)
			{
			  SETFLOAT(t+1,(t_float) x->c[i].ofs + j); // id
			  // type 0 /
			  if (x->p_type[j] == 0)
			    {
			      SETFLOAT(t+2,(t_float) x->p_x0[j]); // x0
			      SETFLOAT(t+3,(t_float) x->p_max[j]); // y0
			      SETFLOAT(t+4,(t_float) x->p_x0[j+1]); // x1
			      SETFLOAT(t+5,(t_float) x->p_max[j+1]); // y1
			    }
			  // type 1 |
			  else if (x->p_type[j] == 1)
			    {
			      SETFLOAT(t+2,(t_float) x->p_x0[j]); // x0
			      if (x->p_max[j+1] > x->p_min[j])
				{
				  SETFLOAT(t+3,(t_float) x->p_max[j]); // y0
				  SETFLOAT(t+4,(t_float) x->p_x0[j]); // x1
				  SETFLOAT(t+5,(t_float) x->p_max[j+1]); // y1
				}
			      else if (x->p_min[j+1] < x->p_max[j])
				{
				  SETFLOAT(t+3,(t_float) x->p_min[j]); // y0
				  SETFLOAT(t+4,(t_float) x->p_x0[j]); // x1
				  SETFLOAT(t+5,(t_float) x->p_min[j+1]); // y1
				}
			      else
				{
				  SETFLOAT(t+3,(t_float) x->p_max[j]); // y0
				  SETFLOAT(t+4,(t_float) x->p_x0[j]); // x1
				  SETFLOAT(t+5,(t_float) x->p_min[j]); // y1
				}
			    }
			  // type 2 |||
			  else
			    {
			      SETFLOAT(t+2,(t_float) x->p_x0[j]); // x0
			      if (x->p_max[j+1] > x->p_min[j])
				{
				  SETFLOAT(t+3,(t_float) x->p_max[j]); // y0
				  SETFLOAT(t+4,(t_float) x->p_x0[j]); // x1
				  SETFLOAT(t+5,(t_float) x->p_max[j+1]); // y1
				}
			      else if (x->p_min[j+1] < x->p_max[j])
				{
				  SETFLOAT(t+3,(t_float) x->p_min[j]); // y0
				  SETFLOAT(t+4,(t_float) x->p_x0[j]); // x1
				  SETFLOAT(t+5,(t_float) x->p_min[j+1]); // y1
				}
			      else
				{
				  SETFLOAT(t+3,(t_float) x->p_max[j]); // y0
				  SETFLOAT(t+4,(t_float) x->p_x0[j]); // x1
				  SETFLOAT(t+5,(t_float) x->p_min[j]); // y1
				}
			    }
			  outlet_anything(x->out, &s_list, 6, t);
			}
		      // last
		      j = x->p_all - 1;
		      SETFLOAT(t+1,(t_float) x->c[i].ofs + j); // id
		      SETFLOAT(t+2,(t_float) x->p_x0[j]); // x0
		      SETFLOAT(t+3,(t_float) x->p_max[j]); // y0
		      SETFLOAT(t+4,(t_float) x->p_x0[j]); // x1
		      SETFLOAT(t+5,(t_float) x->p_max[j]); // y1
		      outlet_anything(x->out, &s_list, 6, t);
		    }
		}
	    }
	}
    }
  
  return (w + x->in_all + 2);
}

//----------------------------------------------------------------------------//
static void n_spectr_dsp(t_n_spectr *x, t_signal **sp)
{
  int i;

  if (x->sr != sp[0]->s_sr)
    {
      x->sr = sp[0]->s_sr;
      n_spectr_reset_sr(x);
    }
  x->n = sp[0]->s_n;
  x->v_d[0] = (t_int *)x;
  for (i = 0; i < x->in_all; i++)
    {
      x->v_d[i + 1] = (t_int *)sp[i]->s_vec;
    }
  dsp_addv(n_spectr_perform, x->in_all + 1, (t_int *)x->v_d);
}

//----------------------------------------------------------------------------//
// setup
//----------------------------------------------------------------------------//
static void *n_spectr_new(t_floatarg f)
{ 
  int i;
  t_n_spectr *x = (t_n_spectr *)pd_new(n_spectr_class);
  AF_CLIP_MINMAX(1, MAX_INLETS, f);
  x->in_all = f;
  x->v_in = getbytes(sizeof(t_sample *) * x->in_all);
  x->v_d  = getbytes(sizeof(t_int *) * (x->in_all + 2));
  for (i = 0; i < x->in_all; i++)
    {
      x->c[i].buf_r = getbytes(sizeof(t_float) * MAX_BS);
      x->c[i].buf_i = getbytes(sizeof(t_float) * MAX_BS);
      x->c[i].buf_e = getbytes(sizeof(t_float) * MAX_BS);
    }
  for (i = 1; i < x->in_all; i++)
    {
      inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    }
  x->out = outlet_new(&x->x_obj, 0);
  x->out_vic = outlet_new(&x->x_obj, 0);
  x->win = getbytes(sizeof(t_float) * MAX_BS);

  // init
  x->on = 0;
  x->grid = 0;
  x->grid_hor = 0;                /* lin, log */
  x->grid_ver = 0;                /* lin, log */
  x->vic = 0;                     /* on, off */
  x->vic_x = 0;                   /* vic coords */
  x->vic_y = 0;
  x->bs = 512;                      /* size fft */
  x->window = 0;                  /* windowing */
  x->wincoef = 1;
  x->hor_lin_min = 0;         /* minmax */
  x->hor_lin_max = 1;
  x->hor_log_min = -100;
  x->hor_log_max = 0;
  x->env = 0;                 /* env */
  x->fps = 8;                 /* fps */
  x->width = 0;                   /* size */
  x->height = 0;
  x->fcol = -65793;
  x->bcol = -69377;
  x->gcol = -468737;
  x->vcol = -70913;
  x->tcol = -70913;
  x->height_1 = x->height + 1;
  x->text_view = 0;               /* text settings */
  x->fs = 4;
  x->grid_hor_dx = 0;
  x->grid_hor_dy = 0;
  x->grid_ver_dx = 0;
  x->grid_ver_dy = 0;
  x->sr = 44100;
  n_spectr_calc_frame(x);
  n_spectr_calc_win(x);
  n_spectr_calc_env(x);
  n_spectr_calc_hor_minmax(x);
  n_spectr_calc_grid_hor(x);
  n_spectr_calc_grid_ver(x);
  n_spectr_calc_points(x);
  n_spectr_calc_maxel(x);
  n_spectr_reset_sr(x);
  n_spectr_reset(x);

  for (i = 0; i < x->in_all; i++)
    {
      x->c[i].on = 0;
    }

  n_spectr_reset(x);

  return (x);
}

//----------------------------------------------------------------------------//
static void n_spectr_free(t_n_spectr *x)
{
  int i;
  freebytes(x->v_in, sizeof(t_sample *) * x->in_all);
  freebytes(x->v_d, sizeof(t_int *) * (x->in_all + 2));
  for (i = 0; i < x->in_all; i++)
    {
      freebytes(x->c[i].buf_r, sizeof(t_float) * MAX_BS);  
      freebytes(x->c[i].buf_i, sizeof(t_float) * MAX_BS);  
      freebytes(x->c[i].buf_e, sizeof(t_float) * MAX_BS);  
    }
  freebytes(x->win, sizeof(t_float) * MAX_BS);  
  mayer_term();
}

//----------------------------------------------------------------------------//
void n_spectr_tilde_setup(void)
{
  n_spectr_class = class_new(gensym("n_spectr~"), (t_newmethod)n_spectr_new, (t_method)n_spectr_free, sizeof(t_n_spectr), 0, A_DEFFLOAT, 0);
  class_addmethod(n_spectr_class, nullfn, gensym("signal"), 0);
  class_addmethod(n_spectr_class, (t_method)n_spectr_dsp, gensym("dsp"), 0);
  class_addmethod(n_spectr_class, (t_method)n_spectr_on, gensym("on"), A_DEFFLOAT, 0);
  class_addmethod(n_spectr_class, (t_method)n_spectr_grid, gensym("grid"), A_DEFFLOAT, 0);
  class_addmethod(n_spectr_class, (t_method)n_spectr_grid_hor, gensym("grid_hor"), A_DEFFLOAT, 0);
  class_addmethod(n_spectr_class, (t_method)n_spectr_grid_ver, gensym("grid_ver"), A_DEFFLOAT, 0);
  class_addmethod(n_spectr_class, (t_method)n_spectr_vic, gensym("vic"), A_DEFFLOAT, 0);
  class_addmethod(n_spectr_class, (t_method)n_spectr_vic_coords, gensym("vic_coords"), A_DEFFLOAT, A_DEFFLOAT, 0);
  class_addmethod(n_spectr_class, (t_method)n_spectr_bs, gensym("bs"), A_DEFFLOAT, 0);
  class_addmethod(n_spectr_class, (t_method)n_spectr_window, gensym("window"), A_DEFFLOAT, A_DEFFLOAT, 0);
  class_addmethod(n_spectr_class, (t_method)n_spectr_hor_minmax_lin, gensym("minmax_lin"), A_DEFFLOAT, A_DEFFLOAT, 0);
  class_addmethod(n_spectr_class, (t_method)n_spectr_hor_minmax_log, gensym("minmax_log"), A_DEFFLOAT, A_DEFFLOAT, 0);
  class_addmethod(n_spectr_class, (t_method)n_spectr_env, gensym("env"), A_DEFFLOAT, 0);
  class_addmethod(n_spectr_class, (t_method)n_spectr_fps, gensym("fps"), A_DEFFLOAT, 0);
  class_addmethod(n_spectr_class, (t_method)n_spectr_size, gensym("size"), A_DEFFLOAT, A_DEFFLOAT, 0);
  class_addmethod(n_spectr_class, (t_method)n_spectr_color, gensym("color"), A_GIMME, 0);
  class_addmethod(n_spectr_class, (t_method)n_spectr_text, gensym("text"), A_GIMME, 0);
  class_addmethod(n_spectr_class, (t_method)n_spectr_in_on, gensym("in_on"), A_DEFFLOAT, A_DEFFLOAT, 0);
  class_addmethod(n_spectr_class, (t_method)n_spectr_in_color, gensym("in_color"), A_DEFFLOAT, A_DEFFLOAT, 0);
  mayer_init();
}
