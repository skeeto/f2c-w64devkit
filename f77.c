// f77 -- transparently invoke f2c and cc together (like fort77)
// $ gcc -nostartfiles -fno-builtin -o f77.exe f77.c
// $ f77 -Os -o example.exe example.f
// $ f77 -shared -O2 -o example.dll example.f
// This is free and unencumbered software released into the public domain.

#ifdef DEBUG
#  define assert(c) if (!(c)) __builtin_trap()
#else
#  define assert(c) (void)sizeof(c)
#endif

typedef _Bool bool;
typedef unsigned char    byte;
typedef __UINT8_TYPE__   u8;
typedef __UINT16_TYPE__  u16;
typedef __UINT32_TYPE__  u32;
typedef __INT32_TYPE__   i32;
typedef __PTRDIFF_TYPE__ size;
typedef __UINTPTR_TYPE__ uptr;
typedef u16 char16_t;  // for GDB
typedef char16_t c16;

typedef struct { uptr h; } handle;
typedef struct { uptr h; } *ign;
typedef struct {
  u32 cb;
  ign a, b, c;
  i32 d, e, f, g, h, i, j, k;
  u16 l, m;
  ign *n, *o, *p, *q;
} si;

typedef struct {
    handle *process;
    handle *thread;
    i32 pid;
    i32 tid;
} pi;

#define W32(r) __declspec(dllimport) r __stdcall
W32(i32)      CloseHandle(handle *);
W32(c16 **)   CommandLineToArgvW(c16 *, i32 *);
W32(i32)      CreateProcessW(c16*,c16*,ign,ign,i32,u32,ign,c16*,si*,pi*);
W32(i32)      DeleteFileW(c16 *);
W32(void)     ExitProcess(u32) __attribute((noreturn));
W32(c16 *)    GetCommandLineW(void);
W32(c16 *)    GetEnvironmentStringsW(void);
W32(i32)      GetExitCodeProcess(handle *, u32 *);
W32(handle *) GetStdHandle(u32);
W32(byte *)   VirtualAlloc(byte *, size, u32, u32);
W32(i32)      WaitForSingleObject(handle *, u32);
W32(byte *)   WriteConsoleW(handle *, c16 *, u32, u32 *, ign);
W32(byte *)   WriteFile(handle *, byte *, u32, u32 *, ign);

typedef struct {
    byte *mem;
    size cap;
    size off;
    uptr oom[5];
} arena;

static arena *newarena(byte *mem, size len)
{
    arena *a = 0;
    if (len >= (size)sizeof(*a)) {
        a = (arena *)mem;
        a->mem = mem;
        a->cap = len;
        a->off = sizeof(*a);
    }
    return a;
}

#define NEW(a, n, t) (t *)alloc(a, n, sizeof(t), _Alignof(t))
static byte *alloc(arena *a, size count, size sz, size align)
{
    assert(count >= 0);
    assert(align > 0);
    assert(sz > 0);
    size avail = a->cap - a->off;
    size pad = -a->off & (align - 1);
    if (count > (avail - pad)/sz) {
        __builtin_longjmp(a->oom, 1);
    }
    size total = count * sz;
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
    bool err;
} c16buf;

static c16buf *newbuf(arena *a, size cap)
{
    c16buf *cc = NEW(a, 1, c16buf);
    cc->cap = cap;
    cc->buf = NEW(a, cap, c16);
    return cc;
}

#define APPEND(dst, s) append(dst, s, sizeof(s)/2-1)
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
    bool simple = 1;
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

    bool simple = 1;
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

static bool matches(c16 *a, u8 *b)
{
    for (; *a && *b; a++, b++) {
        if (*a != *b) {
            return 0;
        }
    }
    return *a == *b;
}

static bool begins(c16 *a, u8 *b)
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
    var v = {0};
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

#define IMATCH(a, n, b) imatch(a, n, (u8 *)b, sizeof(b)-1)
static bool imatch(c16 *a, size alen, u8 *b, size blen)
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
    c16 *cpath = NEW(a, len+1, c16);
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
    handle *h = GetStdHandle(-10 - fd);
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

