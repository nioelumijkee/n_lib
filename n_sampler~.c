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
  int       xr; //in
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
  int group; //in
  t_float bit_pre; //in
  t_float bit_post; //in
  t_float bit_ofs; //in
  t_float start; //in
  t_float end; //in
  t_float l_st; //in
  t_float l_end; //in
  t_float start_c;
  t_float end_c;
  t_float l_st_c;
  t_float l_end_c;
  t_float sdel; //in
  t_float sdel_c;
  t_float semi; //in
  t_float fine; //in
  t_float tune;
  t_float xf; //in
  t_float sp; //in
  t_float g_sz; //in
  t_float g_sz_c;
  int loop; //in
  int stretch; //in
  int eg; //in
  t_float att; //in
  t_float dec; //in
  t_float sus; //in
  t_float rel; //in
  t_float att_c;
  t_float dec_c;
  t_float rel_c;
  int filtype; //in
  t_float freq; //in
  t_float res; //in
  t_float filw;
  t_float l_f; //in
  t_float l_g; //in
  t_float lfw;
  t_float h_f; //in
  t_float h_g; //in
  t_float hfw;
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
    }
}

void n_s_calc_level(t_n_s *x, int n)
{
  t_float l = 1 - x->sampler[n].pan; 
  t_float r = 1 + x->sampler[n].pan;
  t_float s = 0.33333 * x->sampler[n].level;
  x->sampler[n].level_l = s * (4-l);
  x->sampler[n].level_r = s * (4-r);
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
  x->sampler[n].start_c = x->sampler[n].start * (x->sample[x->sampler[n].sample].l
						 - SAMPLE_END_OFS);
  x->sampler[n].end_c = x->sampler[n].end * (x->sample[x->sampler[n].sample].l
					     - SAMPLE_END_OFS);
}

void n_s_calc_l_start_end(t_n_s *x, int n)
{
  x->sampler[n].l_st_c = x->sampler[n].l_st * (x->sample[x->sampler[n].sample].l
					       - SAMPLE_END_OFS);
  x->sampler[n].l_end_c = x->sampler[n].l_end * (x->sample[x->sampler[n].sample].l
						 - SAMPLE_END_OFS);
  if (x->sampler[n].l_end_c < x->sampler[n].start_c)
    x->sampler[n].l_end_c < x->sampler[n].start_c;
  /// ??????
}

void n_s_calc_sdel(t_n_s *x, int n)
{
  x->sampler[n].sdel_c = x->sampler[n].sdel;
  int l = x->sample[x->sampler[n].sample].l - SAMPLE_END_OFS;
  if (x->sampler[n].sdel_c > l)
    x->sampler[n].sdel_c = l;
}

void n_s_calc_tune(t_n_s *x, int n)
{
  t_float f = x->sampler[n].semi + x->sampler[n].fine;
  x->sampler[n].tune = pow(2, f/12);
}

void n_s_calc_g_sz(t_n_s *x, int n)
{
  t_float f = x->sampler[n].g_sz * x->sr * 0.001;
  int l = x->sample[x->sampler[n].sample].l - SAMPLE_END_OFS;
  if (f > l)
    f = l;
  x->sampler[n].g_sz_c = f;
}

void n_s_calc_att(t_n_s *x, int n)
{
  x->sampler[n].att_c = x->div_1_sr / x->sampler[n].att ;
}

void n_s_calc_dec(t_n_s *x, int n)
{
  x->sampler[n].dec_c = x->div_1_sr / x->sampler[n].dec ;
}

