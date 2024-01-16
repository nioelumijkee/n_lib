////////////////////////////////////////////////////////////////////////////////
//  Sampler                                                                   //
////////////////////////////////////////////////////////////////////////////////

#include <math.h>
#include "m_pd.h"
#include "include/pdfunc.h"

#define PI           3.1415927
#define TWOPI        6.2831853

#define MAX_PLAYER 48
#define MAX_PLAYER_1 47
#define MAX_SAMPLE 31
#define MAX_SAMPLE_1 30
#define MIN_SAMPLE_LEN 4
#define MIN_DISP_W 16
#define MAX_DISP_W 1024
#define MIN_DISP_H 16
#define MAX_DISP_H 1024
#define MIN_SAMPLE_RATE 1000
#define SAMPLE_END_OFS 4
#define DEF_TIME_EG 0.0001
#define ENV_SIZE 512
#define ENV_SIZE_1 510
#define PPFPS 30
#define MAX_LINKS 8
#define MAX_LINKS_1 7

#define _mix(a, b, x, out) {			\
    out = (b-a)*x+a;				\
  }

#define _tanh_pade(in, out) {			\
    in = (in > 3) ? 3 : (in < -3) ? -3 : in;	\
    t_float buff = in*in;			\
    out=(in*(27 + buff)) / (27 + (9*buff));	\
  }

enum {
  STG_START=0,
  STG_AFTER,
  STG_PRE_LOOP,
  STG_LOOP,
  STG_END,
};

#define ENV_MINA 0.000001

enum {
  ENV_ATT=0,
  ENV_DEC,
  ENV_SUS,
  ENV_REL,
};

#define _clip_minmax(min, max, v) v=(v>max)?max:(v<min)?min:v;
#define _clip_min(min, v) v=(v<min)?min:v;
#define _clip_max(max, v) v=(v>max)?max:v;

static t_class *n_s_class;

typedef struct _n_linkturn
{
  int       l;
  int       t[MAX_PLAYER];
} t_n_linkturn;

typedef struct _n_sample
{
  t_symbol *s; //in
  t_word   *w;
  t_garray *g;
  int       l;
  int       l_1;
  t_float   sr; //in
} t_n_sample;

typedef struct _n_disp
{
  int       y0[MAX_DISP_W];
  int       y1[MAX_DISP_W];
  int       len,start,end;
  t_float   xr; //in
  t_float   xo; //in
  int       se_start_view;
  int       se_end_view;
  int       se_start_x;
  int       se_end_x;
  int       loop_start_view;
  int       loop_end_view;
  int       loop_start_x;
  int       loop_end_x;
  int       playpos_view;
  int       playpos_x;
} t_n_disp;

typedef struct _n_player
{
  int on;
  int sample; //in
  t_float level; //in
  t_float pan; //in
  t_float s1; //in
  t_float s2; //in
  t_float level_l;
  t_float level_r;
  t_float level_s1;
  t_float level_s2;
  t_float rnda; //in
  t_float rndz; //in
  int group; //in
  t_float bit_pre; //in
  t_float bit_post; //in
  t_float bit_ofs; //in
  t_float drive; // in
  t_float start; //in
  t_float end; //in
  t_float l_start; //in
  t_float l_end; //in
  t_float start_c;
  t_float end_c;
  t_float l_start_c;
  t_float l_end_c;
  t_float l_center_c;
  t_float l_add;
  t_float l_len;
  t_float l_m;
  t_float sdel; //in
  t_float sdel_c;
  t_float semi; //in
  t_float fine; //in
  t_float sp;
  t_float xf; //in
  int loop; //in
  int eg; //in
  t_float att; //in
  t_float dec; //in
  t_float sus; //in
  t_float rel; //in
  t_float att_c;
  t_float dec_c;
  t_float rel_c;
  int env_stage;
  t_float env;
  t_float vel;
  int filtype; //in
  t_float freq; //in
  t_float res; //in
  t_float filw;
  t_float film[4];
  t_float l_f; //in
  t_float l_g; //in
  t_float lfw;
  t_float lfz;
  t_float h_f; //in
  t_float h_g; //in
  t_float hfw;
  t_float hfz;
  int link; // in
  int stage; // sampler
  t_float phase_sample;
  t_float out;
  int mute; //in
  int solo; //in
  int ms;
  t_n_disp disp;
} t_n_player;


typedef struct _n_s
{
  t_object x_obj;
  t_outlet *out_disp;
  t_float sr;
  t_float div_1_sr;
  t_float div_sr_8;
  t_float div_2pi_sr;
  int seed; /* random */
  t_n_player player[MAX_PLAYER];
  t_n_sample sample[MAX_SAMPLE];
  t_float env_t[ENV_SIZE];
  int sel_player;
  int disp_maxel;
  int disp_w;
  int disp_h;
  int color_background;
  int color_frame;
  int color_label;
  int color_foreground;
  int color_playpos;
  int color_se;
  int color_loop;
  int id_se;
  int id_sample;
  int id_loop;
  int id_playpos;
  int mcpp;
  int mcpp_count;
  t_n_linkturn lt[MAX_LINKS];
} t_n_s;

////////////////////////////////////////////////////////////////////////////////
// calc
void n_s_init(t_n_s *x)
{
  x->sel_player=0;
  x->disp_maxel=0;
  x->disp_w=0;
  x->disp_h=0;
  x->mcpp_count=0;
  for (int i=0; i<MAX_PLAYER; i++)
    {
      x->player[i].on=0;
      x->player[i].sample=0;
      x->player[i].att=DEF_TIME_EG;
      x->player[i].dec=DEF_TIME_EG;
      x->player[i].rel=DEF_TIME_EG;
      x->player[i].rndz=1;
      x->player[i].mute=0;
      x->player[i].solo=0;
    }
}

void n_s_reset(t_n_s *x)
{
  for (int i=0; i<MAX_PLAYER; i++)
    {
      x->player[i].on=0;
    }
}

