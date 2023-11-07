////////////////////////////////////////////////////////////////////////////////
//  Sampler                                                                   //
////////////////////////////////////////////////////////////////////////////////

#include <math.h>
#include "m_pd.h"
#include "include/pdfunc.h"



#define PI           3.1415927
#define TWOPI        6.2831853

#define MAX_SAMPLER 48
#define MAX_SAMPLER_1 47
#define MAX_SAMPLE 31
#define MAX_SAMPLE_1 30
#define MIN_SAMPLE_LEN 4
#define MIN_DISP_LEN 16
#define MIN_SAMPLE_RATE 1000
#define SAMPLE_END_OFS 4
#define DEF_TIME_EG 0.0001
#define ENV_SIZE 512
#define ENV_SIZE_1 510

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
  t_symbol *s; //in
  t_word   *w;
  t_garray *g;
  int       l;
  t_float   xr; //in
  t_float   xo; //in
} t_n_disp;

typedef struct _n_sampler
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
  int stage;
  t_float phase_sample;
  t_float out;
  int mute; //in
  int solo; //in
  int ms;
} t_n_sampler;


typedef struct _n_s
{
  t_object x_obj;
  t_float sr;
  t_float div_1_sr;
  t_float div_sr_8;
  t_float div_2pi_sr;
  int seed; /* random */
  t_n_sampler sampler[MAX_SAMPLER];
  t_n_disp disp[MAX_SAMPLER];
  t_n_sample sample[MAX_SAMPLE];
  t_float env_t[ENV_SIZE];
} t_n_s;

////////////////////////////////////////////////////////////////////////////////
// calc
void n_s_init(t_n_s *x)
{
  for (int i=0; i<MAX_SAMPLER; i++)
    {
      x->sampler[i].on=0;
      x->sampler[i].sample=0;
      x->sampler[i].att=DEF_TIME_EG;
      x->sampler[i].dec=DEF_TIME_EG;
      x->sampler[i].rel=DEF_TIME_EG;
      x->sampler[i].rndz=1;
      x->sampler[i].mute=0;
      x->sampler[i].solo=0;
    }
}

void n_s_calc_level(t_n_s *x, int n)
{
  t_float l = 1 - x->sampler[n].pan; 
  t_float r = 1 + x->sampler[n].pan;
  t_float s = 0.33333 * x->sampler[n].level;
  x->sampler[n].level_l = s * (4-l) * l;
  x->sampler[n].level_r = s * (4-r) * r;
}

void n_s_calc_level_s1(t_n_s *x, int n)
{
  x->sampler[n].level_s1 = x->sampler[n].level * x->sampler[n].s1;
}

void n_s_calc_level_s2(t_n_s *x, int n)
{
  x->sampler[n].level_s2 = x->sampler[n].level * x->sampler[n].s2;
}

void n_s_calc_start_end(t_n_s *x, int n)
{
  x->sampler[n].start_c = x->sampler[n].start * x->sample[x->sampler[n].sample].l_1;
  x->sampler[n].end_c = x->sampler[n].end * x->sample[x->sampler[n].sample].l_1;
  if (x->sampler[n].end_c < x->sampler[n].start_c)
    x->sampler[n].end_c = x->sampler[n].start_c;
}

void n_s_calc_l_start_end(t_n_s *x, int n)
{
  x->sampler[n].l_start_c = x->sampler[n].l_start * x->sample[x->sampler[n].sample].l_1;
  x->sampler[n].l_end_c = x->sampler[n].l_end * x->sample[x->sampler[n].sample].l_1;
  if (x->sampler[n].l_end_c < x->sampler[n].l_start_c)
    x->sampler[n].l_end_c = x->sampler[n].l_start_c;
  t_float len = x->sampler[n].l_end_c - x->sampler[n].l_start_c;
  x->sampler[n].l_center_c = x->sampler[n].l_start_c + (len/2.0);
  x->sampler[n].l_add = len/2.0;
  x->sampler[n].l_len = len;
  x->sampler[n].l_m = 1.0 / ((x->sampler[n].xf*0.999)+0.001);
}

