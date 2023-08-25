// f77 -- transparently invoke f2c and cc together (like fort77)
//   $ gcc -nostartfiles -fno-builtin -o f77.exe f77.c
//   $ f77 -Os -o example.exe example.f
//   $ f77 -shared -O2 -o example.dll example.f
// Enable assertions with -fsanitize=undefined -fsanitize-trap
// This is free and unencumbered software released into the public domain.

#define assert(c)    do if (!(c)) __builtin_unreachable(); while (0)
#define new(a, n, t) (t *)alloc(a, n, sizeof(t), _Alignof(t))
#define countof(a)   (size)(sizeof(a) / sizeof(*(a)))
#define lengthof(s)  (countof(s)-1)
#define tupleof(s)   s, lengthof(s)

typedef __UINT16_TYPE__  u16;
typedef __UINT32_TYPE__  u32;
typedef __INT32_TYPE__   i32;
typedef __INT32_TYPE__   b32;
typedef unsigned char    byte;
typedef __PTRDIFF_TYPE__ size;
typedef __UINTPTR_TYPE__ uptr;
typedef u16              wchar_t;  // for GDB
typedef wchar_t          c16;

typedef struct {} *handle;
typedef struct {} *null;

typedef struct {
  u32 cb;
  uptr a, b, c;
  i32 d, e, f, g, h, i, j, k;
  u16 l, m;
  uptr n, o, p, q;
} si;

typedef struct {
    handle process;
    handle thread;
    i32 pid;
    i32 tid;
} pi;

#define W32 __attribute((dllimport,stdcall))
W32 i32    CloseHandle(handle);
W32 c16  **CommandLineToArgvW(c16 *, i32 *);
W32 i32    CreateProcessW(c16*,c16*,null,null,i32,u32,c16*,c16*,si*,pi*);
W32 i32    DeleteFileW(c16 *);
W32 void   ExitProcess(u32) __attribute((noreturn));
W32 c16   *GetCommandLineW(void);
W32 c16   *GetEnvironmentStringsW(void);
W32 i32    GetExitCodeProcess(handle, u32 *);
W32 handle GetStdHandle(u32);
W32 byte  *VirtualAlloc(byte *, size, u32, u32);
W32 i32    WaitForSingleObject(handle, u32);
W32 i32    WriteConsoleW(handle, c16 *, u32, u32 *, null);
W32 i32    WriteFile(handle, byte *, u32, u32 *, null);

typedef struct {
    byte *mem;
    size  cap;
    size  off;
    void *oom[5];
} arena;

static arena *newarena(byte *mem, size len)
{
    assert(len >= (size)sizeof(arena));
    arena *a = (arena *)mem;
    a->mem = mem;
    a->cap = len;
    a->off = sizeof(*a);
    return a;
}

static byte *alloc(arena *a, size count, size objsize, size align)
{
    assert(count >= 0);
    assert(align > 0);
    assert(objsize > 0);
    size avail = a->cap - a->off;
    size pad = -a->off & (align - 1);
    if (count > (avail - pad)/objsize) {
        __builtin_longjmp(a->oom, 1);
    }
    size total = count * objsize;
    byte *p = a->mem + a->off + pad;
    for (size i = 0; i < total; i++) {
        p[i] = 0;
    }
    a->off += pad + total;
    return p;
}

typedef struct {
    c16 *buf;
    size cap;
    size len;
    b32  err;
} c16buf;

static c16buf *newbuf(arena *a, size cap)
{
    c16buf *cc = new(a, 1, c16buf);
    cc->cap = cap;
    cc->buf = new(a, cap, c16);
    return cc;
}

static void append(c16buf *dst, c16 *buf, size len)
{
    size avail = dst->cap - dst->len;
    size count = avail<len ? avail : len;
    c16 *p = dst->buf + dst->len;
    for (size i = 0; i < count; i++) {
        p[i] = buf[i];
    }
    dst->len += count;
    dst->err |= avail < len;
}

static void appendstr(c16buf *dst, c16 *buf)
{
    size len = 0;
    for (; buf[len]; len++) {}
    append(dst, buf, len);
}

static void appendc16(c16buf *dst, c16 c)
{
    append(dst, &c, 1);
}

