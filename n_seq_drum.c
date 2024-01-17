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

#define TRACK_MIN 1
#define TRACK_MAX 16
#define TRACK_MAX_1 15

#define PAR_MIN 0
#define PAR_MAX 64
#define PAR_MAX_1 63

#define NSIZE_MIN 4
#define NSIZE_MAX 256

#define M(OFS) x->w_mem[OFS].w_float

#define NOUSE(x) if(x){};

static t_class *nsqd_class;

typedef struct _nsqd_patt
{
  int patt_ofs;
  int par_ofs;
  int track_ofs[TRACK_MAX];
} t_nsqd_patt;

typedef struct _nsqd
{
  t_object x_obj;
  t_outlet *out_seq;
  t_outlet *out_par;
  t_outlet *out_dump;
  t_outlet *out_sel;
  t_outlet *out_disp;

  int columns;
  int tracks;
  int pars;
  int columns_1;
  int tracks_1;
  int pars_1;

  int sel_patt;

  int size;
  int color_label;
  int color_back;
  int color_frame;
  int color_note;
  int color_playpos;

  int disp_maxel;
  int disp_w;
  int disp_h;

  t_nsqd_patt p[PATT_MAX];
  int state[TRACK_MAX][COLUMN_MAX];
  int id[TRACK_MAX][COLUMN_MAX];
  int id_playpos;

  int draw_state;
  int draw_track;
  int playpos;
  
  int mem_size;
  t_symbol *s_mem;
  t_word *w_mem;
  t_garray * g_mem;
  int l_mem;

  t_float buf[TRACK_MAX][COLUMN_MAX];
  t_float bufp[PAR_MAX];
} t_nsqd;

