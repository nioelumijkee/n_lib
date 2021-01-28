#include <math.h>
#include "constant.h"
#include "math.h"

#define AF_WINDOWING_NONE(START, END, ARRAY, POSTFX, BUF_I)	\
  for (BUF_I = START; BUF_I < END; BUF_I++)			\
    ARRAY[BUF_I]POSTFX = 1.;

#define AF_WINDOWING_BARTLETT(START, LEN, ARRAY, POSTFX, BUF_I, BUF_F0, BUF_F1, COEF) \
  BUF_F0 = LEN / 2.;							\
  for (BUF_I = 0; BUF_I < LEN; BUF_I++)					\
    {									\
      BUF_F1 = BUF_I - BUF_F0;						\
      BUF_F1 = BUF_F1 / BUF_F0;						\
      if (BUF_F1 < 0)							\
	BUF_F1 = 0. - BUF_F1;						\
      ARRAY[BUF_I + START]POSTFX = (1. - BUF_F1) * COEF;	\
    }

#define AF_WINDOWING_BLACKMAN(START, LEN, ARRAY, POSTFX, BUF_I, BUF_F0, BUF_F1, BUF_F2, COEF) \
  BUF_F0 = LEN / 2.;							\
  for (BUF_I = 0; BUF_I < LEN; BUF_I++)					\
    {									\
      BUF_F1 = BUF_I - BUF_F0;						\
      BUF_F1 = BUF_F1 / BUF_F0;						\
      BUF_F2 = 0.5 * cos(BUF_F1 * AC_PI);				\
      BUF_F1 = 0.08 * cos(BUF_F1 * AC_2PI);				\
      ARRAY[BUF_I + START]POSTFX = (0.42 + BUF_F1 + BUF_F2) * COEF;	\
    }

#define AF_WINDOWING_CONNES(START, LEN, ARRAY, POSTFX, BUF_I, BUF_F0, BUF_F1, COEF) \
  BUF_F0 = LEN / 2.;							\
  for (BUF_I = 0; BUF_I < LEN; BUF_I++)					\
    {									\
      BUF_F1 = BUF_I - BUF_F0;						\
      BUF_F1 = BUF_F1 / BUF_F0;						\
      BUF_F1 = 1. - (BUF_F1 * BUF_F1);					\
      ARRAY[BUF_I + START]POSTFX = (BUF_F1 * BUF_F1) * COEF;		\
    }

#define AF_WINDOWING_GAUSSIAN(START, LEN, ARRAY, POSTFX, BUF_I, BUF_F0, BUF_F1, COEF) \
  BUF_F0 = LEN / 2.;							\
  if (COEF == 0)							\
    {									\
      for (BUF_I = 0; BUF_I < LEN; BUF_I++)				\
	{								\
	  ARRAY[BUF_I + START]POSTFX = 0.;				\
	}								\
    }									\
  else									\
    {									\
      for (BUF_I = 0; BUF_I < LEN; BUF_I++)				\
	{								\
	  BUF_F1 = BUF_I - BUF_F0;					\
	  BUF_F1 = BUF_F1 / BUF_F0;					\
	  BUF_F1 = BUF_F1 / COEF;					\
	  ARRAY[BUF_I + START]POSTFX = pow(2., (BUF_F1 * BUF_F1 * -1));	\
	}								\
    }

#define AF_WINDOWING_HANNING(START, LEN, ARRAY, POSTFX, BUF_I, BUF_F0, BUF_F1, COEF) \
  BUF_F0 = LEN / 2.;							\
  for (BUF_I = 0; BUF_I < LEN; BUF_I++)					\
    {									\
      BUF_F1 = BUF_I - BUF_F0;						\
      BUF_F1 = BUF_F1 / BUF_F0;						\
      ARRAY[BUF_I + START]POSTFX = (cos(BUF_F1 * AC_PI) + 1.) * 0.5 * COEF; \
    }

#define AF_WINDOWING_HAMMING(START, LEN, ARRAY, POSTFX, BUF_I, BUF_F0, BUF_F1, COEF) \
  BUF_F0 = LEN / 2.;							\
  for (BUF_I = 0; BUF_I < LEN; BUF_I++)					\
    {									\
      BUF_F1 = BUF_I - BUF_F0;						\
      BUF_F1 = BUF_F1 / BUF_F0;						\
      ARRAY[BUF_I + START]POSTFX = (0.54 + (cos(BUF_F1 * AC_PI) * 0.46)) * COEF; \
    }

#define AF_WINDOWING_LANCZOS(START, LEN, ARRAY, POSTFX, BUF_I, BUF_F0, BUF_F1, COEF) \
  BUF_F0 = LEN / 2.;							\
  for (BUF_I = 0; BUF_I < LEN; BUF_I++)					\
    {									\
      BUF_F1 = BUF_I - BUF_F0;						\
      BUF_F1 = BUF_F1 / BUF_F0;						\
      if (BUF_F1 == 0)							\
	ARRAY[BUF_I + START]POSTFX = COEF;				\
      else								\
	{								\
	  BUF_F1 = BUF_F1 * AC_PI;					\
	  ARRAY[BUF_I + START]POSTFX = (sin(BUF_F1) / BUF_F1) * COEF;	\
	}								\
    }

#define AF_WINDOWING_SIN(START, LEN, ARRAY, POSTFX, BUF_I, BUF_F0, COEF) \
  for (BUF_I = 0; BUF_I < LEN; BUF_I++)					\
    {									\
      BUF_F0 = BUF_I / (float)LEN;					\
      ARRAY[BUF_I + START]POSTFX = (sin(BUF_F0 * AC_PI)) * COEF;	\
    }

#define AF_WINDOWING_WELCH(START, LEN, ARRAY, POSTFX, BUF_I, BUF_F0, BUF_F1, COEF) \
  BUF_F0 = LEN / 2.;							\
  for (BUF_I = 0; BUF_I < LEN; BUF_I++)					\
    {									\
      BUF_F1 = BUF_I - BUF_F0;						\
      BUF_F1 = BUF_F1 / BUF_F0;						\
      ARRAY[BUF_I + START]POSTFX = (1. - (BUF_F1 * BUF_F1)) * COEF;	\
    }

