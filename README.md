# f2c for [w64devkit][]

The main feature is the GNUmakefile in the repository root. The rest of
the source tree is unmodified [f2c][]. The `install` target builds and
installs `f2c.exe`, `libf2c.a`, and `f2c.h` into w64devkit. It's then
available to compile [FORTRAN 77][] programs without the "fortran"
w64devkit variant.

    $ make -j install

Unlike the upstream makefiles, parallel builds work properly. This build
is configured specifically for w64devkit and is not designed for use in
other environments, not even cross-compilation.

## Features and invocation

The `INTEGER*8` 64-bit integer type is enabled by default and available
without special handling. To build a FORTRAN program:

    $ f2c -g example.f
    $ cc -g3 -o example.exe example.c -lf2c

Using `-g` permits a degree of FORTRAN-level debugging. GDB will display
the FORTRAN source and operate in FORTRAN mode. However, it's not entirely
smooth. The C translation leaks into identifiers, and most line-oriented
features only apply to the underlying C source.


[f2c]: https://netlib.org/f2c/
[FORTRAN 77]: https://www.star.le.ac.uk/~cgp/prof77.pdf
[w64devkit]: https://github.com/skeeto/w64devkit
