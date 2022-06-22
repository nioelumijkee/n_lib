#include "g_all_guis.h"

//----------------------------------------------------------------------------//
#define IFARG(N,NAME,DEF)                                    \
  if (ac >= (N) && IS_A_FLOAT(av, (N)-1))                    \
    (NAME)(x, atom_getfloatarg((N)-1,ac,av));                \
  else                                                       \
    (NAME)(x, (DEF));

