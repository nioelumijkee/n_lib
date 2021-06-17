#include "m_pd.h"
#include "include/clip.h"

enum
{
  MODE_CYCLE = 0,
  MODE_FREE
};

#define MAXVOICE 64
#define MAXVOICE_1 63
#define MINVAL -9999
#define MAXOCTAVE 16
#define MINOCTAVE 0

static t_class *n_key2n_class;

typedef struct _n_key2n
{
  t_object x_obj;
  int on;
  int oct;
  int keyadd;
  int voice;
  int vel;
  int mode;
  int tkr;
  int hold;
  int count;
  int key_count;
  int keybuf[MAXVOICE];
  t_clock *td[MAXVOICE];
  int buf[MAXVOICE];
  int key_pressed[MAXVOICE];
  int debug;
} t_n_key2n;

//----------------------------------------------------------------------------//
void n_key2n_noteout(t_n_key2n *x, int voice, int pitch, int gate)
{
  t_atom a[3];
  SETFLOAT(a, (t_float)voice);
  SETFLOAT(a + 1, (t_float)pitch);
  SETFLOAT(a + 2, (t_float)gate);
  outlet_anything(x->x_obj.ob_outlet, &s_list, 3, a);
}

//----------------------------------------------------------------------------//
void n_key2n_reset(t_n_key2n *x)
{
  x->count = 0;
  x->key_count = 0;
  for (int i = 0; i < MAXVOICE; i++)
    {
      x->keybuf[i] = MINVAL;
      x->buf[i] = 0;
      x->key_pressed[i] = 0;
      clock_unset(x->td[i]);
      n_key2n_noteout(x, i, 0, 0);
    }
}

//----------------------------------------------------------------------------//
void n_key2n_on(t_n_key2n *x, t_floatarg f)
{
  x->on = f;
  n_key2n_reset(x);
}

//----------------------------------------------------------------------------//
void n_key2n_octave(t_n_key2n *x, t_floatarg f)
{
  if (f == -2)
    {
      x->oct--;
    }
  else if (f == -1)
    {
      x->oct++;
    }
  else
    {
      x->oct = f;
    }

  AF_CLIP_MINMAX(MINOCTAVE, MAXOCTAVE, x->oct);
  x->keyadd = (x->oct * 12) - 1;
}

//----------------------------------------------------------------------------//
void n_key2n_voice(t_n_key2n *x, t_floatarg f)
{
  AF_CLIP_MINMAX(1, MAXVOICE_1, f);
  x->voice = f;
  n_key2n_reset(x);
}

//----------------------------------------------------------------------------//
void n_key2n_vel(t_n_key2n *x, t_floatarg f)
{
  AF_CLIP_MINMAX(0, 127, f);
  x->vel = f;
}

//----------------------------------------------------------------------------//
void n_key2n_mode(t_n_key2n *x, t_floatarg f)
{
  if (f == 0)
    x->mode = MODE_CYCLE;
  else
    x->mode = MODE_FREE;
  n_key2n_reset(x);
}

//----------------------------------------------------------------------------//
void n_key2n_tkr(t_n_key2n *x, t_floatarg f)
{
  AF_CLIP_MINMAX(1, 1000, f);
  x->tkr = f;
}

//----------------------------------------------------------------------------//
void n_key2n_debug(t_n_key2n *x, t_floatarg f)
{
  x->debug = f;
}

//----------------------------------------------------------------------------//
void n_key2n_hold(t_n_key2n *x)
{
  x->hold = !x->hold;
  if (x->hold && x->debug)
    {
      post("n_key2n : hold on");
    }
  else
    {
      if (x->debug)
	{
	  post("n_key2n : hold off");
	}
      n_key2n_reset(x);
    }
}

//----------------------------------------------------------------------------//
void n_key2n_gateoff(t_n_key2n *x)
{
  int i;
  
  if (x->key_pressed[x->buf[0]] == 1)
    {
      x->key_pressed[x->buf[0]] = 0;
      if (x->debug)
	{
	  post("off : %d", x->buf[0]);
	}
      
      // out noteoff
      for (i = 0; i < MAXVOICE; i++)
        {
	  if (x->keybuf[i] == x->buf[0])
            {
	      n_key2n_noteout(x, i, x->keyadd + x->keybuf[i], 0);
	      x->keybuf[i] = MINVAL;
	      break;
            }
        }
    }
  
  // shift array
  i = 0;
  while (i < MAXVOICE_1)
    {
      x->buf[i] = x->buf[i + 1];
      i++;
    }
  x->buf[i] = 0;
}