void n_s_calc_sdel(t_n_s *x, int n)
{
  x->sampler[n].sdel_c = (x->sampler[n].sdel * x->sample[x->sampler[n].sample].l_1)
    + x->sampler[n].start;
  if (x->sampler[n].sdel_c > x->sample[x->sampler[n].sample].l_1)
    x->sampler[n].sdel_c = x->sample[x->sampler[n].sample].l_1;
}

void n_s_calc_tune(t_n_s *x, int n)
{
  t_float f = x->sampler[n].semi + x->sampler[n].fine;
  x->sampler[n].sp = pow(2, f/12);
}

void n_s_calc_att(t_n_s *x, int n)
{
  x->sampler[n].att_c = x->div_1_sr / x->sampler[n].att;
}

void n_s_calc_dec(t_n_s *x, int n)
{
  x->sampler[n].dec_c = x->div_1_sr / x->sampler[n].dec;
}

void n_s_calc_rel(t_n_s *x, int n)
{
  x->sampler[n].rel_c = x->div_1_sr / x->sampler[n].rel;
}

void n_s_calc_freq(t_n_s *x, int n)
{
  t_float f = x->sampler[n].freq;
  _clip_minmax(10, x->div_sr_8, f);
  x->sampler[n].filw = f * x->div_2pi_sr;
}

void n_s_calc_l_f(t_n_s *x, int n)
{
  t_float f = x->sampler[n].l_f;
  _clip_minmax(10, x->div_sr_8, f);
  x->sampler[n].lfw = f * x->div_2pi_sr;
}

void n_s_calc_h_f(t_n_s *x, int n)
{
  t_float f = x->sampler[n].h_f;
  _clip_minmax(10, x->div_sr_8, f);
  x->sampler[n].hfw = f * x->div_2pi_sr;
}

void n_s_calc_ms(t_n_s *x)
{
  int solo=0;
  for (int i=0; i<MAX_SAMPLER; i++)
    {
      if (x->sampler[i].solo)
	{
	  solo=1;
	  break;
	}
    }
  for (int i=0; i<MAX_SAMPLER; i++)
    {
      x->sampler[i].ms = (x->sampler[i].solo == solo) && (!x->sampler[i].mute);
    }
}


