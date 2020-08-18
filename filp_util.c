/*

    filp - Embeddable, Reverse Polish Notation Programming Language

    Angel Ortega <angel@triptico.com>

    This software is released into the public domain.
    NO WARRANTY. See file LICENSE for details.

*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "filp.h"


/******************
    Code
*******************/

/**
 * filp_new_int_value - Creates a new scalar from an int.
 * @value: the integer to be used as the value
 *
 * Creates a new scalar from the integer @value.
 * Returns the new value.
 */
struct filp_val *filp_new_int_value(int value)
{
    char tmp[64];

    sprintf(tmp, "%i", value);
    return filp_new_value(FILP_SCALAR, tmp, -1);
}


/**
 * filp_val_to_int - Converts a filp value to an int.
 * @v: the value to be converted
 *
 * Converts a filp value into an integer.
 * Returns the integer.
 */
int filp_val_to_int(struct filp_val *v)
{
    int i;

    if (v->type != FILP_SCALAR)
        return 0;

    sscanf(v->value, "%i", &i);
    return i;
}


/**
 * filp_new_real_value - Creates a new scalar from a double.
 * @value: the double to be used as the value
 *
 * Creates a new scalar from the double @value.
 * Returns the new value.
 */
struct filp_val *filp_new_real_value(double value)
{
    char tmp[64];

    sprintf(tmp, "%f", value);
    return filp_new_value(FILP_SCALAR, tmp, -1);
}


/**
 * filp_val_to_real - Converts a filp value to a double.
 * @v: the value to be converted
 *
 * Converts a filp value into a double.
 * Returns the integer.
 */
double filp_val_to_real(struct filp_val *v)
{
    double i;

    if (v->type != FILP_SCALAR)
        return 0;

    sscanf(v->value, "%lf", &i);
    return i;
}


/**
 * filp_new_bin_code_value - Creates a bin code value.
 * @func: the function to be used
 *
 * Creates a new value of the FILP_BIN_CODE type. The @func
 * must have no parameters and return a non-zero integer
 * value in case of error.
 */
struct filp_val *filp_new_bin_code_value(int (*func) (void))
{
    return filp_new_value(FILP_BIN_CODE, (char *) func, 0);
}


/**
 * filp_scalar_push - Pushes a scalar into the stack.
 * @value: the string containing the scalar
 *
 * Pushes a scalar into the stack. The scalar must be a string.
 */
int filp_scalar_push(char *value)
{
    return filp_push(filp_new_value(FILP_SCALAR, value, -1));
}


/**
 * filp_int_push - Pushes a C integer into the stack, as a scalar.
 * @value: integer value
 *
 * Pushes a C integer into the stack, previously converted to its
 * printable value.
 */
int filp_int_push(int value)
{
    return filp_push(filp_new_int_value(value));
}


/**
 * filp_real_push - Pushes a C double into the stack, as a scalar.
 * @value: double float value
 *
 * Pushes a C double flaot into the stack, previously converted to its
 * printable value.
 */
int filp_real_push(double value)
{
    return filp_push(filp_new_real_value(value));
}


/**
 * filp_null_push - Pushes a special NULL value to the stack.
 *
 * Pushes a special NULL value to the stack.
 */
int filp_null_push(void)
{
    return filp_push(_filp_null_value);
}


/**
 * filp_code_push - Pushes a string to the stack, as filp code.
 * @code: the string containing filp code
 *
 * Pushes a string to the stack as filp code. The code is not
 * tested for integrity nor syntax.
 */
int filp_code_push(char *code)
{
    return filp_push(filp_new_value(FILP_CODE, code, -1));
}


int filp_bool_push(int value)
{
    return filp_push(value ? _filp_true_value : _filp_null_value);
}


/**
 * filp_int_pop - Pops a value from the stack, as an integer value.
 *
 * Pops a value from the stack, as a C integer value. If the value
 * has no meaning as a number, 0 is returned.
 */
int filp_int_pop(void)
{
    return filp_val_to_int(filp_pop());
}