void n_s_calc_level(t_n_s *x, int n)
{
  t_float l = 1 - x->player[n].pan; 
  t_float r = 1 + x->player[n].pan;
  t_float s = 0.33333 * x->player[n].level;
  x->player[n].level_l = s * (4-l) * l;
  x->player[n].level_r = s * (4-r) * r;
}

void n_s_calc_level_s1(t_n_s *x, int n)
{
  x->player[n].level_s1 = x->player[n].level * x->player[n].s1;
}

void n_s_calc_level_s2(t_n_s *x, int n)
{
  x->player[n].level_s2 = x->player[n].level * x->player[n].s2;
}

void n_s_calc_start_end(t_n_s *x, int n)
{
  x->player[n].start_c = x->player[n].start * x->sample[x->player[n].sample].l_1;
  x->player[n].end_c = x->player[n].end * x->sample[x->player[n].sample].l_1;
  if (x->player[n].end_c < x->player[n].start_c)
    x->player[n].end_c = x->player[n].start_c;
}

void n_s_calc_l_start_end(t_n_s *x, int n)
{
  x->player[n].l_start_c = x->player[n].l_start * x->sample[x->player[n].sample].l_1;
  x->player[n].l_end_c = x->player[n].l_end * x->sample[x->player[n].sample].l_1;
  if (x->player[n].l_end_c < x->player[n].l_start_c)
    x->player[n].l_end_c = x->player[n].l_start_c;
  t_float len = x->player[n].l_end_c - x->player[n].l_start_c;
  x->player[n].l_center_c = x->player[n].l_start_c + (len/2.0);
  x->player[n].l_add = len/2.0;
  x->player[n].l_len = len;
  x->player[n].l_m = 1.0 / ((x->player[n].xf*0.999)+0.001);
}

void n_s_calc_sdel(t_n_s *x, int n)
{
  x->player[n].sdel_c = (x->player[n].sdel * x->sample[x->player[n].sample].l_1)
    + x->player[n].start;
  if (x->player[n].sdel_c > x->sample[x->player[n].sample].l_1)
    x->player[n].sdel_c = x->sample[x->player[n].sample].l_1;
}

void n_s_calc_tune(t_n_s *x, int n)
{
  t_float f = x->player[n].semi + x->player[n].fine;
  x->player[n].sp = pow(2, f/12);
}

void n_s_calc_att(t_n_s *x, int n)
{
  x->player[n].att_c = x->div_1_sr / x->player[n].att;
}

void n_s_calc_dec(t_n_s *x, int n)
{
  x->player[n].dec_c = x->div_1_sr / x->player[n].dec;
}

void n_s_calc_rel(t_n_s *x, int n)
{
  x->player[n].rel_c = x->div_1_sr / x->player[n].rel;
}

void n_s_calc_freq(t_n_s *x, int n)
{
  t_float f = x->player[n].freq;
  _clip_minmax(10, x->div_sr_8, f);
  x->player[n].filw = f * x->div_2pi_sr;
}

void n_s_calc_l_f(t_n_s *x, int n)
{
  t_float f = x->player[n].l_f;
  _clip_minmax(10, x->div_sr_8, f);
  x->player[n].lfw = f * x->div_2pi_sr;
}

void n_s_calc_h_f(t_n_s *x, int n)
{
  t_float f = x->player[n].h_f;
  _clip_minmax(10, x->div_sr_8, f);
  x->player[n].hfw = f * x->div_2pi_sr;
}

void n_s_calc_ms(t_n_s *x)
{
  int solo=0;
  for (int i=0; i<MAX_PLAYER; i++)
    {
      if (x->player[i].solo)
	{
	  solo=1;
	  break;
	}
    }
  for (int i=0; i<MAX_PLAYER; i++)
    {
      x->player[i].ms = (x->player[i].solo == solo) && (!x->player[i].mute);
      if (x->player[i].ms == 0)
	{
	  x->player[i].on=0;
	}
    }
}

void n_s_calc_constant(t_n_s *x)
{
  x->div_1_sr = 1.0 / x->sr;
  x->div_sr_8 = x->sr / 8.0;
  x->div_2pi_sr = TWOPI / x->sr;
  x->mcpp = x->sr / PPFPS; 
  for(int i=0; i<MAX_PLAYER; i++)
    {
      n_s_calc_att(x, i);
      n_s_calc_dec(x, i);
      n_s_calc_rel(x, i);
      n_s_calc_freq(x, i);
      n_s_calc_l_f(x, i);
      n_s_calc_h_f(x, i);
    }
}

void n_s_calc_env_t(t_n_s *x)
{
  for (int i=0; i<ENV_SIZE; i++)
    {
      t_float f = (t_float)i / (ENV_SIZE -1); // 0 ... 1
      f = (f-0.5) * PI; // -pi/2 ... pi/2
      x->env_t[i]= ((sinf(f)) / 2) + 0.5;
    }
}

////////////////////////////////////////////////////////////////////////////////
// disp
void n_s_calc_xoxr(t_n_s *x, int n)
{
  if (x->disp_w==0) {return;}
  if (x->sample[x->player[n].sample].l < MIN_SAMPLE_LEN) {return;}
  x->player[n].disp.start = x->sample[x->player[n].sample].l * x->player[n].disp.xo;
  x->player[n].disp.len   = x->sample[x->player[n].sample].l * x->player[n].disp.xr;
  if (x->player[n].disp.len<4) 
    x->player[n].disp.len=4;
  x->player[n].disp.end = x->player[n].disp.start + x->player[n].disp.len;
}

