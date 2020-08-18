/*

    filp - Embeddable, Reverse Polish Notation Programming Language

    Angel Ortega <angel@triptico.com>

    This software is released into the public domain.
    NO WARRANTY. See file LICENSE for details.

    Filp function library.
    Level II (system dependent).

    All the system-specific stuff needing #ifdef must be here.

*/

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>

#include "filp.h"

#ifdef CONFOPT_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#ifdef __TURBOC__
/* borland C does not define these
   (other win32 compilers?) so comment them out,
   as they would not work anyway */
#define popen fopen
#define pclose fclose
#endif

#if defined(__WIN32__) || defined(WIN32)

#include <windows.h>

#define MKDIR(a,b) mkdir(a)

#else               /* non win32 */

#ifdef CONFOPT_GLOB_H
#include <glob.h>
#endif

#define MKDIR(a,b) mkdir(a,b)
#include <sys/time.h>
#include <unistd.h>

#endif

#ifdef CONFOPT_TERMIOS
#include <termios.h>
#endif

#ifdef CONFOPT_PCRE
#include <pcreposix.h>
#endif

#ifdef CONFOPT_SYSTEM_REGEX
#include <regex.h>
#endif

#ifdef CONFOPT_INCLUDED_REGEX
#include "gnu_regex.h"
#endif


/*******************
    Code
********************/

#define ASSERT_ISOLATE() if(_filp_isolate) \
    { _filp_error=FILPERR_PERMISSION_DENIED; return FILP_ERROR; }


static long _filp_get_timer(void)
/* high resolution timer for benchmarking */
{
#ifdef CONFOPT_GETTIMEOFDAY

    struct timeval tv;
    unsigned int ret;

    gettimeofday(&tv, NULL);
    ret = (tv.tv_sec * 1000000) + tv.tv_usec;

#else               /* using more portable but less accurate clock() */

    clock_t ret;

    ret = (clock() * 1000) / CLOCKS_PER_SEC;

#endif

    return (long) ret;
}


#ifdef CONFOPT_READLINE

/* readline completion support */

static char *_filp_readline_match(const char *text, int state)
{
    struct filp_val *v;

    v = filp_pop();

    if (v->type != FILP_NULL) {
        char *ptr = v->value;
        v->value = NULL;
        return ptr;
    }

    return (char *) NULL;
}


static char **_filp_readline_completion(const char *text, int start, int end)
{
    char *ptr;

    ptr = (char *) text;
    if (*ptr == '/')
        ptr++;
    filp_push_dict(ptr);

    return rl_completion_matches(text, _filp_readline_match);
}

#endif              /* CONFOPT_READLINE */


