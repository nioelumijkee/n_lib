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
#define ROW_MAX_1 31

#define PAR_MIN 0
#define PAR_MAX 64
#define PAR_MAX_1 63

#define NSIZE_MIN 4
#define NSIZE_MAX 256

#define M(OFS) x->w_mem[OFS].w_float

#define NOUSE(x) if(x){};

static t_class *nsqo_class;

typedef struct _nsqo_patt
{
  int patt_ofs;
  int par_ofs;
} t_nsqo_patt;

typedef struct _nsqo
{
  t_object x_obj;
  t_outlet *out_seq;
  t_outlet *out_par;
  t_outlet *out_dump;
  t_outlet *out_disp;

  int columns;
  int rows;
  int pars;
  int columns_1;
  int rows_1;
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

  t_nsqo_patt p[PATT_MAX];
  int state[COLUMN_MAX];
  int id[COLUMN_MAX][ROW_MAX];
  int id_playpos;

  int draw_state;
  int draw_column_start;
  int draw_column_end;
  int draw_row;
  int playpos;
  
  int mem_size;
  t_symbol *s_mem;
  t_word *w_mem;
  t_garray * g_mem;
  int l_mem;

  t_float buf[COLUMN_MAX];
  t_float bufp[PAR_MAX];
} t_nsqo;

////////////////////////////////////////////////////////////////////////////////
// calc
////////////////////////////////////////////////////////////////////////////////
static void nsqo_init_patt(t_nsqo *x)
{
  x->mem_size = (x->columns + x->pars) * PATT_MAX;
  // calc ofs
  int ofs = 0;
  for (int i=0; i<PATT_MAX; i++)
    {
      x->p[i].patt_ofs = ofs;
      ofs+=x->columns;
      x->p[i].par_ofs = ofs;
      ofs+=x->pars;
    }
}

static void nsqo_init_mem(t_nsqo *x)
{
  // open mem
  x->l_mem = pd_open_array(x->s_mem, &x->w_mem, &x->g_mem);
  if (x->l_mem < 1)
    {
      post("n_seq_one: error: open array: %s", x->s_mem->s_name);
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
      post("n_seq_one: error: resize array: %s", x->s_mem->s_name);
      x->l_mem = MEM_ERROR;
      return;
    }
}