void n_s_calc_disp(t_n_s *x, int n)
{
  if (x->disp_w==0) {return;}
  if (x->sample[x->player[n].sample].l < MIN_SAMPLE_LEN) {return;}
  // to disp
  int i, j, x0, x1, k;
  t_float min, max, count, t_part;
  count = x->player[n].disp.start;
  t_part = (t_float)x->player[n].disp.len / (t_float)x->disp_w;

  for (i = 0; i < x->disp_w; i++)
    {
      x0 = k = count;
      count += t_part;
      x1 = count - 1;
      if (k >= x->sample[x->player[n].sample].l)
	k = x->sample[x->player[n].sample].l-1;
      // find min max
      max = min = x->sample[x->player[n].sample].w[k].w_float;
      for (j = x0 + 1; j < x1; j++)
	{
	  k = j;
	  if (k >= x->sample[x->player[n].sample].l)
	    k = x->sample[x->player[n].sample].l-1;
	  if (max < x->sample[x->player[n].sample].w[k].w_float)
	    max = x->sample[x->player[n].sample].w[k].w_float;
	  if (min > x->sample[x->player[n].sample].w[k].w_float)
	    min = x->sample[x->player[n].sample].w[k].w_float;
	}
      max = ((max * 0.5) + 0.5) * x->disp_h;
      min = ((min * 0.5) + 0.5) * x->disp_h;
      x->player[n].disp.y0[i] = max+2;
      x->player[n].disp.y1[i] = min+1;
    }
}

void n_s_calc_se(t_n_s *x, int n)
{
  // start
  if (x->player[n].start_c > x->player[n].disp.start)
    {
      x->player[n].disp.se_start_view = 1;
      t_float l = x->player[n].start_c - x->player[n].disp.start;
      t_float f = l / (t_float)x->player[n].disp.len;
      if (f>1)
	f=1;
      else if (f<0)
	f=0;
      x->player[n].disp.se_start_x = f * x->disp_w + 1;
    }
  else
    {
      x->player[n].disp.se_start_view = 0;
    }

  // end
  if (x->player[n].end_c < x->player[n].disp.end)
    {
      x->player[n].disp.se_end_view = 1;
      t_float l =  x->player[n].disp.end - x->player[n].end_c;
      t_float f = l / (t_float)x->player[n].disp.len;
      if (f>1)
	f=1;
      else if (f<0)
	f=0;
      f = 1-f;
      x->player[n].disp.se_end_x = f * x->disp_w + 1;
    }
  else
    {
      x->player[n].disp.se_end_view = 0;
    }
}

void n_s_calc_loop(t_n_s *x, int n)
{
  // start
  if (x->player[n].l_start_c > x->player[n].disp.start 
      && x->player[n].l_start_c < x->player[n].disp.end
      && x->player[n].loop)
    {
      x->player[n].disp.loop_start_view = 1;
      t_float l = x->player[n].l_start_c - x->player[n].disp.start;
      t_float f = l / (t_float)x->player[n].disp.len;
      if (f>1)
	f=1;
      else if (f<0)
	f=0;
      x->player[n].disp.loop_start_x = f * x->disp_w + 1;
    }
  else
    {
      x->player[n].disp.loop_start_view = 0;
    }

  // end
  if (x->player[n].l_end_c < x->player[n].disp.end
      && x->player[n].l_end_c > x->player[n].disp.start
      && x->player[n].loop)
    {
      x->player[n].disp.loop_end_view = 1;
      t_float l =  x->player[n].disp.end - x->player[n].l_end_c;
      t_float f = l / (t_float)x->player[n].disp.len;
      if (f>1)
	f=1;
      else if (f<0)
	f=0;
      f = 1-f;
      x->player[n].disp.loop_end_x = f * x->disp_w + 1;
    }
  else
    {
      x->player[n].disp.loop_end_view = 0;
    }
}

void n_s_calc_playpos(t_n_s *x, int n)
{
  if (x->player[n].phase_sample > x->player[n].disp.start 
      && x->player[n].phase_sample < x->player[n].disp.end)
    {
      x->player[n].disp.playpos_view = 1;
      t_float l = x->player[n].phase_sample - x->player[n].disp.start;
      t_float f = l / (t_float)x->player[n].disp.len;
      if (f>1)
	f=1;
      else if (f<0)
	f=0;
      x->player[n].disp.playpos_x = f * x->disp_w + 1;
    }
  else
    {
      x->player[n].disp.playpos_view = 0;
    }
}

void n_s_init_disp(t_n_s *x)
{
  t_atom a[9];
  // set color
  SETFLOAT(a,     (t_float)x->color_background);
  SETFLOAT(a + 1, (t_float)x->color_frame);
  SETFLOAT(a + 2, (t_float)x->color_label);
  outlet_anything(x->out_disp, gensym("color"), 3, a);

  // maxel
  x->disp_maxel = x->disp_w + 2 + 2 + 1;
  SETFLOAT(a,     (t_float)x->disp_maxel);
  outlet_anything(x->out_disp, gensym("maxel"), 1, a);

  // size
  SETFLOAT(a,     (t_float)x->disp_w+2);
  SETFLOAT(a + 1, (t_float)x->disp_h+1);
  outlet_anything(x->out_disp, gensym("size"), 2, a);

  // create se
  SETFLOAT(a,     (t_float)3); // rect filled
  SETFLOAT(a + 2, (t_float)x->color_se); // fcolor
  SETFLOAT(a + 3, (t_float)x->color_se); // bcolor
  SETFLOAT(a + 4, (t_float)1); // width
  SETFLOAT(a + 5, (t_float)10); // x0
  SETFLOAT(a + 6, (t_float)10); // y0
  SETFLOAT(a + 7, (t_float)20); // x1
  SETFLOAT(a + 8, (t_float)20); // y1
  x->id_se = 0;
  for (int i=0; i<2; i++)
    {
      SETFLOAT(a + 1, (t_float)x->id_se+i); // id
      outlet_list(x->out_disp, &s_list, 9, a);
    }

  // create sample
  SETFLOAT(a,     (t_float)1); // line
  SETFLOAT(a + 2, (t_float)x->color_foreground); // color
  SETFLOAT(a + 3, (t_float)1); // width
  SETFLOAT(a + 5, (t_float)30); // y0
  SETFLOAT(a + 7, (t_float)40); // y1
   x->id_sample = 2;
  for (int i=0; i<x->disp_w; i++)
    {
      SETFLOAT(a + 1, (t_float)x->id_sample+i); // id
      SETFLOAT(a + 4, (t_float)i+1); // x0
      SETFLOAT(a + 6, (t_float)i+1); // x1
      outlet_list(x->out_disp, &s_list, 8, a);
    }

  // create loop
  SETFLOAT(a,     (t_float)1); // line
  SETFLOAT(a + 2, (t_float)x->color_loop); // color
  SETFLOAT(a + 3, (t_float)1); // width
  SETFLOAT(a + 5, (t_float)1); // y0 
  SETFLOAT(a + 7, (t_float)x->disp_h); // y1
  x->id_loop = x->disp_w + 2;
  for (int i=0; i<2; i++)
    {
      SETFLOAT(a + 1, (t_float)x->id_loop+i); // id
      SETFLOAT(a + 4, (t_float)50+i*40); // x0
      SETFLOAT(a + 6, (t_float)50+i*40); // x1
      outlet_list(x->out_disp, &s_list, 8, a);
    }

  // create playpos
  x->id_playpos = x->disp_w + 2 + 2;
  SETFLOAT(a,     (t_float)1); // line
  SETFLOAT(a + 1, (t_float)x->id_playpos); // id
  SETFLOAT(a + 2, (t_float)x->color_playpos); // color
  SETFLOAT(a + 3, (t_float)1); // width
  SETFLOAT(a + 4, (t_float)60); // x0
  SETFLOAT(a + 5, (t_float)1); // y0
  SETFLOAT(a + 6, (t_float)60); // x1
  SETFLOAT(a + 7, (t_float)x->disp_h); // y1
  outlet_list(x->out_disp, &s_list, 8, a);
}

