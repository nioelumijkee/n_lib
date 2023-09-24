#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "m_pd.h"

#define OVWR 1

extern char **environ; /* Environment */
static t_class *n_env_class;
typedef struct _n_env
{
  t_object x_obj;
  t_outlet *out;
} t_n_env;

void n_env_get_all(t_n_env *x)
{
  int i,j,k;
  char var[MAXPDSTRING];
  char val[MAXPDSTRING];
  t_atom a[2];
  for (i = 0; environ[i] != NULL; i++)
    {
      // copy var
      j = 0;
      while(environ[i][j] != '=' && j < MAXPDSTRING-1)
	{
	  var[j] = environ[i][j];
	  j++;
	}
      var[j] = '\0';
      // copy val
      j++;
      k = 0;
      while(environ[i][j] != '\0' && j < MAXPDSTRING-1)
	{
	  val[k] = environ[i][j];
	  j++;
	  k++;
	}
      val[k] = '\0';
      // outlet
      SETSYMBOL(a, gensym(var));
      SETSYMBOL(a+1, gensym(val));
      outlet_list(x->out, &s_list, 2, a);
    }
}

void n_env_get(t_n_env *x, t_symbol *s)
{
  char * var = getenv(s->s_name);
  if (var != NULL)
    outlet_symbol(x->x_obj.ob_outlet, gensym(var));
  else
    {
      post("n_env: getenv: cannot get '%s'", s->s_name);
      outlet_float(x->x_obj.ob_outlet, 1);
    }
}

void n_env_set(t_n_env *x, t_symbol *s_env, t_symbol *s_val)
{
  if (setenv (s_env->s_name, s_val->s_name, OVWR) != 0)
    {
      post("n_env: setenv: cannot set '%s'", s_env->s_name);
      outlet_float(x->x_obj.ob_outlet, 1);
    }
  else
    {
      outlet_float(x->x_obj.ob_outlet, 0);
    }
}

void *n_env_new(void)
{
  t_n_env *x = (t_n_env *)pd_new(n_env_class);
  x->out = outlet_new(&x->x_obj, 0);
  return (void *)x;
}

void n_env_setup(void)
{
  n_env_class = class_new(gensym("n_env"), 
			  (t_newmethod)n_env_new,
			  0, 
			  sizeof(t_n_env),
			  0, 0, 0);
  class_addbang(n_env_class, (t_method)n_env_get_all);
  class_addmethod(n_env_class,(t_method)n_env_get,gensym("get"),A_SYMBOL,0);
  class_addmethod(n_env_class,(t_method)n_env_set,gensym("set"),A_SYMBOL,A_SYMBOL,0);
}
