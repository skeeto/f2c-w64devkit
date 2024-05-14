# f2c for [w64devkit][]

The main feature is the GNUmakefile in the repository root. The rest of
the source tree is lightly modified [f2c][]. The `install` target builds
and installs `f2c.exe`, `libf2c.a`, `f2c.h`, and `f77.exe` into w64devkit.
It's then available to compile [FORTRAN 77][] programs without the
"fortran" w64devkit variant.

    $ make -j install

Unlike the upstream makefiles, parallel builds work properly. This build
is configured specifically for w64devkit and is not designed for use in
other environments.

## Features and invocation

The `INTEGER*8` 64-bit integer type is enabled by default and available
without special handling. To build a FORTRAN program:

    $ f2c -g example.f
    $ cc -g3 -fcommon -o example.exe example.c -lf2c

Using `-g` permits a degree of FORTRAN-level debugging. GDB will display
the FORTRAN source and operate in FORTRAN mode. However, it's not entirely
smooth. The C translation leaks into identifiers, and most line-oriented
features only apply to the underlying C source.

`-fcommon` is required when translation units share a `COMMON` block.

## Source amalgamation

This fork features a source amalgamation of both compiler, `f2c.c`, and
runtime, `libf2c.c`. Either is trivially compiled without a build system.
Even more, the runtime can now be embedded into the same translation unit
as the compiled FORTRAN, producing more optimal code, particularly when
compiled with `-fwhole-program`. Because the runtime compiles faster than
it links, it even *builds* faster as a unity build. Without a pre-compiled
runtime, the `f2c.h` configuration is also more easily changed on the fly.

    $ make f2c.c libf2c.c f2c.h

The catch is that f2c has egregious namespace hygiene. There is no rhyme
or reason behind its choice of identifiers, and it clashes with reserved
identifiers (e.g. in `math.h`). Even under conventional circumstances it
can be problematic, but it's worse when all in a single namespace. Though,
for similar reasons, this has revealed new f2c bugs.


[f2c]: https://netlib.org/f2c/
[FORTRAN 77]: https://www.star.le.ac.uk/~cgp/prof77.pdf
[w64devkit]: https://github.com/skeeto/w64devkit