void n_s_redraw_sample(t_n_s *x, int n)
{
  if (x->disp_w==0) {return;}
  if (x->sel_player!=n) {return;}
  t_atom a[6];
  SETFLOAT(a,     (t_float)9); // move
  for (int i=0; i<x->disp_w; i++)
    {
      int cx = i+1;
      SETFLOAT(a + 1, (t_float)x->id_sample+i); // id
      SETFLOAT(a + 2, (t_float)cx); // x0
      SETFLOAT(a + 3, (t_float)x->player[n].disp.y0[i]); // y0
      SETFLOAT(a + 4, (t_float)cx); // x1
      SETFLOAT(a + 5, (t_float)x->player[n].disp.y1[i]); // y1
      outlet_list(x->out_disp, &s_list, 6, a);
    }
}

void n_s_redraw_playpos(t_n_s *x, int n)
{
  if (x->disp_w==0) {return;}
  if (x->sel_player!=n) {return;}
  t_atom a[6];
  SETFLOAT(a,     (t_float)9); // move
  SETFLOAT(a + 1, (t_float)x->id_playpos); // id
  if (x->player[n].disp.playpos_view)
    {
      SETFLOAT(a + 2, (t_float)x->player[n].disp.playpos_x); // x0
      SETFLOAT(a + 3, (t_float)1); // y0
      SETFLOAT(a + 4, (t_float)x->player[n].disp.playpos_x); // x1
      SETFLOAT(a + 5, (t_float)x->disp_h); // y1
    }
  else
    {
      SETFLOAT(a + 2, (t_float)1); // x0
      SETFLOAT(a + 3, (t_float)1); // y0
      SETFLOAT(a + 4, (t_float)1); // x1
      SETFLOAT(a + 5, (t_float)1); // y1
    }
  outlet_list(x->out_disp, &s_list, 6, a);
}

void n_s_redraw_se(t_n_s *x, int n)
{
  if (x->disp_w==0) {return;}
  if (x->sel_player!=n) {return;}
  t_atom a[6];
  SETFLOAT(a,     (t_float)9); // move
  if (x->player[n].disp.se_start_view)
    {
      SETFLOAT(a + 1, (t_float)x->id_se); // id
      SETFLOAT(a + 2, (t_float)1); // x0
      SETFLOAT(a + 3, (t_float)1); // y0
      SETFLOAT(a + 4, (t_float)x->player[n].disp.se_start_x); // x1
      SETFLOAT(a + 5, (t_float)x->disp_h); // y1
      outlet_list(x->out_disp, &s_list, 6, a);
    }
  else
    {
      SETFLOAT(a + 1, (t_float)x->id_se); // id
      SETFLOAT(a + 2, (t_float)1); // x0
      SETFLOAT(a + 3, (t_float)1); // y0
      SETFLOAT(a + 4, (t_float)1); // x1
      SETFLOAT(a + 5, (t_float)1); // y1
      outlet_list(x->out_disp, &s_list, 6, a);
    }
  if (x->player[n].disp.se_end_view)
    {
      SETFLOAT(a + 1, (t_float)x->id_se+1); // id
      SETFLOAT(a + 2, (t_float)x->disp_w+1); // x0
      SETFLOAT(a + 3, (t_float)1); // y0
      SETFLOAT(a + 4, (t_float)x->player[n].disp.se_end_x); // x1
      SETFLOAT(a + 5, (t_float)x->disp_h); // y1
      outlet_list(x->out_disp, &s_list, 6, a);
    }
  else
    {
      SETFLOAT(a + 1, (t_float)x->id_se+1); // id
      SETFLOAT(a + 2, (t_float)1); // x0
      SETFLOAT(a + 3, (t_float)1); // y0
      SETFLOAT(a + 4, (t_float)1); // x1
      SETFLOAT(a + 5, (t_float)1); // y1
      outlet_list(x->out_disp, &s_list, 6, a);
    }
}