/**
 * filp_real_pop - Pops a value from the stack, as a double value.
 *
 * Pops a value from the stack, as a C double float value. If the value
 * has no meaning as a number, 0 is returned.
 */
double filp_real_pop(void)
{
    return filp_val_to_real(filp_pop());
}


/**
 * filp_bin_code - Creates a new binary code symbol.
 * @name: name of the new symbol
 * @func: C code function
 *
 * Creates a new binary code (C code) symbol. This symbol can
 * be directly executed by the filp parser.
 */
int filp_bin_code(char *name, int (*func) (void))
{
    struct filp_val *v;
    struct filp_sym *s;

    if ((s = filp_find_symbol(name)) != NULL)
        return 0;

    if ((s = filp_new_symbol(FILP_BIN_CODE, name)) == NULL)
        return 0;

    if ((v = filp_new_bin_code_value(func)) == NULL)
        return 0;

    filp_set_symbol(s, v);

    return 1;
}


/**
 * filp_ext_int - Creates a new external C integer symbol.
 * @name: name of the new symbol
 * @val: pointer to the integer
 *
 * Creates a new external C integer symbol. This symbol will
 * behave as a scalar filp variable, but any value assigned
 * to it will be converted to integer and stored into the variable.
 */
int filp_ext_int(char *name, int *val)
{
    struct filp_sym *s;

    if ((s = filp_find_symbol(name)) != NULL)
        return 0;

    if ((s = filp_new_symbol(FILP_EXT_INT, name)) == NULL)
        return 0;

    s->value = (struct filp_val *) val;
    return 1;
}


/**
 * filp_ext_real - Creates a new external C double symbol.
 * @name: name of the new symbol
 * @val: pointer to the double float
 *
 * Creates a new external C double float symbol. This symbol will
 * behave as a scalar filp variable, but any value assigned
 * to it will be converted to double float and stored into the variable.
 */
int filp_ext_real(char *name, double *val)
{
    struct filp_sym *s;

    if ((s = filp_find_symbol(name)) != NULL)
        return 0;

    if ((s = filp_new_symbol(FILP_EXT_REAL, name)) == NULL)
        return 0;

    s->value = (struct filp_val *) val;
    return 1;
}


/**
 * filp_ext_string - Creates a new external string symbol.
 * @name: name of the new symbol
 * @val: pointer to the string
 * @size: size of the string
 *
 * Creates a new external C string symbol. This symbol will
 * behave as a scalar filp variable, but any value assigned
 * to it will be stored into the C string. If the assigned
 * value exceeds @size, it will be silently truncated.
 */
int filp_ext_string(char *name, char *val, int size)
{
    struct filp_sym *s;

    if ((s = filp_find_symbol(name)) != NULL)
        return 0;

    if ((s = filp_new_symbol(FILP_EXT_STRING, name)) == NULL)
        return 0;

    s->value = (struct filp_val *) val;
    s->size = size;

    return 1;
}


/**
 * filp_ext_file - Creates a new external file descriptor symbol.
 * @name: name of the new symbol
 * @f: pointer to a FILE C descriptor
 *
 * Creates a new external file descriptor symbol. The file should
 * be alread opened and never closed from the C code.
 */
int filp_ext_file(char *name, FILE * f)
{
    struct filp_sym *s;

    if ((s = filp_find_symbol(name)) != NULL)
        return 0;

    if ((s = filp_new_symbol(FILP_FILE, name)) == NULL)
        return 0;

    s->value = filp_new_value(FILP_FILE, (void *) f, 0);

    return 1;
}


FILE *(*filp_external_fopen) (char *filename, char *mode) = NULL;

FILE *filp_fopen(char *filename, char *mode)
{
    FILE *f = NULL;

    if (filp_external_fopen)
        f = filp_external_fopen(filename, mode);

    if (f == NULL)
        f = fopen(filename, mode);

    return f;
}


/**
 * filp_load_file - Loads a filp code file.
 * @filename: the file to be loaded
 *
 * Loads a filp code file and returns a pointer to it.
 * The file must not be necessary filp code; no integrity
 * or syntax is checked. The returned pointer must be
 * destroyed using free() when no longer needed.
 */
