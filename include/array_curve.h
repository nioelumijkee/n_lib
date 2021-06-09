#include "array_def.h"
#include "constant.h"
#include "clip.h"

//----------------------------------------------------------------------------//
inline void arr_curve_c_breakpoint(ARR_TYPE *arr,
                                   int start,
                                   int end,
                                   int len,
                                   float a,
                                   float b)
{
  int i, j;
  float f,e;
  float h = a + 1e-12;
  float m = (1 - b) / (1 - h);
  float n = b / h;
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (float)len;
      if (f > a)
        {
          e = b + (m * (f - h));
        }
      else
        {
          e = n * f; 
        }
     arr[i]_E = e;
    }
}
