#include "m_pd.h"
#include "include/clip.h"
#include "include/pd_open_array.c"

#define O_ERROR -1
#define DELAY_MAX 44100

#define W0(N) w[0][(N)].w_float
#define W1(N) w[1][(N)].w_float

#define OUTLETF(S, F)                                   \
  {                                                     \
    t_atom a[1];                                        \
    SETFLOAT(a, (t_float)(F));                          \
    outlet_anything(x->out, gensym((S)), 1, a);		    \
  }


static t_class *n_peakdetect_class;

typedef struct _n_peakdetect
{
  t_object x_obj;
  t_outlet *out; /* outlet */
  t_symbol *s[2]; /* array's */
  int disp; /* dispalyed */
  t_float sr;
  t_float level;
  t_float attack;
  t_float decay;
  int window;
  t_float skq;
  t_float skt;
  int skw;
} t_n_peakdetect;

//----------------------------------------------------------------------------//
void n_peakdetect_doit(t_n_peakdetect *x, t_floatarg f0, t_floatarg f1)
{
  int i,j,k,m;
  t_word *w[2];
  t_garray *g[2];
  int l[2];
  int start;
  int len;
  int end;

  // open array source ---------------------------------- //
  l[0] = pd_open_array(x->s[0], &w[0], &g[0]);
  if (l[0] < 2)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      post("n_peakdetect: bad array source");
      return;
    }

  // validate start length ------------------------------ //
  start = f0;
  len = f1;
  if (start < 0)
    {
      start = l[0] + start;
      if (start < 0)
	{
	  start = 0;
	}
    }
  else if (start >= l[0])
    {
      start = l[0] - 1;
    }

 // end 0 ... la
  if (len <= 0)
    {
      end = l[0] + len;
      if (end < 0)
	{
	  end = 0;
	}
    }
  else
    {
      end = start + len;
    }

  if (end > l[0])
    {
      end = l[0];
    }

  // length
  len = end - start;
  if (len < 1)
    {
      len = 0;
    }

  if (len == 0)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      post("n_peakdetect: length = 0");
      return;
    }

  // open array destination ----------------------------- //
  l[1] = pd_open_array(x->s[1], &w[1], &g[1]);
  if (l[1] < 1)
    {
      outlet_float(x->out, (t_float)O_ERROR);
      post("n_peakdetect: bad array destination");
      return;
    }

  if (l[1] != l[0])
    {
      garray_resize(g[1], l[0]);
      l[1] = pd_open_array(x->s[1], &w[1], &g[1]);
    }

  // detect --------------------------------------------- //
  t_float f;
  t_float f_z = 0.;
  t_float e;
  t_float a;
  t_float a_z = 0.;
  t_float b;
  t_float delay_a[DELAY_MAX];
  int delay_count = 0;
  int skw_count = 0;
  int skw_ready = 1;

  // clear del buf
  for (i = 0; i < DELAY_MAX; i++)
    {
      delay_a[i] = 0;
    }

  // proc
  for (i = start; i < end; i++)
    {
      f = W0(i) * x->level;

      // abs
      if (f < 0)
        {
          f = 0. - f;
        }
      
      // envf
      f = f - f_z;
      if (f > 0)
        {
          f = f * x->attack;
        }
      else
        {
          f = f * x->decay;
        }
      f = f + f_z;
      f_z = f;
      
      // delay write
      delay_a[delay_count] = f;
      delay_count++;
      if (delay_count == DELAY_MAX)
        {
          delay_count = 0;
        }
      
      // delay read
      j = delay_count - x->window;
      if (j < 0)
        {
          j = j + DELAY_MAX;
        }
      e = delay_a[j];
      
      //
      a = f / e;
      b = f - e;
      if (b < 0)
        {
          b = 0. - b;
        }
      
      //
      if (a > x->skq)
        {
          k = 1;
        }
      else
        {
          k = 0;
        }
      
      //
      if (b > x->skt)
        {
          m = 1;
        }
      else
        {
          m = 0;
        }
      
      //
      a = k & m;
      
      //
      if (a == 1  &&  a_z == 0  &&  skw_ready)
        {
          W1(i) = 1;
          skw_count = 0;
          skw_ready = 0;
        }
      else
        {
          W1(i) = 0;
        }
      a_z = a;
      
      // skip window counter
      skw_count++;
      if (skw_count == x->skw)
        {
          skw_ready = 1;
        }
    }
  garray_redraw(g[1]);
}