static void appendcmd(c16buf *dst, c16 *arg)
{
    b32 simple = 1;
    for (c16 *s = arg; *s && simple; s++) {
        simple = *s!=' ' && *s!='\t';
    }
    if (simple) {
        appendstr(dst, arg);
    } else {
        appendc16(dst, '"');
        appendstr(dst, arg);
        appendc16(dst, '"');
    }
}

static void appendarg(c16buf *dst, c16 *arg)
{
    appendc16(dst, ' ');

    b32 simple = 1;
    for (size i = 0; simple && arg[i]; i++) {
        switch (arg[i]) {
        case '\n': case ' ': case '"':
            simple = 0;
        }
    }

    if (simple) {
        appendstr(dst, arg);
        return;
    }

    size backslash = 0;
    appendc16(dst, '"');
    for (size i = 0; i < arg[i]; i++) {
        switch (arg[i]) {
        case '\\':
            backslash++;
            break;
        case '"':
            for (size i = 0; i < backslash; i++) {
                appendc16(dst, '\\');
            }
            appendc16(dst, '\\');
            backslash = 0;
            break;
        default:
            backslash = 0;
        }
        appendc16(dst, arg[i]);
    }
    for (size i = 0; i < backslash; i++) {
        appendc16(dst, '\\');
    }
    appendc16(dst, '"');
}

static b32 matches(c16 *a, c16 *b)
{
    for (; *a && *b; a++, b++) {
        if (*a != *b) {
            return 0;
        }
    }
    return *a == *b;
}

static b32 begins(c16 *a, c16 *b)
{
    for (; *a && *b; a++, b++) {
        if (*a != *b) {
            return 0;
        }
    }
    return !*b;
}

typedef struct {
    c16 *key, *val;
    size klen, vlen;
    c16 *next;
} var;

static var parsevar(c16 *s)
{
    var v = {};
    v.key = s;
    for (; v.key[v.klen] && v.key[v.klen]!='='; v.klen++) {}
    v.val = v.key + v.klen + !!v.key[v.klen];
    for (; v.val[v.vlen]; v.vlen++) {}
    v.next = v.val + v.vlen + 1;
    return v;
}

static c16 upper(c16 c)
{
    return c>='a' && c<='z' ? (c16)(c+'A'-'a') : c;
}

static b32 imatch(c16 *a, size alen, c16 *b, size blen)
{
    if (alen != blen) {
        return 0;
    }
    for (size i = 0; i < alen; i++) {
        if (upper(a[i]) != b[i]) {
            return 0;
        }
    }
    return 1;
}

static c16 *makecpath(arena *a, c16 *fpath)
{
    size len = 0;
    for (; fpath[len]; len++) {}
    if (len<3 || fpath[len-2]!='.' || upper(fpath[len-1])!='F') {
        return 0;
    }
    c16 *cpath = new(a, len+1, c16);
    for (size i = 0; i < len-1; i++) {
        cpath[i] = fpath[i];
    }
    cpath[len-1] = 'c';
    return cpath;
}

typedef struct arglist {
    struct arglist *next;
    c16 *arg;
} arglist;

static void safewrite(i32 fd, c16 *buf, size len)
{
    assert((size)(u32)len == len);
    handle h = GetStdHandle(-10 - fd);
    u32 dummy;
    if (!WriteConsoleW(h, buf, (u32)len, &dummy, 0)) {
        // probably redirected to a file
        // TODO: UTF-8, and improve
        for (size i = 0; i < len; i++) {
            byte c = (byte)buf[i];
            WriteFile(h, &c, 1, &dummy, 0);
        }
    }
}

static u32 fatal(c16buf *err, c16 *msg, size len)
{
    append(err, msg, len);
    appendc16(err, '\n');
    safewrite(2, err->buf, err->len);
    return 1;
}

