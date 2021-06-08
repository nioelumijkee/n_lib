#include "array_def.h"

//----------------------------------------------------------------------------//
inline void arr_shift(ARR_TYPE *arr, int start, int end, int len_arr, int dir)
{
  int i,j;
  len_arr -= 1;
  // shift right
  if (dir > 0)
    {
      for(i = end-1; i >=start; i--)
        {
          j = i+dir;
          if (j > len_arr)
            {
              j = len_arr;
            }
          arr[j]_E = arr[i]_E;
        }
    }
  // shift left
  else if (dir < 0)
    {
      for(i = start; i < end; i++)
        {
          j = i+dir;
          if (j < 0)
            {
              j = 0;
            }
          arr[j]_E = arr[i]_E;
        }
    }
}
