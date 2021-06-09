#include "array_def.h"
#include "swap.h"

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

//----------------------------------------------------------------------------//
inline void arr_shift_rotate(ARR_TYPE *arr, 
                             int start, int len, 
                             int size,
                             int sh)
{
  if (size < 2) return;
  if (len < 2)  return;
  if (sh < 0)
    {
      sh = 0 - sh;
      sh = sh % len;
      sh = len - sh;
    }
  else if (sh == 0) return;
  else
    {
      sh = sh % len;
    }

  int i, p1, p2, m;
  int a_s = start;
  int a_l = sh;
  int b_s = start + sh;
  int b_l = len - sh;
  float buf;
  
  // reverse a
  if (a_l > 1)
    {
      m = a_l / 2;
      for (i = 0; i < m; i++)
        {
          p1 = i + a_s;
          p2 = a_l - i - 1 + a_s;
          SWAP(buf, arr[p1]_E, arr[p2]_E);
        }
    }
  
  // reverse b
  if (b_l > 1)
    {
      m = b_l /2;
      for (i = 0; i < m; i++)
        {
          p1 = i + b_s;
          p2 = b_l - i - 1 + b_s;
          SWAP(buf, arr[p1]_E, arr[p2]_E);
        }
    }
  
  // reverse ab
  for (i = 0; i < (len / 2); i++)
    {
      p1 = i + a_s;
      p2 = len - i - 1 + a_s;
      SWAP(buf, arr[p1]_E, arr[p2]_E);
    }
}

//----------------------------------------------------------------------------//
inline void arr_reverse(ARR_TYPE *arr, 
                        int start, int end, int len)
{
  int i, m, p1, p2;
  float buf;
  m = len / 2;
  for (i = 0; i < m; i++)
    {
      p1 = start + i;
      p2 = end - i - 1;
      SWAP(buf, arr[p1]_E, arr[p2]_E);
    }
}