char *filp_readline(char *prompt)
/* GNU readline (subset) clone */
{
    static char line[4096];

#ifdef CONFOPT_READLINE
    char *ptr;

    rl_readline_name = "filp";
    rl_attempted_completion_function = _filp_readline_completion;

    if ((ptr = readline(prompt)) != NULL) {
        strncpy(line, ptr, sizeof(line) - 1);

        if (line[0] != '\0')
            add_history(line);

        free(ptr);
        return line;
    }

    return NULL;
#else

#ifdef CONFOPT_TERMIOS

    int i, n, m;
    unsigned char c;
    struct termios t, s;
    struct filp_val *v;
    int history = 0;

    tcgetattr(0, &s);
    memcpy(&t, &s, sizeof(t));
    t.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    t.c_oflag &= ~OPOST;
    t.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    t.c_cflag &= ~(CSIZE | PARENB);
    t.c_cflag |= CS8;

    tcsetattr(0, TCSANOW, &t);

    printf("%s", prompt);
    fflush(stdout);

    i = 0;

    for (;;) {
        read(0, &c, 1);

        if (c == 0x04 && i == 0) {
            /* ^D: end of file */
            line[0] = c;
            break;
        }
        else if (c == '\r' || c == '\n') {
            /* end of line */

            line[i] = '\0';
            break;
        }
        else if (c == '\b' || c == 0x7f) {
            /* delete char */

            if (i == 0)
                printf("\a");
            else {
                i--;
                printf("\b \b");
            }
        }
        else if (c == '\t') {
            /* Tab: auto completion */

            /* finds the word backwards */
            line[i] = '\0';
            i--;
            while (i > 0 && !filp_issep(line[i]))
                i--;
            if (i)
                i++;

            /* skips if $ or / */
            if (line[i] == '$' || line[i] == '/')
                i++;

            n = filp_push_dict(&line[i]);

            if (n == 0) {
                /* none: destroy NULL value */
                filp_pop();

                while (line[i] != '\0')
                    i++;
                printf("\a");
            }
            else if (n == 1) {
                /* just one value: complete */
                line[i] = '\0';
                printf("\r\n%s%s", prompt, line);

                v = filp_pop();
                for (m = 0; v->value[m] != '\0'; m++, i++) {
                    line[i] = v->value[m];
                    putchar(line[i]);
                }

                /* destroy null */
                filp_pop();

                /* separate */
                line[i++] = ' ';
                putchar(' ');
            }
            else {
                /* many values: print them */
                printf("\r\n");
                for (;;) {
                    v = filp_pop();
                    if (v->type == FILP_NULL)
                        break;

                    printf("%s ", v->value);
                }

                while (line[i] != '\0')
                    i++;

                printf("\r\n%s%s", prompt, line);
            }
        }
        else if (c == 0x15) {
            /* ^U: cancel all line */
            i = 0;
            line[0] = '\0';
            printf("\r\n%s", prompt);
        }
        else if (c == 0x0e || c == 0x10) {
            /* ^N: next in history */
            /* ^P: previous in history */

            m = history;
            if (c == 0x10)
                history++;
            else if (history > 1)
                history--;

            filp_execf("/filp_command_history %d @", history);
            v = filp_pop();

            if (v->type == FILP_NULL) {
                history = m;
                printf("\a");
            }
            else {
                while (i) {
                    printf("\b \b");
                    i--;
                }

                for (m = 0; v->value[m] != '\0'; m++, i++) {
                    line[i] = v->value[m];
                    putchar(line[i]);
                }
            }
        }
        else if (c >= ' ') {
            /* printable char */
            if (i < sizeof(line) - 1) {
                line[i++] = c;
                putchar(c);
            }
        }

        fflush(stdout);
    }

    tcsetattr(0, TCSANOW, &s);

    if (line[0] == 0x04)
        return NULL;

    printf("\n");

    if (line[0] != '\0') {
        filp_exec("/filp_command_history 1 ");
        filp_scalar_push(line);
        filp_exec("ains");
    }

    return line;

#else               /* CONFOPT_TERMIOS */

    printf("%s", prompt);

    return fgets(line, sizeof(line) - 1, stdin);

#endif              /* CONFOPT_TERMIOS */

#endif              /* CONFOPT_READLINE */
}


/* filp functions */

/**
 * getenv - Obtains the value of an environment string.
 * @envvar: the environment string name.
 *
 * Obtains the value of an environment string (as PATH or HOME).
 * Returns the value or NULL if the variable does not exist.
 * [System commands]
 */
static int _filpf_getenv(void)
/** @envvar getenv %value */
{
    struct filp_val *v;
    char *ptr;

    v = filp_pop();

    if ((ptr = getenv(v->value)) != NULL)
        filp_scalar_push(ptr);
    else
        filp_null_push();

    return FILP_OK;
}


/**
 * putenv - Sets the value of an environment string.
 * @envvar: the environment string name
 * @value: the scalar value to be stored
 *
 * Sets the value of an environment string.
 * [System commands]
 */
static int _filpf_putenv(void)
/** @envvar @value putenv */
{
    struct filp_val *v;

    filp_exec("# '=' . # .");

    v = filp_pop();

    ASSERT_ISOLATE();

    putenv(v->value);
    v->value = NULL;

    return FILP_OK;
}


/**
 * print - Prints a value to the standard output.
 * @value: value to be printed
 *
 * Prints a value to the standard output followed by a newline (\n).
 * The command ? can be used as a synonym.
 * [Output commands]
 */
static int _filpf_print(void)
/** @value print */
/** @value ? */
{
    struct filp_val *v;

    v = filp_pop();
    printf("%s\n", v->value);

    return FILP_OK;
}


/**
 * printwn - Prints a value to the standard output without newline.
 * @value: value to be printed
 *
 * Prints a value to the standard output.
 * The command ?? can be used as a synonym.
 * [Output commands]
 */
static int _filpf_printwn(void)
/** @value printwn */
/** @value ?? */
{
    struct filp_val *v;

    v = filp_pop();
    printf("%s", v->value);

    return FILP_OK;
}


/**
 * time - Returns the time in seconds.
 *
 * Returns the time in seconds. This is the value returned by the
 * C function time().
 * [System commands]
 */
