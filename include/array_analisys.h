//----------------------------------------------------------------------------//
inline float sum(ARR_TYPE *arr, int start, int end)
{
  int i;
  float sum = 0;
  for (i = start; i < end; i++)
    {
      sum += arr[i]_E;
    }
  return(sum);
}

//----------------------------------------------------------------------------//
inline float mean_arith(ARR_TYPE *arr, int start, int end, int len)
{
  int i;
  float sum = 0;
  for (i = start; i < end; i++)
    {
      sum += arr[i]_E;
    }
  return(sum / len);
}

//----------------------------------------------------------------------------//
// TODO (zero)
inline float mean_geometric(ARR_TYPE *arr, int start, int end, int len)
{
  int i;
  float sum = 1;
  for (i = start; i < end; i++)
    {
      sum *= arr[i]_E;
    }
  if (sum < 0) sum = 0 - sum;
  sum = pow(sum, (1.0 / len));
  return(sum);
}

//----------------------------------------------------------------------------//
// TODO (zero)
inline float mean_harmonic(ARR_TYPE *arr, int start, int end, int len)
{
  int i;
  float sum = 0;
  for (i = start; i < end; i++)
    {
      sum += 1. / arr[i]_E;
    }
  return(len / sum);
}

//----------------------------------------------------------------------------//
inline float centroid(ARR_TYPE *arr, int start, int end)
{
  int i,j;
  float sum = 0;
  float sum_mul = 0;
  for (i = start, j = 0;  i < end;  i++, j++)
    {
      sum_mul += arr[i]_E * j;
      sum     += arr[i]_E;
    }
  if (sum == 0)
    {
      sum = 0;
    }
  else
    {
      sum = sum_mul / sum;
    }
  return(sum);
}

//----------------------------------------------------------------------------//
inline int find_value(ARR_TYPE *arr, int start, int end,
                      float value,
                      float tolerance)
{
  int i;
  int j = -1; // not found
  float min = value - tolerance;
  float max = value + tolerance;
  for (i = start; i < end; i++)
    {
      if (arr[i]_E >= min && arr[i]_E <= max)
        {
          j = i;
          break;
        }
    }
  return(j);
}

//----------------------------------------------------------------------------//
inline int find_cross_up(ARR_TYPE *arr, int start, int end,
                         float value,
                         float tolerance)
{
  int i;
  int j = -1; // not found
  float min = value - tolerance;
  float max = value + tolerance;
  for (i = start; i < end-1; i++)
    {
      if (arr[i]_E < min && arr[i+1]_E >= max)
        {
          j = i;
          break;
        }
    }
  return(j);
}

//----------------------------------------------------------------------------//
inline int find_cross_down(ARR_TYPE *arr, int start, int end,
                           float value,
                           float tolerance)
{
  int i;
  int j = -1; // not found
  float min = value - tolerance;
  float max = value + tolerance;
  for (i = start; i < end-1; i++)
    {
      if (arr[i]_E > min && arr[i+1]_E <= max)
        {
          j = i;
          break;
        }
    }
  return(j);
}

//----------------------------------------------------------------------------//
inline int find_periods(ARR_TYPE *arr, 
                        int       pos, 
                        int       end,
                        float     value,
                        int       min_len,
                        int      *st,
                        int      *ln)
{
  *st = -1;
  *ln = -1;
  while (pos < end)
    {
      if (arr[pos]_E < value && arr[pos+1]_E >= value)
        {
          if (*st == -1)
            {
              *st = pos;
            }
          else if (*ln == -1)
            {
              *ln = pos - *st;
              if (*ln < min_len)
                {
                  *ln = -1;
                }
              // found
              else
                {
                  break;
                }
            }
        }
      pos++;
    }
  // not found
  return(pos);
}

//----------------------------------------------------------------------------//
inline void find_minmax(ARR_TYPE *arr, int start, int end,
                        int *min_idx,
                        int *max_idx,
                        float *min,
                        float *max)
{
  int i;
  *min_idx = start;
  *max_idx = start;
  *min = arr[start]_E;
  *max = arr[start]_E;
  for (i = start+1; i < end; i++)
    {
      if (arr[i]_E < *min)
        {
          *min_idx = i;
          *min = arr[i]_E;
        } 
      else if (arr[i]_E > *max)
        {
          *max_idx = i;
          *max = arr[i]_E;
        }
   }
}

