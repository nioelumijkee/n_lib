#include "m_pd.h"

#define MINPOS 0
/* (96 * 1000) - 1 = 95999 */
#define MAXPOS 95999

static t_class *n_clock_class;

typedef struct _n_clock
{
  t_object x_obj;
  t_outlet *outlet_96th;
  t_outlet *outlet_12th;
  t_outlet *outlet_1th;
  t_outlet *outlet_run;
  t_outlet *outlet_loop;
  t_float sr;
  t_float time;
  int run;
  int loop;
  int ss;
  int se;
  int ls;
  int ll;
  int le;
  t_float ph;
  t_float inc;
  int count;
  int change;
  int z12th;
} t_n_clock;

// out position
//----------------------------------------------------------------------------//
void n_clock_outpos(t_n_clock *x)
{
  int i;
  // clip
  if (x->count > MAXPOS)
    {
      x->count = MAXPOS;
    }
  // loop
  if (x->loop)
    {
      if (x->count >= x->le)
	{
	  x->count = x->ls;
	}
    }
  // song
  else
    {
      if (x->count >= x->se)
	{
	  x->count = x->ss;
	  outlet_float(x->outlet_run, 0);
	  x->run = 0;
	}
    }
  // 1th
  if ((x->count % 96) == 0)
    {
      outlet_float(x->outlet_1th, x->count / 96);
    }
  // 12th
  i = x->count % 24;
  if (i < 12)
    {
      i = 1;
    }
  else
    {
      i = 0;
    }
  if (i != x->z12th)
    {
      outlet_float(x->outlet_12th, i);
    }
  x->z12th = i;
  // 96th
  outlet_float(x->outlet_96th, x->count);
}

// various function
//----------------------------------------------------------------------------//
void n_clock_calc_constant(t_n_clock *x)
{
  //       [         to Hz      ]   [     sr   ]
  x->inc = 1. / (x->time * 0.001) * (1. / x->sr);
}



// input function
//----------------------------------------------------------------------------//
void n_clock_start(t_n_clock *x)
{
  x->z12th = -1;
  if (x->loop)
    {
      x->count = x->ls;
    }
  else
    {
      x->count = x->ss;
    }
  // run ?
  if (x->run)
    {
      x->change = 1;
    }
  else
    {
      n_clock_outpos(x);
    }
}

//----------------------------------------------------------------------------//
void n_clock_run(t_n_clock *x, t_floatarg f)
{
  x->run = f;
  // run ?
  if (x->run)
    {
      x->change = 1;
    }
  outlet_float(x->outlet_run, x->run);
}

//----------------------------------------------------------------------------//
void n_clock_back(t_n_clock *x)
{
  x->z12th = -1;
  int i;
  i = x->count / 96;
  i--;
  x->count = i * 96;
  if (x->count < MINPOS)
    x->count = MINPOS;
  // run ?
  if (x->run)
    {
      x->change = 1;
    }
  else
    {
      n_clock_outpos(x);
    }
}


//----------------------------------------------------------------------------//
void n_clock_next(t_n_clock *x)
{
  x->z12th = -1;
  int i;
  i = x->count / 96;
  x->count = (i + 1) * 96;
  if (x->count > MAXPOS)
    x->count = MAXPOS;
  // run ?
  if (x->run)
    {
      x->change = 1;
    }
  else
    {
      n_clock_outpos(x);
    }
}

//----------------------------------------------------------------------------//
void n_clock_loop(t_n_clock *x, t_floatarg f)
{
  x->z12th = -1;
  x->loop = f;
  outlet_float(x->outlet_loop, x->loop);
  if (x->loop)
    {
      x->count = x->ls;
    }
  // run ?
  if (x->run)
    {
      x->change = 1;
    }
  else
    {
      n_clock_outpos(x);
    }
}

//----------------------------------------------------------------------------//
void n_clock_pos(t_n_clock *x, t_floatarg f)
{
  x->z12th = -1;
  x->count = f;
  if (x->count < MINPOS)
    x->count = MINPOS;
  // run ?
  if (x->run)
    {
      x->change = 1;
    }
  else
    {
      n_clock_outpos(x);
    }
}

//----------------------------------------------------------------------------//
void n_clock_ss(t_n_clock *x, t_floatarg f)
{
  x->ss = f;
  if (x->ss < MINPOS)
    x->ss = MINPOS;
}

//----------------------------------------------------------------------------//
void n_clock_se(t_n_clock *x, t_floatarg f)
{
  x->se = f;
  if (x->se < MINPOS)
    x->se = MINPOS;
}

//----------------------------------------------------------------------------//
void n_clock_calc_le(t_n_clock *x)
{
  x->le = x->ls + x->ll;
}

