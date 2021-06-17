#include "m_pd.h"
#include "include/clip.h"

#define W_MIN 3
#define W_MAX 64
#define H_MIN 3
#define H_MAX 64
#define MAXALL 1600 /* W_MAX * H_MAX */
#define HISTORY 100000

static t_class *n_life_class;

typedef struct _n_life
{
  t_object x_obj;
  t_outlet *out_cell;
  t_outlet *out_step;
  t_outlet *out_all;
  t_outlet *out_hash;
  t_outlet *out_cycle;
  t_int universe;
  t_int w;
  t_int h;
  unsigned char c[W_MAX][H_MAX];
  unsigned char b[W_MAX][H_MAX];
  t_int hash[HISTORY];
  t_int step;
} t_n_life;

//----------------------------------------------------------------------------//
// game
//----------------------------------------------------------------------------//
static void n_life_step(t_n_life *x)
{
  t_int i, ix, iy;
  t_int ncx[8];
  t_int ncy[8];
  t_int neighbor;
  
  // next generation (buf)
  for (iy = 0; iy < x->h; iy++)
    {
      for (ix = 0; ix < x->w; ix++)
	{
	  // neighbor cell
	  ncx[0] = ix - 1;
	  ncy[0] = iy - 1;
	  ncx[1] = ix;
	  ncy[1] = iy - 1;
	  ncx[2] = ix + 1;
	  ncy[2] = iy - 1;
	  ncx[3] = ix - 1;
	  ncy[3] = iy;
	  ncx[4] = ix + 1;
	  ncy[4] = iy;
	  ncx[5] = ix - 1;
	  ncy[5] = iy + 1;
	  ncx[6] = ix;
	  ncy[6] = iy + 1;
	  ncx[7] = ix + 1;
	  ncy[7] = iy + 1;
	  
	  // bounded universe
	  if (x->universe)
	    {
	      // wrap cell's
	      for (i = 0; i < 8; i++)
		{
		  if (ncx[i] < 0)
		    ncx[i] = x->w - 1;
		  else if (ncx[i] > x->w - 1)
		    ncx[i] = 0;
		  if (ncy[i] < 0)
		    ncy[i] = x->h - 1;
		  else if (ncy[i] > x->h - 1)
		    ncy[i] = 0;
		}
	    }
	  
	  // closed universe
	  else
	    {
	      // clip cell's
	      for (i = 0; i < 8; i++)
		{
		  if (ncx[i] < 0)
		    ncx[i] = 9999;
		  else if (ncx[i] > x->w - 1)
		    ncx[i] = 9999;
		  if (ncy[i] < 0)
		    ncy[i] = 9999;
		  else if (ncy[i] > x->h - 1)
		    ncy[i] = 9999;
		}
	    }
	  
	  // calc
	  neighbor = 0;
	  for (i = 0; i < 8; i++)
	    {
	      if (ncx[i] != 9999 && ncy[i] != 9999)
		{
		  if (x->c[ncx[i]][ncy[i]])
		    neighbor++;
		}
	    }
	  
	  // life ?
	  if (x->c[ix][iy] == 0 && neighbor == 3)
	    x->b[ix][iy] = 1;
	  else if (x->c[ix][iy] == 1 && (neighbor == 3 || neighbor == 2))
	    x->b[ix][iy] = 1;
	  else
	    x->b[ix][iy] = 0;
	}
    }
  
  // copy
  for (iy = 0; iy < x->h; iy++)
    {
      for (ix = 0; ix < x->w; ix++)
	x->c[ix][iy] = x->b[ix][iy];
    }

  // step
  x->step++;
  if (x->step == HISTORY)
    {
      x->step = HISTORY - 1;
      post("n_life: step = %d", x->step);
    }
}

//----------------------------------------------------------------------------//
// add.
//----------------------------------------------------------------------------//
static void n_life_clear(t_n_life *x)
{
  t_int ix, iy;

  // cell's
  for (iy = 0; iy < x->h; iy++)
    {
      for (ix = 0; ix < x->w; ix++)
	{
	  x->c[ix][iy] = 0;
	}
    }
}

//----------------------------------------------------------------------------//
// input func.
//----------------------------------------------------------------------------//
static void n_life_set_universe(t_n_life *x, t_floatarg f)
{
  x->universe = f;
}

//----------------------------------------------------------------------------//
static void n_life_set_width(t_n_life *x, t_floatarg f)
{
  AF_CLIP_MINMAX(W_MIN, W_MAX, f);
  x->w = f;
  n_life_clear(x);
}

