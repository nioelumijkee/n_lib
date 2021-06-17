#include <math.h>
#include "constant.h"
#include "clip.h"

//----------------------------------------------------------------------------//
inline void arr_curve_breakpoint(ARR_TYPE *arr,
                                   int start,
                                   int end,
                                   int len,
                                   float c1,
                                   float c2)
{
  int i, j;
  float f,e;
  float h = c1 + 1e-12;
  float m = (1 - c2) / (1 - h);
  float n = c2 / h;
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (float)len;
      if (f > c1)
        {
          e = c2 + (m * (f - h));
        }
      else
        {
          e = n * f; 
        }
      arr[i]_E = e;
    }
}

//----------------------------------------------------------------------------//
inline void arr_curve_breakpoint_smooth(ARR_TYPE *arr,
                                          int start,
                                          int end,
                                          int len,
                                          float c1,
                                          float c2)
{
  int i, j;
  float f, e;
  float m = c1 + 1e-12;
  float n = c2 + 1e-12;
  float a = c1 / n;
  float b = 1. - c1;
  float c = 1. - c2 + 1e-12;
  float d = b / c;
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (float)len;
      if (f < c1)
        {
          e = f / m;
          e = pow(e, a);
          e = e * c2;
        }
      else
        {
          e = f - 1.;
          e = e * -1.;
          e = e / b;
          e = pow(e, d);
          e = e * c;
          e = e * -1.;
          e = e + 1.;
        }
      arr[i]_E = e;
    }
}

//----------------------------------------------------------------------------//
inline void arr_curve_circular(ARR_TYPE *arr,
                                 int start,
                                 int end,
                                 int len,
                                 float c1)
{
  int i, j;
  float f, e;
  float a = pow(c1, 2.);
  float b = pow(1. - c1, 2.);
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (float)len;
      if (f < c1)
        {
          e = a - pow(f - c1, 2.);
          e = sqrt(e);
        }
      else
        {
          e = b - pow(f - c1, 2.);
          e = 1. - sqrt(e);
        }
      arr[i]_E = e;
    }
}

//----------------------------------------------------------------------------//
inline void arr_curve_circular_sigmoid(ARR_TYPE *arr,
                                         int start,
                                         int end,
                                         int len,
                                         float c1)
{
  int i, j;
  float f, e;
  float a = c1 * c1;
  float b = pow(1. - c1, 2.);
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (float)len;
      if (f < c1)
        {
          e = c1 - sqrt(a - (f*f));
        }
      else
        {
          e = b - pow(f - 1., 2.);
          e = c1 + sqrt(e);
        }
      arr[i]_E = e;
    }
}

//----------------------------------------------------------------------------//
inline void arr_curve_cubic(ARR_TYPE *arr,
                              int start,
                              int end,
                              int len,
                              float c1,
                              float c2)
{
  int i, j;
  float f, e;
  float m = c1 + 1e-6;
  float a = 1. - m;
  float b = 1. - c2;
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (float)len;
      if (f < c1)
        {
          e = 1. - (f / m);
          e = pow(e, 3.);
          e = c2 - (c2 * e);
        }
      else
        {
          e = (f - m) / a;
          e = pow(e, 3.);
          e = c2 + (b * e);
        }
      arr[i]_E = e;
    }
}

//----------------------------------------------------------------------------//
inline void arr_curve_exp(ARR_TYPE *arr,
                            int start,
                            int end,
                            int len)
{
  int i, j;
  float f, e;
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (float)len;
      e = f * 100.;
      e =  exp(AC_LOGTEN_1_20 * (e - 100.)); // db2rms
      arr[i]_E = e;
    }
}

//----------------------------------------------------------------------------//
inline void arr_curve_log(ARR_TYPE *arr,
                            int start,
                            int end,
                            int len)
{
  int i, j;
  float f, e;
  float a = 2 * log(10);
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (float)len;
      if (f > 0.01)
        {
          e = log(f) + a;
          e = e / a; 
          arr[i]_E = e;
        }
      else
        {
          arr[i]_E = f;
        }
    }
}

//----------------------------------------------------------------------------//
inline void arr_curve_elleptic(ARR_TYPE *arr,
                               int start,
                               int end,
                               int len,
                               float c1,
                               float c2)
{
  int i, j;
  float f, e;
  float m = c1 + 1e-12;
  float n = c2 + 1e-12;
  float a = 1. - m;
  float b = 1. - c2 + 1e-12;
  float c = a / b;
  float d = m / n;
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (float)len;
      if (f < c1)
        {
          e = f / m;
          e = pow(e, d);
          e = e * n;
        }
      else
        {
          e = f - 1;
          e = e * -1;
          e = e / a;
          e = pow(e, c);
          e = e * b;
          e = e * -1;
          e = e + 1;
        }
      arr[i]_E = e;
    }
}

//----------------------------------------------------------------------------//
inline void arr_curve_elleptic_seat(ARR_TYPE *arr,
                                    int start,
                                    int end,
                                    int len,
                                    float c1,
                                    float c2)
{
  int i, j;
  float f, e;
  float m = c1 + 1e-12;
  float n = c2 + 1e-12;
  float a = n / m;
  float b = pow(m, 2);
  float c = (1 - n) / (1 - m);
  float d = pow(1 - m, 2);
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (float)len;
      if (f < c1)
        {
          e = a * sqrt(b - pow(f - m, 2));
        }
      else
        {
          e = 1 - (c * sqrt(d - pow(f - m, 2)));
        }
      arr[i]_E = e;
    }
}

