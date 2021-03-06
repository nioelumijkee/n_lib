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

//----------------------------------------------------------------------------//
// get all
void n_env_bang(t_n_env *x)
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

//----------------------------------------------------------------------------//
void n_env_getenv(t_n_env *x, t_symbol *s)
{
  char * var = getenv(s->s_name);
  if (var != NULL)
    {
      outlet_symbol(x->x_obj.ob_outlet, gensym(var));
    }
  else
    {
      outlet_symbol(x->x_obj.ob_outlet, gensym("NULL"));
    }
}

//----------------------------------------------------------------------------//
void n_env_setenv(t_n_env *x, t_symbol *s0, t_symbol *s1)
{
  if (setenv (s0->s_name, s1->s_name, OVWR) != 0)
    {
      post("n_env: setenv: cannot set '%s'\n", s0->s_name);
    }
  if (x) {} // disabled
}

//----------------------------------------------------------------------------//
void *n_env_new(void)
{
  t_n_env *x = (t_n_env *)pd_new(n_env_class);
  x->out = outlet_new(&x->x_obj, 0);
  return (void *)x;
}

//----------------------------------------------------------------------------//
void n_env_setup(void)
{
  n_env_class = class_new(gensym("n_env"), (t_newmethod)n_env_new, 0, sizeof(t_n_env), CLASS_DEFAULT, A_GIMME, 0);
  class_addbang(n_env_class, (t_method)n_env_bang);
  class_addmethod(n_env_class, (t_method)n_env_getenv, gensym("getenv"), A_SYMBOL, 0);
  class_addmethod(n_env_class, (t_method)n_env_setenv, gensym("setenv"), A_SYMBOL, A_SYMBOL, 0);
}
