#include "m_pd.h"
#include "include/parsearg.h"
#include "include/pdfunc.h"

#define _clip_minmax(min, max, v) v=(v>max)?max:(v<min)?min:v;
#define _clip_min(min, v) v=(v<min)?min:v;
#define _clip_max(max, v) v=(v>max)?max:v;

#define MEM_ERROR -1

#define PATT_MAX 64
#define PATT_MAX_1 63

#define COLUMN_MIN 1
#define COLUMN_MAX 64
#define COLUMN_MAX_1 63

#define ROW_MIN 1
#define ROW_MAX 32

#define TRACK_MIN 1
#define TRACK_MAX 32
#define TRACK_MAX_1 31

#define MOD_MIN 0
#define MOD_MAX 8
#define MOD_MAX_1 7

#define PAR_MAX 16

#define NSIZE_MIN 4
#define NSIZE_MAX 256

#define MOD_WIDTH_MIN 1
#define MOD_WIDTH_MAX 256

#define MOD_HEIGHT_MIN 1
#define MOD_HEIGHT_MAX 256

#define FLAM_MAX 8

#define M(OFS) x->w_mem[OFS].w_float

static t_class *nsqd_class;

typedef struct _nsqd_back
{
  int id;
  int x0;
  int y0;
  int x1;
  int y1;
  int color; // back major minor
} t_nsqd_back;

typedef struct _nsqd_note
{
  int id;
  int x0;
  int y0;
  int x1;
  int y1;
  int state; // off on acc
} t_nsqd_note;

typedef struct _nsqd_flam
{
  int id;
  int x0;
  int y0;
  int state; // off ... maxflam
} t_nsqd_flam;

typedef struct _nsqd_mod
{
  int id[MOD_MAX];
  int x0[MOD_MAX];
  int y0[MOD_MAX];
  int x1[MOD_MAX];
  int y1[MOD_MAX];
  int state[MOD_MAX]; // off on
} t_nsqd_mod;

typedef struct _nsqd_column
{
  t_nsqd_back back;
  t_nsqd_note note[ROW_MAX];
  t_nsqd_note flam[ROW_MAX];
  t_nsqd_back mod;
} t_nsqd_column;

typedef struct _nsqd_playpos
{
  int id;
  int x0;
  int y0;
  int x1;
  int y1;
} t_nsqd_playpos;

typedef struct _nsqd_split_hor
{
  int id;
  int x0;
  int y0;
  int x1;
  int y1;
} t_nsqd_split_hor;

typedef struct _nsqd_split_ver
{
  int id;
  int x0;
  int y0;
  int x1;
  int y1;
} t_nsqd_split_ver;

typedef struct _nsqd_ofs_track
{
  int note;
  int flam;
  int mod[MOD_MAX];
} t_nsqd_ofs_track;

typedef struct _nsqd_patt
{
  t_nsqd_ofs_track ofs_track[TRACK_MAX];
  int ofs_major;
  int ofs_minor;
  int ofs_2;
  int ofs_3;
  int ofs_4;
  int ofs_5;
  int ofs_6;
  int ofs_7;
  int ofs_8;
  int ofs_9;
  int ofs_10;
  int ofs_11;
  int ofs_12;
  int ofs_13;
  int ofs_14;
  int ofs_15;
} t_nsqd_patt;

typedef struct _nsqd
{
  t_object x_obj;
  t_outlet *out_seq;
  t_outlet *out_par;
  t_outlet *out_act;
  t_outlet *out_disp;
  int column;
  int row;
  int tracks;
  int mods;
  int patt;
  int track_ofs;
  int sel_track;
  int sel_mod;
  int size;
  int mod_width;
  int mod_height;
  int color_back;
  int color_frame;
  int color_major;
  int color_minor;
  int color_note;
  int color_acc;
  int color_flam;
  int color_mod[MOD_MAX];
  int color_mod_sel;
  int color_playpos;

  int disp_maxel;
  int disp_w;
  int disp_h;

  t_nsqd_patt p[PATT_MAX];
  t_nsqd_column c[COLUMN_MAX];
  t_nsqd_playpos playpos;
  t_nsqd_split_hor split_hor[ROW_MAX];
  t_nsqd_split_hor split_ver[COLUMN_MAX_1];
  
  // 
  t_symbol *flam_sym[71];
  
  int mem_size;
  t_symbol *s_mem;
  t_word *w_mem;
  t_garray * g_mem;
  int l_mem;
} t_nsqd;

