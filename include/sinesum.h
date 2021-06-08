#include <math.h>
#include "constant.h"


//----------------------------------------------------------------------------//
inline void sinesum(float *arr_out,     // sum
                    float *arr_harm,    // harmonics
                    int size_out,       // + 3
                    int size_harm)
{
  int i,j,m;
  int harm;
  float f;
  size_out -= 3;

  // calc
  for (i = 0; i < size_harm; i++)
    {
      // one harmonic
      harm = i + 1;
      for (j = 0; j < size_out; j++)
        {
          f = (float)j / size_out; // 0 ... 1
          f *= harm;
          f += 0.5;                // phase
          m = f;	               // wrap 0 ... 1
          f = f - m;
          f = f + f - 1.;          // -1 ... 1
          f *= AC_PI;	           // -pi ... pi
          f = sin(f);              // sin
          f = f * arr_harm[i];     // amplitude
          arr_out[j+1] += f;
        }
    }

  // for interpolation
  arr_out[0]          = arr_out[size_out];
  arr_out[size_out+1] = arr_out[1];
  arr_out[size_out+2] = arr_out[2];
}
