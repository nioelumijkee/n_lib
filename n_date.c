#include "m_pd.h"
#include <time.h>

static t_class *n_date_class;
typedef struct _n_date
{
  t_object x_obj;
} t_n_date;

void n_date_bang(t_n_date *x)
{
  t_atom a[10];
  struct tm *s_tm;
  time_t lt;

  // get time struct
  lt = time(NULL);
  s_tm = localtime(&lt);

  // var
  int year = s_tm->tm_year + 1900;
  int mon = s_tm->tm_mon;
  int mday = s_tm->tm_mday;
  int yday = s_tm->tm_yday;  // day 0 - 365
  int hour = s_tm->tm_hour;
  int min = s_tm->tm_min;
  int sec = s_tm->tm_sec;

  char weekday[40];
  strftime(weekday, 40,"%A", s_tm);

  char mon_name[40];
  strftime(mon_name, 40,"%B", s_tm);

  char mon_name_short[40];
  strftime(mon_name_short, 40,"%b", s_tm);

  // out
  SETFLOAT(a, (t_float)year);
  SETFLOAT(a+1, (t_float)mon);
  SETFLOAT(a+2, (t_float)mday);
  SETFLOAT(a+3, (t_float)hour);
  SETFLOAT(a+4, (t_float)min);
  SETFLOAT(a+5, (t_float)sec);
  SETSYMBOL(a+6, gensym(weekday));
  SETSYMBOL(a+7, gensym(mon_name));
  SETSYMBOL(a+8, gensym(mon_name_short));
  SETFLOAT(a+9, (t_float)yday);
  outlet_list(x->x_obj.ob_outlet, &s_list, 10, a);
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
