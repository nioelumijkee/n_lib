#include "array_def.h"
#include "mix.h"


//----------------------------------------------------------------------------//
inline void arr_mix(ARR_TYPE *arr_in0,
                    ARR_TYPE *arr_in1,
                    ARR_TYPE *arr_x,
                    ARR_TYPE *arr_out,
                    int start_in0,
                    int start_in1,
                    int start_x,
                    int start_out,
                    int len)
{
  int i;
  for (i = 0; i < len; i++)
    {
      AF_MIX(arr_in0[i + start_in0]_E,
             arr_in1[i + start_in1]_E,
             arr_x[i   + start_x]_E,
             arr_out[i + start_out]_E);
    }
}

//----------------------------------------------------------------------------//
inline void arr_mix_scalar(ARR_TYPE *arr_in0,
                           ARR_TYPE *arr_in1,
                           ARR_TYPE *arr_out,
                           int start_in0,
                           int start_in1,
                           int start_out,
                           int len,
                           float mix)
{
  int i;
  for (i = 0; i < len; i++)
    {
      AF_MIX(arr_in0[i + start_in0]_E,
             arr_in1[i + start_in1]_E,
             mix,
             arr_out[i + start_out]_E);
    }
}
