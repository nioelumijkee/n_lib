#include "array_def.h"
#include "random.h"

//----------------------------------------------------------------------------//
inline void arr_dc(ARR_TYPE *arr, int start, int end)
{
  int i;
  float min = arr[start]_E;
  float max = arr[start]_E;
  for (i = start+1; i < end; i++)
    {
      if (arr[i]_E < min)
        {
          min = arr[i]_E;
        }
      else if (arr[i]_E > max)
        {
          max = arr[i]_E;
        }
    }
  float offset = ((max - min) * 0.5) + min;
  for (i = start; i < end; i++)
    {
      arr[i]_E  -= offset;
    } 
}

//----------------------------------------------------------------------------//
inline void arr_normalize(ARR_TYPE *arr, int start, int end, float value)
{
  int i;
  float min = arr[start]_E;
  float max = arr[start]_E;
  for (i = start+1; i < end; i++)
    {
      if (arr[i]_E < min)
        {
          min = arr[i]_E;
        }
      else if (arr[i]_E > max)
        {
          max = arr[i]_E;
        }
    }
  float diff = (max - min) * 0.5;
  if (diff != 0)
    {
      diff = (1 / diff) * value;
      for (i = start; i < end; i++)
        {
          arr[i]_E  *= diff;
        } 
    }
}

//----------------------------------------------------------------------------//
inline void arr_constant(ARR_TYPE *arr, int start, int end, float value)
{
  int i;
  for (i = start; i < end; i++)
    {
      arr[i]_E = value;
    }
}

//----------------------------------------------------------------------------//
inline void arr_random(ARR_TYPE *arr, int start, int end, float value, int *seed)
{
  int i;
  for (i = start; i < end; i++)
    {
      AF_RANDOM(*seed);
      arr[i]_E = *seed * AC_RND_NORM * value;
    }
}

//----------------------------------------------------------------------------//
inline void arr_towt(ARR_TYPE *arr_src, 
                     int start, int end, int len,
                     ARR_TYPE *arr_dst,
                     int st,
                     int en)
{
  int i;
  int j = 0;
  // copy start
  for(i=0; i<st; i++, j++)
    {
      arr_dst[j]_E = arr_src[end - st + i]_E;
    }
  // copy body
  for(i=0; i<len; i++, j++)
    {
      arr_dst[j]_E = arr_src[start + i]_E;
    }
  // copy end
  for(i=0; i<en; i++, j++)
    {
      arr_dst[j]_E = arr_src[start + i]_E;
    }
}

//----------------------------------------------------------------------------//
inline void arr_fromwt(ARR_TYPE *arr_src, 
                       int len,
                       ARR_TYPE *arr_dst,
                       int st,
                       int en)
{
  int i;
  int j = 0;
  for(i=st; i<len-en; i++, j++)
    {
      arr_dst[j]_E = arr_src[i]_E;
    }
}

//----------------------------------------------------------------------------//
inline void arr_blur(ARR_TYPE *arr, int start, int end, int len, float value)
{
  if (len < 3)
    {
      return;
    }
  int i;
  float p_s = arr[start]_E;
  float p_e = arr[end-1]_E;
  float inc = (p_e - p_s) / (len-1);
  float base = p_s + inc;
  for (i = start+1; i < end-1; i++)
    {
      arr[i]_E = ((base - arr[i]_E) * value) + arr[i]_E;
      base += inc;
    }
}

//----------------------------------------------------------------------------//
inline int arr_unique(ARR_TYPE *arr, int start, int end)
{
  int i, j, k;
  int find;
  k  = start+1;
  for (i = start+1; i < end; i++)
    {
      find = 0;
      for (j = start; j < k; j++)
        {
          if (arr[i]_E == arr[j]_E)
            {
              find = 1;
              break;
            }
        }
      // copy
      if (find == 0 && k != i)
        {
          arr[k]_E = arr[i]_E;
          k++;
        }
    }
  return(k - start - 1);
}

//----------------------------------------------------------------------------//
inline void arr_count(ARR_TYPE *arr,
                      int start, int end, 
                      float base,
                      float inc)
{
  int i;
  for (i = start; i < end; i++)
    {
      arr[i]_E = base;
      base += inc;
    }
}
