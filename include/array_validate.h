//----------------------------------------------------------------------------//
inline void arr_validate(int len_arr, int *start, int *end, int *len)
{
  // start: 0 ... len_arr-1
  if (*start < 0)
    {
      *start = len_arr + *start;
      if (*start < 0)
        {
          *start = 0;
        }
    }
  else if (*start >= len_arr)
    {
      *start = len_arr - 1;
    }
  
  // end: 0 ... len_arr
  if (*len <= 0)
    {
      *end = len_arr + *len;
      if (*end < 0)
        {
          *end = 0;
        }
    }
  else
    {
      *end = *start + *len;
    }
  if (*end > len_arr)
    {
      *end = len_arr;
    }
  
  // len:
  *len = *end - *start;
  if (*len < 1)
    {
      *len = 0;
    }
}

