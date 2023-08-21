CC      = gcc
AR      = ar
CFLAGS  = -O2 -g
LDFLAGS = -s

all: f2c.exe libf2c.a f2c.h f77.exe f2c.c libf2c.c

install: f2c.exe libf2c.a f2c.h f77.exe
	cp f2c.exe  "$$W64DEVKIT_HOME"/bin/
	cp libf2c.a "$$W64DEVKIT_HOME"/$$(gcc -dumpmachine)/lib/
	cp f2c.h    "$$W64DEVKIT_HOME"/$$(gcc -dumpmachine)/include/
	cp f77.exe  "$$W64DEVKIT_HOME"/bin/

uninstall:
	rm -f "$$W64DEVKIT_HOME"/bin/f2c.exe \
	      "$$W64DEVKIT_HOME"/$$(gcc -dumpmachine)/lib/libf2c.a \
	      "$$W64DEVKIT_HOME"/$$(gcc -dumpmachine)/include/f2c.h \
	      "$$W64DEVKIT_HOME"/bin/f2c.exe

src_OBJx = \
  main.o init.o gram.o lex.o proc.o equiv.o data.o format.o expr.o     \
  exec.o intr.o io.o misc.o error.o mem.o names.o output.o p1output.o  \
  pread.o put.o putpcc.o vax.o formatdata.o parse_args.o niceprintf.o  \
  cds.o sysdep.o version.o
src_OBJ = $(addprefix src/,$(src_OBJx))

lib_OBJx = \
  f77vers.o i77vers.o main.o s_rnge.o abort_.o exit_.o getarg_.o           \
  iargc_.o getenv_.o s_stop.o s_paus.o system_.o cabs.o ctype.o derf_.o    \
  derfc_.o erf_.o erfc_.o sig_die.o uninit.o pow_ci.o pow_dd.o pow_di.o    \
  pow_hh.o pow_ii.o pow_ri.o pow_zi.o pow_zz.o c_abs.o c_cos.o c_div.o     \
  c_exp.o c_log.o c_sin.o c_sqrt.o z_abs.o z_cos.o z_div.o z_exp.o z_log.o \
  z_sin.o z_sqrt.o r_abs.o r_acos.o r_asin.o r_atan.o r_atn2.o r_cnjg.o    \
  r_cos.o r_cosh.o r_dim.o r_exp.o r_imag.o r_int.o r_lg10.o r_log.o       \
  r_mod.o r_nint.o r_sign.o r_sin.o r_sinh.o r_sqrt.o r_tan.o r_tanh.o     \
  d_abs.o d_acos.o d_asin.o d_atan.o d_atn2.o d_cnjg.o d_cos.o d_cosh.o    \
  d_dim.o d_exp.o d_imag.o d_int.o d_lg10.o d_log.o d_mod.o d_nint.o       \
  d_prod.o d_sign.o d_sin.o d_sinh.o d_sqrt.o d_tan.o d_tanh.o i_abs.o     \
  i_dim.o i_dnnt.o i_indx.o i_len.o i_mod.o i_nint.o i_sign.o lbitbits.o   \
  lbitshft.o h_abs.o h_dim.o h_dnnt.o h_indx.o h_len.o h_mod.o h_nint.o    \
  h_sign.o l_ge.o l_gt.o l_le.o l_lt.o hl_ge.o hl_gt.o hl_le.o hl_lt.o     \
  ef1asc_.o ef1cmc_.o f77_aloc.o s_cat.o s_cmp.o s_copy.o backspac.o       \
  close.o dfe.o dolio.o due.o endfile.o err.o fmt.o fmtlib.o ftell_.o      \
  iio.o ilnw.o inquire.o lread.o lwrite.o open.o rdfmt.o rewind.o rsfe.o   \
  rsli.o rsne.o sfe.o sue.o typesize.o uio.o util.o wref.o wrtfmt.o wsfe.o \
  wsle.o wsne.o xwsne.o pow_qq.o qbitbits.o qbitshft.o ftell64_.o dtime_.o \
  etime_.o
lib_OBJ = $(addprefix lib/,$(lib_OBJx))

src/%.o: src/%.c
	$(CC) -c -o $@ $(CFLAGS) -DMSDOS -DNO_MKDTEMP -DNO_TEMPDIR $<

lib/%.o: lib/%.c
	$(CC) -c -o $@ $(CFLAGS) -DMSDOS -DAllow_TYQUAD -DGCC_COMPARE_BUG_FIXED $<