////////////////////////////////////////////////////////////////////////////////
// calc
////////////////////////////////////////////////////////////////////////////////
static void nsqd_init_patt(t_nsqd *x)
{
  x->mem_size = ((x->tracks * x->columns) + x->pars) * PATT_MAX;
  // calc ofs
  int ofs = 0;
  for (int i=0; i<PATT_MAX; i++)
    {
      x->p[i].patt_ofs = ofs;
      for (int j=0; j<x->tracks; j++)
	{
	  x->p[i].track_ofs[j] = ofs;
	  ofs+=x->columns;
	}
      x->p[i].par_ofs = ofs;
      ofs+=x->pars;
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

////////////////////////////////////////////////////////////////////////////////
// display
////////////////////////////////////////////////////////////////////////////////
static void nsqd_init_disp(t_nsqd *x)
{
  t_atom a[9];

  // disp size
  x->disp_w = (x->size * x->columns);
  x->disp_h = (x->size * x->tracks);

  // set color
  SETFLOAT(a,     (t_float)x->color_back);
  SETFLOAT(a + 1, (t_float)x->color_frame);
  SETFLOAT(a + 2, (t_float)x->color_label);
  outlet_anything(x->out_disp, gensym("color"), 3, a);

  // maxel
  x->disp_maxel = (x->columns * x->tracks) + 1;
  SETFLOAT(a,     (t_float)x->disp_maxel);
  outlet_anything(x->out_disp, gensym("maxel"), 1, a);

  // size
  SETFLOAT(a,     (t_float)x->disp_w);
  SETFLOAT(a + 1, (t_float)x->disp_h);
  outlet_anything(x->out_disp, gensym("size"), 2, a);

  // notes
  SETFLOAT(a,     (t_float)3); // rect filled
  SETFLOAT(a + 2, (t_float)x->color_frame); // fcolor
  SETFLOAT(a + 3, (t_float)x->color_back); // bcolor
  SETFLOAT(a + 4, (t_float)1); // width
  int id = 0;
  for (int i=0; i<x->tracks; i++)
    {
      int y0 = (i     * x->size);
      int y1 = ((i+1) * x->size);
      for (int j=0; j<x->columns; j++)
	{
	  x->state[i][j] = 0;
	  x->id[i][j] = id;

	  int x0 = (j     * x->size);
	  int x1 = ((j+1) * x->size);
	  SETFLOAT(a + 1, (t_float)id); // id
	  SETFLOAT(a + 5, (t_float)x0); // x0
	  SETFLOAT(a + 6, (t_float)y0); // y0
	  SETFLOAT(a + 7, (t_float)x1); // x1
	  SETFLOAT(a + 8, (t_float)y1); // y1
	  outlet_list(x->out_disp, &s_list, 9, a);

	  id++;
	}
    }

  // create playpos
  SETFLOAT(a,     (t_float)2); // rect
  SETFLOAT(a + 1, (t_float)id); // id
  SETFLOAT(a + 2, (t_float)x->color_playpos); // fcolor
  SETFLOAT(a + 3, (t_float)2); // width
  SETFLOAT(a + 4, (t_float)1); // x0
  SETFLOAT(a + 5, (t_float)1); // y0
  SETFLOAT(a + 6, (t_float)1); // x1
  SETFLOAT(a + 7, (t_float)1); // y1
  outlet_list(x->out_disp, &s_list, 8, a);
  x->id_playpos = id;
}

static void nsqd_redraw_note(t_nsqd *x, int track, int column, int state)
{
  t_atom a[4];
  SETFLOAT(a,     (t_float)10); // color2
  if (state==1)
    {
      SETFLOAT(a + 1, (t_float)x->id[track][column]); // id
      SETFLOAT(a + 2, (t_float)x->color_frame); // fcolor
      SETFLOAT(a + 3, (t_float)x->color_note); // bcolor
      outlet_list(x->out_disp, &s_list, 4, a);
    }
  else
    {
      SETFLOAT(a + 1, (t_float)x->id[track][column]); // id
      SETFLOAT(a + 2, (t_float)x->color_frame); // fcolor
      SETFLOAT(a + 3, (t_float)x->color_back); // bcolor
      outlet_list(x->out_disp, &s_list, 4, a);
    }
}

static void nsqd_redraw_disp(t_nsqd *x)
{
  for (int i=0; i<x->tracks; i++)
    {
      for (int j=0; j<x->columns; j++)
	{
	  int state = M(x->p[x->sel_patt].track_ofs[i] + j);
	  if (state != x->state[i][j])
	    {
	      x->state[i][j] = state;
	      nsqd_redraw_note(x, i, j, state);
	    }
	}
    }
}

static void nsqd_redraw_playpos(t_nsqd *x)
{
  t_atom a[6];
  int x0 = (x->playpos * x->size);
  int x1 = ((x->playpos+1) * x->size);
  SETFLOAT(a,     (t_float)9); // move
  SETFLOAT(a + 1, (t_float)x->id_playpos); // id
  SETFLOAT(a + 2, (t_float)x0); // x0
  SETFLOAT(a + 3, (t_float)1); // y0
  SETFLOAT(a + 4, (t_float)x1); // x1
  SETFLOAT(a + 5, (t_float)x->disp_h); // y1
  outlet_list(x->out_disp, &s_list, 6, a);
}

////////////////////////////////////////////////////////////////////////////////
// pars
////////////////////////////////////////////////////////////////////////////////
static void nsqd_dump_par(t_nsqd *x)
{
  t_atom a[2];
  for (int i=0; i<x->pars; i++)
    {
      SETFLOAT(a, (t_float)i);
      SETFLOAT(a + 1, M(x->p[x->sel_patt].par_ofs + i));
      outlet_list(x->out_par, &s_list, 2, a);
    }
}

////////////////////////////////////////////////////////////////////////////////
// methods
////////////////////////////////////////////////////////////////////////////////
static void nsqd_mouse(t_nsqd *x,
		       t_floatarg ml,
		       t_floatarg cx,
		       t_floatarg cy,
		       t_floatarg sh,
		       t_floatarg ctrl)
{
  if (ml==1)
    {
      int column = cx / (t_float)x->size;
      int track = cy / (t_float)x->size;
      _clip_minmax(0, x->columns_1, column);
      _clip_minmax(0, x->tracks_1, track);
      int state = M(x->p[x->sel_patt].track_ofs[track] + column);
      x->draw_state = !state;
      x->draw_track = track;

      M(x->p[x->sel_patt].track_ofs[track] + column) = x->draw_state;
      x->state[track][column] = x->draw_state;
      nsqd_redraw_note(x, track, column, x->draw_state);
      outlet_float(x->out_sel, track);
    }
  else if (ml==2 && sh)
    {
      int column = cx / (t_float)x->size;
      _clip_minmax(0, x->columns_1, column);

      M(x->p[x->sel_patt].track_ofs[x->draw_track] + column) = x->draw_state;
      x->state[x->draw_track][column] = x->draw_state;
      nsqd_redraw_note(x, x->draw_track, column, x->draw_state);
    }
  NOUSE(ctrl);
}

static void nsqd_pos(t_nsqd *x, t_floatarg f)
{
  int pos = f;
  _clip_minmax(0, x->columns_1, pos);
  x->playpos = pos;
  t_atom a[2];
  for (int i=0; i<x->tracks; i++)
    {
      SETFLOAT(a, (t_float)i);
      SETFLOAT(a + 1, M(x->p[x->sel_patt].track_ofs[i] + pos));
      outlet_list(x->out_seq, &s_list, 2, a);
    }
  nsqd_redraw_playpos(x);
}

static void nsqd_posl(t_nsqd *x, t_floatarg f)
{
  int pos = f;
  _clip_minmax(0, x->columns_1, pos);
  x->playpos = pos;
  t_atom a[TRACK_MAX];
  for (int i=0; i<x->tracks; i++)
    {
      SETFLOAT(a + i, M(x->p[x->sel_patt].track_ofs[i] + pos));
    }
  outlet_list(x->out_seq, &s_list, x->tracks, a);
  nsqd_redraw_playpos(x);
}

////////////////////////////////////////////////////////////////////////////////
// const
////////////////////////////////////////////////////////////////////////////////
static void nsqd_init(t_nsqd *x, t_symbol *s)
{
  x->s_mem = s;
  nsqd_init_patt(x);
  nsqd_init_mem(x);
  nsqd_init_disp(x);
  nsqd_redraw_disp(x);
}

static void nsqd_columns(t_nsqd *x, t_floatarg f)
{
  _clip_minmax(COLUMN_MIN, COLUMN_MAX, f);
  x->columns=f;
  x->columns_1=f-1;
}

static void nsqd_tracks(t_nsqd *x, t_floatarg f)
{
  _clip_minmax(TRACK_MIN, TRACK_MAX, f);
  x->tracks=f;
  x->tracks_1=f-1;
}

static void nsqd_pars(t_nsqd *x, t_floatarg f)
{
  _clip_minmax(PAR_MIN, PAR_MAX, f);
  x->pars=f;
  x->pars_1=f-1;
}

////////////////////////////////////////////////////////////////////////////////
// var
////////////////////////////////////////////////////////////////////////////////
static void nsqd_patt(t_nsqd *x, t_floatarg f)
{
  _clip_minmax(0, PATT_MAX_1, f);
  x->sel_patt=f;
  nsqd_redraw_disp(x);
  nsqd_dump_par(x);
}

static void nsqd_par(t_nsqd *x, t_floatarg n, t_floatarg f)
{
  int npar = n;
  _clip_minmax(0, x->pars_1, npar);
  M(x->p[x->sel_patt].par_ofs + npar) = f;
}

////////////////////////////////////////////////////////////////////////////////
// view
////////////////////////////////////////////////////////////////////////////////
static void nsqd_size(t_nsqd *x, t_floatarg f)
{
  _clip_minmax(NSIZE_MIN, NSIZE_MAX, f);
  x->size=f;
}

static void nsqd_color_back(t_nsqd *x, t_floatarg f)
{
  x->color_back = f;
}

static void nsqd_color_frame(t_nsqd *x, t_floatarg f)
{
  x->color_frame = f;
}

static void nsqd_color_note(t_nsqd *x, t_floatarg f)
{
  x->color_note = f;
}

static void nsqd_color_playpos(t_nsqd *x, t_floatarg f)
{
  x->color_playpos = f;
}

////////////////////////////////////////////////////////////////////////////////
// set/dump
////////////////////////////////////////////////////////////////////////////////
static void nsqd_set(t_nsqd *x, t_floatarg p, t_floatarg t, t_floatarg c, t_floatarg v)
{
  _clip_minmax(0, PATT_MAX_1, p);
  _clip_minmax(0, x->tracks_1, t);
  _clip_minmax(0, x->columns_1, c);
  M(x->p[(int)p].track_ofs[(int)t] + (int)c) = (v>0);
  if (p == x->sel_patt)
    {
      nsqd_redraw_disp(x);
    }
}

static void nsqd_dump(t_nsqd *x, t_floatarg p, t_floatarg t, t_floatarg c)
{
  _clip_minmax(0, PATT_MAX_1, p);
  _clip_minmax(0, x->tracks_1, t);
  _clip_minmax(0, x->columns_1, c);
  t_float v = M(x->p[(int)p].track_ofs[(int)t] + (int)c);
  outlet_float(x->out_dump, v);
}

static void nsqd_dump_par_list(t_nsqd *x, t_symbol *s, int ac, t_atom *av)
{
  t_atom a[PAR_MAX];
  int p = atom_getfloatarg(0, ac, av);
  _clip_minmax(0, PATT_MAX_1, p);
  _clip_minmax(0, PAR_MAX, ac);
  for (int i=1; i<ac; i++)
    {
      int n = atom_getfloatarg(i, ac, av);
      _clip_minmax(0, x->pars_1, n);
      t_float v = M(x->p[(int)p].par_ofs + n);
      SETFLOAT(a + i - 1, v);
    }
  outlet_list(x->out_dump, &s_list, ac-1, a);
  NOUSE(s);
}

////////////////////////////////////////////////////////////////////////////////
// add
////////////////////////////////////////////////////////////////////////////////
static void nsqd_copy(t_nsqd *x)
{
  for (int i=0; i<x->tracks; i++)
    {
      for (int j=0; j<x->columns; j++)
	{
	  x->buf[i][j] = M(x->p[x->sel_patt].patt_ofs + (i * x->columns) + j);
	}
    }
  for (int i=0; i<x->pars; i++)
    {
      x->bufp[i] = M(x->p[x->sel_patt].par_ofs + i);
    }
}

static void nsqd_paste(t_nsqd *x)
{
  for (int i=0; i<x->tracks; i++)
    {
      for (int j=0; j<x->columns; j++)
	{
	  M(x->p[x->sel_patt].patt_ofs + (i * x->columns) + j) = x->buf[i][j];
	}
    }
  for (int i=0; i<x->pars; i++)
    {
      M(x->p[x->sel_patt].par_ofs + i) = x->bufp[i];
    }
  nsqd_redraw_disp(x);
  nsqd_dump_par(x);
}

static void nsqd_default(t_nsqd *x, t_symbol *s, int ac, t_atom *av)
{
  for (int i=0; i<x->tracks; i++)
    {
      for (int j=0; j<x->columns; j++)
	{
	  M(x->p[x->sel_patt].patt_ofs + (i * x->columns) + j) = 0;
	}
    }
  
  for (int i=0; i<x->pars && i<ac; i++)
    {
      M(x->p[x->sel_patt].par_ofs + i) = atom_getfloatarg(i,ac,av);
    }
  nsqd_redraw_disp(x);
  nsqd_dump_par(x);
  NOUSE(s);
}

////////////////////////////////////////////////////////////////////////////////
// setup
////////////////////////////////////////////////////////////////////////////////
static void *nsqd_new(void)
{
  t_nsqd *x = (t_nsqd *)pd_new(nsqd_class);
  x->out_seq = outlet_new(&x->x_obj, 0);
  x->out_par = outlet_new(&x->x_obj, 0);
  x->out_dump = outlet_new(&x->x_obj, 0);
  x->out_sel = outlet_new(&x->x_obj, 0);
  x->out_disp = outlet_new(&x->x_obj, 0);
  x->color_label=22;
  return (void*)x;
}

void n_seq_drum_setup(void)
{
  nsqd_class = class_new(gensym("n_seq_drum"),
			 (t_newmethod)nsqd_new,
			 0, 
			 sizeof(t_nsqd),
			 0, 0, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_init, gensym("init"),
		  A_SYMBOL, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_columns, gensym("columns"),
		  A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_tracks, gensym("tracks"),
		  A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_pars, gensym("pars"),
		  A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_patt, gensym("patt"),
		  A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_size, gensym("size"),
		  A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_color_back,
		  gensym("color_back"), A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_color_frame,
		  gensym("color_frame"), A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_color_note,
		  gensym("color_note"), A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_color_playpos,
		  gensym("color_playpos"), A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_mouse, gensym("mouse"),
		  A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_pos, gensym("pos"),
		  A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_posl, gensym("posl"),
		  A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_par,
		  gensym("par"), A_FLOAT, A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_copy, gensym("copy"), 0);
  class_addmethod(nsqd_class, (t_method)nsqd_paste, gensym("paste"), 0);
  class_addmethod(nsqd_class, (t_method)nsqd_default, gensym("default"), A_GIMME, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_set,
		  gensym("set"), A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_dump,
		  gensym("dump"), A_FLOAT, A_FLOAT, A_FLOAT, 0);
  class_addmethod(nsqd_class, (t_method)nsqd_dump_par_list,
		  gensym("dump_par"), A_GIMME, 0);
}
