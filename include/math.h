//----------------------------------------------------------------------------//
// interpolation 2 points
#define AF_INTERPOL_2P(IN0, IN1, X, OUT)	\
  (OUT) = ((IN1) - (IN0)) * (X) + (IN0);

//----------------------------------------------------------------------------//
// interpolation 4 points
#define AF_INTERPOL_4P(IN0, IN1, IN2, IN3, X, BUF0_F, BUF1_F, BUF2_F, BUF3_F, OUT) \
  (BUF0_F) = ((IN2) - (IN0)) * 0.5;					\
  (BUF1_F) = (IN1) - (IN2);						\
  (BUF2_F) = (BUF0_F) + (BUF1_F);					\
  (BUF3_F) = (((IN3) - (IN1)) * 0.5) + (BUF2_F) + (BUF1_F);		\
  (OUT) = ((((((X) * (BUF3_F)) - ((BUF2_F) + (BUF3_F))) * (X)) + (BUF0_F)) * (X)) + (IN1);

//----------------------------------------------------------------------------//
// int fract
#define AF_INTFRACT(IN, INT0_I, INT1_I, FRACT_F)	\
  (INT0_I) = (IN);					\
  (INT1_I) = (INT0_I) + 1;				\
  (FRACT_F) = (IN) - (INT0_I);

//----------------------------------------------------------------------------//
// clip min max
#define AF_CLIP_MINMAX(MIN, MAX, IN) \
  if ((IN) < (MIN))		     \
    (IN) = (MIN);		     \
  else if ((IN) > (MAX))	     \
    (IN) = (MAX);

//----------------------------------------------------------------------------//
// clip min
#define AF_CLIP_MIN(MIN, IN) \
  if ((IN) < (MIN))	     \
    (IN) = (MIN);

//----------------------------------------------------------------------------//
// clip max
#define AF_CLIP_MAX(MAX, IN) \
  if ((IN) > (MAX))	     \
    (IN) = (MAX);

//----------------------------------------------------------------------------//
// abs
#define AF_ABS(IN, OUT)   \
  if ((IN) < 0)		  \
    (OUT) = 0 - (IN);	  \
  else			  \
    (OUT) = (IN);

//----------------------------------------------------------------------------//
// floor (round down)
#define AF_FLOOR(IN, OUT_I)			\
  (OUT_I) = (IN);

//----------------------------------------------------------------------------//
// ceil (round up)
#define AF_CEIL(IN, OUT_I)			\
  if ((IN) < 0)					\
    (OUT_I) = (IN) - 0.5;			\
  else						\
    (OUT_I) = (IN) + 0.5;

//----------------------------------------------------------------------------//
// wrap (-0.5 ... 0.5)
#define AF_WRAP(IN, BUF_I, OUT)			\
  if ((IN) < 0)					\
    (BUF_I) = (IN) - 0.5;			\
  else						\
    (BUF_I) = (IN) + 0.5;			\
  (OUT) = (IN) - (BUF_I);

