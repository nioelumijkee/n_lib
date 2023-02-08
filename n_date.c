#include "m_pd.h"
#include <time.h>

static t_class *n_date_class;
typedef struct _n_date
{
  t_object x_obj;
} t_n_date;

void n_date_bang(t_n_date *x)
{
  t_atom a[16];
  struct tm *s_tm;
  time_t lt;
  char s[40];
  lt = time(NULL);
  s_tm = localtime(&lt);
  SETFLOAT(a, (t_float)s_tm->tm_year + 1900);  // year
  SETFLOAT(a+1, (t_float)s_tm->tm_mon + 1);  // month
  SETFLOAT(a+2, (t_float)s_tm->tm_mday);  // day
  SETFLOAT(a+3, (t_float)s_tm->tm_hour);  // hour
  SETFLOAT(a+4, (t_float)s_tm->tm_min);  // min
  SETFLOAT(a+5, (t_float)s_tm->tm_sec);  // sec
  strftime(s, 40,"%A", s_tm);
  SETSYMBOL(a+6, gensym(s));  // weekday
  strftime(s, 40,"%b", s_tm);
  SETSYMBOL(a+7, gensym(s));  // name month short
  strftime(s, 40,"%B", s_tm);
  SETSYMBOL(a+8, gensym(s));  // name month
  SETFLOAT(a+9, (t_float)s_tm->tm_yday);  // day 0 - 365
  SETFLOAT(a+10, (t_float)s_tm->tm_isdst);  // isdst
  SETFLOAT(a+11, (t_float)CLOCKS_PER_SEC);  // clocks per second
  SETFLOAT(a+12, (t_float)lt);  // time(sec)
  sprintf(s, "%ld", lt);
  SETSYMBOL(a+13, gensym(s));  // time(sec) as symbol
  int fsec = lt>>16;
  int lsec = lt&0b1111111111111111; // 2^16-1
  // int res = (fsec<<16)+lsec;
  SETFLOAT(a+14, (t_float)fsec);  // time(sec) first 16 bit
  SETFLOAT(a+15, (t_float)lsec);  // time(sec) last 16 bit
  outlet_list(x->x_obj.ob_outlet, &s_list, 16, a);
}

void *n_date_new(void)
{
  t_n_date *x = (t_n_date *)pd_new(n_date_class);
  outlet_new(&x->x_obj, 0);
  return (void *)x;
}

void n_date_setup(void)
{
  n_date_class = class_new(gensym("n_date"),
			   (t_newmethod)n_date_new,
			   0,
			   sizeof(t_n_date),
			   0, 0, 0);
  class_addbang(n_date_class, (t_method)n_date_bang);
}