#define FATAL(err, msg) fatal(err, L##msg, sizeof(msg)-1)
static u32 fatal(c16buf *err, c16 *msg, size len)
{
    append(err, msg, len);
    appendc16(err, '\n');
    safewrite(2, err->buf, err->len);
    return 1;
}

static bool usage(i32 fd)
{
    handle *h = GetStdHandle(-10 - fd);
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
    return WriteFile(h, usage, sizeof(usage)-1, &dummy, 0);
}

static i32 f77main(i32 argc, c16 **argv)
{
    c16 errbuf[256];
    c16buf err[1] = {0};
    err->buf = errbuf;
    err->cap = sizeof(errbuf)/2;
    APPEND(err, L"f77: fatal: ");

    arena *perm = newarena(VirtualAlloc(0, 1<<24, 0x3000, 4), 1<<21);
    if (!perm || __builtin_setjmp(perm->oom)) {
        return FATAL(err, "out of memory");
    }

    arglist *inputs  = 0, **lastinput  = &inputs;
    arglist *outputs = 0, **lastoutput = &outputs;
    arglist *libs    = 0, **lastlib    = &libs;

    c16 *cmd_f2c = L"f2c";
    c16 *cmd_cc  = L"cc";
    c16 *env = GetEnvironmentStringsW();
    while (*env) {
        var v = parsevar(env);
        if (IMATCH(v.key, v.klen, "F2C")) {
            cmd_f2c = v.val;
        } else if (IMATCH(v.key, v.klen, "CC")) {
            cmd_cc = v.val;
        }
        env = v.next;
    }
    c16buf *f2c = newbuf(perm, 1<<14);
    appendcmd(f2c, cmd_f2c);
    c16buf *cc = newbuf(perm, 1<<14);
    appendcmd(cc, cmd_cc);

    bool dolink = 1;
    bool keep = 0;
    bool verbose = 0;
    bool stopargs = 0;

    i32 optind = 1;
    for (; optind < argc; optind++) {
        c16 *arg = argv[optind];
        if (stopargs || *arg != '-') {
            arglist *input = NEW(perm, 1, arglist);
            input->arg = arg;
            *lastinput = input;
            lastinput = &input->next;
        } else if (matches(arg, (u8 *)"--")) {
            stopargs = 1;
        } else if (begins(arg, (u8 *)"-C")) {
            appendarg(f2c, arg);
        } else if (begins(arg, (u8 *)"-D")) {
            appendarg(cc, arg);
        } else if (matches(arg, (u8 *)"-E")) {
            appendarg(cc, arg);
        } else if (begins(arg, (u8 *)"-I")) {
            appendarg(cc, arg);
        } else if (begins(arg, (u8 *)"-L")) {
            appendarg(cc, arg);
        } else if (begins(arg, (u8 *)"-O")) {
            appendarg(cc, arg);
        } else if (matches(arg, (u8 *)"-S")) {
            appendarg(cc, arg);
        } else if (begins(arg, (u8 *)"-U")) {
            appendarg(cc, arg);
        } else if (begins(arg, (u8 *)"-W")) {
            appendarg(cc, arg);
        } else if (begins(arg, (u8 *)"-Xf2c")) {
            if (++optind == argc) {
                return FATAL(err, "missing filename after '-Xf2c'");
            }
            appendarg(f2c, argv[optind]);
        } else if (matches(arg, (u8 *)"-a")) {
            appendarg(f2c, arg);
        } else if (matches(arg, (u8 *)"-c")) {
            dolink = 0;
            appendarg(cc, arg);
        } else if (begins(arg, (u8 *)"-f")) {
            appendarg(cc, arg);
        } else if (matches(arg, (u8 *)"-g")) {
            appendarg(cc, L"-g3");
            appendarg(f2c, arg);
        } else if (matches(arg, (u8 *)"--help")) {
            return !usage(1);
        } else if (matches(arg, (u8 *)"-k")) {
            keep = 1;
        } else if (begins(arg, (u8 *)"-l")) {
            arglist *lib = NEW(perm, 1, arglist);
            lib->arg = arg;
            *lastlib = lib;
            lastlib = &lib->next;
        } else if (begins(arg, (u8 *)"-m")) {
            appendarg(cc, arg);
        } else if (matches(arg, (u8 *)"-pipe")) {
            appendarg(cc, arg);
        } else if (begins(arg, (u8 *)"-o")) {
            appendarg(cc, arg);
            if (!arg[2]) {
                if (++optind == argc) {
                    return FATAL(err, "missing filename after '-o'");
                }
                appendarg(cc, argv[optind]);
            }
        } else if (matches(arg, (u8 *)"-s")) {
            appendarg(cc, arg);
        } else if (matches(arg, (u8 *)"-shared")) {
            appendarg(cc, arg);
        } else if (matches(arg, (u8 *)"-static")) {
            appendarg(cc, arg);
        } else if (matches(arg, (u8 *)"-trapuv")) {
            appendarg(f2c, arg);
        } else if (matches(arg, (u8 *)"-v")) {
            appendarg(cc, arg);
        } else if (matches(arg, (u8 *)"-w")) {
            appendarg(cc, arg);
            appendarg(f2c, arg);
        } else if (matches(arg, (u8 *)"-x")) {
            verbose = 1;
        } else {
            usage(2);
            APPEND(err, L"unknown option: ");
            appendstr(err, arg);
            return FATAL(err, "");
        }
    }

    if (!inputs) {
        return FATAL(err, "no input files");
    }

    // Finally append -lf2c. -lm would go last, but Mingw-w64 does not
    // have a separate math library.
    *lastlib = NEW(perm, 1, arglist);
    (*lastlib)->arg = L"-lf2c";

    u32 status = 0;
    for (arglist *input = inputs; input; input = input->next) {
        c16 *arg = input->arg;
        c16 *cpath = makecpath(perm, arg);

        c16buf cmd = *f2c;
        appendarg(&cmd, arg);
        appendc16(&cmd, 0);
        if (cmd.err) {
            status = FATAL(err, "f2c command too long");
            break;
        }
        if (verbose) {
            cmd.buf[cmd.len-1] = '\n';
            safewrite(2, cmd.buf, cmd.len);
            cmd.buf[cmd.len-1] = 0;
        }

        si si = {0};
        si.cb = sizeof(si);
        pi pi;
        if (!CreateProcessW(0, cmd.buf, 0, 0, 1, 0, 0, 0, &si, &pi)) {
            status = FATAL(err, "could not exec f2c");
            break;
        }
        WaitForSingleObject(pi.process, -1);
        GetExitCodeProcess(pi.process, &status);
        CloseHandle(pi.thread);
        CloseHandle(pi.process);
        if (status) {
            break;
        }

        // f2c probably complained already, but double check
        if (!cpath) {
            APPEND(err, L"could not guess .c filename: ");
            appendstr(err, arg);
            status = FATAL(err, "");
            break;
        }
        appendarg(cc, cpath);
        arglist *output = NEW(perm, 1, arglist);
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
            status = FATAL(err, "cc command too long");
            break;
        }
        if (verbose) {
            cc->buf[cc->len-1] = '\n';
            safewrite(2, cc->buf, cc->len);
            cc->buf[cc->len-1] = 0;
        }

        si si = {0};
        si.cb = sizeof(si);
        pi pi;
        if (CreateProcessW(0, cc->buf, 0, 0, 1, 0, 0, 0, &si, &pi)) {
            WaitForSingleObject(pi.process, -1);
            GetExitCodeProcess(pi.process, &status);
            CloseHandle(pi.thread);
            CloseHandle(pi.process);
        } else {
            status = FATAL(err, "could not exec cc");
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
    i32 r = f77main(argc, argv);
    ExitProcess(r);
}
