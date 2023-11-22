#include "m_pd.h"
#include "include/parsearg.h"

#define _clip_minmax(min, max, v) v=(v>max)?max:(v<min)?min:v;
#define _clip_min(min, v) v=(v<min)?min:v;
#define _clip_max(max, v) v=(v>max)?max:v;

#define PATT_MAX 32 // const for sss
#define PATT_MAX_1 31

#define COLUMN_MIN 1
#define COLUMN_MAX 256
#define COLUMN_MAX_1 255

#define ROW_MIN 1
#define ROW_MAX 32

#define TRACK_MIN 1
#define TRACK_MAX 32
#define TRACK_MAX_1 31

#define MOD_MIN 0
#define MOD_MAX 8
#define MOD_MAX_1 7

#define SIZE_MIN 4
#define SIZE_MAX 256

#define MOD_WIDTH_MIN 1
#define MOD_WIDTH_MAX 256

#define MOD_HEIGHT_MIN 1
#define MOD_HEIGHT_MAX 256

static t_class *n_seq_drum_class;

typedef struct _n_seq_drum_patt
{
  int major;
  int minor;
} t_n_seq_drum_patt;

typedef struct _n_seq_drum
{
  t_object x_obj;
  t_outlet *out_seq;
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
  t_n_seq_drum_patt p[PATT_MAX];
  int size;
  int mod_width;
  int mod_height;
  t_symbol *s_mem;
  int color_back;
  int color_frame;
  int color_major;
  int color_minor;
  int color_note;
  int color_acc;
  int color_flam;
  int color_mod[MOD_MAX];
} t_n_seq_drum;

////////////////////////////////////////////////////////////////////////////////
// const
////////////////////////////////////////////////////////////////////////////////
static void n_seq_drum_column(t_n_seq_drum *x, t_floatarg f)
{
  _clip_minmax(COLUMN_MIN, COLUMN_MAX, f);
  x->column=f;
}

static void n_seq_drum_row(t_n_seq_drum *x, t_floatarg f)
{
  _clip_minmax(ROW_MIN, ROW_MAX, f);
  x->row=f;
}

static void n_seq_drum_mem(t_n_seq_drum *x, t_symbol *s)
{
  x->s_mem = s;
}

static void n_seq_drum_tracks(t_n_seq_drum *x, t_floatarg f)
{
  _clip_minmax(TRACK_MIN, TRACK_MAX, f);
  x->tracks=f;
}

static void n_seq_drum_mods(t_n_seq_drum *x, t_floatarg f)
{
  _clip_minmax(MOD_MIN, MOD_MAX, f);
  x->mods=f;
}

////////////////////////////////////////////////////////////////////////////////
// var
////////////////////////////////////////////////////////////////////////////////
static void n_seq_drum_patt(t_n_seq_drum *x, t_floatarg f)
{
  _clip_minmax(0, PATT_MAX_1, f);
  x->patt=f;
}

static void n_seq_drum_track_ofs(t_n_seq_drum *x, t_floatarg f)
{
  _clip_minmax(0, TRACK_MAX, f);
  x->track_ofs=f;
}

static void n_seq_drum_sel_track(t_n_seq_drum *x, t_floatarg f)
{
  _clip_minmax(0, TRACK_MAX_1, f);
  x->sel_track=f;
}

static void n_seq_drum_sel_mod(t_n_seq_drum *x, t_floatarg f)
{
  _clip_minmax(0, MOD_MAX_1, f);
  x->sel_mod=f;
}

static void n_seq_drum_major(t_n_seq_drum *x, t_floatarg f)
{
  _clip_minmax(1, COLUMN_MAX, f);
  x->p[x->patt].major=f;
}

static void n_seq_drum_minor(t_n_seq_drum *x, t_floatarg f)
{
  _clip_minmax(1, COLUMN_MAX, f);
  x->p[x->patt].minor=f;
}

////////////////////////////////////////////////////////////////////////////////
// methods
////////////////////////////////////////////////////////////////////////////////
static void n_seq_drum_init(t_n_seq_drum *x)
{
  post("init");
}

static void n_seq_drum_redraw(t_n_seq_drum *x)
{
  post("redraw");
}

static void n_seq_drum_disp(t_n_seq_drum *x,
			    t_floatarg ml,
			    t_floatarg dbl,
			    t_floatarg cx,
			    t_floatarg cy)
{
  post("disp");
}

static void n_seq_drum_pos(t_n_seq_drum *x, t_floatarg f)
{
  _clip_minmax(0, COLUMN_MAX_1, f);
  post("pos: %g", f);
}

////////////////////////////////////////////////////////////////////////////////
// view
////////////////////////////////////////////////////////////////////////////////
static void n_seq_drum_size(t_n_seq_drum *x, t_floatarg f)
{
  _clip_minmax(SIZE_MIN, SIZE_MAX, f);
  x->size=f;
}

static void n_seq_drum_mod_width(t_n_seq_drum *x, t_floatarg f)
{
  _clip_minmax(MOD_WIDTH_MIN, MOD_WIDTH_MAX, f);
  x->mod_width = f;
}

static void n_seq_drum_mod_height(t_n_seq_drum *x, t_floatarg f)
{
  _clip_minmax(MOD_HEIGHT_MIN, MOD_HEIGHT_MAX, f);
  x->mod_height = f;
}

static void n_seq_drum_color_back(t_n_seq_drum *x, t_floatarg f)
{
  x->color_back = f;
}

static void n_seq_drum_color_frame(t_n_seq_drum *x, t_floatarg f)
{
  x->color_frame = f;
}