void n_s_calc_constant(t_n_s *x)
{
  x->div_1_sr = 1.0 / x->sr;
  x->div_sr_8 = x->sr / 8.0;
  x->div_2pi_sr = TWOPI / x->sr;
  for(int i=0; i<MAX_SAMPLER; i++)
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

void n_s_calc_disp(t_n_s *x, int n)
{
  if (x->sample[x->sampler[n].sample].l < MIN_SAMPLE_LEN) {return;}
  if (x->disp[n].l < MIN_DISP_LEN) {return;}
  if (x->disp[n].l%2 == 1) {return;}
  // calc xr xo
  int l = x->sample[x->sampler[n].sample].l - MIN_SAMPLE_LEN;
  int start = l * x->disp[n].xo;
  int len = x->sample[x->sampler[n].sample].l * x->disp[n].xr;
  if (len<4)
    len=4;
  int end = start + len;
  if (end > x->sample[x->sampler[n].sample].l)
    end = x->sample[x->sampler[n].sample].l;
  // to disp
  int i, j, x0, x1, k;
  t_float min, max;
  t_float count;
  t_float t_part;
  int disp_half = x->disp[n].l / 2;
  count = start;
  t_part = (t_float)len / (t_float)disp_half;

  for (i = 0; i < disp_half; i++)
    {
      x0 = k = count;
      count += t_part;
      x1 = count - 1;
      if (k >= l)
	k = l-1;
      // find min max
      max = min = x->sample[x->sampler[n].sample].w[k].w_float;
      for (j = x0 + 1; j < x1; j++)
	{
	  k = j;
	  if (k >= l)
	    k = l-1;
	  if (max < x->sample[x->sampler[n].sample].w[k].w_float)
	    max = x->sample[x->sampler[n].sample].w[k].w_float;
	  if (min > x->sample[x->sampler[n].sample].w[k].w_float)
	    min = x->sample[x->sampler[n].sample].w[k].w_float;
	}
      x->disp[n].w[i].w_float = max;
      x->disp[n].w[i+disp_half].w_float = min;
    }
  garray_redraw(x->disp[n].g);
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
  n_s_calc_disp(x, i);
}

void n_s_sample(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  _clip_minmax(0, MAX_SAMPLE_1, f);
  x->sampler[i].sample=f;
  n_s_calc_start_end(x, i);
  n_s_calc_l_start_end(x, i);
  n_s_calc_sdel(x, i);
}

void n_s_level(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  _clip_min(0, f);
  x->sampler[i].level=f;
  n_s_calc_level(x, i);
  n_s_calc_level_s1(x, i);
  n_s_calc_level_s2(x, i);
}

void n_s_pan(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  _clip_minmax(-1, 1, f);
  x->sampler[i].pan=f;
  n_s_calc_level(x, i);
}

void n_s_s1(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  _clip_min(0, f);
  x->sampler[i].s1=f;
  n_s_calc_level_s1(x, i);
}

void n_s_s2(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  _clip_min(0, f);
  x->sampler[i].s2=f;
  n_s_calc_level_s2(x, i);
}

void n_s_rnda(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  f=f*0.01; // 1/10000
  _clip_min(0, f);
  x->sampler[i].rnda=f;
}

void n_s_group(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  _clip_minmax(0, 3, f);
  x->sampler[i].group=f;
}

void n_s_bit(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  _clip_minmax(2, 16, f);
  f-=1;
  x->sampler[i].bit_pre = pow(2., f);
  x->sampler[i].bit_post = 1. / x->sampler[i].bit_pre;
  x->sampler[i].bit_ofs = (x->sampler[i].bit_post * 0.5) - 1.;
}

void n_s_start(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  _clip_minmax(0, 1, f);
  x->sampler[i].start=f;
  n_s_calc_start_end(x, i);
  n_s_calc_sdel(x, i);
}

void n_s_end(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  _clip_minmax(0, 1, f);
  x->sampler[i].end=f;
  n_s_calc_start_end(x, i);
}

void n_s_l_start(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  _clip_minmax(0, 1, f);
  x->sampler[i].l_start=f;
  n_s_calc_l_start_end(x, i);
}

void n_s_l_end(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  _clip_minmax(0, 1, f);
  x->sampler[i].l_end=f;
  n_s_calc_l_start_end(x, i);
}

void n_s_sdel(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  _clip_minmax(0, 1, f);
  x->sampler[i].sdel=f;
  n_s_calc_sdel(x, i);
}

void n_s_semi(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  x->sampler[i].semi=f;
  n_s_calc_tune(x, i);
}

void n_s_fine(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  x->sampler[i].fine=f*0.01;
  n_s_calc_tune(x, i);
}

void n_s_xf(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  _clip_minmax(0, 1, f);
  x->sampler[i].xf=f;
  n_s_calc_l_start_end(x, i);
}

void n_s_loop(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  x->sampler[i].loop=(f>0);
  x->sampler[i].on=0;
}

void n_s_att(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  _clip_minmax(0, 100, f);
  x->sampler[i].att = pow(1.122, f-60);
  n_s_calc_att(x, i);
}

void n_s_dec(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  _clip_minmax(0, 100, f);
  x->sampler[i].dec = pow(1.122, f-60);
  n_s_calc_dec(x, i);
}

void n_s_sus(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  _clip_minmax(0, 100, f);
  x->sampler[i].sus = f*0.01;
}

void n_s_rel(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  _clip_minmax(0, 100, f);
  x->sampler[i].rel = pow(1.122, f-60);
  n_s_calc_rel(x, i);
}

void n_s_eg(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  x->sampler[i].eg=(f>0);
}

void n_s_freq(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  _clip_minmax(0, 120, f);
  x->sampler[i].freq = 8.1758 * pow(2, f/12); // ptof
  n_s_calc_freq(x, i);
}

void n_s_res(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  _clip_minmax(0, 1, f);
  f=(f*0.88)+0.1;
  f=1-f;
  x->sampler[i].res=f+f;
}

void n_s_filtype(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  x->sampler[i].filtype=f;
}

void n_s_l_f(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  _clip_minmax(0, 120, f);
  x->sampler[i].l_f = 8.1758 * pow(2, f/12); // ptof
  n_s_calc_l_f(x, i);
}

void n_s_l_g(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  _clip_minmax(-1, 1, f);
  if (f>0) f*=4;
  x->sampler[i].l_g = f;
}

void n_s_h_f(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  _clip_minmax(0, 120, f);
  x->sampler[i].h_f = 8.1758 * pow(2, f/12); // ptof
  n_s_calc_h_f(x, i);
}

void n_s_h_g(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  _clip_minmax(-1, 1, f);
  if (f>0) f*=4;
  x->sampler[i].h_g = f;
}

void n_s_disp(t_n_s *x, t_floatarg n, t_symbol *s)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  x->disp[i].s = s;
  x->disp[i].l = pd_open_array(x->disp[i].s, &x->disp[i].w, &x->disp[i].g);
  if (x->disp[i].l <= MIN_DISP_LEN || x->disp[i].l%2==1)
    {
      post("n_sampler: error: bad disp array: %s %d", 
	   x->disp[i].s->s_name,
	   x->disp[i].l);
    }
}

