#include "clip.h"


//----------------------------------------------------------------------------//
inline void arr_clip_minmax(ARR_TYPE *arr,
                            ARR_TYPE *arr_min,
                            ARR_TYPE *arr_max,
                            int start,
                            int start_min,
                            int start_max,
                            int len)
{
  int i;
  for (i = 0; i < len; i++)
    {
      AF_CLIP_MINMAX(arr_min[i + start_min]_E,
                     arr_max[i + start_max]_E,
                     arr[i + start]_E);
    }
}

//----------------------------------------------------------------------------//
inline void arr_clip_minmax_scalar(ARR_TYPE *arr,
                                   int start,
                                   int end,
                                   float min,
                                   float max)
{
  int i;
  for (i = start; i < end; i++)
    {
      AF_CLIP_MINMAX(min,
                     max,
                     arr[i]_E);
    }
}

//----------------------------------------------------------------------------//
inline void arr_clip_min(ARR_TYPE *arr,
                         ARR_TYPE *arr_min,
                         int start,
                         int start_min,
                         int len)
{
  int i;
  for (i = 0; i < len; i++)
    {
      AF_CLIP_MIN(arr_min[i + start_min]_E,
                  arr[i + start]_E);
    }
}

//----------------------------------------------------------------------------//
inline void arr_clip_min_scalar(ARR_TYPE *arr,
                                int start,
                                int end,
                                float min)
{
  int i;
  for (i = start; i < end; i++)
    {
      AF_CLIP_MIN(min,
                  arr[i]_E);
    }
}

//----------------------------------------------------------------------------//
inline void arr_clip_max(ARR_TYPE *arr,
                         ARR_TYPE *arr_max,
                         int start,
                         int start_max,
                         int len)
{
  int i;
  for (i = 0; i < len; i++)
    {
      AF_CLIP_MAX(arr_max[i + start_max]_E,
                  arr[i + start]_E);
    }
}

//----------------------------------------------------------------------------//
inline void arr_clip_max_scalar(ARR_TYPE *arr,
                                int start,
                                int end,
                                float max)
{
  int i;
  for (i = start; i < end; i++)
    {
      AF_CLIP_MAX(max,
                  arr[i]_E);
    }
}

