#include "interpolation.h"


//----------------------------------------------------------------------------//
inline void arr_interpolation(ARR_TYPE *arr_in,
                              ARR_TYPE *arr_out,
                              int size_in,
                              int size_out,
                              int type) // no | lin | 4p
{
  int i, j;
  int i0, i1, i2, i3;
  float f;
  float p0, p1, p2, p3;
  float c1, c2, c3, c4;

  // no interpolation
  if (type == 0)
    {
      for (i = 0; i < size_out; i++)
        {
          f = ((float)i / (float)size_out) * (float)size_in;
          j = f;
          arr_out[i]_E = arr_in[j]_E;
        }
    }

  // lin 2-p interpolation
  else if (type == 1)
    {
      for (i = 0; i < size_out; i++)
        {
          f = ((float)i / ((float)size_out-1.0)) * (float)size_in;
          i0 = f;
          i1 = i0 + 1; 
          if (i0 >= size_in) i0 -= size_in;
          if (i1 >= size_in) i1 -= size_in;
          f = f - (int)f;
          AF_INTERPOL_2P(arr_in[i0]_E, arr_in[i1]_E, f, arr_out[i]_E);
        }
    }

  // 4-p interpolation
  else if (type == 2)
    {
      for (i = 0; i < size_out; i++)
        {
          f = ((float)i / ((float)size_out-1.0)) * (float)size_in;
          i1 = f;      
          i0 = i1 - 1; if (i0 < 0)        i0 += size_in;
                       if (i0 >= size_in) i0 -= size_in;
          i2 = i1 + 1; if (i2 >= size_in) i2 -= size_in;
          i3 = i1 + 2; if (i3 >= size_in) i3 -= size_in;
                       if (i1 >= size_in) i1 -= size_in;
          f = f - (int)f;
          p0 = arr_in[i0]_E;
          p1 = arr_in[i1]_E;
          p2 = arr_in[i2]_E;
          p3 = arr_in[i3]_E;
          AF_INTERPOL_4P(p0, p1, p2, p3, f, c1, c2, c3, c4, arr_out[i]_E);
        }
    }
}
