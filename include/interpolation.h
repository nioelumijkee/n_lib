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
