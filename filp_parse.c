/*

    filp - Embeddable, Reverse Polish Notation Programming Language

    Angel Ortega <angel@triptico.com>

    This software is released into the public domain.
    NO WARRANTY. See file LICENSE for details.

    Code Parser.

*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "filp.h"


/********************
    Code
*********************/

/**
 * filp_push_symbol_value - Pushes a symbol value to the stack
 * @symbol: the name of the symbol
 *
 * Pushes the value of a symbol to the stack, or a FILP_NULL
 * value if symbol is not found.
 */
void filp_push_symbol_value(char *symbol)
{
    struct filp_sym *s;

    if ((s = filp_find_symbol(symbol)) != NULL)
        filp_push(filp_get_symbol(s));
    else
        filp_null_push();
}


/**
 * _filp_parse_string - Parses a string
 * @str: the string to be parsed
 * @slash: flag to process backslashed chars (\n, etc).
 * @interp: flag to interpolate variables (ala Perl).
 *
 * Parses a string, converting escaped sequences (e.g. \n)
 * to their corresponden equivalences if @slash is set.
 * If @interp is set, any variable name prefixed by $
 * will have its value interpolated (as in Perl strings).
 * Returns a pointer to a newly allocated string. This
 * string must be freed when is not useful anymore.
 */
static char *_filp_parse_string(char *str, int slash, int interp)
{
    char *ptr;
    int size, n, c, i, quote;
    struct filp_val *v;

    ptr = NULL;
    quote = *str;

    for (n = size = 0, str++; *str && *str != quote; str++) {
        if (slash && *str == '\\') {
            str++;
            if (*str == 'n')
                c = '\n';
            else if (*str == 'r')
                c = '\r';
            else if (*str == 't')
                c = '\t';
            else if (*str == 'a')
                c = '\a';
            else if (*str == 'e')
                c = 27;
            else if (*str == '\n')
                c = -1;
            else
                c = *str;
        }
        else if (interp && *str == '$') {
            str++;
            for (i = n; isalpha((int) *str) || *str == '_'; str++, n++)
                ptr = filp_poke(ptr, &size, n, *str);
            ptr = filp_poke(ptr, &size, n, '\0');

            n = i;
            filp_push_symbol_value(ptr + n);
            v = filp_pop();

            if (v->type != FILP_NULL) {
                for (i = 0; v->value[i]; i++, n++)
                    ptr = filp_poke(ptr, &size, n, v->value[i]);
            }

            c = *str;
        }
        else
            c = *str;

        if (c == '\0' || c == quote)
            break;

        if (c != -1) {
            ptr = filp_poke(ptr, &size, n, c);
            n++;
        }
    }

    ptr = filp_poke(ptr, &size, n, '\0');

    return ptr;
}


static void _filp_push_literal_string(char *str, int slash, int interp)
{
    char *pstr;

    if ((pstr = _filp_parse_string(str, slash, interp)) == NULL)
        return;

    filp_scalar_push(pstr);

    free(pstr);
}


static int _filp_isspchar(char c)
{
    return strchr("{}()[]", c) != NULL;
}


static char *_filp_parse_token(char *token, int *t_size, char **code_ptr)
{
    char *code;
    int n = 0;

    /* use local copy of code pointer */
    code = *code_ptr;

    /* separate token */
    while (filp_issep(*code))
        code++;

    /* string literal? */
    if (*code == '"' || *code == '\'') {
        char quote = *code;

        for (; *code; code++, n++) {
            token = filp_poke(token, t_size, n, *code);

            if (n && *code == quote) {
                code++;
                n++;
                break;
            }
        }
    }
    else
        /* special character? */
    if (_filp_isspchar(*code)) {
        token = filp_poke(token, t_size, n++, *code);
        code++;
    }
    else {
        for (; *code && !_filp_isspchar(*code) && !filp_issep(*code); code++, n++)
            token = filp_poke(token, t_size, n, *code);
    }

    /* null-terminate token */
    token = filp_poke(token, t_size, n, '\0');

    *code_ptr = code;

    return token;
}