void n_s_redraw_loop(t_n_s *x, int n)
{
  if (x->disp_w==0) {return;}
  if (x->sel_player!=n) {return;}
  t_atom a[6];
  SETFLOAT(a,     (t_float)9); // move
  if (x->player[n].disp.loop_start_view)
    {
      SETFLOAT(a + 1, (t_float)x->id_loop); // id
      SETFLOAT(a + 2, (t_float)x->player[n].disp.loop_start_x); // x0
      SETFLOAT(a + 3, (t_float)1); // y0
      SETFLOAT(a + 4, (t_float)x->player[n].disp.loop_start_x); // x1
      SETFLOAT(a + 5, (t_float)x->disp_h); // y1
      outlet_list(x->out_disp, &s_list, 6, a);
    }
  else
    {
      SETFLOAT(a + 1, (t_float)x->id_loop); // id
      SETFLOAT(a + 2, (t_float)1); // x0
      SETFLOAT(a + 3, (t_float)1); // y0
      SETFLOAT(a + 4, (t_float)1); // x1
      SETFLOAT(a + 5, (t_float)1); // y1
      outlet_list(x->out_disp, &s_list, 6, a);
    }
  if (x->player[n].disp.loop_end_view)
    {
      SETFLOAT(a + 1, (t_float)x->id_loop+1); // id
      SETFLOAT(a + 2, (t_float)x->player[n].disp.loop_end_x); // x0
      SETFLOAT(a + 3, (t_float)1); // y0
      SETFLOAT(a + 4, (t_float)x->player[n].disp.loop_end_x); // x1
      SETFLOAT(a + 5, (t_float)x->disp_h); // y1
      outlet_list(x->out_disp, &s_list, 6, a);
    }
  else
    {
      SETFLOAT(a + 1, (t_float)x->id_loop+1); // id
      SETFLOAT(a + 2, (t_float)1); // x0
      SETFLOAT(a + 3, (t_float)1); // y0
      SETFLOAT(a + 4, (t_float)1); // x1
      SETFLOAT(a + 5, (t_float)1); // y1
      outlet_list(x->out_disp, &s_list, 6, a);
    }
}

void n_s_redraw_disp(t_n_s *x, int n)
{
  if (x->disp_w==0) {return;}
  if (x->sel_player!=n) {return;}
  n_s_redraw_sample(x, n);
  n_s_redraw_playpos(x, n);
  n_s_redraw_se(x, n);
  n_s_redraw_loop(x, n);
}

////////////////////////////////////////////////////////////////////////////////
// link turn
void n_s_turn_remove(t_n_s *x, int n, int t)
{
  int i,j;
  for (i=0, j=0; i<MAX_PLAYER; i++)
    {
      if (x->lt[t].t[i] != n)
	{
	  x->lt[t].t[j] = x->lt[t].t[i];
	  j++;
	}
    }
  x->lt[t].l = j;
}

void n_s_turn_add(t_n_s *x, int n, int t)
{
  x->lt[t].t[x->lt[t].l] = n;
  x->lt[t].l++;
}

////////////////////////////////////////////////////////////////////////////////
// input
void n_s_array(t_n_s *x, t_floatarg n, t_symbol *s, t_floatarg sr)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLE_1, i);
  x->sample[i].s = s;
  x->sample[i].l = pd_open_array(x->sample[i].s, &x->sample[i].w, &x->sample[i].g);
  x->sample[i].l_1 = x->sample[i].l - 1;
  if (x->sample[i].l <= MIN_SAMPLE_LEN)
    {
      post("n_sampler: error: bad sample array: %s %d", 
	   x->sample[i].s->s_name,
	   x->sample[i].l);
    }
  if (sr < MIN_SAMPLE_RATE)
    {
      post("n_sampler: warning: bad sample rate: %g", sr);
      sr = MIN_SAMPLE_RATE;
    }
  x->sample[i].sr=sr;
  n_s_calc_start_end(x, i);
  n_s_calc_l_start_end(x, i);
  n_s_calc_sdel(x, i);
  n_s_calc_xoxr(x, i);
  n_s_calc_se(x, i);
  n_s_calc_loop(x, i);
  n_s_calc_disp(x, i);
  n_s_reset(x);
  n_s_redraw_disp(x, i);
}

void n_s_sample(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  _clip_minmax(0, MAX_SAMPLE_1, f);
  x->player[i].sample=f;
  n_s_calc_start_end(x, i);
  n_s_calc_l_start_end(x, i);
  n_s_calc_sdel(x, i);
  n_s_calc_xoxr(x, i);
  n_s_calc_se(x, i);
  n_s_calc_loop(x, i);
  n_s_calc_disp(x, i);
  n_s_reset(x);
  n_s_redraw_disp(x, i);
}

void n_s_level(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  _clip_min(0, f);
  x->player[i].level=f;
  n_s_calc_level(x, i);
  n_s_calc_level_s1(x, i);
  n_s_calc_level_s2(x, i);
}

void n_s_pan(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  _clip_minmax(-1, 1, f);
  x->player[i].pan=f;
  n_s_calc_level(x, i);
}

void n_s_s1(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  _clip_min(0, f);
  x->player[i].s1=f;
  n_s_calc_level_s1(x, i);
}

void n_s_s2(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  _clip_min(0, f);
  x->player[i].s2=f;
  n_s_calc_level_s2(x, i);
}

void n_s_rnda(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  f=f*0.01; // 1/10000
  _clip_min(0, f);
  x->player[i].rnda=f;
}

void n_s_group(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  _clip_minmax(0, 3, f);
  x->player[i].group=f;
}

void n_s_link(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  int t = f;
  _clip_minmax(0, MAX_LINKS_1, t);
  if (x->player[i].link != 0)
    {
      n_s_turn_remove(x, i, t);
    }
  if (t != 0)
    {
      n_s_turn_add(x, i, t);
    }
  x->player[i].link=t;
}

void n_s_bit(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  _clip_minmax(2, 16, f);
  f-=1;
  x->player[i].bit_pre = pow(2., f);
  x->player[i].bit_post = 1. / x->player[i].bit_pre;
  x->player[i].bit_ofs = (x->player[i].bit_post * 0.5) - 1.;
}

void n_s_drive(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  _clip_minmax(0, 1, f);
  x->player[i].drive = f;
}

void n_s_start(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  _clip_minmax(0, 1, f);
  x->player[i].start=f;
  n_s_calc_start_end(x, i);
  n_s_calc_sdel(x, i);
  n_s_calc_se(x, i);
  n_s_redraw_se(x, i);
}