void n_s_calc_rel(t_n_s *x, int n)
{
  x->sampler[n].rel_c = x->div_1_sr / x->sampler[n].rel ;
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

void n_s_calc_constant(t_n_s *x)
{
  x->div_1_sr = 1.0 / x->sr;
  x->div_sr_8 = x->sr / 8.0;
  x->div_2pi_sr = TWOPI / x->sr;
  /// ??????
  for(int i=0; i<MAX_SAMPLER; i++)
    {
      n_s_calc_g_sz(x, i);
      n_s_calc_att(x, i);
      n_s_calc_dec(x, i);
      n_s_calc_rel(x, i);
      n_s_calc_freq(x, i);
      n_s_calc_l_f(x, i);
      n_s_calc_h_f(x, i);
    }
}

void n_s_calc_disp(t_n_s *x, int n)
{
  // calc xr xo
  // to disp
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
  n_s_calc_g_sz(x, i);
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
  n_s_calc_g_sz(x, i);
}

void n_s_level(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  f=f*0.01;
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
  f=f*0.01;
  _clip_minmax(-1, 1, f);
  x->sampler[i].pan=f;
  n_s_calc_level(x, i);
}

void n_s_s1(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  f=f*0.01;
  _clip_min(0, f);
  x->sampler[i].s1=f;
  n_s_calc_level_s1(x, i);
}

void n_s_s2(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  f=f*0.01;
  _clip_min(0, f);
  x->sampler[i].s2=f;
  n_s_calc_level_s2(x, i);
}

void n_s_rnda(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  f=f*0.0001; // 1/10000
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
  _clip_minmax(0, 100, f);
  f=f*0.01;
  x->sampler[i].start=f;
  n_s_calc_start_end(x, i);
}

void n_s_end(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  _clip_minmax(0, 100, f);
  f=f*0.01;
  x->sampler[i].end=f;
  n_s_calc_start_end(x, i);
}

void n_s_l_st(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  _clip_minmax(0, 100, f);
  f=f*0.01;
  x->sampler[i].l_st=f;
  n_s_calc_l_start_end(x, i);
}

void n_s_l_end(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  _clip_minmax(0, 100, f);
  f=f*0.01;
  x->sampler[i].l_end=f;
  n_s_calc_l_start_end(x, i);
}

void n_s_sdel(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  _clip_min(0, f);
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
  f=f*0.01;
  _clip_minmax(0, 1, f);
  x->sampler[i].xf=f;
}

void n_s_sp(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  f=f*0.01;
  _clip_min(0, f);
  x->sampler[i].sp=f;
}

void n_s_g_sz(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  _clip_min(1, f);
  x->sampler[i].g_sz=f;
  n_s_calc_g_sz(x, i);
}

void n_s_loop(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  x->sampler[i].loop=(f>0);
}

void n_s_stretch(t_n_s *x, t_floatarg n, t_floatarg f)
{
  int i=n;
  _clip_minmax(0, MAX_SAMPLER_1, i);
  x->sampler[i].stretch=(f>0);
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
  _clip_minmax(0, 100, f);
  f=(f*0.0088)+0.1;
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
  _clip_minmax(-100, 100, f);
  x->sampler[i].l_g = f * 0.01;
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
  _clip_minmax(-100, 100, f);
  x->sampler[i].h_g = f * 0.01;
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
  _clip_min(1, f);
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

void n_s_g(t_n_s *x, t_floatarg n, t_floatarg f)
{
}

////////////////////////////////////////////////////////////////////////////////
// dsp
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
  int n = (int)(w[12]);
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
  class_addmethod(n_s_class,(t_method)n_s_l_st,gensym("l-st"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_l_end,gensym("l-end"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_sdel,gensym("sdel"),A_FLOAT,A_FLOAT,0);

  class_addmethod(n_s_class,(t_method)n_s_semi,gensym("semi"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_fine,gensym("fine"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_xf,gensym("xf"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_sp,gensym("sp"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_g_sz,gensym("g-sz"),A_FLOAT,A_FLOAT,0);

  class_addmethod(n_s_class,(t_method)n_s_loop,gensym("loop"),A_FLOAT,A_FLOAT,0);
  class_addmethod(n_s_class,(t_method)n_s_stretch,gensym("stretch"),A_FLOAT,A_FLOAT,0);

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

  class_addmethod(n_s_class,(t_method)n_s_g,gensym("g"),A_FLOAT,A_FLOAT,0);
}