static b32 usage(i32 fd)
{
    handle h = GetStdHandle(-10 - fd);
    static byte usage[] =
    "usage: f77 [OPTIONS] <FILES...>\n"
    "  -a         automatic local variables (f2c)\n"
    "  -C         check subscript range (f2c)\n"
    "  -c         product object file, do not link (cc)\n"
    "  --help     print this usage message\n"
    "  -g         produce debugging information (f2c, cc)\n"
    "  -k         keep intermediate .c file\n"
    "  -Xf2c ARG  pass ARG through to f2c\n"
    "  -o FILE    write output to FILE (cc)\n"
    "  -trapuv    detect use of uninitialized floats (f2c)\n"
    "  -x         print f2c and cc invocations\n"
    "Environment variables:\n"
    "  CC=CMD     path/command for C compiler (cc)\n"
    "  F2C=CMD    path/command for f2c (f2c)\n"
    "Options passed through to cc:\n"
    "  -D* -E -f* -I* -L* -l* -m* -O* -pipe -S -s -shared\n"
    "  -static -U* -v -W* -w\n";
    u32 dummy;
    return WriteFile(h, usage, lengthof(usage), &dummy, 0);
}

static u32 f77main(i32 argc, c16 **argv)
{
    c16 errbuf[256];
    c16buf err[1] = {};
    err->buf = errbuf;
    err->cap = lengthof(errbuf);
    append(err, tupleof(L"f77: fatal: "));

    arena *perm = newarena(VirtualAlloc(0, 1<<24, 0x3000, 4), 1<<21);
    if (!perm || __builtin_setjmp(perm->oom)) {
        return fatal(err, tupleof(L"out of memory"));
    }

    arglist *inputs  = 0, **lastinput  = &inputs;
    arglist *outputs = 0, **lastoutput = &outputs;
    arglist *libs    = 0, **lastlib    = &libs;

    c16 *cmd_f2c = L"f2c";
    c16 *cmd_cc  = L"cc";
    c16 *env = GetEnvironmentStringsW();
    while (*env) {
        var v = parsevar(env);
        if (imatch(v.key, v.klen, tupleof(L"F2C"))) {
            cmd_f2c = v.val;
        } else if (imatch(v.key, v.klen, tupleof(L"CC"))) {
            cmd_cc = v.val;
        }
        env = v.next;
    }
    c16buf *f2c = newbuf(perm, 1<<14);
    appendcmd(f2c, cmd_f2c);
    c16buf *cc = newbuf(perm, 1<<14);
    appendcmd(cc, cmd_cc);

    b32 dolink = 1;
    b32 keep = 0;
    b32 verbose = 0;
    b32 stopargs = 0;

    i32 optind = 1;
    for (; optind < argc; optind++) {
        c16 *arg = argv[optind];
        if (stopargs || *arg != '-') {
            arglist *input = new(perm, 1, arglist);
            input->arg = arg;
            *lastinput = input;
            lastinput = &input->next;
        } else if (matches(arg, L"--")) {
            stopargs = 1;
        } else if (begins(arg, L"-C")) {
            appendarg(f2c, arg);
        } else if (begins(arg, L"-D")) {
            appendarg(cc, arg);
        } else if (matches(arg, L"-E")) {
            appendarg(cc, arg);
        } else if (begins(arg, L"-I")) {
            appendarg(cc, arg);
        } else if (begins(arg, L"-L")) {
            appendarg(cc, arg);
        } else if (begins(arg, L"-O")) {
            appendarg(cc, arg);
        } else if (matches(arg, L"-S")) {
            appendarg(cc, arg);
        } else if (begins(arg, L"-U")) {
            appendarg(cc, arg);
        } else if (begins(arg, L"-W")) {
            appendarg(cc, arg);
        } else if (begins(arg, L"-Xf2c")) {
            if (++optind == argc) {
                return fatal(err, tupleof(L"missing filename after '-Xf2c'"));
            }
            appendarg(f2c, argv[optind]);
        } else if (matches(arg, L"-a")) {
            appendarg(f2c, arg);
        } else if (matches(arg, L"-c")) {
            dolink = 0;
            appendarg(cc, arg);
        } else if (begins(arg, L"-f")) {
            appendarg(cc, arg);
        } else if (matches(arg, L"-g")) {
            appendarg(cc, L"-g3");
            appendarg(f2c, arg);
        } else if (matches(arg, L"--help")) {
            return !usage(1);
        } else if (matches(arg, L"-k")) {
            keep = 1;
        } else if (begins(arg, L"-l")) {
            arglist *lib = new(perm, 1, arglist);
            lib->arg = arg;
            *lastlib = lib;
            lastlib = &lib->next;
        } else if (begins(arg, L"-m")) {
            appendarg(cc, arg);
        } else if (matches(arg, L"-pipe")) {
            appendarg(cc, arg);
        } else if (begins(arg, L"-o")) {
            appendarg(cc, arg);
            if (!arg[2]) {
                if (++optind == argc) {
                    return fatal(err, tupleof(L"missing filename after '-o'"));
                }
                appendarg(cc, argv[optind]);
            }
        } else if (matches(arg, L"-s")) {
            appendarg(cc, arg);
        } else if (matches(arg, L"-shared")) {
            appendarg(cc, arg);
        } else if (matches(arg, L"-static")) {
            appendarg(cc, arg);
        } else if (matches(arg, L"-trapuv")) {
            appendarg(f2c, arg);
        } else if (matches(arg, L"-v")) {
            appendarg(cc, arg);
        } else if (matches(arg, L"-w")) {
            appendarg(cc, arg);
            appendarg(f2c, arg);
        } else if (matches(arg, L"-x")) {
            verbose = 1;
        } else {
            usage(2);
            append(err, tupleof(L"unknown option: "));
            appendstr(err, arg);
            return fatal(err, 0, 0);
        }
    }

    if (!inputs) {
        return fatal(err, tupleof(L"no input files"));
    }

    // Finally append -lf2c. -lm would go last, but Mingw-w64 does not
    // have a separate math library.
    *lastlib = new(perm, 1, arglist);
    (*lastlib)->arg = L"-lf2c";

    u32 status = 0;
    for (arglist *input = inputs; input; input = input->next) {
        c16 *arg = input->arg;
        c16 *cpath = makecpath(perm, arg);
        if (!cpath) {
            appendarg(cc, arg);
            continue;
        }

        c16buf cmd = *f2c;
        appendarg(&cmd, arg);
        appendc16(&cmd, 0);
        if (cmd.err) {
            status = fatal(err, tupleof(L"f2c command too long"));
            break;
        }
        if (verbose) {
            cmd.buf[cmd.len-1] = '\n';
            safewrite(2, cmd.buf, cmd.len);
            cmd.buf[cmd.len-1] = 0;
        }

        si si = {};
        si.cb = sizeof(si);
        pi pi;
        if (!CreateProcessW(0, cmd.buf, 0, 0, 1, 0, 0, 0, &si, &pi)) {
            status = fatal(err, tupleof(L"could not exec f2c"));
            break;
        }
        WaitForSingleObject(pi.process, -1);
        GetExitCodeProcess(pi.process, &status);
        CloseHandle(pi.thread);
        CloseHandle(pi.process);
        if (status) {
            break;
        }

        appendarg(cc, cpath);
        arglist *output = new(perm, 1, arglist);
        output->arg = cpath;
        *lastoutput = output;
        lastoutput = &output->next;
    }

    while (!status) {
        if (dolink) {
            for (arglist *lib = libs; lib; lib = lib->next) {
                appendarg(cc, lib->arg);
            }
        }
        appendc16(cc, 0);
        if (cc->err) {
            status = fatal(err, tupleof(L"cc command too long"));
            break;
        }
        if (verbose) {
            cc->buf[cc->len-1] = '\n';
            safewrite(2, cc->buf, cc->len);
            cc->buf[cc->len-1] = 0;
        }

        si si = {};
        si.cb = sizeof(si);
        pi pi;
        if (CreateProcessW(0, cc->buf, 0, 0, 1, 0, 0, 0, &si, &pi)) {
            WaitForSingleObject(pi.process, -1);
            GetExitCodeProcess(pi.process, &status);
            CloseHandle(pi.thread);
            CloseHandle(pi.process);
        } else {
            status = fatal(err, tupleof(L"could not exec cc"));
        }
        break;
    }

    if (!keep) {
        for (arglist *output = outputs; output; output = output->next) {
            DeleteFileW(output->arg);
        }
    }
    return status;
}

__attribute((force_align_arg_pointer))
void mainCRTStartup(void)
{
    c16 *cmdline = GetCommandLineW();
    i32 argc;
    c16 **argv = CommandLineToArgvW(cmdline, &argc);
    u32 r = f77main(argc, argv);
    ExitProcess(r);
}