//----------------------------------------------------------------------------//
void n_clock_ls(t_n_clock *x, t_floatarg f)
{
  x->ls = f;
  if (x->ls < MINPOS)
    x->ls = MINPOS;
  n_clock_calc_le(x);
  if (x->loop)
    {
      x->count = x->ls;
    }
  // run ?
  if (x->run)
    {
      x->change = 1;
    }
  else
    {
      n_clock_outpos(x);
    }
}

//----------------------------------------------------------------------------//
void n_clock_ll(t_n_clock *x, t_floatarg f)
{
  if (f < 1)
    f = 1;
  x->ll = f;
  n_clock_calc_le(x);
}

//----------------------------------------------------------------------------//
void n_clock_time(t_n_clock *x, t_floatarg f)
{
  x->time = f;
  if (x->time < 1.)
    x->time = 1.;
  n_clock_calc_constant(x);
}

// dsp
//----------------------------------------------------------------------------//
t_int *n_clock_perform(t_int *w)
{
  t_n_clock *x = (t_n_clock *)(w[1]);
  t_float *out = (t_float *)(w[2]);
  int n = (int)(w[3]);
  
  if (x->run)
    {
      if (x->change)
	{
	  x->count--;
	  x->ph = 1.;
	  x->change = 0;
	}

      // dsp
      while (n--)
        {
	  x->ph += x->inc;
	  if (x->ph > 1.)
            {
	      x->ph -= 1.;
	      x->count++;
	      n_clock_outpos(x);
            }
	  *out++ = x->ph;
        }
    }
  else
    {
      // dsp
      while (n--)
        {
	  *out++ = x->ph;
        }
    }
  return (w + 4);
}

//----------------------------------------------------------------------------//
void n_clock_dsp(t_n_clock *x, t_signal **sp)
{
  dsp_add(n_clock_perform,
	  3,
	  x,
	  sp[0]->s_vec,
	  sp[0]->s_n);
  if (x->sr != sp[0]->s_sr)
    {
      x->sr = sp[0]->s_sr;
      n_clock_calc_constant(x);
    }
}



// setup
//----------------------------------------------------------------------------//
void *n_clock_new(void)
{
  t_n_clock *x = (t_n_clock *)pd_new(n_clock_class);
  x->sr = 44100.;
  x->time = 100.;
  x->run = 0;
  x->loop = 0;
  x->ss = 0;
  x->se = 96;
  x->ls = 0;
  x->le = 96;
  x->ph = 1.;
  x->inc = 0.;
  x->count = 0;
  x->z12th = -1;
  outlet_new(&x->x_obj, &s_signal);
  x->outlet_96th  = outlet_new(&x->x_obj, &s_float);
  x->outlet_12th  = outlet_new(&x->x_obj, &s_float);
  x->outlet_1th   = outlet_new(&x->x_obj, &s_float);
  x->outlet_run   = outlet_new(&x->x_obj, &s_float);
  x->outlet_loop  = outlet_new(&x->x_obj, &s_float);
  n_clock_calc_constant(x);
  return (void *)x;
}

//----------------------------------------------------------------------------//
void n_clock_tilde_setup(void)
{
  n_clock_class = class_new(gensym("n_clock~"), (t_newmethod)n_clock_new, 0, sizeof(t_n_clock), 0, 0, 0);
  class_addmethod(n_clock_class, (t_method)n_clock_dsp,   gensym("dsp"),              0);
  class_addmethod(n_clock_class, (t_method)n_clock_start, gensym("start"),            0);
  class_addmethod(n_clock_class, (t_method)n_clock_run,   gensym("run"),  A_DEFFLOAT, 0);
  class_addmethod(n_clock_class, (t_method)n_clock_back,  gensym("back"),             0);
  class_addmethod(n_clock_class, (t_method)n_clock_next,  gensym("next"),             0);
  class_addmethod(n_clock_class, (t_method)n_clock_loop,  gensym("loop"), A_DEFFLOAT, 0);
  class_addmethod(n_clock_class, (t_method)n_clock_pos,   gensym("pos"),  A_DEFFLOAT, 0);
  class_addmethod(n_clock_class, (t_method)n_clock_ss,    gensym("ss"),   A_DEFFLOAT, 0);
  class_addmethod(n_clock_class, (t_method)n_clock_se,    gensym("se"),   A_DEFFLOAT, 0);
  class_addmethod(n_clock_class, (t_method)n_clock_ls,    gensym("ls"),   A_DEFFLOAT, 0);
  class_addmethod(n_clock_class, (t_method)n_clock_ll,    gensym("ll"),   A_DEFFLOAT, 0);
  class_addmethod(n_clock_class, (t_method)n_clock_time,  gensym("time"), A_DEFFLOAT, 0);
}