static int _filp_process_token(char *token)
{
    struct filp_val *v = NULL;
    struct filp_sym *s;
    int (*func) (void);
    int ret = FILP_OK;

    /* break? */
    if (strcmp(token, "break") == 0)
        ret = FILP_BREAK;
    else
        /* end? */
    if (strcmp(token, "end") == 0)
        ret = FILP_END;
    else
        /* is it a single quoted literal string? */
    if (*token == '\'')
        _filp_push_literal_string(token, 0, 0);
    else
        /* is it a double quoted literal string? */
    if (*token == '"')
        _filp_push_literal_string(token, 1, 1);
    else
        /* is it a symbol value? */
    if (*token == '$')
        filp_push_symbol_value(token + 1);
    else
        /* is it a symbol name? */
    if ((s = filp_find_symbol(token)) != NULL) {
        v = s->value;

        if (s->type == FILP_BIN_CODE) {
            /* execute, if binary code */
            func = (int (*)()) (v->value);
            if (func)
                ret = func();

            v = NULL;
        }
        else if (s->type == FILP_CODE) {
            /* execute, if filp code */
            ret = filp_execv(v);
            v = NULL;

            /* don't propagate 'break' */
            if (ret == FILP_BREAK)
                ret = FILP_OK;
        }
        else
            v = filp_new_value(FILP_SCALAR, token, -1);
    }
    else {
        if (*token == '/')
            token++;
        else if (!_filp_bareword) {
            /* bang if it's not a number and we
               don't want barewords (we don't) */
            if (!isdigit((int) *token) && *token != '-') {
                /* token not found */
                _filp_error = FILPERR_TOKEN_NOT_FOUND;

                strncpy(_filp_error_info, token, sizeof(_filp_error_info));
                return -1;
            }
        }

        /* store as is, as a literal */
        v = filp_new_value(FILP_SCALAR, token, -1);
    }

    if (v != NULL)
        filp_push(v);

    return ret;
}


/**
 * filp_exec - Executes filp code.
 * @code: filp code to run
 *
 * Executes the string as filp code. Returns 0 if everything is ok,
 * <0 on error or >0 if execution is intentionally interrupted
 * (by using break or end).
 */
int filp_exec(char *code)
/* runs a program (main parser) */
{
    int in_comment;
    char *token = NULL;
    char *p_code = NULL;
    int t_size;
    int post_code, p_size, p_n;
    int n, ret;

    _in_filp++;

    in_comment = post_code = 0;
    t_size = p_size = p_n = 0;
    ret = FILP_OK;

    /* if code starts with #!, ignore first line */
    if (code[0] == '#' && code[1] == '!') {
        while (*code != '\0' && *code != '\n')
            code++;

        if (*code == '\0')
            return FILP_OK;
    }

    while (ret == FILP_OK) {
        /* parse token */
        token = _filp_parse_token(token, &t_size, &code);

        if (*token == '\0')
            break;

        /* comment? */
        if (strncmp(token, "/*", 2) == 0) {
            in_comment++;
            continue;
        }
        if (strcmp(token, "*/") == 0) {
            in_comment--;
            continue;
        }

        if (in_comment)
            continue;

        if (strcmp(token, "}") == 0) {
            post_code--;

            if (post_code == -1)
                break;

            if (post_code == 0) {
                p_code = filp_poke(p_code, &p_size, p_n, '\0');
                filp_code_push(p_code);
                continue;
            }
        }
        if (strcmp(token, "{") == 0) {
            post_code++;

            if (post_code == 1) {
                p_n = 0;
                continue;
            }
        }

        if (post_code) {
            /* stores a separator and a token */
            if (p_n)
                p_code = filp_poke(p_code, &p_size, p_n++, ' ');

            for (n = 0; token[n]; n++, p_n++)
                p_code = filp_poke(p_code, &p_size, p_n, token[n]);

            continue;
        }

        /* process it */
        ret = _filp_process_token(token);

        /* collect garbage */
        filp_sweeper(0);
    }

    if (token)
        free(token);
    if (p_code)
        free(p_code);

    return ret;
}


/**
 * filp_execf - Executes filp code, with formatting.
 * @code: filp code with printf-like formatting
 *
 * Formats @code as a printf() -like string, and executes it
 * as filp code. See filp_exec() for return values.
 */
int filp_execf(char *code, ...)
{
    char buf[4096];
    va_list argptr;

    va_start(argptr, code);
    vsprintf(buf, code, argptr);
    va_end(argptr);

    return filp_exec(buf);
}


/**
 * filp_execv - Executes filp code inside a filp value.
 * @v: the value.
 *
 * Executes filp code inside a value. The value @v must
 * be binary code, filp code or a scalar containing
 * filp code.
 */
int filp_execv(struct filp_val *v)
{
    int ret = FILP_OK;
    int (*func) (void);

    if (v == NULL)
        return 0;

    filp_ref_value(v);

    if (v->type == FILP_SCALAR || v->type == FILP_CODE)
        ret = filp_exec(v->value);
    else if (v->type == FILP_BIN_CODE) {
        func = (int (*)()) (v->value);
        if (func)
            ret = func();
    }

    filp_unref_value(v);

    return ret;
}


/**
 * filp_load_exec - Loads and executes a filp code file.
 * @filename: the name of the file contaning filp code
 *
 * Loads a file and executes it. See filp_exec() for the
 * return values.
 */
int filp_load_exec(char *filename)
{
    int ret;
    char *code;

    if ((code = filp_load_file(filename)) == NULL) {
        strncpy(_filp_error_info, filename, sizeof(_filp_error_info));
        _filp_error = FILPERR_FILE_NOT_FOUND;
        return -1;
    }

    ret = filp_exec(code);

    free(code);

    return ret;
}
