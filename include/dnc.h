#include <math.h>
#define AC_DNC 0.000000001

// dnc
//----------------------------------------------------------------------------//
#define AF_DNC(IN)      \
  if ((IN) > 0)		\
    (IN) += AC_DNC;	\
  else			\
    (IN) -= AC_DNC;