//----------------------------------------------------------------------------//
void n_peakdetect_set_array(t_n_peakdetect *x, t_symbol *s0, t_symbol *s1)
{
  x->s[0] = s0;
  x->s[1] = s1;
}

//----------------------------------------------------------------------------//
void n_peakdetect_sr(t_n_peakdetect *x, t_floatarg f)
{
  x->sr = f;
}

//----------------------------------------------------------------------------//
void n_peakdetect_level(t_n_peakdetect *x, t_floatarg f)
{
  x->level = f;
}

//----------------------------------------------------------------------------//
void n_peakdetect_decay(t_n_peakdetect *x, t_floatarg f)
{
  AF_CLIP_MINMAX(1., 500., f);
  f = f * 0.001 * x->sr;
  AF_CLIP_MIN(1., f);
  x->decay = 0.693147 / f;
}

//----------------------------------------------------------------------------//
void n_peakdetect_window(t_n_peakdetect *x, t_floatarg f)
{
  x->window = f * 0.001 * x->sr;
  AF_CLIP_MINMAX(0, DELAY_MAX, x->window);
}

//----------------------------------------------------------------------------//
void n_peakdetect_skq(t_n_peakdetect *x, t_floatarg f)
{
  x->skq = f;
}

//----------------------------------------------------------------------------//
void n_peakdetect_skt(t_n_peakdetect *x, t_floatarg f)
{
  x->skt = f;
}

//----------------------------------------------------------------------------//
void n_peakdetect_skw(t_n_peakdetect *x, t_floatarg f)
{
  AF_CLIP_MIN(0., f);
  x->skw = f * 0.001 * x->sr;
}

//----------------------------------------------------------------------------//
static void *n_peakdetect_new(void)
{
  t_n_peakdetect *x = (t_n_peakdetect *)pd_new(n_peakdetect_class);
  x->out = outlet_new(&x->x_obj, 0);
  x->s[0] = gensym("");
  x->s[1] = gensym("");
  x->sr = 44100;
  x->attack = 0.693147;
  return (x);
}

//----------------------------------------------------------------------------//
void n_peakdetect_setup(void)
{
  n_peakdetect_class = class_new(gensym("n_peakdetect"), (t_newmethod)n_peakdetect_new, NULL, sizeof(t_n_peakdetect), 0, A_GIMME, 0);
  class_addmethod(n_peakdetect_class, (t_method)n_peakdetect_doit, gensym("doit"), A_DEFFLOAT, A_DEFFLOAT, 0);
  class_addmethod(n_peakdetect_class, (t_method)n_peakdetect_set_array, gensym("set_array"), A_DEFSYMBOL, A_DEFSYMBOL, 0);
  class_addmethod(n_peakdetect_class, (t_method)n_peakdetect_sr, gensym("sr"), A_DEFFLOAT, 0);
  class_addmethod(n_peakdetect_class, (t_method)n_peakdetect_level, gensym("level"), A_DEFFLOAT, 0);
  class_addmethod(n_peakdetect_class, (t_method)n_peakdetect_decay, gensym("decay"), A_DEFFLOAT, 0);
  class_addmethod(n_peakdetect_class, (t_method)n_peakdetect_window, gensym("window"), A_DEFFLOAT, 0);
  class_addmethod(n_peakdetect_class, (t_method)n_peakdetect_skq, gensym("skq"), A_DEFFLOAT, 0);
  class_addmethod(n_peakdetect_class, (t_method)n_peakdetect_skt, gensym("skt"), A_DEFFLOAT, 0);
  class_addmethod(n_peakdetect_class, (t_method)n_peakdetect_skw, gensym("skw"), A_DEFFLOAT, 0);
}
