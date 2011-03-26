
#ifndef UTILS_H
#define UTILS_H

#include "config.h"

void err(const char* prompt, ...);
int number_cpus(void);

int random_int(int max);

extern char* progname;

#define min(A, B) (((A) < (B)) ? (A) : (B))
#define max(A, B) (((A) > (B)) ? (A) : (B))

#endif /* UTILS_H */
