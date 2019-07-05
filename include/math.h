#ifndef _MATH_H_
#define _MATH_H_

#include <sys/cdefs.h>

#define HUGE_VAL __builtin_huge_val()

__BEGIN_DECLS
double acos(double);
double asin(double);
double atan(double);
double atan2(double, double);
double cos(double);
double sin(double);
double tan(double);

double cosh(double);
double sinh(double);
double tanh(double);

double exp(double);
double exp2(double);
double frexp(double, int *);
double ldexp(double, int);
double log(double);
double log10(double);
double modf(double, double *);

double pow(double, double);
double sqrt(double);

double ceil(double);
double fabs(double);
double floor(double);
double fmod(double, double);

double copysign(double, double);
double scalbn(double, int);
double scalbln(double, long);

double expm1(double);
__END_DECLS

/* 7.12.3.3 int isinf(real-floating x) */
#define isinf(__x) __builtin_isinf(__x)

/* 7.12.3.4 int isnan(real-floating x) */
#define isnan(__x) __builtin_isnan(__x)

#endif /* !_MATH_H_ */