//----------------------------------------------------------------------------//
inline void arr_curve_elleptic_sigmoid(ARR_TYPE *arr,
                                       int start,
                                       int end,
                                       int len,
                                       float c1,
                                       float c2)
{
  int i, j;
  float f, e;
  float m = c1 + 1e-12;
  float n = c2 + 1e-12;
  float a = pow(m, 2);
  float b = (1 - n) / (1 - m);
  float c = pow(1 - m, 2);
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (float)len;
      if (f < c1)
        {
          e = sqrt(a - pow(f, 2));
          e = n * (1 - (e / m));
        }
      else
        {
          e = sqrt(c - pow(f - 1, 2));
          e = n + (b * e);
        }
      arr[i]_E = e;
    }
}

//----------------------------------------------------------------------------//
inline void arr_curve_expotential(ARR_TYPE *arr,
                                  int start,
                                  int end,
                                  int len,
                                  float c1)
{
  c1 = 0.01 + (c1 * 0.98);
  int i, j;
  float f, e;
  float a = c1 * 2;
  float b = 1 / (1 - ((c1 - 0.5) * 2));
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (float)len;
      if (c1 < 0.5)
        {
          e = pow(f,a);
        }
      else
        {
          e = pow(f,b);
        }
      arr[i]_E = e;
    }
}

//----------------------------------------------------------------------------//
inline void arr_curve_expotential_seat(ARR_TYPE *arr,
                                       int start,
                                       int end,
                                       int len,
                                       float c1)
{
  int i, j;
  float f, e;
  float a = 1 - c1;
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (float)len;
      if (f < 0.5)
        {
          e = pow(f * 2,a) / 2;
        }
      else
        {
          e = 1 - (pow(2 * (1 - f), a) / 2);
        }
      arr[i]_E = e;
    }
}

//----------------------------------------------------------------------------//
inline void arr_curve_expotential_sigmoid(ARR_TYPE *arr,
                                          int start,
                                          int end,
                                          int len,
                                          float c1)
{
  c1 += 1e-6;
  int i, j;
  float f, e;
  float a = 1 / c1;
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (float)len;
      if (f < 0.5)
        {
          e = pow(f * 2,a) / 2;
        }
      else
        {
          e = 1 - (pow(2 * (1 - f), a) / 2);
        }
      arr[i]_E = e;
    }
}

//----------------------------------------------------------------------------//
inline void arr_curve_logistic_sigmoid(ARR_TYPE *arr,
                                       int start,
                                       int end,
                                       int len,
                                       float c1)
{
  c1 = 0.001 + (c1 * 0.998);
  c1 = (1 / (1 - c1)) - 1;
  int i, j;
  float f, e;
  float a = 1 / (1 + exp(c1));
  float b = 1 / (1 + exp(0 - c1));
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (float)len;
      e = 1 / (1 + exp(0 -((f - 0.5) * c1 * 2)));
      e = (e - a) / (b - a);
      arr[i]_E = e;
    }
}

//----------------------------------------------------------------------------//
inline void arr_curve_quadratic(ARR_TYPE *arr,
                                int start,
                                int end,
                                int len,
                                float c1,
                                float c2)
{
  if (c1 == 0.5)
    {
      c1 = 0.50001;
    }
  int i, j;
  float f, e;
  float a = 1 - (2 * c1);
  float b = c1 * c1;
  float c = 2 * c2;
  float d = 1 - (2 * c2);
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (float)len;
      e = (sqrt(b + (f * a)) - c1) / a;
      e = (d * e * e) + (c * e);
      arr[i]_E = e;
    }
}

//----------------------------------------------------------------------------//
inline void arr_curve_quartic(ARR_TYPE *arr,
                              int start,
                              int end,
                              int len,
                              float c1,
                              float c2)
{
  c1 = 1 - c1;
  int i, j;
  float f, e;
  float a = 1 - (2 * c1);
  float b = 2 * c1;
  float c = 1 - (2 * c2);
  float d = 2 * c2;
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (float)len;
      e = (a * pow(f, 2)) + (b * f);
      e = (c * pow(e, 2)) + (d * e);
      arr[i]_E = e;
    }
}

//----------------------------------------------------------------------------//
inline void arr_curve_simplified_cubic_seat(ARR_TYPE *arr,
                                            int start,
                                            int end,
                                            int len,
                                            float c1,
                                            float c2)
{
  c1 += 1e-6;
  c2 = 1 - c2;
  int i, j;
  float f, e;
  float a = (1 - c2) * c1;
  float b = 1 - c1;
  float c = 1 - c2;
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (float)len;
      if (f < c1)
        {
          e = (f * c2) + (a * (1 - pow(1 - (f / c1), 3)));
        }
      else
        {
          e = (f * c2) + (c * (c1  + (b * pow((f - c1) / b, 3))));
        }
      arr[i]_E = e;
    }
}

//----------------------------------------------------------------------------//
inline void arr_curve_simplified_quadratic(ARR_TYPE *arr,
                                           int start,
                                           int end,
                                           int len,
                                           float c1)
{
  c1 = c1 + c1;
  int i, j;
  float f, e;
  float a = c1;
  float b = c1;
  if (a > 1.0)   a = 1.0; // clip
  if (b < 1.0)   b = 1.0; // clip
  if (a == 0.5)  a = 0.50001;
  b = b - 1;
  float c = 1 - (2 * a);
  float d = a * a;
  float m = 2 * b;
  for (i = start, j = 0; i < end; i++, j++)
    {
      f = j / (float)len;
      e = (sqrt(d + (f * c)) - a) / c;
      e = (1 - m) * (e * e) + (m * e);
      arr[i]_E = e;
    }
}