//----------------------------------------------------------------------------//
static void n_life_set_height(t_n_life *x, t_floatarg f)
{
  AF_CLIP_MINMAX(H_MIN, H_MAX, f);
  x->h = f;
  n_life_clear(x);
}

//----------------------------------------------------------------------------//
static void n_life_set_cell(t_n_life *x, t_floatarg n, t_floatarg f)
{
  t_int ix, iy;
  iy = (int)n / x->w;
  ix = (int)n % x->w;
  AF_CLIP_MINMAX(0, W_MAX, ix);
  AF_CLIP_MINMAX(0, W_MAX, iy);
  if (f > 0)
    x->c[(int)ix][(int)iy] = 1;
  else
    x->c[(int)ix][(int)iy] = 0;
}

//----------------------------------------------------------------------------//
static void n_life_dump(t_n_life *x)
{
  t_atom a[2];
  t_int ix, iy;

  // cell's
  for (iy = 0; iy < x->h; iy++)
    {
      for (ix = 0; ix < x->w; ix++)
	{
	  SETFLOAT(a, (t_float) (iy * x->w) + ix);
	  SETFLOAT(a + 1, (t_float) x->c[ix][iy]);
	  outlet_list(x->out_cell, &s_list, 2, a);
	}
    }

  // stat
  t_int all = 0;
  t_int hash = 0;
  for (iy = 0; iy < x->h; iy++)
    {
      for (ix = 0; ix < x->w; ix++)
	{
	  // sum
	  all += x->c[ix][iy];
	  // hash
	  if (x->c[ix][iy])
	    {
	      hash = (hash + 1234567) * 1103515245;
	    }
	  else
	    {
	      hash = hash + 1234569;
	    }
	}
    }
  x->hash[x->step] = hash;


  // find cycle
  t_int cycle = 0;
  t_int find_first = x->step;
  t_int find_pos = find_first - 1;
  t_int len;
  t_int i,j;


  while(find_pos >= 0)
    {
      // seq
      if (x->hash[find_first] == x->hash[find_pos])
	{
	  len = x->step - find_pos;
	  // test
	  if (len == 1)
	    {
	      cycle = 1;
	    }
	  else
	    {
	      cycle = len;
	      len--;
	      i = find_first - 1;
	      j = find_pos - 1;
	      while(len--)
		{
		  if (x->hash[i] != x->hash[j])
		    {
		      cycle = 0;
		      len = 0;
		    }
		  i--;
		  j--;
		}
	    }
	  break;
	}
      find_pos--;
    }

  // out
  outlet_float(x->out_cycle,(t_float) cycle);
  outlet_float(x->out_hash, (t_float) hash);
  outlet_float(x->out_all, (t_float) all);
  outlet_float(x->out_step,(t_float) x->step);
}

//----------------------------------------------------------------------------//
static void n_life_start(t_n_life *x)
{
  x->step = 0;
}

//----------------------------------------------------------------------------//
static void *n_life_new(void)
{
  t_n_life *x = (t_n_life *)pd_new(n_life_class);
  x->out_cell = outlet_new(&x->x_obj, 0);
  x->out_step = outlet_new(&x->x_obj, 0);
  x->out_all = outlet_new(&x->x_obj, 0);
  x->out_hash = outlet_new(&x->x_obj, 0);
  x->out_cycle = outlet_new(&x->x_obj, 0);
  x->step = 0;
  return (x);
}

//----------------------------------------------------------------------------//
void n_life_setup(void)
{
  n_life_class = class_new(gensym("n_life"), (t_newmethod)n_life_new, 0, sizeof(t_n_life), 0, A_GIMME, 0);
  class_addmethod(n_life_class, (t_method)n_life_set_universe, gensym("universe"), A_DEFFLOAT, 0);
  class_addmethod(n_life_class, (t_method)n_life_set_width, gensym("width"), A_DEFFLOAT, 0);
  class_addmethod(n_life_class, (t_method)n_life_set_height, gensym("height"), A_DEFFLOAT, 0);
  class_addmethod(n_life_class, (t_method)n_life_set_cell, gensym("set_cell"), A_DEFFLOAT, A_DEFFLOAT, 0);
  class_addmethod(n_life_class, (t_method)n_life_dump, gensym("dump"), 0);
  class_addmethod(n_life_class, (t_method)n_life_start, gensym("start"), 0);
  class_addmethod(n_life_class, (t_method)n_life_step, gensym("step"), 0);
}
