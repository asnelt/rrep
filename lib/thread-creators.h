/* ANSI-C code produced by gperf version 3.3 */
/* Command-line: gperf -m 10 thread-creators.gperf  */
/* Computed positions: -k'5' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gperf@gnu.org>."
#endif


#define TOTAL_KEYWORDS 13
#define MIN_WORD_LENGTH 8
#define MAX_WORD_LENGTH 14
#define MIN_HASH_VALUE 8
#define MAX_HASH_VALUE 20
/* maximum key range = 13, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
thread_creators_hash (register const char *str, register size_t len)
{
  static const unsigned char asso_values[] =
    {
      21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
      21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
      21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
      21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
      21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
      21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
      21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
      21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
      21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
      21, 21, 21, 21, 21,  9, 21, 21, 21, 21,
       6,  4,  5, 21, 21, 21, 21, 21,  3, 21,
      21,  8, 21, 21,  0, 21, 21, 21, 21,  0,
      21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
      21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
      21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
      21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
      21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
      21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
      21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
      21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
      21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
      21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
      21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
      21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
      21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
      21, 21, 21, 21, 21, 21
    };
  return len + asso_values[(unsigned char)str[4]];
}

struct stringpool_t
  {
    char stringpool_str8[sizeof("aio_read")];
    char stringpool_str9[sizeof("aio_write")];
    char stringpool_str10[sizeof("aio_read64")];
    char stringpool_str11[sizeof("aio_write64")];
    char stringpool_str12[sizeof("timer_create")];
    char stringpool_str13[sizeof("lio_listio")];
    char stringpool_str14[sizeof("aio_fsync")];
    char stringpool_str15[sizeof("lio_listio64")];
    char stringpool_str16[sizeof("aio_fsync64")];
    char stringpool_str17[sizeof("mq_notify")];
    char stringpool_str18[sizeof("pthread_create")];
    char stringpool_str19[sizeof("getaddrinfo_a")];
    char stringpool_str20[sizeof("thrd_create")];
  };
static const struct stringpool_t stringpool_contents =
  {
    "aio_read",
    "aio_write",
    "aio_read64",
    "aio_write64",
    "timer_create",
    "lio_listio",
    "aio_fsync",
    "lio_listio64",
    "aio_fsync64",
    "mq_notify",
    "pthread_create",
    "getaddrinfo_a",
    "thrd_create"
  };
#define stringpool ((const char *) &stringpool_contents)

static const int thread_creators_symbols[] =
  {
    -1, -1, -1, -1, -1, -1, -1, -1,
    (int)(size_t)&((struct stringpool_t *)0)->stringpool_str8,
    (int)(size_t)&((struct stringpool_t *)0)->stringpool_str9,
    (int)(size_t)&((struct stringpool_t *)0)->stringpool_str10,
    (int)(size_t)&((struct stringpool_t *)0)->stringpool_str11,
    (int)(size_t)&((struct stringpool_t *)0)->stringpool_str12,
    (int)(size_t)&((struct stringpool_t *)0)->stringpool_str13,
    (int)(size_t)&((struct stringpool_t *)0)->stringpool_str14,
    (int)(size_t)&((struct stringpool_t *)0)->stringpool_str15,
    (int)(size_t)&((struct stringpool_t *)0)->stringpool_str16,
    (int)(size_t)&((struct stringpool_t *)0)->stringpool_str17,
    (int)(size_t)&((struct stringpool_t *)0)->stringpool_str18,
    (int)(size_t)&((struct stringpool_t *)0)->stringpool_str19,
    (int)(size_t)&((struct stringpool_t *)0)->stringpool_str20
  };

static const char *
thread_creators_lookup (register const char *str, register size_t len)
{
  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register unsigned int key = thread_creators_hash (str, len);

      if (key <= MAX_HASH_VALUE)
        {
          register int o = thread_creators_symbols[key];
          if (o >= 0)
            {
              register const char *s = o + stringpool;

              if (*str == *s && !strcmp (str + 1, s + 1))
                return s;
            }
        }
    }
  return (const char *) 0;
}
