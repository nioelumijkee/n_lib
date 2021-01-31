/* file browser */
#include <string.h>
#include <m_pd.h>
#include <pd/g_all_guis.h>
#include "include/browser.c"
#include "include/math.h"

#define MAXD 64
#define C_B_HEADER 0
#define C_B_NOSEL 1
#define C_B_SEL 2

static t_class *n_browser_class;

typedef struct _n_browser_pos
{
  short int abspos;
  short int offset;
  short int cursor;
} t_n_browser_pos;

typedef struct _n_browser
{
  t_object x_obj;
  t_outlet *out;
  t_browser b;
  int size_x;
  int size_y;
  int dir_pos;
  int scroll_clip;
  int scroll_max;
  t_n_browser_pos bp[MAXD];
} t_n_browser;

//----------------------------------------------------------------------------//
void n_browser_dir_read(t_n_browser *x)
{
  while (read_dir(&x->b))
    {
      error("read directory: %s\n", x->b.cdir);
      dir_up(x->b.cdir);
      x->dir_pos = analyz_path(x->b.cdir);
    }
  x->scroll_max = x->b.all - x->size_y;
  AF_CLIP_MIN(0, x->scroll_max);
  x->scroll_clip = x->b.all - 1;
  AF_CLIP_MAX(x->size_y - 1, x->scroll_clip);
}

//----------------------------------------------------------------------------//
void n_browser_dir_print(t_n_browser *x)
{
  int i, j, k;
  int color_b;
  t_atom a[4];
  char buf[MAXFILENAME];
  
  // make atom
  // [N] [COLOR_F] [COLOR_B] [STRING]
  SETFLOAT(a, (t_float)0);
  SETFLOAT(a + 1, (t_float)0);
  SETFLOAT(a + 2, (t_float)C_B_HEADER);
  sprintf(buf, "[%d][%s]", x->b.all, x->b.cdir);
  buf[x->size_x] = '\0';
  SETSYMBOL(a + 3, gensym(buf));
  
  outlet_anything(x->out, &s_list, 4, a);
  
  for (i = 0; i < x->size_y; i++)
    {
      SETFLOAT(a, (t_float)i + 1);
      
      k = x->bp[x->dir_pos].offset + i;
      j = x->b.idx[k];
      
      // select
      if (x->bp[x->dir_pos].cursor == i)
	color_b = C_B_SEL;
      // no select
      else
	color_b = C_B_NOSEL;
      SETFLOAT(a + 2, (t_float)color_b);
      
      if (k < x->b.all)
	{
	  SETFLOAT(a + 1, (t_float)x->b.mode[j]);
	  strcpy(buf, x->b.filename[j]);
	  buf[x->size_x] = '\0';
	  SETSYMBOL(a + 3, gensym(buf));
	}
      else
	{
	  SETFLOAT(a + 1, (t_float)color_b);
	  SETSYMBOL(a + 3, gensym(" "));
	}
      
      outlet_anything(x->out, &s_list, 4, a);
    }
}

//----------------------------------------------------------------------------//
void n_browser_init(t_n_browser *x)
{
  if (get_cur_dir(x->b.cdir))
    {
      error("get_cur_dir");
      return;
    }
  add_slash(x->b.cdir);
  x->dir_pos = analyz_path(x->b.cdir);
  for (int i = 0; i < x->dir_pos; i++)
    {
      x->bp[i].abspos = 0;
      x->bp[i].offset = 0;
      x->bp[i].cursor = 0;
    }
}

//----------------------------------------------------------------------------//
void n_browser_dir_up(t_n_browser *x)
{
  dir_up(x->b.cdir);
  x->dir_pos = analyz_path(x->b.cdir);
}

//----------------------------------------------------------------------------//
void n_browser_change_dir_this(t_n_browser *x)
{
  char buf[MAXPATHNAME];
  t_atom a[2];
  int i = x->b.idx[x->bp[x->dir_pos].abspos];
  if (x->b.mode[i] == 0)
    {
      sprintf(buf, "%s%s/", x->b.cdir, x->b.filename[i]);
      strcpy(x->b.cdir, buf);
		//
      x->dir_pos = analyz_path(x->b.cdir);
      x->bp[x->dir_pos].abspos = 0;
      x->bp[x->dir_pos].offset = 0;
      x->bp[x->dir_pos].cursor = 0;
      //
      n_browser_dir_read(x);
      n_browser_dir_print(x);
    }
  else
    {
      SETFLOAT(a, (t_float)-1);
      sprintf(buf, "%s%s", x->b.cdir, x->b.filename[i]);
      SETSYMBOL(a + 1, gensym(buf));
      outlet_anything(x->out, &s_list, 2, a);
    }
}

//----------------------------------------------------------------------------//
void n_browser_change_dir(t_n_browser *x, t_symbol *s)
{
  strcpy(x->b.cdir, s->s_name);
  add_slash(x->b.cdir);
  x->dir_pos = analyz_path(x->b.cdir);
  x->bp[x->dir_pos].abspos = 0;
  x->bp[x->dir_pos].offset = 0;
  x->bp[x->dir_pos].cursor = 0;
}