void n_s_xr(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  _clip_minmax(0, 1, f);
  x->disp[i].xr = f;
  n_s_calc_disp(x, i);
}

void n_s_xo(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  _clip_minmax(0, 1, f);
  x->disp[i].xo = f;
  n_s_calc_disp(x, i);
}

void n_s_mute(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  x->sampler[i].mute=(f>0);
  n_s_calc_ms(x);
}

void n_s_solo(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  x->sampler[i].solo=(f>0);
  n_s_calc_ms(x);
}


void n_s_g(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  if (f > 0)
    {
      x->sampler[i].on = 1;
      x->sampler[i].stage = STG_START;
      x->sampler[i].phase_sample = x->sampler[i].start_c;
      x->sampler[i].env_stage = ENV_ATT;
      x->sampler[i].vel = f;
    }
  else
    {
      if (x->sampler[i].loop)
	{
	  x->sampler[i].stage = STG_END;
	}
      x->sampler[i].env_stage = ENV_REL;
    }
}

////////////////////////////////////////////////////////////////////////////////
// dsp
#define S x->sampler[s]
#define A x->sample[x->sampler[s].sample]
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
  t_float out, f;
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
  for (int s=0; s<MAX_SAMPLER; s++)
    {
      if (S.on)
	{
	  for (n=0; n<bs; n++)
	    {
	      // sampler ///////////////////////////////////////////////////////
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
		  out = out * S.vel * S.env * S.ms;
		}
	      else
		{
		  out = out * S.vel * S.ms;
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
  class_addmethod(n_s_class,(t_method)n_s_bit,gensym("bit"),A_FLOAT,A_FLOAT,0);

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

  class_addmethod(n_s_class,(t_method)n_s_disp,gensym("disp"),A_FLOAT,A_SYMBOL,0);
  class_addmethod(n_s_class,(t_method)n_s_xr,gensym("xr"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_xo,gensym("xo"),A_FLOAT,A_FLOAT,0);

  class_addmethod(n_s_class,(t_method)n_s_mute,gensym("mute"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_solo,gensym("solo"),A_FLOAT,A_FLOAT,0);

  class_addmethod(n_s_class,(t_method)n_s_g,gensym("g"),A_FLOAT,A_FLOAT,0);
}
