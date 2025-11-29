inline char hex_digit(const uint8_t bb)
{
  uint8_t b = bb & 0x0f;
  if (b > 9) {
    return ('a'+b-10);
  } else {
    return ('0'+b);
  }
}

inline uint8_t char2hex(const char c)
{
  if        (('0' <= c) && (c <= '9')) {
    return  (c - '0');
  } else if (('a' <= c) && (c <= 'f')) {
    return  (c - 'a' + 10);
  } else if (('A' <= c) && (c <= 'F')) {
    return  (c - 'A' + 10);
  }
  return 0;
}