//----------------------------------------------------------------------------//
void n_browser_scroll_up(t_n_browser *x, t_floatarg f)
{
  int i;
  if (x->b.all > 0)
    {
      for (i = 0; i < f; i++)
	{
	  // one step
	  // abspos
	  x->bp[x->dir_pos].abspos--;
	  AF_CLIP_MIN(0, x->bp[x->dir_pos].abspos);
	  // offset
	  if (x->bp[x->dir_pos].abspos < (x->bp[x->dir_pos].offset + 1))
	    x->bp[x->dir_pos].offset = x->bp[x->dir_pos].abspos;
	  // cursor
	  x->bp[x->dir_pos].cursor--;
	  AF_CLIP_MIN(0, x->bp[x->dir_pos].cursor);
	}
    }
}

//----------------------------------------------------------------------------//
void n_browser_scroll_down(t_n_browser *x, t_floatarg f)
{
  int i;
  if (x->b.all > 0)
    {
      for (i = 0; i < f; i++)
	{
	  // one step
	  // abspos
	  x->bp[x->dir_pos].abspos++;
	  if (x->bp[x->dir_pos].abspos >= x->b.all)
	    x->bp[x->dir_pos].abspos = x->b.all - 1;
	  // offset
	  if (x->bp[x->dir_pos].abspos >= (x->bp[x->dir_pos].offset + x->size_y))
	    x->bp[x->dir_pos].offset++;
	  AF_CLIP_MAX(x->scroll_max, x->bp[x->dir_pos].offset);
	  // cursor
	  x->bp[x->dir_pos].cursor++;
	  AF_CLIP_MAX(x->scroll_clip, x->bp[x->dir_pos].cursor);
	}
    }
}

//----------------------------------------------------------------------------//
void n_browser_size(t_n_browser *x, t_floatarg f1, t_floatarg f2)
{
  x->size_x = f1;
  AF_CLIP_MINMAX(8, 128, x->size_x);
  x->size_y = f2;
  AF_CLIP_MINMAX(8, 64, x->size_y);
}

//----------------------------------------------------------------------------//
void n_browser_get_this(t_n_browser *x)
{
  char buf[MAXPATHNAME];
  t_atom a[2];
  int i = x->b.idx[x->bp[x->dir_pos].abspos];
  SETFLOAT(a, (t_float)-1);
  sprintf(buf, "%s%s", x->b.cdir, x->b.filename[i]);
  SETSYMBOL(a + 1, gensym(buf));
  outlet_anything(x->out, &s_list, 2, a);
}

//----------------------------------------------------------------------------//
void n_browser_filter(t_n_browser *x, t_symbol *s, t_floatarg f)
{
  if (!(strcmp(s->s_name, "hidden")))
    {
      if (f > 0)
	x->b.filter = x->b.filter | f_hidden;
      else
	x->b.filter = x->b.filter & (~f_hidden);
    }
  else if (!(strcmp(s->s_name, "name")))
    {
      if (f > 0)
	x->b.filter = x->b.filter | f_name;
      else
	x->b.filter = x->b.filter & (~f_name);
    }
  else if (!(strcmp(s->s_name, "ex")))
    {
      if (f > 0)
	x->b.filter = x->b.filter | f_ex;
      else
	x->b.filter = x->b.filter & (~f_ex);
    }
}

//----------------------------------------------------------------------------//
void *n_browser_new(void)
{
  t_n_browser *x = (t_n_browser *)pd_new(n_browser_class);
  
  x->b.filter = x->b.filter | f_mode;
  x->b.filter = x->b.filter | f_name;
  x->b.filter = x->b.filter & (~f_hidden);
  x->out = outlet_new(&x->x_obj, 0);
  
  return (void *)x;
}

//----------------------------------------------------------------------------//
void n_browser_setup(void)
{
  n_browser_class = class_new(gensym("n_browser"), (t_newmethod)n_browser_new, NULL, sizeof(t_n_browser), CLASS_DEFAULT, A_GIMME, 0);
  class_addmethod(n_browser_class, (t_method)n_browser_init, gensym("init"), 0);
  class_addmethod(n_browser_class, (t_method)n_browser_dir_up, gensym("dir_up"), 0);
  class_addmethod(n_browser_class, (t_method)n_browser_change_dir_this, gensym("change_dir_this"), 0);
  class_addmethod(n_browser_class, (t_method)n_browser_change_dir, gensym("change_dir"), A_SYMBOL, 0);
  class_addmethod(n_browser_class, (t_method)n_browser_scroll_up, gensym("scroll_up"), A_FLOAT, 0);
  class_addmethod(n_browser_class, (t_method)n_browser_scroll_down, gensym("scroll_down"), A_FLOAT, 0);
  class_addmethod(n_browser_class, (t_method)n_browser_dir_read, gensym("dir_read"), 0);
  class_addmethod(n_browser_class, (t_method)n_browser_dir_print, gensym("dir_print"), 0);
  class_addmethod(n_browser_class, (t_method)n_browser_get_this, gensym("get_this"), 0);
  class_addmethod(n_browser_class, (t_method)n_browser_filter, gensym("filter"), A_SYMBOL, A_DEFFLOAT, 0);
  class_addmethod(n_browser_class, (t_method)n_browser_size, gensym("size"), A_FLOAT, A_FLOAT, 0);
}
