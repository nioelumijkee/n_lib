#include "array_def.h"

//----------------------------------------------------------------------------//
// very fast and simple alghorhytm:)
inline void arr_todisp(ARR_TYPE *arr_in,
                       ARR_TYPE *arr_min,
                       ARR_TYPE *arr_max, 
                       int start_in,                 int len_in,
                       int start_disp, int end_disp, int len_disp)
{
  int i, j, x0, x1, k;
  float min, max;
  float count;
  float t_part;
  
  count = start_in;
  t_part = (t_float)len_in / (t_float)len_disp;
  for (i = start_disp; i < end_disp; i++)
    {
      x0 = k = count;
      count += t_part;
      x1 = count - 1;
      if (k >= len_in)  k -= len_in;
      // find min max
      max = min = arr_in[k]_E;
      for (j = x0 + 1; j < x1; j++)
        {
          k = j;
          if (k >= len_in)   k -= len_in;
          if (max < arr_in[k]_E)  max = arr_in[k]_E;
          if (min > arr_in[k]_E)  min = arr_in[k]_E;
        }
      arr_min[i]_E = min;
      arr_max[i]_E = max;
    }
}