static int _filpf_time(void)
/** time %time_t */
{
    filp_int_push((int) time(NULL));

    return FILP_OK;
}


/**
 * glob - Returns a list with a file expansion set.
 * @filespec: the filespec
 *
 * Returns a list with all the files that matches the @filespec
 * expression. This @filespec is system dependent, but any combination
 * of characters plus * or ? should work in all systems. The directory
 * separator is /, though \ can also be used on MS Windows; anyway, the
 * output list will always have / separators, that can also be used
 * in all supported systems as an argument for open() or similar.
 * [File and directory commands]
 */
static int _filpf_glob(void)
/** @filespec glob [ %file_list ] */
{
#if defined(__WIN32__) || defined(WIN32)

    struct filp_val *v;
    WIN32_FIND_DATA f;
    HANDLE h;
    char *ptr;

    v = filp_pop();
    filp_ref_value(v);

    ASSERT_ISOLATE();

    filp_null_push();

    /* convert MS directory separators to Unix ones */
    for (ptr = v->value; *ptr; ptr++)
        if (*ptr == '\\')
            *ptr = '/';

    if ((h = FindFirstFile(v->value, &f)) != INVALID_HANDLE_VALUE) {
        if ((ptr = strrchr(v->value, '/')) != NULL) {
            *(ptr + 1) = '\0';
            ptr = v->value;
        }
        else
            ptr = "";

        do {
            if (strcmp(f.cFileName, ".") == 0 || strcmp(f.cFileName, "..") == 0)
                continue;

            filp_scalar_push(ptr);
            filp_scalar_push(f.cFileName);
            filp_exec(".");

            if (f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                filp_exec("'/' .");
        }
        while (FindNextFile(h, &f));

        FindClose(h);
    }

    filp_unref_value(v);
#else

#ifdef CONFOPT_GLOB_H

    /* has glob */

    struct filp_val *v;
    glob_t globbuf;
    int n;

    v = filp_pop();

    ASSERT_ISOLATE();

    globbuf.gl_offs = 1;
    glob(v->value, GLOB_MARK, NULL, &globbuf);

    filp_null_push();

    for (n = 0; globbuf.gl_pathv != NULL && globbuf.gl_pathv[n] != NULL; n++)
        filp_scalar_push(globbuf.gl_pathv[n]);

    globfree(&globbuf);
#else               /* CONFOPT_GLOB_H */

    filp_pop();
    filp_null_push();

#endif              /* CONFOPT_GLOB_H */

#endif              /* WIN32 */

    return FILP_OK;
}


/**
 * open - Opens a file.
 * @filename: the name of the file to be opened.
 *
 * Opens a file. If no prefix is used, the file is open for reading;
 * The valid prefixes are: '>', open for writing; '>>', open for appending;
 * '+', reading and writing, positioned in the beginning; '+>', reading and
 * writing, truncating; '|', treat the file name as a program which standard
 * input will be the file. The valid suffixes are: '|', treat the file name
 * as a program which standard output will be the file. Pipes doesn't work
 * in MS Windows by now.
 * Returns a file descriptor, or NULL if file could not be opened.
 * [File and directory commands]
 */
static int _filpf_open(void)
/** "filename" open %fdes */
/** ">filename" open %fdes */
/** ">>filename" open %fdes */
/** "+filename" open %fdes */
/** "+>filename" open %fdes */
/** "|program" open %fdes */
/** "program|" open %fdes */
{
    struct filp_val *nv;
    struct filp_val *fv;
    FILE *f;
    char *name;
    char *mode;
    int pipe = 0;

    nv = filp_pop();

    ASSERT_ISOLATE();

    /* take name and mode */
    name = nv->value;

    if (name == NULL || *name == '\0') {
        filp_null_push();
        return FILP_OK;
    }

    if (*name == '>') {
        name++;
        if (*name == '>') {
            name++;
            mode = "a";
        }
        else
            mode = "w";
    }
    else if (*name == '<') {
        name++;
        mode = "r";
    }
    else if (*name == '+') {
        name++;
        if (*name == '<')
            mode = "r+";
        else
            mode = "w+";
        name++;
    }
    else if (*name == '|') {
        name++;
        pipe = 1;
        mode = "w";
    }
    else if (name[strlen(name) - 1] == '|') {
        name[strlen(name) - 1] = '\0';
        pipe = 1;
        mode = "r";
    }
    else
        mode = "r";

    if (pipe)
        f = popen(name, mode);
    else
        f = filp_fopen(name, mode);

    if (f == NULL)
        filp_null_push();
    else {
        fv = filp_new_value(FILP_FILE, (void *) f, 0);
        fv->pipe = pipe;

        filp_push(fv);
    }

    return FILP_OK;
}


/**
 * close - Closes a file.
 * @fdes: file descriptor to be closed.
 *
 * Closes a file.
 * [File and directory commands]
 */
static int _filpf_close(void)
/** @fdes close */
{
    struct filp_val *v;
    FILE *f;

    v = filp_pop();

    ASSERT_ISOLATE();

    if (v->type == FILP_FILE) {
        f = (FILE *) v->value;

        if (v->pipe)
            pclose(f);
        else
            fclose(f);
    }

    return FILP_OK;
}


/**
 * read - Reads a line from a file.
 * @fdes: file descriptor
 *
 * Reads a line from a file. Returns the line or NULL
 * on EOF.
 * [File and directory commands]
 */
static int _filpf_read(void)
/** @fdes read %line */
{
    struct filp_val *v;
    char *ptr = NULL;
    FILE *f;
    int c, size, offset;
    int ret;

    v = filp_pop();

    ASSERT_ISOLATE();

    if (v->type != FILP_FILE) {
        _filp_error = FILPERR_FILE_EXPECTED;
        ret = FILP_ERROR;
    }
    else {
        f = (FILE *) v->value;
        size = offset = 0;

        while ((c = getc(f)) != EOF) {
            ptr = filp_poke(ptr, &size, offset++, c);
            if (c == '\n')
                break;
        }

        if (ptr == NULL && c == EOF)
            filp_null_push();
        else {
            ptr = filp_poke(ptr, &size, offset, '\0');
            filp_scalar_push(ptr);
            free(ptr);
        }

        ret = FILP_OK;
    }

    return ret;
}


/**
 * write - Writes a string to a file.
 * @string: string to be written to the file
 * @fdes: file descriptor
 *
 * Writes a string to a file.
 * [File and directory commands]
 */
static int _filpf_write(void)
/** @string @fdes write */
{
    struct filp_val *lv;
    struct filp_val *fv;
    FILE *f;
    int ret;

    fv = filp_pop();
    lv = filp_pop();

    ASSERT_ISOLATE();

    if (fv->type != FILP_FILE) {
        _filp_error = FILPERR_FILE_EXPECTED;
        ret = FILP_ERROR;
    }
    else {
        f = (FILE *) fv->value;
        fprintf(f, "%s", lv->value);
        ret = FILP_OK;
    }

    return ret;
}


/**
 * bread - Reads a block of bytes from a file.
 * @size: number of bytes to be read
 * @fdes: file descriptor
 *
 * Reads a block of bytes from a file. The block of data
 * and its size is returned. On EOF, the returned size will be zero.
 * [File and directory commands]
 */
static int _filpf_bread(void)
/** @size @fdes bread %size %data */
{
    struct filp_val *v;
    int size;
    char *ptr;
    FILE *f;

    v = filp_pop();
    size = filp_int_pop();

    ASSERT_ISOLATE();

    if (v->type != FILP_FILE) {
        _filp_error = FILPERR_FILE_EXPECTED;
        return FILP_ERROR;
    }
    else {
        if ((ptr = malloc(size + 1)) == NULL) {
            _filp_error = FILPERR_OUT_OF_MEMORY;
            return FILP_ERROR;
        }

        f = (FILE *) v->value;
        size = fread(ptr, 1, size, f);
        ptr[size] = '\0';

        if (size) {
            v = filp_new_value(FILP_SCALAR, ptr, size);

            filp_int_push(size);
            filp_push(v);
        }
        else {
            free(ptr);
            filp_null_push();
        }
    }

    return FILP_OK;
}


/**
 * bwrite - Writes a block of bytes to a file.
 * @size: size of the block of data
 * @data: the block of data
 * @fdes: the file descriptor
 *
 * Writes a block of bytes to a file.
 * [File and directory commands]
 */
static int _filpf_bwrite(void)
/** @size @data @fdes bwrite */
{
    struct filp_val *lv;
    struct filp_val *fv;
    int size;
    FILE *f;
    int ret;

    fv = filp_pop();
    lv = filp_pop();
    size = filp_int_pop();

    ASSERT_ISOLATE();

    if (fv->type != FILP_FILE) {
        _filp_error = FILPERR_FILE_EXPECTED;
        ret = FILP_ERROR;
    }
    else {
        f = (FILE *) fv->value;
        fwrite(lv->value, 1, size, f);
        ret = FILP_OK;
    }

    return ret;
}


/**
 * mkdir - Creates a directory.
 * @dirname: directory name
 *
 * Creates a directory.
 * [File and directory commands]
 */
/** @dirname mkdir */
static int _filpf_mkdir(void)
{
    struct filp_val *d;

    d = filp_pop();

    ASSERT_ISOLATE();

    MKDIR(d->value, 0755);

    return FILP_OK;
}


/**
 * strerror - Returns the system error string.
 * @errno: the error number
 *
 * Returns the system error string. @errno must be the value
 * of the filp_errno variable.
 * [System commands]
 */
/** @errno strerror */
static int _filpf_strerror(void)
{
    char *ptr;

    ptr = strerror(filp_int_pop());

    filp_scalar_push(ptr);
    return FILP_OK;
}


/**
 * regex - Matches a POSIX regular expression.
 * @re: string containing the regular expression
 * @str: the string to test
 *
 * Tests if @str matches the @re POSIX regular
 * expression. Returns true if the string matches.
 * See the regex(7) man page for more information
 * about POSIX regular expressions.
 * [String manipulation commands]
 * [Boolean commands]
 */
/** @str @re =~ %bool_value */
/** @str @re regex %bool_value */
static int _filpf_regex(void)
{
    regex_t r;
    struct filp_val *re;
    struct filp_val *str;
    int err;

    re = filp_pop();
    str = filp_pop();

    if (re->type != FILP_SCALAR || str->type != FILP_SCALAR) {
        _filp_error = FILPERR_SCALAR_EXPECTED;
        return FILP_ERROR;
    }

    /* compile regex */
    if ((err = regcomp(&r, re->value, REG_EXTENDED | REG_NOSUB | REG_ICASE))) {
        _filp_error = FILPERR_SYNTAX_ERROR;

        regerror(err, &r, _filp_error_info, sizeof(_filp_error_info));

        return FILP_ERROR;
    }

    /* match */
    if (regexec(&r, str->value, 0, NULL, 0) == 0)
        filp_bool_push(1);
    else
        filp_bool_push(0);

    return FILP_OK;
}


/**
 * timer - millisecond timer
 *
 * Returns a millisecond timer.
 * [System commands]
 */
/** timer %msec_timer */
static int _filpf_timer(void)
{
    filp_int_push((int) _filp_get_timer());
    return FILP_OK;
}


void filp_slib_startup(void)
{
    filp_bin_code("print", _filpf_print);
    filp_bin_code("?", _filpf_print);
    filp_bin_code("printwn", _filpf_printwn);
    filp_bin_code("??", _filpf_printwn);
    filp_bin_code("getenv", _filpf_getenv);
    filp_bin_code("putenv", _filpf_putenv);
    filp_bin_code("time", _filpf_time);
    filp_bin_code("glob", _filpf_glob);

    filp_bin_code("open", _filpf_open);
    filp_bin_code("close", _filpf_close);
    filp_bin_code("read", _filpf_read);
    filp_bin_code("write", _filpf_write);
    filp_bin_code("bread", _filpf_bread);
    filp_bin_code("bwrite", _filpf_bwrite);

    filp_bin_code("mkdir", _filpf_mkdir);
    filp_bin_code("strerror", _filpf_strerror);

    filp_bin_code("regex", _filpf_regex);
    filp_bin_code("=~", _filpf_regex);

    filp_bin_code("timer", _filpf_timer);

    filp_exec("/filp_command_history ( ) =");

    /**
     * errno - The C library errno variable.
     *
     * This variable contains the C library errno variable.
     * [Special variables]
     */
    /** errno */
    filp_ext_int("errno", &errno);

    /**
     * STDIN - Standard input file descriptor.
     *
     * This is the standard input file descriptor.
     * [Special variables]
     */
    /** STDIN */
    filp_ext_file("STDIN", stdin);

    /**
     * STDOUT - Standard output file descriptor.
     *
     * This is the standard output file descriptor.
     * [Special variables]
     */
    /** STDOUT */
    filp_ext_file("STDOUT", stdout);

    /**
     * STDERR - Standard error file descriptor.
     *
     * This is the standard error file descriptor.
     * [Special variables]
     */
    /** STDERR */
    filp_ext_file("STDERR", stderr);

    /**
     * shell - Executes an external program.
     *
     * Executes an external program and sends all of its output
     * to the stack.
     * [System commands]
     */
    /** @program shell %output_from_program */
    filp_exec("/shell { '|' . '' # open { dup read } { 3 rot # . # } while close } set");

    /**
     * filp_arch - Architecture id string.
     *
     * This string contains 'linux' if filp is running under
     * any flavour of linux, 'win32' if under any flavour of
     * MS Windows, 'beos' if under BeOS or 'unix' otherwise.
     * [Special constants]
     */
    /** filp_arch */
    /* ; */

#if defined(linux)
    filp_exec("/filp_arch 'linux' set");
#elif defined(WIN32) || defined(__WIN32__)
    filp_exec("/filp_arch 'win32' set");
#elif defined(BEOS)
    filp_exec("/filp_arch 'beos' set");
#else
    filp_exec("/filp_arch 'unix' set");
#endif

    /**
     * filp_uname - Operating system information.
     *
     * This string contains human-readable information
     * about the operating system filp is running on.
     * [Special constants]
     */
    /** filp_uname */
    filp_exec("/filp_uname '" CONFOPT_UNAME "' set");

    /**
     * filp_compile_options - Compilation options information.
     *
     * This string contains human-readable information
     * about possible compilation options or limitations.
     * [Special constants]
     */
    /** filp_compile_options */

    filp_exec("/filp_compile_options [ ''");
#ifdef CONFOPT_GETTIMEOFDAY
    filp_scalar_push("CONFOPT_GETTIMEOFDAY");
#endif
#ifdef CONFOPT_TERMIOS
    filp_scalar_push("CONFOPT_TERMIOS");
#endif
#ifdef CONFOPT_GLOB_H
    filp_scalar_push("CONFOPT_GLOB_H");
#endif
#ifdef FILP_SHARED
    filp_scalar_push("FILP_SHARED");
#endif
#ifdef __TURBOC__
    filp_scalar_push("WITHOUT_POPEN");
#endif
    filp_exec("' ' join =");

    /**
     * filp_lang - Two letter code of the preferred language.
     *
     * This variable contains the two letter code of the
     * preferred language defined by the user. It mainly
     * takes its value from the LANG environment variable,
     * but other sources (mainly for win32) are tried
     * as well.
     * [Special variables]
     */
    /** filp_lang */
    filp_exec("\"LANG\" getenv dupnz { /filp_lang # 1 2 substr = } if");

#if defined(WIN32) || defined(__WIN32__)
    {
        short s;
        char *ptr;

        s = GetSystemDefaultLangID() & 0x00ff;

        /* try only some 'famous' languages:
           the complete world language database should
           be implemented */
        switch (s) {
        case 0x01:
            ptr = "ar";
            break;  /* arabic */
        case 0x02:
            ptr = "bg";
            break;  /* bulgarian */
        case 0x03:
            ptr = "ca";
            break;  /* catalan */
        case 0x04:
            ptr = "zh";
            break;  /* chinese */
        case 0x05:
            ptr = "cz";
            break;  /* czech */
        case 0x06:
            ptr = "da";
            break;  /* danish */
        case 0x07:
            ptr = "de";
            break;  /* german */
        case 0x08:
            ptr = "el";
            break;  /* greek */
        case 0x09:
            ptr = "en";
            break;  /* english */
        case 0x0a:
            ptr = "es";
            break;  /* spanish */
        case 0x0b:
            ptr = "fi";
            break;  /* finnish */
        case 0x0c:
            ptr = "fr";
            break;  /* french */
        case 0x0d:
            ptr = "he";
            break;  /* hebrew */
        case 0x0e:
            ptr = "hu";
            break;  /* hungarian */
        case 0x0f:
            ptr = "is";
            break;  /* icelandic */
        case 0x10:
            ptr = "it";
            break;  /* italian */
        case 0x11:
            ptr = "jp";
            break;  /* japanese */
        case 0x13:
            ptr = "nl";
            break;  /* dutch */
        case 0x14:
            ptr = "no";
            break;  /* norwegian */
        case 0x15:
            ptr = "po";
            break;  /* polish */
        case 0x16:
            ptr = "pt";
            break;  /* portuguese */
        case 0x1d:
            ptr = "se";
            break;  /* swedish */
        default:
            ptr = "en";
            break;
        }

        filp_execf("$filp_lang { /filp_lang '%s' = } unless", ptr);
    }
#else
    /* if no language definition, default to english */
    filp_exec("$filp_lang { /filp_lang 'en' = } unless");
#endif
}