static void n_seq_drum_color_major(t_n_seq_drum *x, t_floatarg f)
{
  x->color_major = f;
}

static void n_seq_drum_color_minor(t_n_seq_drum *x, t_floatarg f)
{
  x->color_minor = f;
}

static void n_seq_drum_color_note(t_n_seq_drum *x, t_floatarg f)
{
  x->color_note = f;
}

static void n_seq_drum_color_acc(t_n_seq_drum *x, t_floatarg f)
{
  x->color_acc = f;
}

static void n_seq_drum_color_flam(t_n_seq_drum *x, t_floatarg f)
{
  x->color_flam = f;
}

static void n_seq_drum_color_mod(t_n_seq_drum *x, t_floatarg n, t_floatarg f)
{
  _clip_minmax(0, MOD_MAX_1, n);
  x->color_mod[(int)n] = f;
}

////////////////////////////////////////////////////////////////////////////////
// setup
////////////////////////////////////////////////////////////////////////////////
static void *n_seq_drum_new(t_symbol *s, int ac, t_atom *av)
{
  t_n_seq_drum *x = (t_n_seq_drum *)pd_new(n_seq_drum_class);
  x->out_seq = outlet_new(&x->x_obj, 0);
  x->out_act = outlet_new(&x->x_obj, 0);
  x->out_disp = outlet_new(&x->x_obj, 0);
  return (void*)x;
}

void n_seq_drum_setup(void)
{
  n_seq_drum_class = class_new(gensym("n_seq_drum"),
			      (t_newmethod)n_seq_drum_new,
			      0, 
			      sizeof(t_n_seq_drum),
			      0, A_GIMME, 0);
  // const
  class_addmethod(n_seq_drum_class, (t_method)n_seq_drum_column, gensym("column"),
		  A_FLOAT, 0);
  class_addmethod(n_seq_drum_class, (t_method)n_seq_drum_row, gensym("row"),
		  A_FLOAT, 0);
  class_addmethod(n_seq_drum_class, (t_method)n_seq_drum_mem, gensym("mem"),
		  A_SYMBOL, 0);
  class_addmethod(n_seq_drum_class, (t_method)n_seq_drum_tracks, gensym("tracks"),
		  A_FLOAT, 0);
  class_addmethod(n_seq_drum_class, (t_method)n_seq_drum_mods, gensym("mods"),
		  A_FLOAT, 0);
  // var
  class_addmethod(n_seq_drum_class, (t_method)n_seq_drum_patt, gensym("patt"),
		  A_FLOAT, 0);
  class_addmethod(n_seq_drum_class, (t_method)n_seq_drum_track_ofs, gensym("track_ofs"),
		  A_FLOAT, 0);
  class_addmethod(n_seq_drum_class, (t_method)n_seq_drum_sel_track, gensym("sel_track"),
		  A_FLOAT, 0);
  class_addmethod(n_seq_drum_class, (t_method)n_seq_drum_sel_mod, gensym("sel_mod"),
		  A_FLOAT, 0);
  class_addmethod(n_seq_drum_class, (t_method)n_seq_drum_major, gensym("major"),
		  A_FLOAT, 0);
  class_addmethod(n_seq_drum_class, (t_method)n_seq_drum_minor, gensym("minor"),
		  A_FLOAT, 0);
  // methods
  class_addmethod(n_seq_drum_class, (t_method)n_seq_drum_init, gensym("init"), 0);
  class_addmethod(n_seq_drum_class, (t_method)n_seq_drum_redraw, gensym("redraw"), 0);
  class_addmethod(n_seq_drum_class, (t_method)n_seq_drum_disp, gensym("disp"),
		  A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
  class_addmethod(n_seq_drum_class, (t_method)n_seq_drum_pos, gensym("pos"),
		  A_FLOAT, 0);
  // view
  class_addmethod(n_seq_drum_class, (t_method)n_seq_drum_size, gensym("size"),
		  A_FLOAT, 0);
  class_addmethod(n_seq_drum_class, (t_method)n_seq_drum_mod_width, gensym("mod_width"),
		  A_FLOAT, 0);
  class_addmethod(n_seq_drum_class, (t_method)n_seq_drum_mod_height, gensym("mod_height"),
		  A_FLOAT, 0);
  class_addmethod(n_seq_drum_class, (t_method)n_seq_drum_color_frame,
		  gensym("color_frame"), A_FLOAT, 0);
  class_addmethod(n_seq_drum_class, (t_method)n_seq_drum_color_back,
		  gensym("color_back"), A_FLOAT, 0);
  class_addmethod(n_seq_drum_class, (t_method)n_seq_drum_color_major,
		  gensym("color_major"), A_FLOAT, 0);
  class_addmethod(n_seq_drum_class, (t_method)n_seq_drum_color_minor,
		  gensym("color_minor"), A_FLOAT, 0);
  class_addmethod(n_seq_drum_class, (t_method)n_seq_drum_color_note,
		  gensym("color_note"), A_FLOAT, 0);
  class_addmethod(n_seq_drum_class, (t_method)n_seq_drum_color_acc,
		  gensym("color_acc"), A_FLOAT, 0);
  class_addmethod(n_seq_drum_class, (t_method)n_seq_drum_color_flam,
		  gensym("color_flam"), A_FLOAT, 0);
  class_addmethod(n_seq_drum_class, (t_method)n_seq_drum_color_mod,
		  gensym("color_mod"), A_FLOAT, A_FLOAT, 0);
}
