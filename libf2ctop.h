// libf2c -- runtime library for f2c
//
// Build:
//   $ cc -c -O libf2c.c
//   $ ar r libf2c.a libf2c.o
// Usage:
//   $ f2c example.f
//   $ cc -O example.c -lf2c
// Or skipping the static library:
//   $ f2c example.f
//   $ cc -O example.c libf2c.c
// Or embedding the runtime (most optimized):
//   $ f2c example.f
//   $ sed -i 's/f2c\.h/libf2c.c/' example.c
//   $ cc -O -fwhole-program example.c
//
// Copyright 1990, 1993, 1994, 2000 by AT&T, Lucent Technologies and Bellcore.
//
// Permission to use, copy, modify, and distribute this software and its
// documentation for any purpose and without fee is hereby granted,
// provided that the above copyright notice appear in all copies and that
// both that the copyright notice and this permission notice and warranty
// disclaimer appear in supporting documentation, and that the names of
// AT&T, Bell Laboratories, Lucent or Bellcore or any of their entities
// not be used in advertising or publicity pertaining to distribution of
// the software without specific, written prior permission.
//
// AT&T, Lucent and Bellcore disclaim all warranties with regard to this
// software, including all implied warranties of merchantability and
// fitness.  In no event shall AT&T or Bellcore be liable for any
// special, indirect or consequential damages or any damages whatsoever
// resulting from loss of use, data or profits, whether in an action of
// contract, negligence or other tortious action, arising out of or in
// connection with the use or performance of this software.
#define MSDOS
#define Allow_TYQUAD
#define GCC_COMPARE_BUG_FIXED
#define My_ctype_DEF
# include <errno.h>
# include <float.h>
# include <math.h>
# include <signal.h>
# include <stddef.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <time.h>
# include <unistd.h>
# include "f2c.h"
