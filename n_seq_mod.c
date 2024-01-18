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
#define TRACK_MAX 32
#define TRACK_MAX_1 31

#define PAR_MIN 0
#define PAR_MAX 64
#define PAR_MAX_1 63

#define NSIZEW_MIN 4
#define NSIZEW_MAX 256
#define NSIZEH_MIN 4
#define NSIZEH_MAX 256

#define M(OFS) x->w_mem[OFS].w_float

#define NOUSE(x) if(x){};

static t_class *nsqm_class;

typedef struct _nsqm_patt
{
  int patt_ofs;
  int track_ofs[TRACK_MAX];
  int par_ofs;
} t_nsqm_patt;

typedef struct _nsqm
{
  t_object x_obj;
  t_outlet *out_seq;
  t_outlet *out_par;
  t_outlet *out_dump;
  t_outlet *out_disp;

  int columns;
  int tracks;
  int pars;
  int columns_1;
  int tracks_1;
  int pars_1;

  int sel_patt;
  int sel_track;

  int size_w;
  int size_h;
  int color_label;
  int color_back;
  int color_frame;
  int color_note;
  int color_playpos;

  int disp_maxel;
  int disp_w;
  int disp_h;

  t_nsqm_patt p[PATT_MAX];
  t_float state[COLUMN_MAX];
  int id[COLUMN_MAX];
  int id_playpos;

  /* int draw_state; */
  /* int draw_column_start; */
  /* int draw_column_end; */
  /* int draw_row; */

  int playpos;
  
  int mem_size;
  t_symbol *s_mem;
  t_word *w_mem;
  t_garray * g_mem;
  int l_mem;

  t_float buf[TRACK_MAX][COLUMN_MAX];
  t_float bufp[PAR_MAX];
} t_nsqm;