char *flam_symbols =
  "0123456789-+=*$#@!%abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
////////////////////////////////////////////////////////////////////////////////
// calc
////////////////////////////////////////////////////////////////////////////////
static void nsqd_init_patt(t_nsqd *x)
{
  //               note        flam        mods                                par
  x->mem_size = (((x->tracks + x->tracks + (x->tracks * x->mods)) * x->column)+16)
    * PATT_MAX;
  // calc ofs
  // patt:
  //     track:
  //        note
  //        flam
  //        mod0
  //        ...
  //        modN
  // par
  int ofs = 0;
  for (int i=0; i<PATT_MAX; i++)
    {
      for (int j=0; j<x->tracks; j++)
	{
	  x->p[i].ofs_track[j].note = ofs; ofs+=x->column;
	  x->p[i].ofs_track[j].flam = ofs; ofs+=x->column;
	  for (int k=0; k<x->mods; k++)
	    {
	      x->p[i].ofs_track[j].mod[k] = ofs; ofs+=x->column;
	    }
	}
      x->p[i].ofs_major = ofs;
      x->p[i].ofs_minor = ofs+1;
      ofs+=PAR_MAX;
    }
  /* post("%d = %d", x->mem_size, ofs); */
}

static void nsqd_check_mem(t_nsqd *x)
{
  for (int i=0; i<PATT_MAX; i++)
    {
      int j;
      // check major
      j = M(x->p[i].ofs_major);
      _clip_minmax(1, COLUMN_MAX, j);
      M(x->p[i].ofs_major) = j;
 
      // check minor
      j = M(x->p[i].ofs_minor);
      _clip_minmax(1, COLUMN_MAX, j);
      M(x->p[i].ofs_minor) = j;

      for (int k=0; k<x->tracks; k++)
	{
	  // check note
	  j = M(x->p[i].ofs_track[k].note);
	  _clip_minmax(0, 2, j);
	  M(x->p[i].ofs_track[k].note) = j;
	  
	  // check flam
	  j = M(x->p[i].ofs_track[k].flam);
	  _clip_minmax(0, FLAM_MAX, j);
	  M(x->p[i].ofs_track[k].flam) = j;
	  
	  // check mod
	  for (int m=0; m<x->mods; m++)
	    {
	      float f = M(x->p[i].ofs_track[k].mod[m]);
	      _clip_minmax(0, 1, f);
	      M(x->p[i].ofs_track[k].mod[m]) = f;
	    }
	}
    }
}

static void nsqd_init_mem(t_nsqd *x)
{
  // open mem
  x->l_mem = pd_open_array(x->s_mem, &x->w_mem, &x->g_mem);
  if (x->l_mem < 1)
    {
      post("n_seq_drum: error: open array: %s", x->s_mem->s_name);
      x->l_mem = MEM_ERROR;
      return;
    }

  // resize
  if (x->l_mem != x->mem_size)
    {
      garray_resize(x->g_mem, x->mem_size);
      x->l_mem = pd_open_array(x->s_mem, &x->w_mem, &x->g_mem);
    }
  if (x->l_mem != x->mem_size)
    {
      post("n_seq_drum: error: resize array: %s", x->s_mem->s_name);
      x->l_mem = MEM_ERROR;
      return;
    }
}

static void nsqd_calc_major_minor(t_nsqd *x)
{
  for (int i=0; i<x->column; i++)
    {
      if      (i % (int)M(x->p[x->patt].ofs_major)==0)
	{
	  x->c[i].back.color = x->color_major;
	}
      else if (i % (int)M(x->p[x->patt].ofs_minor)==0)
	{
	  x->c[i].back.color = x->color_minor;
	}
      else
	{
	  x->c[i].back.color = x->color_back;
	}
    }
}

