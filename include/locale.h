#ifndef _LOCALE_H_
#define _LOCALE_H_

#include <sys/cdefs.h>
#include <_locale.h>

struct lconv {
  char *decimal_point;
  char *thousands_sep;
  char *grouping;
  char *int_curr_symbol;
  char *currency_symbol;
  char *mon_decimal_point;
  char *mon_thousands_sep;
  char *mon_grouping;
  char *positive_sign;
  char *negative_sign;
  char int_frac_digits;
  char frac_digits;
  char p_cs_precedes;
  char p_sep_by_space;
  char n_cs_precedes;
  char n_sep_by_space;
  char p_sign_posn;
  char n_sign_posn;
  char int_p_cs_precedes;
  char int_n_cs_precedes;
  char int_p_sep_by_space;
  char int_n_sep_by_space;
  char int_p_sign_posn;
  char int_n_sign_posn;
};

#define LC_ALL 0
#define LC_COLLATE 1
#define LC_CTYPE 2
#define LC_MONETARY 3
#define LC_NUMERIC 4
#define LC_TIME 5
#define LC_MESSAGES 6
#define _LC_LAST 7 /* marks end */

__BEGIN_DECLS
struct lconv *localeconv(void);
char *setlocale(int, const char *);

#define LC_ALL_MASK ((int)~0)
#define LC_COLLATE_MASK ((int)(1 << LC_COLLATE))
#define LC_CTYPE_MASK ((int)(1 << LC_CTYPE))
#define LC_MONETARY_MASK ((int)(1 << LC_MONETARY))
#define LC_NUMERIC_MASK ((int)(1 << LC_NUMERIC))
#define LC_TIME_MASK ((int)(1 << LC_TIME))
#define LC_MESSAGES_MASK ((int)(1 << LC_MESSAGES))

struct lconv *localeconv_l(locale_t);

extern struct _locale _lc_global_locale;
#define LC_GLOBAL_LOCALE (&_lc_global_locale)
extern const struct _locale _lc_C_locale;
#define LC_C_LOCALE ((locale_t)__UNCONST(&_lc_C_locale))

__END_DECLS

#endif /* !_LOCALE_H_ */