////////////////////////////////////////////////////////////////////////////////
// calc
////////////////////////////////////////////////////////////////////////////////
static void nsqm_init_patt(t_nsqm *x)
{
  x->mem_size = ((x->columns * x->tracks)+ x->pars) * PATT_MAX;
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

static void nsqm_init_mem(t_nsqm *x)
{
  // open mem
  x->l_mem = pd_open_array(x->s_mem, &x->w_mem, &x->g_mem);
  if (x->l_mem < 1)
    {
      post("n_seq_mod: error: open array: %s", x->s_mem->s_name);
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
      post("n_seq_mod: error: resize array: %s", x->s_mem->s_name);
      x->l_mem = MEM_ERROR;
      return;
    }
}

////////////////////////////////////////////////////////////////////////////////
// display
////////////////////////////////////////////////////////////////////////////////
static void nsqm_init_disp(t_nsqm *x)
{
  t_atom a[9];

  // disp size
  x->disp_w = (x->size_w * x->columns);
  x->disp_h = x->size_h;

  // set color
  SETFLOAT(a,     (t_float)x->color_back);
  SETFLOAT(a + 1, (t_float)x->color_frame);
  SETFLOAT(a + 2, (t_float)x->color_label);
  outlet_anything(x->out_disp, gensym("color"), 3, a);

  // maxel
  x->disp_maxel = (x->columns*2) + 1;
  SETFLOAT(a,     (t_float)x->disp_maxel);
  outlet_anything(x->out_disp, gensym("maxel"), 1, a);

  // size
  SETFLOAT(a,     (t_float)x->disp_w);
  SETFLOAT(a + 1, (t_float)x->disp_h);
  outlet_anything(x->out_disp, gensym("size"), 2, a);

  int id = 0;

  // notes
  SETFLOAT(a,     (t_float)3); // rect filled
  SETFLOAT(a + 2, (t_float)x->color_note); // fcolor
  SETFLOAT(a + 3, (t_float)x->color_note); // bcolor
  SETFLOAT(a + 4, (t_float)1); // width
  SETFLOAT(a + 6, (t_float)1); // y0
  SETFLOAT(a + 8, (t_float)1); // y1
  for (int i=0; i<x->columns; i++)
    {
      int x0 = (i     * x->size_w)+1;
      int x1 = ((i+1) * x->size_w)-1;
      x->state[i] = -1;
      x->id[i] = id;
      SETFLOAT(a + 1, (t_float)id); // id
      SETFLOAT(a + 5, (t_float)x0); // x0
      SETFLOAT(a + 7, (t_float)x1); // x1
      outlet_list(x->out_disp, &s_list, 9, a);
      id++;
    }

  // back
  SETFLOAT(a,     (t_float)2); // rect filled
  SETFLOAT(a + 2, (t_float)x->color_frame); // fcolor
  SETFLOAT(a + 3, (t_float)1); // width
  SETFLOAT(a + 5, (t_float)0); // y0
  SETFLOAT(a + 7, (t_float)x->disp_h); // y1
  for (int i=0; i<x->columns; i++)
    {
      int x0 = (i     * x->size_w);
      int x1 = ((i+1) * x->size_w);
      SETFLOAT(a + 1, (t_float)id); // id
      SETFLOAT(a + 4, (t_float)x0); // x0
      SETFLOAT(a + 6, (t_float)x1); // x1
      outlet_list(x->out_disp, &s_list, 8, a);
      id++;
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

static void nsqm_redraw_note(t_nsqm *x, int column)
{
  t_atom a[6];
  t_float state = M(x->p[x->sel_patt].track_ofs[x->sel_track] + column);
  if (state>0)
    {
      int x0 = (column     * x->size_w)+1;
      int x1 = ((column+1) * x->size_w)-1;
      int y0 = x->disp_h-1;
      int y1 = ((1-state) * x->disp_h);
      SETFLOAT(a,     (t_float)9); // move
      SETFLOAT(a + 1, (t_float)x->id[column]); // id
      SETFLOAT(a + 2, (t_float)x0); // x0
      SETFLOAT(a + 3, (t_float)y0); // y0
      SETFLOAT(a + 4, (t_float)x1); // x1
      SETFLOAT(a + 5, (t_float)y1); // y1
    }
  else
    {
      SETFLOAT(a,     (t_float)9); // move
      SETFLOAT(a + 1, (t_float)x->id[column]); // id
      SETFLOAT(a + 2, (t_float)0); // x0
      SETFLOAT(a + 3, (t_float)0); // y0
      SETFLOAT(a + 4, (t_float)0); // x1
      SETFLOAT(a + 5, (t_float)0); // y1
    }
  outlet_list(x->out_disp, &s_list, 6, a);
}

static void nsqm_redraw_disp(t_nsqm *x)
{
  for (int j=0; j<x->columns; j++)
    {
      t_float state = M(x->p[x->sel_patt].track_ofs[x->sel_track] + j);
      if (state != x->state[j])
	{
	  nsqm_redraw_note(x, j);
	  x->state[j] = state;
	}
    }
}

static void nsqm_redraw_playpos(t_nsqm *x)
{
  t_atom a[6];
  int x0 = (x->playpos * x->size_w);
  int x1 = ((x->playpos+1) * x->size_w);
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
static void nsqm_dump_par(t_nsqm *x)
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
static void nsqm_mouse(t_nsqm *x,
		       t_floatarg ml,
		       t_floatarg cx,
		       t_floatarg cy,
		       t_floatarg sh,
		       t_floatarg ctrl)
{
  if (ml>=1)
    {
      int column = cx / (t_float)x->size_w;
      t_float state = cy / (t_float)x->size_h;
      _clip_minmax(0, x->columns_1, column);
      _clip_minmax(0, 1, state);
      state = 1-state;
      if (x->state[column] != state)
	{
	  M(x->p[x->sel_patt].track_ofs[x->sel_track] + column) = state;
	  nsqm_redraw_note(x, column);
	  x->state[column] = state;
	}
    }
  NOUSE(sh);
  NOUSE(ctrl);
}

static void nsqm_pos(t_nsqm *x, t_floatarg f)
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
  nsqm_redraw_playpos(x);
}

static void nsqm_posl(t_nsqm *x, t_floatarg f)
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
  nsqm_redraw_playpos(x);
}

////////////////////////////////////////////////////////////////////////////////
// const
////////////////////////////////////////////////////////////////////////////////
static void nsqm_init(t_nsqm *x, t_symbol *s)
{
  x->s_mem = s;
  nsqm_init_patt(x);
  nsqm_init_mem(x);
  nsqm_init_disp(x);
  nsqm_redraw_disp(x);
}

static void nsqm_columns(t_nsqm *x, t_floatarg f)
{
  _clip_minmax(COLUMN_MIN, COLUMN_MAX, f);
  x->columns=f;
  x->columns_1=f-1;
}

static void nsqm_tracks(t_nsqm *x, t_floatarg f)
{
  _clip_minmax(TRACK_MIN, TRACK_MAX, f);
  x->tracks=f;
  x->tracks_1=f-1;
}

static void nsqm_pars(t_nsqm *x, t_floatarg f)
{
  _clip_minmax(PAR_MIN, PAR_MAX, f);
  x->pars=f;
  x->pars_1=f-1;
}

////////////////////////////////////////////////////////////////////////////////
// var
////////////////////////////////////////////////////////////////////////////////
static void nsqm_patt(t_nsqm *x, t_floatarg f)
{
  _clip_minmax(0, PATT_MAX_1, f);
  x->sel_patt=f;
  nsqm_redraw_disp(x);
  nsqm_dump_par(x);
}

static void nsqm_track(t_nsqm *x, t_floatarg f)
{
  _clip_minmax(0, x->tracks, f);
  x->sel_track=f;
  nsqm_redraw_disp(x);
}

static void nsqm_par(t_nsqm *x, t_floatarg n, t_floatarg f)
{
  int npar = n;
  _clip_minmax(0, x->pars_1, npar);
  M(x->p[x->sel_patt].par_ofs + npar) = f;
}

////////////////////////////////////////////////////////////////////////////////
// view
////////////////////////////////////////////////////////////////////////////////
static void nsqm_size_w(t_nsqm *x, t_floatarg f)
{
  _clip_minmax(NSIZEW_MIN, NSIZEW_MAX, f);
  x->size_w=f;
}

static void nsqm_size_h(t_nsqm *x, t_floatarg f)
{
  _clip_minmax(NSIZEH_MIN, NSIZEH_MAX, f);
  x->size_h=f;
}

static void nsqm_color_back(t_nsqm *x, t_floatarg f)
{
  x->color_back = f;
}

static void nsqm_color_frame(t_nsqm *x, t_floatarg f)
{
  x->color_frame = f;
}

static void nsqm_color_note(t_nsqm *x, t_floatarg f)
{
  x->color_note = f;
}

static void nsqm_color_playpos(t_nsqm *x, t_floatarg f)
{
  x->color_playpos = f;
}

////////////////////////////////////////////////////////////////////////////////
// set/dump
////////////////////////////////////////////////////////////////////////////////
static void nsqm_set(t_nsqm *x, t_floatarg p, t_floatarg t, t_floatarg c, t_floatarg v)
{
  _clip_minmax(0, PATT_MAX_1, p);
  _clip_minmax(0, x->tracks_1, t);
  _clip_minmax(0, x->columns_1, c);
  _clip_minmax(0, 1, v);
  M(x->p[(int)p].track_ofs[(int)t] + (int)c) = v;
  if (p == x->sel_patt)
    {
      nsqm_redraw_disp(x);
    }
}

static void nsqm_dump(t_nsqm *x, t_floatarg p, t_floatarg t, t_floatarg c)
{
  _clip_minmax(0, PATT_MAX_1, p);
  _clip_minmax(0, x->tracks_1, t);
  _clip_minmax(0, x->columns_1, c);
  t_float v = M(x->p[(int)p].track_ofs[(int)t] + (int)c);
  outlet_float(x->out_dump, v);
}

static void nsqm_dump_par_list(t_nsqm *x, t_symbol *s, int ac, t_atom *av)
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
static void nsqm_copy(t_nsqm *x)
{
  for (int i=0; i<x->tracks; i++)
    {
      for (int j=0; j<x->columns; j++)
	{
	  x->buf[i][j] = M(x->p[x->sel_patt].track_ofs[i] + j);
	}
    }
  for (int i=0; i<x->pars; i++)
    {
      x->bufp[i] = M(x->p[x->sel_patt].par_ofs + i);
    }
}

static void nsqm_paste(t_nsqm *x)
{
  for (int i=0; i<x->tracks; i++)
    {
      for (int j=0; j<x->columns; j++)
	{
	  M(x->p[x->sel_patt].track_ofs[i] + j) = x->buf[i][j];
	}
    }
  for (int i=0; i<x->pars; i++)
    {
      M(x->p[x->sel_patt].par_ofs + i) = x->bufp[i];
    }
  nsqm_redraw_disp(x);
  nsqm_dump_par(x);
}

static void nsqm_default(t_nsqm *x, t_symbol *s, int ac, t_atom *av)
{
  for (int i=0; i<x->tracks; i++)
    {
      for (int j=0; j<x->columns; j++)
	{
	  M(x->p[x->sel_patt].track_ofs[i] + j) = 0;
	}
    }
  for (int i=0; i<x->pars && i<ac; i++)
    {
      M(x->p[x->sel_patt].par_ofs + i) = atom_getfloatarg(i,ac,av);
    }
  nsqm_redraw_disp(x);
  nsqm_dump_par(x);
  NOUSE(s);
}

////////////////////////////////////////////////////////////////////////////////
// setup
////////////////////////////////////////////////////////////////////////////////
static void *nsqm_new(void)
{
  t_nsqm *x = (t_nsqm *)pd_new(nsqm_class);
  x->out_seq = outlet_new(&x->x_obj, 0);
  x->out_par = outlet_new(&x->x_obj, 0);
  x->out_dump = outlet_new(&x->x_obj, 0);
  x->out_disp = outlet_new(&x->x_obj, 0);
  x->color_label=22;
  return (void*)x;
}

void n_seq_mod_setup(void)
{
  nsqm_class = class_new(gensym("n_seq_mod"),
			 (t_newmethod)nsqm_new,
			 0, 
			 sizeof(t_nsqm),
			 0, 0, 0);
  class_addmethod(nsqm_class, (t_method)nsqm_init, gensym("init"),
		  A_SYMBOL, 0);
  class_addmethod(nsqm_class, (t_method)nsqm_columns, gensym("columns"),
		  A_FLOAT, 0);
  class_addmethod(nsqm_class, (t_method)nsqm_tracks, gensym("tracks"),
		  A_FLOAT, 0);
  class_addmethod(nsqm_class, (t_method)nsqm_pars, gensym("pars"),
		  A_FLOAT, 0);
  class_addmethod(nsqm_class, (t_method)nsqm_patt, gensym("patt"),
		  A_FLOAT, 0);
  class_addmethod(nsqm_class, (t_method)nsqm_track, gensym("track"),
		  A_FLOAT, 0);
  class_addmethod(nsqm_class, (t_method)nsqm_size_w, gensym("size_w"),
		  A_FLOAT, 0); 
  class_addmethod(nsqm_class, (t_method)nsqm_size_h, gensym("size_h"),
		  A_FLOAT, 0);
  class_addmethod(nsqm_class, (t_method)nsqm_color_back,
		  gensym("color_back"), A_FLOAT, 0);
  class_addmethod(nsqm_class, (t_method)nsqm_color_frame,
		  gensym("color_frame"), A_FLOAT, 0);
  class_addmethod(nsqm_class, (t_method)nsqm_color_note,
		  gensym("color_note"), A_FLOAT, 0);
  class_addmethod(nsqm_class, (t_method)nsqm_color_playpos,
		  gensym("color_playpos"), A_FLOAT, 0);
  class_addmethod(nsqm_class, (t_method)nsqm_mouse, gensym("mouse"),
		  A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
  class_addmethod(nsqm_class, (t_method)nsqm_pos, gensym("pos"),
		  A_FLOAT, 0);
  class_addmethod(nsqm_class, (t_method)nsqm_posl, gensym("posl"),
		  A_FLOAT, 0);
  class_addmethod(nsqm_class, (t_method)nsqm_par,
		  gensym("par"), A_FLOAT, A_FLOAT, 0);
  class_addmethod(nsqm_class, (t_method)nsqm_copy, gensym("copy"), 0);
  class_addmethod(nsqm_class, (t_method)nsqm_paste, gensym("paste"), 0);
  class_addmethod(nsqm_class, (t_method)nsqm_default, gensym("default"), A_GIMME, 0);
  class_addmethod(nsqm_class, (t_method)nsqm_set,
		  gensym("set"), A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
  class_addmethod(nsqm_class, (t_method)nsqm_dump,
		  gensym("dump"), A_FLOAT, A_FLOAT, A_FLOAT, 0);
  class_addmethod(nsqm_class, (t_method)nsqm_dump_par_list,
		  gensym("dump_par"), A_GIMME, 0);
}