static void nsqd_calc_disp(t_nsqd *x)
{
  int i,j;
  int id = 0;

  // disp size
  x->disp_w = (x->size * x->column);
  x->disp_h = (x->size * x->row) + x->mod_height;

  // back
  for (i=0; i<x->column; i++)
    {
      int cx0 = (i * x->size) + 1;
      int cx1 = ((i+1) * x->size) + 1;
      x->c[i].back.id = id;
      x->c[i].back.x0 = cx0;
      x->c[i].back.y0 = 1;
      x->c[i].back.x1 = cx1;
      x->c[i].back.y1 = x->disp_h;
      id++;
    }

  // split hor
  for (i=0; i<x->row; i++)
    {
      int cy = ((i+1) * x->size) + 1;
      x->split_hor[i].id = id;
      x->split_hor[i].x0 = 1;
      x->split_hor[i].y0 = cy;
      x->split_hor[i].x1 = x->disp_w+1;
      x->split_hor[i].y1 = cy;
      id++;
    }

  // split vert
  for (i=0; i<x->column-1; i++)
    {
      int cx = ((i+1) * x->size) + 1;
      x->split_ver[i].id = id;
      x->split_ver[i].x0 = cx;
      x->split_ver[i].y0 = 1;
      x->split_ver[i].x1 = cx;
      x->split_ver[i].y1 = x->disp_h+1;
      id++;
    }

  // note
  for (i=0; i<x->column; i++)
    {
      int cx0 = (i * x->size) + 1 + 2;
      int cx1 = ((i+1) * x->size) + 1 - 3;
      for (j=0; j<x->row; j++)
	{
	  int cy0 = (j * x->size) + 1 + 2;
	  int cy1 = ((j+1) * x->size) + 1 - 3;
	  x->c[i].note[j].id = id;
	  x->c[i].note[j].x0 = cx0;
	  x->c[i].note[j].y0 = cy0;
	  x->c[i].note[j].x1 = cx1;
	  x->c[i].note[j].y1 = cy1;
	  id++;
	}
    }
  // flam
  for (i=0; i<x->column; i++)
    {
      int cx0 = (i * x->size) + 1;
      for (j=0; j<x->row; j++)
	{
	  int cy0 = (j * x->size) + 1;
	  x->c[i].flam[j].id = id;
	  x->c[i].flam[j].x0 = cx0;
	  x->c[i].flam[j].y1 = cy0;
	  id++;
	}
    }
  // mod
  // playpos
  {
      x->playpos.id = id;
      x->playpos.x0 = 0;
      x->playpos.y0 = 0;
      x->playpos.x1 = 0;
      x->playpos.y1 = x->disp_h;
      id++;
  }
  // maxel
  x->disp_maxel = id;
}

static void nsqd_draw_disp(t_nsqd *x)
{
  int i,j;
  t_atom a[10];
  // maxel
  SETFLOAT(a, (t_float)x->disp_maxel);
  outlet_anything(x->out_disp, gensym("maxel"), 1, a);
  // disp size
  SETFLOAT(a, (t_float)x->disp_w+1);
  SETFLOAT(a+1, (t_float)x->disp_h+1);
  outlet_anything(x->out_disp, gensym("size"), 2, a);
  // back (rect filled)
  SETFLOAT(a, (t_float)3);
  for (i=0; i<x->column; i++)
    {
      SETFLOAT(a+1, (t_float)x->c[i].back.id);
      SETFLOAT(a+2, (t_float)x->c[i].back.color);
      SETFLOAT(a+3, (t_float)x->c[i].back.color);
      SETFLOAT(a+4, (t_float)1);
      SETFLOAT(a+5, (t_float)x->c[i].back.x0);
      SETFLOAT(a+6, (t_float)x->c[i].back.y0);
      SETFLOAT(a+7, (t_float)x->c[i].back.x1);
      SETFLOAT(a+8, (t_float)x->c[i].back.y1);
      outlet_list(x->out_disp, &s_list, 9, a);
    }
  // split ver(line)
  SETFLOAT(a, (t_float)1);
  for (i=0; i<x->column-1; i++)
    {
      SETFLOAT(a+1, (t_float)x->split_ver[i].id);
      SETFLOAT(a+2, (t_float)x->color_frame);
      SETFLOAT(a+3, (t_float)1);
      SETFLOAT(a+4, (t_float)x->split_ver[i].x0);
      SETFLOAT(a+5, (t_float)x->split_ver[i].y0);
      SETFLOAT(a+6, (t_float)x->split_ver[i].x1);
      SETFLOAT(a+7, (t_float)x->split_ver[i].y1);
      outlet_list(x->out_disp, &s_list, 8, a);
    }
  // split hor
  SETFLOAT(a, (t_float)1);
  for (i=0; i<x->row; i++)
    {
      SETFLOAT(a+1, (t_float)x->split_hor[i].id);
      SETFLOAT(a+2, (t_float)x->color_frame);
      SETFLOAT(a+3, (t_float)1);
      SETFLOAT(a+4, (t_float)x->split_hor[i].x0);
      SETFLOAT(a+5, (t_float)x->split_hor[i].y0);
      SETFLOAT(a+6, (t_float)x->split_hor[i].x1);
      SETFLOAT(a+7, (t_float)x->split_hor[i].y1);
      outlet_list(x->out_disp, &s_list, 8, a);
    }
  // note (rect filled)
  SETFLOAT(a, (t_float)3);
  for (i=0; i<x->column; i++)
    {
      for (j=0; j<x->row; j++)
	{
	  SETFLOAT(a+1, (t_float)x->c[i].note[j].id);
	  SETFLOAT(a+2, (t_float)x->color_note);
	  SETFLOAT(a+3, (t_float)x->color_note);
	  SETFLOAT(a+4, (t_float)1);
	  SETFLOAT(a+5, (t_float)x->c[i].note[j].x0);
	  SETFLOAT(a+6, (t_float)x->c[i].note[j].y0);
	  SETFLOAT(a+7, (t_float)x->c[i].note[j].x1);
	  SETFLOAT(a+8, (t_float)x->c[i].note[j].y1);
	  outlet_list(x->out_disp, &s_list, 9, a);
	}
    }
  // flam
  // mod
  // playpos
}

