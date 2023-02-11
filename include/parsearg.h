#include "g_all_guis.h"

#define IFARGF(N,NAME,DEF)				     \
  if (ac >= (N) && IS_A_FLOAT(av, (N)-1))                    \
    (NAME)(x, atom_getfloatarg((N)-1,ac,av));                \
  else                                                       \
    (NAME)(x, (DEF));

#define IFARGS(N,NAME,DEF)                                    \
  if (ac >= (N) && IS_A_SYMBOL(av, (N)-1))                    \
    (NAME)(x, atom_getsymbolarg((N)-1,ac,av));                \
  else							      \
    (NAME)(x, (DEF));