void n_s_end(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  _clip_minmax(0, 1, f);
  x->player[i].end=f;
  n_s_calc_start_end(x, i);
  n_s_calc_se(x, i);
  n_s_redraw_se(x, i);
}

void n_s_l_start(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  _clip_minmax(0, 1, f);
  x->player[i].l_start=f;
  n_s_calc_l_start_end(x, i);
  n_s_calc_loop(x, i);
  n_s_redraw_loop(x, i);
}

void n_s_l_end(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  _clip_minmax(0, 1, f);
  x->player[i].l_end=f;
  n_s_calc_l_start_end(x, i);
  n_s_calc_loop(x, i);
  n_s_redraw_loop(x, i);
}

void n_s_sdel(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  _clip_minmax(0, 1, f);
  x->player[i].sdel=f;
  n_s_calc_sdel(x, i);
}

void n_s_semi(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  x->player[i].semi=f;
  n_s_calc_tune(x, i);
}

void n_s_fine(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  x->player[i].fine=f*0.01;
  n_s_calc_tune(x, i);
}

void n_s_xf(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  _clip_minmax(0, 1, f);
  x->player[i].xf=f;
  n_s_calc_l_start_end(x, i);
}

void n_s_loop(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  x->player[i].loop=(f>0);
  x->player[i].on=0;
  n_s_calc_loop(x, i);
  n_s_redraw_loop(x, i);
}

void n_s_att(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  _clip_minmax(0, 100, f);
  x->player[i].att = pow(1.122, f-60);
  n_s_calc_att(x, i);
}

void n_s_dec(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  _clip_minmax(0, 100, f);
  x->player[i].dec = pow(1.122, f-60);
  n_s_calc_dec(x, i);
}

void n_s_sus(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  _clip_minmax(0, 100, f);
  x->player[i].sus = f*0.01;
}

void n_s_rel(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  _clip_minmax(0, 100, f);
  x->player[i].rel = pow(1.122, f-60);
  n_s_calc_rel(x, i);
}

void n_s_eg(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  x->player[i].eg=(f>0);
}

void n_s_freq(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  _clip_minmax(0, 120, f);
  x->player[i].freq = 8.1758 * pow(2, f/12); // ptof
  n_s_calc_freq(x, i);
}

void n_s_res(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  _clip_minmax(0, 1, f);
  f=(f*0.88)+0.1;
  f=1-f;
  x->player[i].res=f+f;
}

void n_s_filtype(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  x->player[i].filtype=f;
}

void n_s_l_f(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  _clip_minmax(0, 120, f);
  x->player[i].l_f = 8.1758 * pow(2, f/12); // ptof
  n_s_calc_l_f(x, i);
}

void n_s_l_g(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  _clip_minmax(-1, 1, f);
  if (f>0) f*=4;
  x->player[i].l_g = f;
}

void n_s_h_f(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  _clip_minmax(0, 120, f);
  x->player[i].h_f = 8.1758 * pow(2, f/12); // ptof
  n_s_calc_h_f(x, i);
}

void n_s_h_g(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  _clip_minmax(-1, 1, f);
  if (f>0) f*=4;
  x->player[i].h_g = f;
}

void n_s_xr(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  _clip_minmax(0, 1, f);
  x->player[i].disp.xr = f;
  n_s_calc_xoxr(x, i);
  n_s_calc_se(x, i);
  n_s_calc_loop(x, i);
  n_s_calc_disp(x, i);
  n_s_redraw_disp(x, i);
}

void n_s_xo(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  _clip_minmax(0, 1, f);
  x->player[i].disp.xo = f;
  n_s_calc_xoxr(x, i);
  n_s_calc_se(x, i);
  n_s_calc_loop(x, i);
  n_s_calc_disp(x, i);
  n_s_redraw_disp(x, i);
}

void n_s_mute(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  x->player[i].mute=(f>0);
  n_s_calc_ms(x);
}

void n_s_solo(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  x->player[i].solo=(f>0);
  n_s_calc_ms(x);
}

void n_s_sel_sampler(t_n_s *x, t_floatarg n)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  x->sel_player=i;
  n_s_redraw_disp(x, i);
}

void n_s_disp(t_n_s *x, t_floatarg w, t_floatarg h)
{
  _clip_minmax(MIN_DISP_W, MAX_DISP_W, w);
  _clip_minmax(MIN_DISP_H, MAX_DISP_H, h);
  x->disp_w=w;
  x->disp_h=h;
  for (int i=0; i<MAX_PLAYER; i++)
    {
      n_s_calc_xoxr(x, i);
      n_s_calc_se(x, i);
      n_s_calc_loop(x, i);
      n_s_calc_disp(x, i);
    }
  n_s_init_disp(x);
  n_s_redraw_disp(x, x->sel_player);
}

void n_s_color_background(t_n_s *x, t_floatarg c)
{
  x->color_background = c;
  n_s_redraw_disp(x, x->sel_player);
}

void n_s_color_foreground(t_n_s *x, t_floatarg c)
{
  x->color_foreground = c;
  n_s_redraw_disp(x, x->sel_player);
}

void n_s_color_playpos(t_n_s *x, t_floatarg c)
{
  x->color_playpos = c;
  n_s_redraw_disp(x, x->sel_player);
}

void n_s_color_se(t_n_s *x, t_floatarg c)
{
  x->color_se = c;
  n_s_redraw_disp(x, x->sel_player);
}

void n_s_color_loop(t_n_s *x, t_floatarg c)
{
  x->color_loop = c;
  n_s_redraw_disp(x, x->sel_player);
}

void n_s_g(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_PLAYER_1, i);
  if (f > 0 && x->player[i].ms)
    {
      // mute links
      if (x->player[i].link != 0)
	{
	  for (int k=0; k<x->lt[x->player[i].link].l; k++)
	    {
	      x->player[k].on = 0;
	    }
	}
      x->player[i].on = 1;
      x->player[i].stage = STG_START;
      x->player[i].phase_sample = x->player[i].start_c;
      x->player[i].env_stage = ENV_ATT;
      x->player[i].vel = f;
    }
  else
    {
      if (x->player[i].loop)
	{
	  x->player[i].stage = STG_END;
	}
      x->player[i].env_stage = ENV_REL;
    }
}