//----------------------------------------------------------------------------//
void n_key2n_key(t_n_key2n *x, t_floatarg f)
{
  if (!x->on)
    return;
  
  int i, v;
  
  v = f;
  AF_CLIP_MINMAX(0, MAXVOICE_1, v);
  clock_unset(x->td[v]);
  
  // delete in turn
  for (i = 0; i < MAXVOICE; i++)
    {
      if (x->buf[i] == v)
        {
	  // shift array
	  while (i < MAXVOICE_1)
            {
	      x->buf[i] = x->buf[i + 1];
	      i++;
            }
	  x->buf[i] = 0;
        }
    }
  
  if (x->key_pressed[v] == 0)
    {
      x->key_pressed[v] = 1;
      // voice mode
      if (x->debug)
	{
	  post("on  : %d", v);
	}
      // MODE CYCLE
      if (x->mode == MODE_CYCLE)
        {
	  x->keybuf[x->count] = v;
	  n_key2n_noteout(x, x->count, x->keyadd + v, x->vel);
	  x->count++;
	  x->count = x->count % x->voice;
        }
      // MODE FREE
      else
        {
	  for (i = 0; i < x->voice; i++)
            {
	      if (x->keybuf[i] == MINVAL)
                {
		  x->keybuf[i] = v;
		  n_key2n_noteout(x, i, x->keyadd + v, x->vel);
		  i = x->voice;
                }
            }
        }
    }
}

//----------------------------------------------------------------------------//
void n_key2n_keyup(t_n_key2n *x, t_floatarg f)
{
  if (!x->on || x->hold)
    return;
  
  int i, v;
  
  v = f;
  AF_CLIP_MINMAX(0, MAXVOICE_1, v);
  clock_delay(x->td[v], x->tkr);
  
  // set turn
  i = 0;
  while (i < MAXVOICE)
    {
      if (x->buf[i] == 0)
        {
	  x->buf[i] = v;
	  i = MAXVOICE;
        }
      i++;
    }
}

//----------------------------------------------------------------------------//
void *n_key2n_new(void)
{
  t_n_key2n *x = (t_n_key2n *)pd_new(n_key2n_class);
  for (int i = 0; i < MAXVOICE; i++)
    {
      x->td[i] = clock_new(x, (t_method)n_key2n_gateoff);
    }
  x->on = 1;
  x->oct = 5;
  x->keyadd = 59;
  x->voice = 1;
  x->vel = 127;
  x->mode = MODE_CYCLE;
  x->tkr = 50;
  x->hold = 0;
  x->count = 0;
  x->key_count = 0;
  x->debug = 0;
  outlet_new(&x->x_obj, 0);
  return (void *)x;
}

//----------------------------------------------------------------------------//
void n_key2n_ff(t_n_key2n *x)
{
  for (int i = 0; i < MAXVOICE; i++)
    clock_free(x->td[i]);
}

//----------------------------------------------------------------------------//
void n_key2n_setup(void)
{
  n_key2n_class = class_new(gensym("n_key2n"), (t_newmethod)n_key2n_new, (t_method)n_key2n_ff, sizeof(t_n_key2n), CLASS_DEFAULT, A_GIMME, 0);
  class_addmethod(n_key2n_class, (t_method)n_key2n_on, gensym("on"), A_DEFFLOAT, 0);
  class_addmethod(n_key2n_class, (t_method)n_key2n_octave, gensym("octave"), A_DEFFLOAT, 0);
  class_addmethod(n_key2n_class, (t_method)n_key2n_voice, gensym("voice"), A_DEFFLOAT, 0);
  class_addmethod(n_key2n_class, (t_method)n_key2n_vel, gensym("vel"), A_DEFFLOAT, 0);
  class_addmethod(n_key2n_class, (t_method)n_key2n_mode, gensym("mode"), A_DEFFLOAT, 0);
  class_addmethod(n_key2n_class, (t_method)n_key2n_tkr, gensym("tkr"), A_DEFFLOAT, 0);
  class_addmethod(n_key2n_class, (t_method)n_key2n_hold, gensym("hold"), 0);
  class_addmethod(n_key2n_class, (t_method)n_key2n_reset, gensym("reset"), 0);
  class_addmethod(n_key2n_class, (t_method)n_key2n_key, gensym("key"), A_DEFFLOAT, 0);
  class_addmethod(n_key2n_class, (t_method)n_key2n_keyup, gensym("keyup"), A_DEFFLOAT, 0);
  class_addmethod(n_key2n_class, (t_method)n_key2n_debug, gensym("debug"), A_DEFFLOAT, 0);
}
