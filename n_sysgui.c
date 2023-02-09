/* hcs/sys_gui.c */
#include <string.h>
#include "m_pd.h"
#include "g_canvas.h"

#define NOUSE(X) if(X){};

static t_class *n_sysgui_class;

typedef struct _n_sysgui
{
  t_object x_obj;
} t_n_sysgui;

void n_sysgui_anything(t_n_sysgui *x, t_symbol *s, int ac, t_atom *av)
{
  int i;
  char str[MAXPDSTRING];
  char buf[MAXPDSTRING];
  sprintf(str, "%s", s->s_name);
  for (i = 0; i < ac; i++)
    {
      atom_string(av + i, buf, MAXPDSTRING);
      strcat(str, " ");
      strcat(str, buf);
    }
  strcat(str, " ;\n");
  sys_vgui(str);
  NOUSE(x);
}

void *n_sysgui_new(void)
{
  t_n_sysgui *x = (t_n_sysgui *)pd_new(n_sysgui_class);
  return (void *)x;
}

void n_sysgui_setup(void)
{
  n_sysgui_class = class_new(gensym("n_sysgui"),
			     (t_newmethod)n_sysgui_new,
			     0,
			     sizeof(t_n_sysgui),
			     0, 0, 0);
  class_addanything(n_sysgui_class, (t_method)n_sysgui_anything);
}