////////////////////////////////////////////////////////////////////////////////
// display
////////////////////////////////////////////////////////////////////////////////
static void nsqo_init_disp(t_nsqo *x)
{
  t_atom a[9];

  // disp size
  x->disp_w = (x->size * x->columns);
  x->disp_h = (x->size * x->rows);

  // set color
  SETFLOAT(a,     (t_float)x->color_back);
  SETFLOAT(a + 1, (t_float)x->color_frame);
  SETFLOAT(a + 2, (t_float)x->color_label);
  outlet_anything(x->out_disp, gensym("color"), 3, a);

  // maxel
  x->disp_maxel = (x->columns * x->rows) + 1;
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
  for (int i=0; i<x->columns; i++)
    {
      int x0 = (i     * x->size);
      int x1 = ((i+1) * x->size);
      x->state[i] = 0;
      for (int j=0; j<x->rows; j++)
	{
	  int y0 = (j     * x->size);
	  int y1 = ((j+1) * x->size);
	  x->id[i][j] = id;

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

static void nsqo_redraw_note(t_nsqo *x, int column, int state_old, int state_new)
{
  t_atom a[4];
  SETFLOAT(a,     (t_float)10); // color2
  if (state_new==0)
    {
      SETFLOAT(a + 1, (t_float)x->id[column][state_old-1]); // id
      SETFLOAT(a + 2, (t_float)x->color_frame); // fcolor
      SETFLOAT(a + 3, (t_float)x->color_back); // bcolor
      outlet_list(x->out_disp, &s_list, 4, a);
    }
  else
    {
      if (state_old > 0)
	{
	  SETFLOAT(a + 1, (t_float)x->id[column][state_old-1]); // id
	  SETFLOAT(a + 2, (t_float)x->color_frame); // fcolor
	  SETFLOAT(a + 3, (t_float)x->color_back); // bcolor
	  outlet_list(x->out_disp, &s_list, 4, a);
	}
      SETFLOAT(a + 1, (t_float)x->id[column][state_new-1]); // id
      SETFLOAT(a + 2, (t_float)x->color_frame); // fcolor
      SETFLOAT(a + 3, (t_float)x->color_note); // bcolor
      outlet_list(x->out_disp, &s_list, 4, a);
    }
}

static void nsqo_redraw_disp(t_nsqo *x)
{
  for (int j=0; j<x->columns; j++)
    {
      int state = M(x->p[x->sel_patt].patt_ofs + j);
      if (state != x->state[j])
	{
	  nsqo_redraw_note(x, j, x->state[j], state);
	  x->state[j] = state;
	}
    }
}

static void nsqo_redraw_playpos(t_nsqo *x)
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
static void nsqo_dump_par(t_nsqo *x)
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
static void nsqo_mouse(t_nsqo *x,
		       t_floatarg ml,
		       t_floatarg cx,
		       t_floatarg cy,
		       t_floatarg sh,
		       t_floatarg ctrl)
{
  if (ml==1)
    {
      int column = cx / (t_float)x->size;
      int row = cy / (t_float)x->size;
      _clip_minmax(0, x->columns_1, column);
      _clip_minmax(0, x->rows_1, row);
      row += 1;
      int state = M(x->p[x->sel_patt].patt_ofs + column);
      if (state == 0)
	{
	  x->draw_state = 1;
	}
      else
	{
	  if (state == row)
	    {
	      x->draw_state = 0;
	    }
	  else
	    {
	      x->draw_state = 1;
	    }
	}
      x->draw_column_start = column;
      x->draw_column_end = column;
      x->draw_row = row;

      state = x->draw_state * x->draw_row;
      M(x->p[x->sel_patt].patt_ofs + column) = state;
      nsqo_redraw_note(x, column, x->state[column], state);
      x->state[column] = state;
    }
  else if (ml==2 && sh)
    {
      int column = cx / (t_float)x->size;
      int row = cy / (t_float)x->size;
      _clip_minmax(0, x->columns_1, column);
      _clip_minmax(0, x->rows_1, row);
      row += 1;
      int st, end;
      x->draw_column_end = column;
      if (column < x->draw_column_start)
	{
	  st = x->draw_column_end;
	  end = x->draw_column_start;
	}
      else
	{	
	  st = x->draw_column_start;
	  end = x->draw_column_end;
	}
      x->draw_row = row;

      int state = x->draw_state * x->draw_row;
      for (int i=st; i<=end; i++)
	{
	  if (x->state[i] != state)
	    {
	      M(x->p[x->sel_patt].patt_ofs + i) = state;
	      nsqo_redraw_note(x, i, x->state[i], state);
	      x->state[i] = state;
	    }
	}
    }
  NOUSE(ctrl);
}

static void nsqo_pos(t_nsqo *x, t_floatarg f)
{
  int pos = f;
  _clip_minmax(0, x->columns_1, pos);
  x->playpos = pos;
  t_float v = M(x->p[x->sel_patt].patt_ofs + pos);
  outlet_float(x->out_seq, v);
  nsqo_redraw_playpos(x);
}

////////////////////////////////////////////////////////////////////////////////
// const
////////////////////////////////////////////////////////////////////////////////
static void nsqo_init(t_nsqo *x, t_symbol *s)
{
  x->s_mem = s;
  nsqo_init_patt(x);
  nsqo_init_mem(x);
  nsqo_init_disp(x);
  nsqo_redraw_disp(x);
}

static void nsqo_columns(t_nsqo *x, t_floatarg f)
{
  _clip_minmax(COLUMN_MIN, COLUMN_MAX, f);
  x->columns=f;
  x->columns_1=f-1;
}

static void nsqo_rows(t_nsqo *x, t_floatarg f)
{
  _clip_minmax(ROW_MIN, ROW_MAX, f);
  x->rows=f;
  x->rows_1=f-1;
}

static void nsqo_pars(t_nsqo *x, t_floatarg f)
{
  _clip_minmax(PAR_MIN, PAR_MAX, f);
  x->pars=f;
  x->pars_1=f-1;
}

////////////////////////////////////////////////////////////////////////////////
// var
////////////////////////////////////////////////////////////////////////////////
static void nsqo_patt(t_nsqo *x, t_floatarg f)
{
  _clip_minmax(0, PATT_MAX_1, f);
  x->sel_patt=f;
  nsqo_redraw_disp(x);
  nsqo_dump_par(x);
}

static void nsqo_par(t_nsqo *x, t_floatarg n, t_floatarg f)
{
  int npar = n;
  _clip_minmax(0, x->pars_1, npar);
  M(x->p[x->sel_patt].par_ofs + npar) = f;
}

////////////////////////////////////////////////////////////////////////////////
// view
////////////////////////////////////////////////////////////////////////////////
static void nsqo_size(t_nsqo *x, t_floatarg f)
{
  _clip_minmax(NSIZE_MIN, NSIZE_MAX, f);
  x->size=f;
}

static void nsqo_color_back(t_nsqo *x, t_floatarg f)
{
  x->color_back = f;
}

static void nsqo_color_frame(t_nsqo *x, t_floatarg f)
{
  x->color_frame = f;
}

static void nsqo_color_note(t_nsqo *x, t_floatarg f)
{
  x->color_note = f;
}

static void nsqo_color_playpos(t_nsqo *x, t_floatarg f)
{
  x->color_playpos = f;
}

////////////////////////////////////////////////////////////////////////////////
// set/dump
////////////////////////////////////////////////////////////////////////////////
static void nsqo_set(t_nsqo *x, t_floatarg p, t_floatarg c, t_floatarg v)
{
  _clip_minmax(0, PATT_MAX_1, p);
  _clip_minmax(0, x->columns_1, c);
  M(x->p[(int)p].patt_ofs + (int)c) = v;
  if (p == x->sel_patt)
    {
      nsqo_redraw_disp(x);
    }
}

static void nsqo_dump(t_nsqo *x, t_floatarg p, t_floatarg c)
{
  _clip_minmax(0, PATT_MAX_1, p);
  _clip_minmax(0, x->columns_1, c);
  t_float v = M(x->p[(int)p].patt_ofs + (int)c);
  outlet_float(x->out_dump, v);
}

////////////////////////////////////////////////////////////////////////////////
// add
////////////////////////////////////////////////////////////////////////////////
static void nsqo_copy(t_nsqo *x)
{
  for (int j=0; j<x->columns; j++)
    {
      x->buf[j] = M(x->p[x->sel_patt].patt_ofs + j);
    }
  for (int i=0; i<x->pars; i++)
    {
      x->bufp[i] = M(x->p[x->sel_patt].par_ofs + i);
    }
}

static void nsqo_paste(t_nsqo *x)
{
  for (int j=0; j<x->columns; j++)
    {
      M(x->p[x->sel_patt].patt_ofs + j) = x->buf[j];
    }
  for (int i=0; i<x->pars; i++)
    {
      M(x->p[x->sel_patt].par_ofs + i) = x->bufp[i];
    }
  nsqo_redraw_disp(x);
  nsqo_dump_par(x);
}

static void nsqo_default(t_nsqo *x, t_symbol *s, int ac, t_atom *av)
{
  for (int j=0; j<x->columns; j++)
    {
      M(x->p[x->sel_patt].patt_ofs + j) = 0;
    }
  for (int i=0; i<x->pars && i<ac; i++)
    {
      M(x->p[x->sel_patt].par_ofs + i) = atom_getfloatarg(i,ac,av);
    }
  nsqo_redraw_disp(x);
  nsqo_dump_par(x);
  NOUSE(s);
}

////////////////////////////////////////////////////////////////////////////////
// setup
////////////////////////////////////////////////////////////////////////////////
static void *nsqo_new(void)
{
  t_nsqo *x = (t_nsqo *)pd_new(nsqo_class);
  x->out_seq = outlet_new(&x->x_obj, 0);
  x->out_par = outlet_new(&x->x_obj, 0);
  x->out_dump = outlet_new(&x->x_obj, 0);
  x->out_disp = outlet_new(&x->x_obj, 0);
  x->color_label=22;
  return (void*)x;
}

void n_seq_one_setup(void)
{
  nsqo_class = class_new(gensym("n_seq_one"),
			 (t_newmethod)nsqo_new,
			 0, 
			 sizeof(t_nsqo),
			 0, 0, 0);
  class_addmethod(nsqo_class, (t_method)nsqo_init, gensym("init"),
		  A_SYMBOL, 0);
  class_addmethod(nsqo_class, (t_method)nsqo_columns, gensym("columns"),
		  A_FLOAT, 0);
  class_addmethod(nsqo_class, (t_method)nsqo_rows, gensym("rows"),
		  A_FLOAT, 0);
  class_addmethod(nsqo_class, (t_method)nsqo_pars, gensym("pars"),
		  A_FLOAT, 0);
  class_addmethod(nsqo_class, (t_method)nsqo_patt, gensym("patt"),
		  A_FLOAT, 0);
  class_addmethod(nsqo_class, (t_method)nsqo_size, gensym("size"),
		  A_FLOAT, 0);
  class_addmethod(nsqo_class, (t_method)nsqo_color_back,
		  gensym("color_back"), A_FLOAT, 0);
  class_addmethod(nsqo_class, (t_method)nsqo_color_frame,
		  gensym("color_frame"), A_FLOAT, 0);
  class_addmethod(nsqo_class, (t_method)nsqo_color_note,
		  gensym("color_note"), A_FLOAT, 0);
  class_addmethod(nsqo_class, (t_method)nsqo_color_playpos,
		  gensym("color_playpos"), A_FLOAT, 0);
  class_addmethod(nsqo_class, (t_method)nsqo_mouse, gensym("mouse"),
		  A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
  class_addmethod(nsqo_class, (t_method)nsqo_pos, gensym("pos"),
		  A_FLOAT, 0);
  class_addmethod(nsqo_class, (t_method)nsqo_par,
		  gensym("par"), A_FLOAT, A_FLOAT, 0);
  class_addmethod(nsqo_class, (t_method)nsqo_copy, gensym("copy"), 0);
  class_addmethod(nsqo_class, (t_method)nsqo_paste, gensym("paste"), 0);
  class_addmethod(nsqo_class, (t_method)nsqo_default, gensym("default"), A_GIMME, 0);
  class_addmethod(nsqo_class, (t_method)nsqo_set,
		  gensym("set"), A_FLOAT, A_FLOAT, A_FLOAT, 0);
  class_addmethod(nsqo_class, (t_method)nsqo_dump,
		  gensym("dump"), A_FLOAT, A_FLOAT, 0);
}
