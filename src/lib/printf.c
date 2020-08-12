#include <console.h>
#include <printf.h>

typedef unsigned int u32;

#define LDOUBLE long double
#define LLONG long long

static u32 dopr(char *buffer, u32 maxlen, const char *format, va_list args);
static void fmtstr(char *buffer, u32 *currlen, u32 maxlen, char *value,
                   int flags, int min, int max);
static void fmtint(char *buffer, u32 *currlen, u32 maxlen, long value, int base,
                   int min, int max, int flags);
static void dopr_outch(char *buffer, u32 *currlen, u32 maxlen, char c);

/*
 * dopr(): poor man's version of doprintf
 */

/* format read states */
#define DP_S_DEFAULT 0
#define DP_S_FLAGS 1
#define DP_S_MIN 2
#define DP_S_DOT 3
#define DP_S_MAX 4
#define DP_S_MOD 5
#define DP_S_CONV 6
#define DP_S_DONE 7

/* format flags - Bits */
#define DP_F_MINUS (1 << 0)
#define DP_F_PLUS (1 << 1)
#define DP_F_SPACE (1 << 2)
#define DP_F_NUM (1 << 3)
#define DP_F_ZERO (1 << 4)
#define DP_F_UP (1 << 5)
#define DP_F_UNSIGNED (1 << 6)

/* Conversion Flags */
#define DP_C_SHORT 1
#define DP_C_LONG 2
#define DP_C_LDOUBLE 3
#define DP_C_LLONG 4

#define char_to_int(p) ((p) - '0')
#ifndef MAX
#define MAX(p, q) (((p) >= (q)) ? (p) : (q))
#endif

