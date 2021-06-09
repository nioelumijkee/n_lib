#include "array_def.h"
#include "constant.h"
#include "math.h"

//----------------------------------------------------------------------------//
inline void arr_vec2pol(ARR_TYPE *arr_re,
                        ARR_TYPE *arr_im,
                        ARR_TYPE *arr_a,
                        ARR_TYPE *arr_ph,
                        int size)
{
  int i;
  float bsc = 1. / ((t_float)size / 2.);
  float re, im, a, b;
  float div_im_re;
  float ab;
  float f;
  for (i = 0; i < size; i++)
    {
      re = arr_re[i]_E * bsc;
      im = arr_im[i]_E * bsc;

      // a
      a = re * re;
      b = im * im;
      a = a + b + 1e-12;
      arr_a[i]_E = sqrt(a);
      
      // FAtan2
      // safe
      if (re > 0)
        {
          div_im_re = im / (re + 1e-12);
        }
      else
        {
          div_im_re = im / (re - 1e-12);
        }
      
      // abs
      AF_ABS(div_im_re, ab);
      
      if (ab < 1)
        {
          f = (div_im_re * div_im_re * 0.28) + 1.;
          f = div_im_re / f;
          if (re < 0)
            {
              if (im > 0)
                {
                  f += AC_PI;
                }
              else
                {
                  f -= AC_PI;
                }
            }
        }
      else
        {
          f = div_im_re / ((div_im_re * div_im_re) + 0.28);
          f = 1.5708 - f;
          if (im < 0)
            {
              f -= AC_PI;
            }
        }
      arr_ph[i]_E = f;
    }
}

//----------------------------------------------------------------------------//
inline void arr_pol2vec(ARR_TYPE *arr_a,
                        ARR_TYPE *arr_ph,
                        ARR_TYPE *arr_re,
                        ARR_TYPE *arr_im,
                        int size)
{
  int i;
  t_float a, ph;
  t_float cos_f;
  t_float sin_f;
  t_float f;
  t_float m = size * 0.5;
  for (i = 0; i < size; i++)
    {
      a  = arr_a[i]_E;   // a
      ph = arr_ph[i]_E;  // -pi ... pi
      f = ph * ph;
      // cos -pi ... pi
      cos_f = (((((((((f * -2.605e-7) + 2.47609e-5) * f) - 0.00138884) * f) 
                  + 0.0416666) * f) - 0.499923) * f) + 1.;
      // sin -pi ... pi
      sin_f = ((((((((((f * -2.39E-008) + 2.7526E-006) * f) - 0.000198409) * f)
                   + 0.00833333) * f) - 0.166667) * f) + 1.) * ph;
      arr_re[i]_E = a * cos_f * m;
      arr_im[i]_E = a * sin_f * m;
    }
}