char *filp_load_file(char *filename)
{
    char *code = NULL;
    FILE *f;
    int c, n, size;

    if ((f = filp_fopen(filename, "r")) != NULL) {
        size = 0;
        code = NULL;

        for (n = 0; (c = fgetc(f)) != EOF; n++)
            code = filp_poke(code, &size, n, c);

        code = filp_poke(code, &size, n, '\0');

        fclose(f);
    }

    return code;
}


#define FILP_DUMPCHAR(c) ptr=filp_poke(ptr, size, (*offset)++, (c))

static char *_filp_dumper(struct filp_val *v, int lvl, int max,
              char *ptr, int *size, int *offset)
{
    int n = 0;
    char *pre = "";
    char *val = "";
    char *post = " ";

    if (lvl < max) {
        for (n = 0; n < lvl; n++)
            FILP_DUMPCHAR('\t');
    }

    switch (v->type) {
    case FILP_SCALAR:

        val = v->value;

        pre = "'";
        post = "' ";

        if (isdigit((int) *v->value) ||
            (*v->value == '-' && isdigit((int) *(v->value + 1)))) {
            pre = "";
            post = "";
        }

        break;

    case FILP_CODE:

        val = v->value;

        if (strchr(v->value, '\n') == NULL) {
            pre = "{ ";
            post = " } ";
        }
        else {
            pre = "\n{\n";
            post = "\n}\n";
        }

        break;

    case FILP_BIN_CODE:

        pre = "'";
        val = "[BIN_CODE]";
        post = "' ";
        break;

    case FILP_EXT_INT:

        pre = "'";
        val = "[EXT_INT]";
        post = "' ";
        break;

    case FILP_EXT_REAL:

        pre = "'";
        val = "[EXT_REAL]";
        post = "' ";
        break;

    case FILP_EXT_STRING:

        pre = "'";
        val = "[EXT_STRING]";
        post = "' ";
        break;

    case FILP_NULL:

        pre = "'";
        val = "[NULL]";
        post = "' ";
        break;

    case FILP_FILE:

        pre = "'";
        val = "[FILE]";
        post = "' ";
        break;

    case FILP_ARRAY:

        pre = "";
        val = "";
        post = "";

        FILP_DUMPCHAR('(');
        FILP_DUMPCHAR(' ');

        lvl++;
        if (lvl < max)
            FILP_DUMPCHAR('\n');

        for (n = 1; n <= filp_array_size(v); n++)
            ptr = _filp_dumper(filp_array_get(v, n), lvl, max, ptr, size, offset);

        lvl--;

        if (lvl + 1 < max) {
            for (n = 0; n < lvl; n++)
                FILP_DUMPCHAR('\t');
        }

        FILP_DUMPCHAR(')');
        FILP_DUMPCHAR(' ');

        break;
    }

    while (*pre)
        FILP_DUMPCHAR(*pre++);
    while (*val)
        FILP_DUMPCHAR(*val++);
    while (*post)
        FILP_DUMPCHAR(*post++);

    if (lvl < max)
        FILP_DUMPCHAR('\n');

    return ptr;
}


char *filp_dumper(struct filp_val *v, int max)
{
    int size = 0;
    int offset = 0;
    char *ptr;

    ptr = _filp_dumper(v, 0, max, NULL, &size, &offset);
    return filp_poke(ptr, &size, offset, '\0');
}


int filp_array_to_doubles(struct filp_val *v, double *d, int max)
/* converts a filp_array to an array of doubles */
{
    int n;

    for (n = 0; n < filp_array_size(v) && n < max; n++)
        d[n] = filp_val_to_real(filp_array_get(v, n + 1));

    return n;
}


struct filp_val *filp_doubles_to_array(double *d, int num)
/* converts an array of doubles to a filp_array */
{
    struct filp_val *v;
    int n;

    v = filp_new_value(FILP_ARRAY, NULL, num);

    for (n = 0; n < num; n++)
        filp_array_set(v, filp_new_real_value(d[n]), n + 1);

    return v;
}