static u32 dopr(char *buffer, u32 maxlen, const char *format, va_list args)
{
    char ch;
    LLONG value;
    char *strvalue;
    int min;
    int max;
    int state;
    int flags;
    int cflags;
    u32 currlen;

    state = DP_S_DEFAULT;
    currlen = flags = cflags = min = 0;
    max = -1;
    ch = *format++;

    while (state != DP_S_DONE) {
        if (ch == '\0') state = DP_S_DONE;

        switch (state) {
            case DP_S_DEFAULT:
                if (ch == '%')
                    state = DP_S_FLAGS;
                else
                    dopr_outch(buffer, &currlen, maxlen, ch);
                ch = *format++;
                break;
            case DP_S_FLAGS:
                switch (ch) {
                    case '-':
                        flags |= DP_F_MINUS;
                        ch = *format++;
                        break;
                    case '+':
                        flags |= DP_F_PLUS;
                        ch = *format++;
                        break;
                    case ' ':
                        flags |= DP_F_SPACE;
                        ch = *format++;
                        break;
                    case '#':
                        flags |= DP_F_NUM;
                        ch = *format++;
                        break;
                    case '0':
                        flags |= DP_F_ZERO;
                        ch = *format++;
                        break;
                    default:
                        state = DP_S_MIN;
                        break;
                }
                break;
            case DP_S_MIN: {
                if ((unsigned char)ch >= '0' && (unsigned char)ch <= '9') {
                    min = 10 * min + char_to_int(ch);
                    ch = *format++;
                } else if (ch == '*') {
                    min = va_arg(args, int);
                    ch = *format++;
                    state = DP_S_DOT;
                } else {
                    state = DP_S_DOT;
                }
            } break;
            case DP_S_DOT:
                if (ch == '.') {
                    state = DP_S_MAX;
                    ch = *format++;
                } else {
                    state = DP_S_MOD;
                }
                break;
            case DP_S_MAX: {
                if ((unsigned char)ch >= '0' && (unsigned char)ch <= '9') {
                    if (max < 0) max = 0;
                    max = 10 * max + char_to_int(ch);
                    ch = *format++;
                } else if (ch == '*') {
                    max = va_arg(args, int);
                    ch = *format++;
                    state = DP_S_MOD;
                } else {
                    state = DP_S_MOD;
                }
            } break;
            case DP_S_MOD:
                switch (ch) {
                    case 'h':
                        cflags = DP_C_SHORT;
                        ch = *format++;
                        break;
                    case 'l':
                        cflags = DP_C_LONG;
                        ch = *format++;
                        if (ch == 'l') { /* It's a long long */
                            cflags = DP_C_LLONG;
                            ch = *format++;
                        }
                        break;
                    case 'L':
                        cflags = DP_C_LDOUBLE;
                        ch = *format++;
                        break;
                    default:
                        break;
                }
                state = DP_S_CONV;
                break;
            case DP_S_CONV:
                switch (ch) {
                    case 'd':
                    case 'i':
                        if (cflags == DP_C_SHORT)
                            value = va_arg(args, int);
                        else if (cflags == DP_C_LONG)
                            value = va_arg(args, long int);
                        else if (cflags == DP_C_LLONG)
                            value = va_arg(args, LLONG);
                        else
                            value = va_arg(args, int);
                        fmtint(buffer, &currlen, maxlen, value, 10, min, max,
                               flags);
                        break;
                    case 'o':
                        flags |= DP_F_UNSIGNED;
                        if (cflags == DP_C_SHORT)
                            value = va_arg(args, unsigned int);
                        else if (cflags == DP_C_LONG)
                            value = (long)va_arg(args, unsigned long int);
                        else if (cflags == DP_C_LLONG)
                            value = (long)va_arg(args, unsigned LLONG);
                        else
                            value = (long)va_arg(args, unsigned int);
                        fmtint(buffer, &currlen, maxlen, value, 8, min, max,
                               flags);
                        break;
                    case 'u':
                        flags |= DP_F_UNSIGNED;
                        if (cflags == DP_C_SHORT)
                            value = va_arg(args, unsigned int);
                        else if (cflags == DP_C_LONG)
                            value = (long)va_arg(args, unsigned long int);
                        else if (cflags == DP_C_LLONG)
                            value = (LLONG)va_arg(args, unsigned LLONG);
                        else
                            value = (long)va_arg(args, unsigned int);
                        fmtint(buffer, &currlen, maxlen, value, 10, min, max,
                               flags);
                        break;
                    case 'X':
                        flags |= DP_F_UP;
                    case 'x':
                        flags |= DP_F_UNSIGNED;
                        if (cflags == DP_C_SHORT)
                            value = va_arg(args, unsigned int);
                        else if (cflags == DP_C_LONG)
                            value = (long)va_arg(args, unsigned long int);
                        else if (cflags == DP_C_LLONG)
                            value = (LLONG)va_arg(args, unsigned LLONG);
                        else
                            value = (long)va_arg(args, unsigned int);
                        fmtint(buffer, &currlen, maxlen, value, 16, min, max,
                               flags);
                        break;
                        //                    case 'f':
                        //                        if (cflags == DP_C_LDOUBLE)
                        //                            fvalue = va_arg (args,
                        //                            LDOUBLE);
                        //                        else
                        //                            fvalue = va_arg (args,
                        //                            double);
                        //                        /* um, floating point? */
                        //                        //fmtfp (buffer, &currlen,
                        //                        maxlen, fvalue, min, max,
                        //                        flags); break;
                    case 'E':
                        flags |= DP_F_UP;
                        //                    case 'e':
                        //                        if (cflags == DP_C_LDOUBLE)
                        //                            fvalue = va_arg (args,
                        //                            LDOUBLE);
                        //                        else
                        //                            fvalue = va_arg (args,
                        //                            double);
                        //                        break;
                    case 'G':
                        flags |= DP_F_UP;
                        //                    case 'g':
                        //                        if (cflags == DP_C_LDOUBLE)
                        //                            fvalue = va_arg (args,
                        //                            LDOUBLE);
                        //                        else
                        //                            fvalue = va_arg (args,
                        //                            double);
                        //                        break;
                    case 'c':
                        dopr_outch(buffer, &currlen, maxlen, va_arg(args, int));
                        break;
                    case 's':
                        strvalue = va_arg(args, char *);
                        if (max == -1) {
                            u32 r;
                            for (r = 0; strvalue[r] != '\0'; r++)
                                ;
                            max = (int)r;
                        }
                        if (min > 0 && max >= 0 && min > max) max = min;
                        fmtstr(buffer, &currlen, maxlen, strvalue, flags, min,
                               max);
                        break;
                    case 'p':
                        strvalue = va_arg(args, void *);
                        fmtint(buffer, &currlen, maxlen, (long)strvalue, 16,
                               min, max, flags);
                        break;
                    case 'n':
                        if (cflags == DP_C_SHORT) {
                            short int *num;
                            num = va_arg(args, short int *);
                            *num = currlen;
                        } else if (cflags == DP_C_LONG) {
                            long int *num;
                            num = va_arg(args, long int *);
                            *num = (long int)currlen;
                        } else if (cflags == DP_C_LLONG) {
                            LLONG *num;
                            num = va_arg(args, LLONG *);
                            *num = (LLONG)currlen;
                        } else {
                            int *num;
                            num = va_arg(args, int *);
                            *num = (int)currlen;
                        }
                        break;
                    case '%':
                        dopr_outch(buffer, &currlen, maxlen, ch);
                        break;
                    case 'w':
                        /* not supported yet, treat as next char */
                        ch = *format++;
                        break;
                    default:
                        /* Unknown, skip */
                        break;
                }
                ch = *format++;
                state = DP_S_DEFAULT;
                flags = cflags = min = 0;
                max = -1;
                break;
            case DP_S_DONE:
                break;
            default:
                /* hmm? */
                break; /* some picky compilers need this */
        }
    }
    if (maxlen != 0) {
        if (currlen < maxlen - 1)
            buffer[currlen] = '\0';
        else if (maxlen > 0)
            buffer[maxlen - 1] = '\0';
    }

    return currlen;
}

