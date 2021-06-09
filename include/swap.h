#define SWAP(BUF, E0, E1) \
  (BUF) = (E0);           \
  (E0) = (E1);            \
  (E1) = (BUF);