////////////////////////////////////////////////////////////////////////////////
// dsp
#define S x->player[s]
#define A x->sample[x->player[s].sample]
t_int *n_s_perform(t_int *w)
{
  t_float *out_l[4], *out_r[4], *out_s1, *out_s2;
  t_n_s *x = (t_n_s *)(w[1]);
  out_l[0] = (t_float *)(w[2]);
  out_r[0] = (t_float *)(w[3]);
  out_l[1] = (t_float *)(w[4]);
  out_r[1] = (t_float *)(w[5]);
  out_l[2] = (t_float *)(w[6]);
  out_r[2] = (t_float *)(w[7]);
  out_l[3] = (t_float *)(w[8]);
  out_r[3] = (t_float *)(w[9]);
  out_s1 = (t_float *)(w[10]);
  out_s2 = (t_float *)(w[11]);
  int n, bs = (int)(w[12]);
  t_float out, f, e;
  int j;
  // clear
  for (n=0; n<bs; n++)
    {
      out_l[0][n]=0;
      out_r[0][n]=0;
      out_l[1][n]=0;
      out_r[1][n]=0;
      out_l[2][n]=0;
      out_r[2][n]=0;
      out_l[3][n]=0;
      out_r[3][n]=0;
      out_s1[n]=0;
      out_s2[n]=0;
    }
  for (int s=0; s<MAX_PLAYER; s++)
    {
      if (S.on)
	{
	  for (n=0; n<bs; n++)
	    {
	      // player ///////////////////////////////////////////////////////
	      if (S.stage==STG_START)
		{
		  if (S.phase_sample >= A.l_1)
		    {
		      S.phase_sample = A.l_1;
		    }
		  // read
		  out = A.w[(int)S.phase_sample].w_float;
		  // phase next
		  S.phase_sample += 1;
		  if (S.phase_sample >= S.sdel_c)
		    {
		      if (S.loop)
			{
			  S.stage = STG_PRE_LOOP;
			}
		      else
			{
			  S.stage = STG_AFTER;
			}
		    }
		}
	      else if (S.stage==STG_AFTER)
		{
		  // phase
		  if (S.phase_sample >= S.end_c)
		    {
		      S.phase_sample = S.end_c;
		      S.on=0;
		    }
		  // read
		  out = A.w[(int)S.phase_sample].w_float;
		  // phase next
		  S.phase_sample += S.sp;
		}
	      else if (S.stage==STG_PRE_LOOP)
		{
		  // phase
		  if (S.phase_sample >= S.l_end_c)
		    {
		      S.phase_sample = S.l_start_c;
		    }
		  if (S.phase_sample >= S.l_center_c)
		    {
		      S.stage = STG_LOOP;
		    }
		  // read
		  out = A.w[(int)S.phase_sample].w_float;
		  // phase next
		  S.phase_sample += S.sp;
		}
	      else if (S.stage==STG_LOOP)
		{
		  t_float phase2, s1, s2, env;
		  // phase
		  if (S.phase_sample >= S.l_end_c)
		    {
		      S.phase_sample = S.l_start_c;
		    }
		  // phase 2
		  phase2 = S.phase_sample + S.l_add;
		  while(phase2 > S.l_end_c)
		    phase2 -= S.l_len;
		  // env
		  env = (S.phase_sample - S.l_start_c) / S.l_len; // 0 ... 1
		  env = env+env-1; // -1 ... 1
		  env = 1 - fabs(env); // tri
		  env = env * S.l_m; //
		  _clip_minmax(0, 1, env);
		  env = env * ENV_SIZE_1;
		  j = env;
		  env = env-j;
		  env = (x->env_t[j+1]-x->env_t[j]) * env + x->env_t[j]; // 0...1
		  // read
		  s1 = A.w[(int)S.phase_sample].w_float;
		  s2 = A.w[(int)phase2].w_float;
		  out = (s1-s2) * env + s2;
		  // phase next
		  S.phase_sample += S.sp;
		}
	      else // STG_END
		{
		  // phase
		  if (S.phase_sample >= S.end_c)
		    {
		      S.phase_sample = S.end_c;
		      S.on=0;
		    }
		  // read
		  out = A.w[(int)S.phase_sample].w_float;
		  // phase sample
		  S.phase_sample += S.sp;
		}
	      // fx ////////////////////////////////////////////////////////////
	      j = (out+1) * S.bit_pre;
	      out = (j * S.bit_post) + S.bit_ofs;
	      // random ////////////////////////////////////////////////////////
	      if (out >= 0 && S.out < 0)
		{
		  x->seed *= 1103515245;
		  x->seed += 12345;
		  f = (t_float)x->seed * 0.000000000464;
		  S.rndz = (f * S.rnda) + 1;
		}
	      out *= S.rndz;
	      // drive /////////////////////////////////////////////////////////
	      e = out*4;
	      _tanh_pade(e, f);
	      _mix(out, f, S.drive, out);
	      // filter ////////////////////////////////////////////////////////
	      if (S.filtype>0)
		{
		  S.film[3] = out - (S.film[2] * S.res) - S.film[1];
		  S.film[2] = S.film[2] + (S.film[3] * S.filw);
		  S.film[1] = S.film[1] + (S.film[2] * S.filw);
		  out = S.film[S.filtype];
		}
	      // eq ////////////////////////////////////////////////////////////
	      S.lfz = ((out-S.lfz) * S.lfw) + S.lfz; // ls
	      out = out + (S.lfz * S.l_g);
	      S.hfz = ((out-S.hfz) * S.hfw) + S.hfz; // hs
	      f = out - S.hfz;
	      out = out + (f * S.h_g);
	      // vca ///////////////////////////////////////////////////////////
	      if (S.eg)
		{
		  if (S.env_stage==ENV_ATT)
		    {
		      S.env += S.att_c;
		      if (S.env>1)
			{
			  S.env=1;
			  S.env_stage=ENV_DEC;
			}
		    }
		  else if (S.env_stage==ENV_DEC)
		    {
		      S.env -= S.dec_c * S.env;
		      if (S.env<S.sus)
			{
			  S.env=S.sus;
			  S.env_stage=ENV_SUS;
			}
		    }
		  else if (S.env_stage==ENV_REL)
		    {
		      S.env -= S.rel_c * S.env;
		      if (S.env<ENV_MINA)
			{
			  S.env=ENV_MINA;
			  S.on=0;
			}
		    }
		  out = out * S.vel * S.env;
		}
	      else
		{
		  out = out * S.vel;
		}
	      // out ///////////////////////////////////////////////////////////
	      out_l[S.group][n] += out * S.level_l;
	      out_r[S.group][n] += out * S.level_r;
	      out_s1[n] += out * S.level_s1;
	      out_s2[n] += out * S.level_s2;
	      S.out=out;
	    }
	}
    }
  x->mcpp_count+=bs;
  if (x->mcpp_count>x->mcpp)
    {
      x->mcpp_count=0;
      n_s_calc_playpos(x, x->sel_player);
      n_s_redraw_playpos(x, x->sel_player);
    }
  return (w + 13);
}