static void fmtstr(char *buffer, u32 *currlen, u32 maxlen, char *value,
                   int flags, int min, int max)
{
    int padlen, strln; /* amount to pad */
    int cnt = 0;

    if (value == 0) {
        value = "<NULL>";
    }

    for (strln = 0; value[strln]; ++strln)
        ; /* strlen */
    padlen = min - strln;
    if (padlen < 0) padlen = 0;
    if (flags & DP_F_MINUS) padlen = -padlen; /* Left Justify */

    while ((padlen > 0) && (cnt < max)) {
        dopr_outch(buffer, currlen, maxlen, ' ');
        --padlen;
        ++cnt;
    }
    while (*value && (cnt < max)) {
        dopr_outch(buffer, currlen, maxlen, *value++);
        ++cnt;
    }
    while ((padlen < 0) && (cnt < max)) {
        dopr_outch(buffer, currlen, maxlen, ' ');
        ++padlen;
        ++cnt;
    }
}

/* Have to handle DP_F_NUM (ie 0x and 0 alternates) */

static void fmtint(char *buffer, u32 *currlen, u32 maxlen, long value, int base,
                   int min, int max, int flags)
{
    int signvalue = 0;
    unsigned long uvalue;
    char convert[20];
    int place = 0;
    int spadlen = 0; /* amount to space pad */
    int zpadlen = 0; /* amount to zero pad */
    int caps = 0;

    if (max < 0) max = 0;

    uvalue = value;

    if (!(flags & DP_F_UNSIGNED)) {
        if (value < 0) {
            signvalue = '-';
            uvalue = -value;
        } else {
            if (flags & DP_F_PLUS) /* Do a sign (+/i) */
                signvalue = '+';
            else if (flags & DP_F_SPACE)
                signvalue = ' ';
        }
    }

    if (flags & DP_F_UP) caps = 1; /* Should characters be upper case? */

    do {
        convert[place++] = (caps ? "0123456789ABCDEF"
                                 : "0123456789abcdef")[uvalue % (unsigned)base];
        uvalue = (uvalue / (unsigned)base);
    } while (uvalue && (place < 20));
    if (place == 20) place--;
    convert[place] = 0;

    zpadlen = max - place;
    spadlen = min - MAX(max, place) - (signvalue ? 1 : 0);
    if (zpadlen < 0) zpadlen = 0;
    if (spadlen < 0) spadlen = 0;
    if (flags & DP_F_ZERO) {
        zpadlen = MAX(zpadlen, spadlen);
        spadlen = 0;
    }
    if (flags & DP_F_MINUS) spadlen = -spadlen; /* Left Justifty */

    /* Spaces */
    while (spadlen > 0) {
        dopr_outch(buffer, currlen, maxlen, ' ');
        --spadlen;
    }

    /* Sign */
    if (signvalue) dopr_outch(buffer, currlen, maxlen, signvalue);

    /* Zeros */
    if (zpadlen > 0) {
        while (zpadlen > 0) {
            dopr_outch(buffer, currlen, maxlen, '0');
            --zpadlen;
        }
    }

    /* Digits */
    while (place > 0) dopr_outch(buffer, currlen, maxlen, convert[--place]);

    /* Left Justified spaces */
    while (spadlen < 0) {
        dopr_outch(buffer, currlen, maxlen, ' ');
        ++spadlen;
    }
}

static void dopr_outch(char *buffer, u32 *currlen, u32 maxlen, char c)
{
    if (*currlen < maxlen) {
        buffer[(*currlen)] = c;
    }
    (*currlen)++;
}

u32 vsnprintf(char *str, u32 count, const char *fmt, va_list args)
{
    return dopr(str, count, fmt, args);
}

u32 snprintf(char *str, u32 count, const char *fmt, ...)
{
    u32 ret;
    va_list ap;

    va_start(ap, fmt);
    ret = vsnprintf(str, count, fmt, ap);
    va_end(ap);
    return ret;
}

int printf(const char *fmt, ...)
{
    u32 len;
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    len = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (len < sizeof(buf)) {
        buf[len] = '\0';
        console_write(buf);
    }
    return len;
}