////////////////////////////////////////////////////////////////////////////////
// methods
////////////////////////////////////////////////////////////////////////////////
static void nsqd_init(t_nsqd *x)
{
  nsqd_init_patt(x);
  nsqd_init_mem(x);
  nsqd_check_mem(x);
  nsqd_calc_major_minor(x);
  nsqd_calc_disp(x);
  nsqd_draw_disp(x);
}

static void nsqd_redraw(t_nsqd *x)
{
  nsqd_calc_major_minor(x);
  nsqd_calc_disp(x);
  nsqd_draw_disp(x);
  post("redraw");
}

static void nsqd_disp(t_nsqd *x,
		      t_floatarg ml,
		      t_floatarg dbl,
		      t_floatarg cx,
		      t_floatarg cy)
{
  post("disp");
}

static void nsqd_pos(t_nsqd *x, t_floatarg f)
{
  _clip_minmax(0, COLUMN_MAX_1, f);
  post("pos: %g", f);
}

////////////////////////////////////////////////////////////////////////////////
// const
////////////////////////////////////////////////////////////////////////////////
static void nsqd_column(t_nsqd *x, t_floatarg f)
{
  _clip_minmax(COLUMN_MIN, COLUMN_MAX, f);
  x->column=f;
}

static void nsqd_row(t_nsqd *x, t_floatarg f)
{
  _clip_minmax(ROW_MIN, ROW_MAX, f);
  x->row=f;
}

static void nsqd_mem(t_nsqd *x, t_symbol *s)
{
  x->s_mem = s;
}

static void nsqd_tracks(t_nsqd *x, t_floatarg f)
{
  _clip_minmax(TRACK_MIN, TRACK_MAX, f);
  x->tracks=f;
}

static void nsqd_mods(t_nsqd *x, t_floatarg f)
{
  _clip_minmax(MOD_MIN, MOD_MAX, f);
  x->mods=f;
}

////////////////////////////////////////////////////////////////////////////////
// var
////////////////////////////////////////////////////////////////////////////////
static void nsqd_patt(t_nsqd *x, t_floatarg f)
{
  _clip_minmax(0, PATT_MAX_1, f);
  x->patt=f;
}

static void nsqd_track_ofs(t_nsqd *x, t_floatarg f)
{
  _clip_minmax(0, TRACK_MAX, f);
  x->track_ofs=f;
}

static void nsqd_sel_track(t_nsqd *x, t_floatarg f)
{
  _clip_minmax(0, TRACK_MAX_1, f);
  x->sel_track=f;
}

static void nsqd_sel_mod(t_nsqd *x, t_floatarg f)
{
  _clip_minmax(0, MOD_MAX_1, f);
  x->sel_mod=f;
}

static void nsqd_major(t_nsqd *x, t_floatarg f)
{
  _clip_minmax(1, COLUMN_MAX, f);
  M(x->p[x->patt].ofs_major)=f;
}

static void nsqd_minor(t_nsqd *x, t_floatarg f)
{
  _clip_minmax(1, COLUMN_MAX, f);
  M(x->p[x->patt].ofs_minor)=f;
}

////////////////////////////////////////////////////////////////////////////////
// view
////////////////////////////////////////////////////////////////////////////////
static void nsqd_size(t_nsqd *x, t_floatarg f)
{
  _clip_minmax(NSIZE_MIN, NSIZE_MAX, f);
  x->size=f;
}

static void nsqd_mod_width(t_nsqd *x, t_floatarg f)
{
  _clip_minmax(MOD_WIDTH_MIN, MOD_WIDTH_MAX, f);
  x->mod_width = f;
}