f2c.exe: $(src_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

libf2c.a: $(lib_OBJ)
	$(AR) r libf2c.a $^

f2c.h: lib/f2c.h
	cp $^ $@

src/sysdep.o: src/sysdep.hd
src/sysdep.hd:
	echo >$@

$(lib_OBJ): lib/f2c.h lib/sysdep1.h
lib/f2c.h: lib/f2c.h0
	sed 's/#ifdef INTEGER_STAR_8/#if 1/' $^ >$@
lib/sysdep1.h: lib/sysdep1.h0
	cp $^ $@

lib/main.o lib/s_paus.o: lib/signal1.h
lib/signal1.h: lib/signal1.h0
	cp $^ $@

lib/uninit.o: lib/arith.h
lib/arith.h:
	echo >$@

f77.exe: f77.c
	$(CC) -nostartfiles -fno-builtin $(CFLAGS) $(LDFLAGS) -o $@ $^

f2c_SRC = \
  ftypes.h defines.h machdefs.h niceprintf.h sysdep.h defs.h format.h     \
  iob.h names.h output.h p1defs.h parse.h pccdefs.h tokdefs.h usignal.h   \
  cds.c data.c equiv.c error.c exec.c expr.c format.c formatdata.c gram.c \
  init.c intr.c io.c lex.c main.c mem.c misc.c names.c niceprintf.c       \
  output.c p1output.c parse_args.c pread.c proc.c put.c putpcc.c sysdep.c \
  vax.c version.c

f2c.c: f2ctop.h $(addprefix src/,$(f2c_SRC))
	sed '/^#include/d' f2ctop.h $(addprefix src/,$(f2c_SRC)) | \
	  sed '/#pragma once/d' >$@

libf2c_SRC = \
  sysdep1.h arith.h ctype.h fio.h fp.h lio.h signal1.h abort_.c backspac.c \
  c_abs.c c_cos.c c_div.c c_exp.c c_log.c c_sin.c c_sqrt.c cabs.c close.c  \
  d_abs.c d_acos.c d_asin.c d_atan.c d_atn2.c d_cnjg.c d_cos.c d_cosh.c    \
  d_dim.c d_exp.c d_imag.c d_int.c d_lg10.c d_log.c d_mod.c d_nint.c       \
  d_prod.c d_sign.c d_sin.c d_sinh.c d_sqrt.c d_tan.c d_tanh.c derf_.c     \
  derfc_.c dolio.c dtime_.c due.c ef1asc_.c ef1cmc_.c endfile.c erf_.c     \
  erfc_.c etime_.c exit_.c f77_aloc.c f77vers.c fmtlib.c ftell64_.c        \
  ftell_.c getarg_.c getenv_.c h_abs.c h_dim.c h_dnnt.c h_indx.c h_len.c   \
  h_mod.c h_nint.c h_sign.c hl_ge.c hl_gt.c hl_le.c hl_lt.c i77vers.c      \
  i_abs.c i_dim.c i_dnnt.c i_indx.c i_len.c i_mod.c i_nint.c i_sign.c      \
  iargc_.c ilnw.c inquire.c l_ge.c l_gt.c l_le.c l_lt.c lbitbits.c         \
  lbitshft.c main.c open.c pow_ci.c pow_dd.c pow_di.c pow_hh.c pow_ii.c    \
  pow_qq.c pow_ri.c pow_zi.c pow_zz.c qbitbits.c qbitshft.c r_abs.c        \
  r_acos.c r_asin.c r_atan.c r_atn2.c r_cnjg.c r_cos.c r_cosh.c r_dim.c    \
  r_exp.c r_imag.c r_int.c r_lg10.c r_log.c r_mod.c r_nint.c r_sign.c      \
  r_sin.c r_sinh.c r_sqrt.c r_tan.c r_tanh.c rewind.c rsne.c s_cat.c       \
  s_cmp.c s_copy.c s_paus.c s_rnge.c s_stop.c sfe.c sig_die.c sue.c        \
  system_.c typesize.c uio.c uninit.c util.c z_abs.c z_cos.c z_div.c       \
  z_exp.c z_log.c z_sin.c z_sqrt.c fmt.h dfe.c err.c fmt.c iio.c lread.c   \
  lwrite.c rdfmt.c rsfe.c rsli.c wref.c wrtfmt.c wsfe.c wsle.c wsne.c      \
  xwsne.c

libf2c.c: libf2ctop.h $(addprefix lib/,$(libf2c_SRC))
	sed '/^#include/d' libf2ctop.h $(addprefix lib/,$(libf2c_SRC)) | \
	  sed -r '/#undef (abs|min|max)/d' >$@

clean:
	rm -f $(src_OBJ) $(lib_OBJ) \
	      f2c.exe libf2c.a f2c.h f77.exe f2c.c libf2c.c \
	      src/sysdep.hd lib/f2c.h lib/signal1.h lib/sysdep1.h lib/arith.h
