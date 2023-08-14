CC      = gcc
CFLAGS  = -Os -g -DMSDOS -DNO_MKDTEMP -DNO_TEMPDIR
LDFLAGS = -s

all: f2c.exe libf2c.a f2c.h

install: f2c.exe libf2c.a f2c.h
	cp f2c.exe  "$$W64DEVKIT_HOME"/bin/
	cp libf2c.a "$$W64DEVKIT_HOME"/$$(gcc -dumpmachine)/lib/
	cp f2c.h    "$$W64DEVKIT_HOME"/$$(gcc -dumpmachine)/include/

uninstall:
	rm -f "$$W64DEVKIT_HOME"/bin/f2c.exe \
	      "$$W64DEVKIT_HOME"/$$(gcc -dumpmachine)/lib/libf2c.a \
	      "$$W64DEVKIT_HOME"/$$(gcc -dumpmachine)/include/f2c.h

src_OBJx = \
  main.o init.o gram.o lex.o proc.o equiv.o data.o format.o expr.o     \
  exec.o intr.o io.o misc.o error.o mem.o names.o output.o p1output.o  \
  pread.o put.o putpcc.o vax.o formatdata.o parse_args.o niceprintf.o  \
  cds.o sysdep.o version.o
src_OBJ = $(addprefix src/,$(src_OBJx))

lib_OBJx = \
  f77vers.o i77vers.o main.o s_rnge.o abort_.o exit_.o getarg_.o       \
  iargc_.o getenv_.o signal_.o s_stop.o s_paus.o system_.o cabs.o      \
  ctype.o derf_.o derfc_.o erf_.o erfc_.o sig_die.o uninit.o pow_ci.o  \
  pow_dd.o pow_di.o pow_hh.o pow_ii.o pow_ri.o pow_zi.o pow_zz.o       \
  c_abs.o c_cos.o c_div.o c_exp.o c_log.o c_sin.o c_sqrt.o z_abs.o     \
  z_cos.o z_div.o z_exp.o z_log.o z_sin.o z_sqrt.o r_abs.o r_acos.o    \
  r_asin.o r_atan.o r_atn2.o r_cnjg.o r_cos.o r_cosh.o r_dim.o r_exp.o \
  r_imag.o r_int.o r_lg10.o r_log.o r_mod.o r_nint.o r_sign.o r_sin.o  \
  r_sinh.o r_sqrt.o r_tan.o r_tanh.o d_abs.o d_acos.o d_asin.o         \
  d_atan.o d_atn2.o d_cnjg.o d_cos.o d_cosh.o d_dim.o d_exp.o d_imag.o \
  d_int.o d_lg10.o d_log.o d_mod.o d_nint.o d_prod.o d_sign.o d_sin.o  \
  d_sinh.o d_sqrt.o d_tan.o d_tanh.o i_abs.o i_dim.o i_dnnt.o i_indx.o \
  i_len.o i_mod.o i_nint.o i_sign.o lbitbits.o lbitshft.o h_abs.o      \
  h_dim.o h_dnnt.o h_indx.o h_len.o h_mod.o h_nint.o h_sign.o l_ge.o   \
  l_gt.o l_le.o l_lt.o hl_ge.o hl_gt.o hl_le.o hl_lt.o ef1asc_.o       \
  ef1cmc_.o f77_aloc.o s_cat.o s_cmp.o s_copy.o backspac.o close.o     \
  dfe.o dolio.o due.o endfile.o err.o fmt.o fmtlib.o ftell_.o iio.o    \
  ilnw.o inquire.o lread.o lwrite.o open.o rdfmt.o rewind.o rsfe.o     \
  rsli.o rsne.o sfe.o sue.o typesize.o uio.o util.o wref.o wrtfmt.o    \
  wsfe.o wsle.o wsne.o xwsne.o pow_qq.o qbitbits.o qbitshft.o          \
  ftell64_.o dtime_.o etime_.o
lib_OBJ = $(addprefix lib/,$(lib_OBJx))

src/%.o: src/%.c
	$(CC) -c -o $@ $(CFLAGS) $<

lib/%.o: lib/%.c
	$(CC) -c -o $@ $(CFLAGS) -DSkip_f2c_Undefs $<

f2c.exe: $(src_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

libf2c.a: $(lib_OBJ)
	ar r libf2c.a $?

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

lib/main.o lib/s_paus.o lib/signal_.o: lib/signal1.h
lib/signal1.h: lib/signal1.h0
	cp $^ $@

lib/signbit.o lib/uninit.o: lib/arith.h
lib/arithchk.exe: lib/arithchk.c
	$(CC) -DNO_FPINIT -o $@ $^
lib/arith.h: lib/arithchk.exe
	$^ >$@

clean:
	rm -f $(src_OBJ) $(lib_OBJ) src/sysdep.hd f2c.exe libf2c.a f2c.h \
	      lib/f2c.h lib/signal1.h lib/sysdep1.h lib/arithchk.exe lib/arith.h