void n_s_dsp(t_n_s *x, t_signal **sp)
{
  if (x->sr != sp[0]->s_sr)
    {
      x->sr = sp[0]->s_sr;
      n_s_calc_constant(x);
    }
  dsp_add(n_s_perform,
          12,
          x,
          sp[0]->s_vec,
          sp[1]->s_vec,
          sp[2]->s_vec,
          sp[3]->s_vec,
          sp[4]->s_vec,
          sp[5]->s_vec,
          sp[6]->s_vec,
          sp[7]->s_vec,
          sp[8]->s_vec,
          sp[9]->s_vec,
          sp[0]->s_n);
}

////////////////////////////////////////////////////////////////////////////////
// setup
void *n_s_new()
{
  t_n_s *x = (t_n_s *)pd_new(n_s_class);
  x->seed = (long)x;
  x->sr = 44100;
  x->color_frame = 22;
  x->color_label = 22;
  n_s_init(x);
  n_s_calc_constant(x);
  n_s_calc_env_t(x);
  outlet_new(&x->x_obj, &s_signal); // group 0
  outlet_new(&x->x_obj, &s_signal);
  outlet_new(&x->x_obj, &s_signal); // group 1
  outlet_new(&x->x_obj, &s_signal);
  outlet_new(&x->x_obj, &s_signal); // group 2
  outlet_new(&x->x_obj, &s_signal);
  outlet_new(&x->x_obj, &s_signal); // group 3
  outlet_new(&x->x_obj, &s_signal);
  outlet_new(&x->x_obj, &s_signal); // s1
  outlet_new(&x->x_obj, &s_signal); // s2
  x->out_disp = outlet_new(&x->x_obj, 0);
  return (void *)x;
}

void n_sampler_tilde_setup(void)
{
  n_s_class = class_new(gensym("n_sampler~"),
			(t_newmethod)n_s_new,
			0,
			sizeof(t_n_s),
			0, 0, 0);
  class_addmethod(n_s_class,(t_method)n_s_dsp,gensym("dsp"),0);

  class_addmethod(n_s_class,(t_method)n_s_array,gensym("array"),A_FLOAT,A_SYMBOL,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_sample,gensym("sample"),A_FLOAT,A_FLOAT,0);

  class_addmethod(n_s_class,(t_method)n_s_level,gensym("level"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_pan,gensym("pan"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_s1,gensym("s1"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_s2,gensym("s2"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_rnda,gensym("rnda"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_group,gensym("group"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_link,gensym("link"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_bit,gensym("bit"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_drive,gensym("drive"),A_FLOAT,A_FLOAT,0);

  class_addmethod(n_s_class,(t_method)n_s_start,gensym("start"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_end,gensym("end"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_l_start,gensym("l-st"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_l_end,gensym("l-end"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_sdel,gensym("sdel"),A_FLOAT,A_FLOAT,0);

  class_addmethod(n_s_class,(t_method)n_s_semi,gensym("semi"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_fine,gensym("fine"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_xf,gensym("xf"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_loop,gensym("loop"),A_FLOAT,A_FLOAT,0);

  class_addmethod(n_s_class,(t_method)n_s_att,gensym("att"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_dec,gensym("dec"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_sus,gensym("sus"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_rel,gensym("rel"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_eg,gensym("eg"),A_FLOAT,A_FLOAT,0);

  class_addmethod(n_s_class,(t_method)n_s_freq,gensym("freq"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_res,gensym("res"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_filtype,gensym("filtype"),A_FLOAT,A_FLOAT,0);

  class_addmethod(n_s_class,(t_method)n_s_l_f,gensym("l-f"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_l_g,gensym("l-g"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_h_f,gensym("h-f"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_h_g,gensym("h-g"),A_FLOAT,A_FLOAT,0);

  class_addmethod(n_s_class,(t_method)n_s_xr,gensym("xr"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_xo,gensym("xo"),A_FLOAT,A_FLOAT,0);

  class_addmethod(n_s_class,(t_method)n_s_mute,gensym("mute"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_solo,gensym("solo"),A_FLOAT,A_FLOAT,0);

  class_addmethod(n_s_class,(t_method)n_s_sel_sampler,gensym("sel_sampler"),A_FLOAT,0);

  class_addmethod(n_s_class,(t_method)n_s_disp,gensym("disp"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_color_background,
		  gensym("color_background"),A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_color_foreground,
		  gensym("color_foreground"),A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_color_playpos,
		  gensym("color_playpos"),A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_color_se,
		  gensym("color_se"),A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_color_loop,
		  gensym("color_loop"),A_FLOAT,0);

  class_addmethod(n_s_class,(t_method)n_s_g,gensym("g"),A_FLOAT,A_FLOAT,0);
}