static void nsqd_mod_height(t_nsqd *x, t_floatarg f)
{
  _clip_minmax(MOD_HEIGHT_MIN, MOD_HEIGHT_MAX, f);
  x->mod_height = f;
}

static void nsqd_color_back(t_nsqd *x, t_floatarg f)
{
  x->color_back = f;
}

static void nsqd_color_frame(t_nsqd *x, t_floatarg f)
{
  x->color_frame = f;
}

static void nsqd_color_major(t_nsqd *x, t_floatarg f)
{
  x->color_major = f;
}

static void nsqd_color_minor(t_nsqd *x, t_floatarg f)
{
  x->color_minor = f;
}

static void nsqd_color_note(t_nsqd *x, t_floatarg f)
{
  x->color_note = f;
}

static void nsqd_color_acc(t_nsqd *x, t_floatarg f)
{
  x->color_acc = f;
}

static void nsqd_color_flam(t_nsqd *x, t_floatarg f)
{
  x->color_flam = f;
}

static void nsqd_color_mod(t_nsqd *x, t_floatarg n, t_floatarg f)
{
  _clip_minmax(0, MOD_MAX_1, n);
  x->color_mod[(int)n] = f;
}

static void nsqd_color_mod_sel(t_nsqd *x, t_floatarg f)
{
  x->color_mod_sel = f;
}

static void nsqd_color_playpos(t_nsqd *x, t_floatarg f)
{
  x->color_playpos = f;
}

////////////////////////////////////////////////////////////////////////////////
// setup
////////////////////////////////////////////////////////////////////////////////
static void *nsqd_new(t_symbol *s, int ac, t_atom *av)
{
  t_nsqd *x = (t_nsqd *)pd_new(nsqd_class);
  x->out_seq = outlet_new(&x->x_obj, 0);
  x->out_par = outlet_new(&x->x_obj, 0);
  x->out_act = outlet_new(&x->x_obj, 0);
  x->out_disp = outlet_new(&x->x_obj, 0);
  // flam symbols
  char buf[2];
  buf[1]='\0';
  int i=0;
  while(flam_symbols[i]!=0)
    {
      buf[0]=flam_symbols[i];
      x->flam_sym[i] = gensym(buf);
      i++;
    }
  return (void*)x;
}

void n_seq_drum_setup(void)
{
  nsqd_class = class_new(gensym("n_seq_drum"),
			 (t_newmethod)nsqd_new,
			 0, 
			 sizeof(t_nsqd),
			 0, A_GIMME, 0);
  // const
  class_addmethod(nsqd_class, (t_method)nsqd_column, gensym("column"),
		  A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_row, gensym("row"),
		  A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_mem, gensym("mem"),
		  A_SYMBOL, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_tracks, gensym("tracks"),
		  A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_mods, gensym("mods"),
		  A_FLOAT, 0);
  // var
  class_addmethod(nsqd_class, (t_method)nsqd_patt, gensym("patt"),
		  A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_track_ofs, gensym("track_ofs"),
		  A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_sel_track, gensym("sel_track"),
		  A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_sel_mod, gensym("sel_mod"),
		  A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_major, gensym("major"),
		  A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_minor, gensym("minor"),
		  A_FLOAT, 0);
  // methods
  class_addmethod(nsqd_class, (t_method)nsqd_init, gensym("init"), 0);
  class_addmethod(nsqd_class, (t_method)nsqd_redraw, gensym("redraw"), 0);
  class_addmethod(nsqd_class, (t_method)nsqd_disp, gensym("disp"),
		  A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_pos, gensym("pos"),
		  A_FLOAT, 0);
  // view
  class_addmethod(nsqd_class, (t_method)nsqd_size, gensym("size"),
		  A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_mod_width, gensym("mod_width"),
		  A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_mod_height, gensym("mod_height"),
		  A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_color_frame,
		  gensym("color_frame"), A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_color_back,
		  gensym("color_back"), A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_color_major,
		  gensym("color_major"), A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_color_minor,
		  gensym("color_minor"), A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_color_note,
		  gensym("color_note"), A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_color_acc,
		  gensym("color_acc"), A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_color_flam,
		  gensym("color_flam"), A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_color_mod,
		  gensym("color_mod"), A_FLOAT, A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_color_mod_sel,
		  gensym("color_mod_sel"), A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_color_playpos,
		  gensym("color_playpos"), A_FLOAT, 0);
}
