/*

    filp - Embeddable, Reverse Polish Notation Programming Language

    Angel Ortega <angel@triptico.com>

    This software is released into the public domain.
    NO WARRANTY. See file LICENSE for details.

    Filp function library.
    Level I (system independent).

    All filp functions using only filp stuff or basic, system-independent
    code must be here. Also here, the main startup/shutdown functions.

*/

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "filp.h"


/******************
    Data
*******************/

/******************
    Code
*******************/


void filp_console(void)
/* the filp console */
{
    char prompt[256];
    char *buffer;

    printf("filp " VERSION " - Embeddable, Reverse Polish Notation Programming Language\n\
Angel Ortega <angel@triptico.com>\n\
This is free software with ABSOLUTELY NO WARRANTY.\n\
For license details type: license ?\nTo exit type: end\n\n");

    for (;;) {
        if (_filp_error) {
            filp_execf
                ("'%s' /filp_error_strings %d @ %d 'Error %%d (%%s) [%%s]' sprintf ?\n",
                 _filp_error_info, _filp_error, _filp_error);

            _filp_error = 0;
        }

        sprintf(prompt, "filp (%d) > ", _filp_stack_elems);

        if ((buffer = filp_readline(prompt)) == NULL)
            break;

        if (filp_exec(buffer) == -2)
            break;
    }
}


/* filp functions */

/**
 * size - Returns the size of a value.
 * @value: the value which size is queried
 *
 * Returns the size in bytes of the @value. Note that this number
 * is not necessary the same as the length.
 * [Symbol management commands]
 */
static int _filpf_size(void)
/** value size %size_in_bytes */
{
    struct filp_val *v;

    v = filp_pop();
    filp_int_push(v->size);

    return FILP_OK;
}


/**
 * lsize - Returns the size of a list.
 * @list: the list
 *
 * Returns the number of elements of a list.
 * [Symbol management commands]
 * [List processing commands]
 */
int _filpf_lsize(void)
/** [ @list_elements ] lsize %number */
{
    filp_int_push(filp_list_size());

    return FILP_OK;
}

/**
 * dup - Duplicates the value in the top of stack.
 * @value: the value to be duplicated
 *
 * Duplicates the value in the top of stack.
 * [Stack manipulation commands]
 */
static int _filpf_dup(void)
/** value dup %value %value */
{
    filp_push(filp_stack_value(1));

    return FILP_OK;
}


/**
 * dupnz - Duplicates the value in the top of stack if true.
 * @value: the value to be duplicated
 *
 * Duplicates the value in the top of stack if it has
 * a 'true' value.
 * [Stack manipulation commands]
 */
static int _filpf_dupnz(void)
/** value dupnz %value %value */
{
    struct filp_val *v;

    v = filp_stack_value(1);

    if (filp_is_true(v))
        filp_push(v);

    return FILP_OK;
}


/**
 * idup - Duplicates a specific value from the stack.
 * @i: the position number of the value to be duplicated.
 *
 * Creates a copy of the element number @i from the stack,
 * and leaves it in the first position. Stack elements
 * start from 1.
 * [Stack manipulation commands]
 */
static int _filpf_idup(void)
/** @val#i @val#i-1 ... @val#1 i idup %val#i */
{
    filp_push(filp_stack_value(filp_int_pop()));

    return FILP_OK;
}


/**
 * swap - Swaps the two values on top of the stack.
 * @val2: second value
 * @val1: first value
 *
 * Swaps the two values on top of the stack. The commands
 * xchg or # are synonyms.
 * [Stack manipulation commands]
 */
static int _filpf_swap(void)
/** @value2 @value1 swap %value1 %value2 */
/** @value2 @value1 xchg %value1 %value2 */
/** @value2 @value1 # %value1 %value2 */
{
    filp_rot(2);

    return FILP_OK;
}


/**
 * rot - Rotates the stack.
 * @i: the element number from where the stack will be rotated.
 *
 * Rotates the stack. The element number @i moves to the top
 * of the stack, shifting all the upper ones one position down.
 * [Stack manipulation commands]
 */
static int _filpf_rot(void)
/** @val#i @val#i-1 ... @val#1 @i rot %val#i-1 ... %val#1 %val#i */
{
    filp_rot(filp_int_pop());

    return FILP_OK;
}


/**
 * pop - Drops a value from the top of stack.
 * @value: the value to be dropped
 *
 * Drops a value from the top of stack. The value is destroyed.
 * [Stack manipulation commands]
 */
static int _filpf_pop(void)
/** @value pop */
{
    filp_pop();

    return FILP_OK;
}


/**
 * swstack - Swaps between the two stacks.
 *
 * Filp has two stacks, one on use and the other 'dormant'. This command
 * swaps between the two stacks. It's used basicly when a running process
 * can destroy valid information stored in the current stack, so this
 * command puts it in a safe place. The variable filp_stack_elems is set
 * accordingly.
 * [Stack manipulation commands]
 */
static int _filpf_swstack(void)
/** swstack */
{
    filp_swap_stack();

    return FILP_OK;
}


/**
 * true - Stores a true value on the top of stack.
 *
 * Stores a conditional value equal to 'true' on the top of stack.
 * [Special constants]
 */
static int _filpf_true(void)
/** true %true_value */
{
    filp_bool_push(1);

    return FILP_OK;
}


/**
 * false - Stores a false value on the top of stack.
 *
 * Stores a conditional value equal to 'false' on the top of stack.
 * [Special constants]
 */
static int _filpf_false(void)
/** false %false_value */
{
    filp_bool_push(0);

    return FILP_OK;
}


/**
 * NULL - Stores a NULL value on the top of stack.
 *
 * Stores a NULL value on the top of stack. Though [ is used as a
 * delimiter for lists and ( for arrays, in practice all of them are the
 * same.
 * [Special constants]
 */
static int _filpf_NULL(void)
/** NULL NULL */
/** [ NULL */
/** ( NULL */
{
    filp_null_push();

    return FILP_OK;
}


/**
 * set - Sets the value of a symbol.
 * @symbol: symbol name
 * @content: content to be stored in the symbol
 *
 * Sets the value of a symbol. If the @content
 * is code surrounded by { } , the symbol is marked as executable
 * and becomes a command, otherwise it will be a plain variable. The
 * commands def and = can be used as synonyms of set.
 * [Symbol management commands]
 */
static int _filpf_set(void)
/** @variable @content set */
/** @variable @content def */
/** @variable @content = */
/** @command { @content } set */
{
    struct filp_val *value;
    struct filp_val *name;
    struct filp_sym *s;

    /* gets value and name */
    value = filp_pop();
    name = filp_pop();

    if (name->type != FILP_SCALAR) {
        _filp_error = FILPERR_SCALAR_EXPECTED;
        return FILP_ERROR;
    }

    /* gets the symbol name or creates it if does not exist */
    if ((s = filp_find_symbol(name->value)) == NULL)
        s = filp_new_symbol(value->type, name->value);

    filp_set_symbol(s, value);

    return FILP_OK;
}


/**
 * unset - Undefines a variable.
 * @variable: variable name to be undefined
 *
 * Undefines a variable. The command undef is a synonym.
 * [Symbol management commands]
 */
static int _filpf_unset(void)
/** <variable> unset */
/** <variable> undef */
{
    struct filp_val *v;
    struct filp_sym *s;

    v = filp_pop();

    if (v->type != FILP_SCALAR) {
        _filp_error = FILPERR_SCALAR_EXPECTED;
        return FILP_ERROR;
    }

    if ((s = filp_find_symbol(v->value)) != NULL)
        filp_destroy_symbol(s);

    return FILP_OK;
}


/**
 * type - Returns a string describing the type of a value.
 * @value: the symbol or value.
 *
 * Returns a string describing the type of a value. If value is
 * a name of a symbol, the type of its content is returned; otherwise,
 * the value type itself is returned.
 * The returned value can be one of SCALAR, CODE, BIN_CODE, EXT_INT,
 * EXT_REAL, EXT_STRING, NULL, FILE or ARRAY.
 * [Symbol management commands]
 */
static int _filpf_type(void)
/** @value type %type_string */
{
    struct filp_val *v;
    struct filp_sym *s;
    static char *types[] = { "SCALAR", "CODE", "BIN_CODE", "EXT_INT",
        "EXT_REAL", "EXT_STRING", "NULL", "FILE", "ARRAY"
    };

    v = filp_pop();

    if ((s = filp_find_symbol(v->value)) == NULL)
        filp_scalar_push(types[v->type]);
    else
        filp_scalar_push(types[s->type]);

    return FILP_OK;
}


/**
 * defined - Tests if a symbol is defined.
 * @symbol: the symbol name to be tested
 *
 * Returns a boolean value telling whether the symbol is
 * defined or not.
 * [Symbol management commands]
 */
static int _filpf_defined(void)
/** @symbol defined %bool_result */
{
    struct filp_val *v;

    v = filp_pop();

    if (filp_find_symbol(v->value) == NULL)
        filp_bool_push(0);
    else
        filp_bool_push(1);

    return FILP_OK;
}


/**
 * val - Returns the value of a symbol.
 * @string: the symbol name which value has to be gotten
 *
 * Treats the string as a symbol name and returns its value.
 * If it does not exist as a symbol, NULL is returned instead.
 * [Symbol management commands]
 */
static int _filpf_val(void)
/** @string val %content */
{
    struct filp_val *v;

    v = filp_pop();
    filp_push_symbol_value(v->value);

    return FILP_OK;
}


/**
 * exec - Executes a value as filp code.
 * @value: value to be executed
 *
 * Executes a value as filp code. It can be a string or a
 * block of code.
 * [Code execution commands]
 */
static int _filpf_exec(void)
/** @value exec */
{
    return filp_execv(filp_pop());
}


/**
 * eval - Executes a value as filp code and returns its error code.
 * @value: value to be executed
 *
 * Executes a value as filp code. It can be a string or a
 * block of code. The error code will be put on the top of stack,
 * and the execution is never interrupted.
 * [Code execution commands]
 */
static int _filpf_eval(void)
/** @value eval %error_code */
{
    int tmp;

    _filp_error = FILPERR_NONE;
    tmp = _filp_isolate;

    filp_execv(filp_pop());

    filp_int_push(_filp_error);

    _filp_error = FILPERR_NONE;
    _filp_isolate = tmp;

    return FILP_OK;
}


/**
 * symbol - Sends symbol names to the stack.
 * @prefix: prefix of the symbol names to send to the stack
 *
 * Sends to the stack as a list all symbol names beginning with @prefix.
 * If @prefix is "" (the empty string), all symbol names will be pushed.
 * [Symbol management commands]
 */
static int _filpf_symbol(void)
/** @prefix symbol [ %symbols ... ] */
{
    struct filp_val *v;

    v = filp_pop();
    filp_push_dict(v->value);

    return FILP_OK;
}


/**
 * add - Math sum.
 * @op1: first operand
 * @op2: second operand
 *
 * Sums both values and sends the result to the stack.
 * If filp_real is set, the operation is made in real mode,
 * or integer otherwise.
 * [Math commands]
 */
/** @op1 @op2 add %result */
/** @op1 @op2 + %result */
/* ; */

/**
 * sub - Math substraction.
 * @min: minuend
 * @subt: subtrahend
 *
 * Substracts @subt from @min and sends the result to the stack.
 * If filp_real is set, the operation is made in real mode,
 * or integer otherwise.
 * [Math commands]
 */
/** @min @subt sub %result */
/** @min @subt - %result */
/* ; */

/**
 * mul - Math multiply.
 * @op1: first operand
 * @op2: second operand
 *
 * Multiplies both values and sends the result to the stack.
 * If filp_real is set, the operation is made in real mode,
 * or integer otherwise.
 * [Math commands]
 */
/** @op1 @op2 mul %result */
/** @op1 @op2 * %result */
/* ; */

/**
 * div - Math division.
 * @divd: dividend
 * @divs: divisor
 *
 * Divides @divd by @divs and sends the result to the stack.
 * If filp_real is set, the operation is made in real mode,
 * or integer otherwise.
 * [Math commands]
 */
/** @divd @divs div %result */
/** @divd @divs / %result */
/* ; */

/**
 * mod - Math modulo.
 * @divd: dividend
 * @divs: divisor
 *
 * Divides @divd by @divs and sends the remainder to the stack.
 * This operation is always made in integer mode regardless
 * of the value of filp_real.
 * [Math commands]
 */
/** @divd @divs mod %remainder */
/** @divd @divs % %remainder */
/* ; */

static int _filpf_bmath(char *token)
{
    struct filp_val *v1;
    struct filp_val *v2;
    struct filp_val *r;
    int res;
    double rres;

    v2 = filp_pop();
    v1 = filp_pop();

    /* if operator is mod (%), always work in integer mode */
    if (!_filp_real || strcmp(token, "%") == 0) {
        if (strcmp(token, "+") == 0)
            res = filp_val_to_int(v1) + filp_val_to_int(v2);
        else if (strcmp(token, "-") == 0)
            res = filp_val_to_int(v1) - filp_val_to_int(v2);
        else if (strcmp(token, "*") == 0)
            res = filp_val_to_int(v1) * filp_val_to_int(v2);
        else if (strcmp(token, "/") == 0)
            res = filp_val_to_int(v1) / filp_val_to_int(v2);
        else if (strcmp(token, "%") == 0)
            res = filp_val_to_int(v1) % filp_val_to_int(v2);
        else {
            _filp_error = FILPERR_INTERNAL_ERROR;
            res = -1;
        }

        r = filp_new_int_value(res);
    }
    else {
        if (strcmp(token, "+") == 0)
            rres = filp_val_to_real(v1) + filp_val_to_real(v2);
        else if (strcmp(token, "-") == 0)
            rres = filp_val_to_real(v1) - filp_val_to_real(v2);
        else if (strcmp(token, "*") == 0)
            rres = filp_val_to_real(v1) * filp_val_to_real(v2);
        else if (strcmp(token, "/") == 0)
            rres = filp_val_to_real(v1) / filp_val_to_real(v2);
        else {
            _filp_error = FILPERR_INTERNAL_ERROR;
            rres = -1;
        }

        r = filp_new_real_value(rres);
    }

    filp_push(r);

    return FILP_OK;
}


static int _filpf_bmath_add(void)
{
    return _filpf_bmath("+");
}
static int _filpf_bmath_sub(void)
{
    return _filpf_bmath("-");
}
static int _filpf_bmath_mul(void)
{
    return _filpf_bmath("*");
}
static int _filpf_bmath_div(void)
{
    return _filpf_bmath("/");
}
static int _filpf_bmath_mod(void)
{
    return _filpf_bmath("%");
}


static int _filpf_bimath(char *token)
/** @symbol @value += */
/** @symbol @value -= */
/** @symbol @value *= */
/** @symbol @value /= */
{
    struct filp_val *v1;
    struct filp_val *v2;
    struct filp_val *v3;
    struct filp_val *name;
    struct filp_sym *s;
    int ret;
    double rret;

    v1 = filp_pop();
    name = filp_pop();

    if ((s = filp_find_symbol(name->value)) != NULL) {
        /* the variable exists: get its value */
        v2 = filp_get_symbol(s);

        if (_filp_real) {
            if (strcmp(token, "+=") == 0)
                rret = filp_val_to_real(v2) + filp_val_to_real(v1);
            else if (strcmp(token, "-=") == 0)
                rret = filp_val_to_real(v2) - filp_val_to_real(v1);
            else if (strcmp(token, "*=") == 0)
                rret = filp_val_to_real(v2) * filp_val_to_real(v1);
            else if (strcmp(token, "/=") == 0)
                rret = filp_val_to_real(v2) / filp_val_to_real(v1);
            else {
                _filp_error = FILPERR_INTERNAL_ERROR;
                rret = -1;
            }

            v3 = filp_new_real_value(rret);
        }
        else {
            if (strcmp(token, "+=") == 0)
                ret = filp_val_to_int(v2) + filp_val_to_int(v1);
            else if (strcmp(token, "-=") == 0)
                ret = filp_val_to_int(v2) - filp_val_to_int(v1);
            else if (strcmp(token, "*=") == 0)
                ret = filp_val_to_int(v2) * filp_val_to_int(v1);
            else if (strcmp(token, "/=") == 0)
                ret = filp_val_to_int(v2) / filp_val_to_int(v1);
            else {
                _filp_error = FILPERR_INTERNAL_ERROR;
                ret = -1;
            }

            v3 = filp_new_int_value(ret);
        }

        filp_set_symbol(s, v3);
    }

    return FILP_OK;
}


static int _filpf_bimath_add(void)
{
    return _filpf_bimath("+=");
}
static int _filpf_bimath_sub(void)
{
    return _filpf_bimath("-=");
}
static int _filpf_bimath_mul(void)
{
    return _filpf_bimath("*=");
}
static int _filpf_bimath_div(void)
{
    return _filpf_bimath("/=");
}


/**
 * eq - String equality test.
 * @string1: the first string
 * @string2: the second string
 *
 * Compares the two strings and returns true if both are equal.
 * [String manipulation commands]
 * [Boolean commands]
 */
/** @string1 @string2 eq %bool_value */
/* ; */

/**
 * gt - String 'greater than' test.
 * @string1: the first string
 * @string2: the second string
 *
 * Compares the two strings and returns true if the first one
 * is greater (in ASCII) than the second one.
 * [String manipulation commands]
 * [Boolean commands]
 */
/** @string1 @string2 gt %bool_value */
/* ; */

/**
 * lt - String 'lower than' test.
 * @string1: the first string
 * @string2: the second string
 *
 * Compares the two strings and returns true if the first one
 * is lower (in ASCII) than the second one.
 * [String manipulation commands]
 * [Boolean commands]
 */
/** @string1 @string2 lt %bool_value */
/* ; */

static int _filpf_cmp(char *token)
{
    struct filp_val *v1;
    struct filp_val *v2;
    int ret, ret2;

    v1 = filp_pop();
    v2 = filp_pop();

    ret = filp_cmp(v1, v2);

    ret2 = 0;
    if (strcmp(token, "eq") == 0)
        ret2 = (ret == 0) ? 1 : 0;
    else if (strcmp(token, "gt") == 0)
        ret2 = (ret < 0) ? 1 : 0;
    else if (strcmp(token, "lt") == 0)
        ret2 = (ret > 0) ? 1 : 0;

    filp_bool_push(ret2);

    return FILP_OK;
}


static int _filpf_cmp_eq(void)
{
    return _filpf_cmp("eq");
}
static int _filpf_cmp_gt(void)
{
    return _filpf_cmp("gt");
}
static int _filpf_cmp_lt(void)
{
    return _filpf_cmp("lt");
}

static int _filpf_mcmp(char *token)
{
    struct filp_val *v1;
    struct filp_val *v2;
    double n1, n2, ret;
    int ret2;

    v1 = filp_pop();
    v2 = filp_pop();

    n1 = filp_val_to_real(v1);
    n2 = filp_val_to_real(v2);
    ret = n1 - n2;

    ret2 = 0;
    if (strcmp(token, "==") == 0)
        ret2 = (ret == 0) ? 1 : 0;
    else if (strcmp(token, ">") == 0)
        ret2 = (ret < 0) ? 1 : 0;
    else if (strcmp(token, "<") == 0)
        ret2 = (ret > 0) ? 1 : 0;

    filp_bool_push(ret2);

    return FILP_OK;
}


static int _filpf_mcmp_eq(void)
{
    return _filpf_mcmp("==");
}
static int _filpf_mcmp_gt(void)
{
    return _filpf_mcmp(">");
}
static int _filpf_mcmp_lt(void)
{
    return _filpf_mcmp("<");
}


/**
 * and - Boolean and.
 * @bool1: first boolean value.
 * @bool2: second boolean value.
 *
 * Returns true if both values are true. The command &&
 * can be used as a synonym.
 * [Boolean commands]
 */
/** @bool1 @bool2 and @bool_result */
/** @bool1 @bool2 && @bool_result */
/* ; */

/**
 * or - Boolean or.
 * @bool1: first boolean value.
 * @bool2: second boolean value.
 *
 * Returns true if any value are true. The command ||
 * can be used as a synonym.
 * [Boolean commands]
 */
/** @bool1 @bool2 or @bool_result */
/** @bool1 @bool2 || @bool_result */
/* ; */

static int _filpf_bool(char *token)
{
    struct filp_val *v1;
    struct filp_val *v2;
    int b1, b2;
    int ret;

    v1 = filp_pop();
    v2 = filp_pop();

    b1 = filp_is_true(v1);
    b2 = filp_is_true(v2);

    if (strcmp(token, "&&") == 0)
        ret = (b1 && b2);
    else if (strcmp(token, "||") == 0)
        ret = (b1 || b2);
    else
        ret = 0;

    filp_bool_push(ret);

    return FILP_OK;
}


static int _filpf_bool_and(void)
{
    return _filpf_bool("&&");
}
static int _filpf_bool_or(void)
{
    return _filpf_bool("||");
}

/**
 * if - Conditional execution of code.
 * @bool: boolean value
 * @code_block: code block to execute
 *
 * Executes the block of code if @bool is true.
 * [Control structures]
 */
/** @bool @code_block if */
/* ; */

/**
 * unless - Conditional execution of code.
 * @bool: boolean value
 * @code_block: code block to execute
 *
 * Executes the block of code if @bool is false.
 * [Control structures]
 */
/** @bool @code_block unless */
/* ; */

static int _filpf_if_unless(char *token)
{
    struct filp_val *code;
    struct filp_val *cond;
    int ret;

    code = filp_pop();
    cond = filp_pop();

    ret = filp_is_true(cond);
    if (strcmp(token, "unless") == 0)
        ret = !ret;

    if (ret)
        ret = filp_execv(code);

    /* if & unless must propagate a FILP_BREAK */

    return ret;
}


static int _filpf_if(void)
{
    return _filpf_if_unless("if");
}
static int _filpf_unless(void)
{
    return _filpf_if_unless("unless");
}


/**
 * ifelse - Conditional execution of code.
 * @bool: boolean value
 * @code_block_true: code block to execute when bool is true
 * @code_block_false: code block to execute when bool is false
 *
 * Executes the first block of code if @bool is true
 * or the second if false.
 * [Control structures]
 */
static int _filpf_ifelse(void)
/** @bool @code_block_true @code_block_false ifelse */
{
    struct filp_val *if_code;
    struct filp_val *else_code;
    struct filp_val *cond;
    int ret;

    else_code = filp_pop();
    if_code = filp_pop();
    cond = filp_pop();

    if (filp_is_true(cond))
        ret = filp_execv(if_code);
    else
        ret = filp_execv(else_code);

    return ret;
}

/**
 * repeat - Executes a block of code a number of times.
 * @times: number of repetitions
 * @code_block: block of code to execute
 *
 * Executes a block of code a number of @times.
 * [Control structures]
 */
static int _filpf_repeat(void)
/** @times @code_block repeat */
{
    struct filp_val *code;
    int ret;
    int n, i;

    code = filp_pop();
    i = filp_int_pop();

    for (ret = 0, n = 0; n < i && !ret; n++)
        ret = filp_execv(code);

    /* repeat *must not* propagate a FILP_BREAK */
    if (ret == FILP_BREAK)
        ret = FILP_OK;

    return ret;
}


/**
 * loop - The infinite loop.
 * @code_block: the block of code to execute eternally
 *
 * Executes @code_block until the end of times.
 * [Control structures]
 */
static int _filpf_loop(void)
/** @code_block loop */
{
    int ret;
    struct filp_val *v;

    v = filp_pop();

    while ((ret = filp_execv(v)) == FILP_OK);

    /* loop *must not* propagate a FILP_BREAK */
    if (ret == FILP_BREAK)
        ret = FILP_OK;

    return ret;
}


/**
 * foreach - Executes a block of code for each element of a list.
 * @list_elements: the list elements
 * @code_block: block of code
 *
 * Executes a block of code for each element of a list, that is,
 * until there is a NULL in the top of stack. The code block must
 * take the values from the top of stack on each iteration. The NULL
 * value is automatically dropped.
 * [Control structures]
 * [List processing commands]
 */
static int _filpf_foreach(void)
/** [ @list_elements ... ] @code_block foreach */
{
    struct filp_val *v;
    struct filp_val *code;
    int ret = 0;

    code = filp_pop();

    for (;;) {
        v = filp_pop();
        if (v->type == FILP_NULL)
            break;

        filp_push(v);
        if ((ret = filp_execv(code)) != FILP_OK)
            break;
    }

    while (v->type != FILP_NULL)
        v = filp_pop();

    /* foreach *must not* propagate FILP_BREAK */
    if (ret == FILP_BREAK)
        ret = FILP_OK;

    return ret;
}


/**
 * for - Executes a block incrementing a value.
 * @from: initial value
 * @inc: increment to the value
 * @to: final value
 *
 * Executes the block of code, sending previously a value
 * to the stack. This value ranges from the initial @from
 * value, being incremented by @inc until @to (inclusive).
 * The block of code must take this value from the stack.
 * [Control structures]
 */
static int _filpf_for(void)
/** @from @inc @to @code_block for */
{
    struct filp_val *code;
    int n, inc, ev, iv;
    int ret = 0;

    code = filp_pop();

    ev = filp_int_pop();
    inc = filp_int_pop();
    iv = filp_int_pop();

    for (n = iv; n <= ev; n += inc) {
        filp_int_push(n);

        if ((ret = filp_execv(code)) != FILP_OK)
            break;
    }

    /* for *must not* propagate FILP_BREAK */
    if (ret == FILP_BREAK)
        ret = FILP_OK;

    return ret;
}


/**
 * while - Conditional loop.
 * @condition_code: Code to execute to evaluate condition
 * @repeat_code: Code to execute if condition code is not NULL
 *
 * While the execution of @condition_code returns a non-NULL value,
 * @repeat_code is also executed, having that value available in the top
 * of stack for being processed. The NULL result of condition_code
 * is clean from the stack.
 * [Control structures]
 */
static int _filpf_while(void)
/** @condition_code @repeat_code while */
{
    struct filp_val *code;
    struct filp_val *cond;
    struct filp_val *v;
    int ret = 0;

    code = filp_pop();
    cond = filp_pop();

    filp_ref_value(code);
    filp_ref_value(cond);

    while (!ret) {
        if ((ret = filp_execv(cond)) != FILP_OK)
            break;

        v = filp_pop();
        if (v->type == FILP_NULL)
            break;

        filp_push(v);
        ret = filp_execv(code);
    }

    filp_unref_value(cond);
    filp_unref_value(code);

    /* while *must not* propagate FILP_BREAK */
    if (ret == FILP_BREAK)
        ret = FILP_OK;

    return ret;
}


/**
 * switch - Multiple conditional execution.
 * @condition_code: Code to be executed for the condition test
 * @execution_code: Code to be executed if condition is true
 *
 * Executes from top to down the condition block codes and, if
 * one of them returns a true value, executes the associated execution
 * code block and continues on the next instruction. The first
 * condition / execution code pairs being run are the nearest to
 * the switch instruction, and all of them are really a list, so
 * the [ terminator must not be forgotten. To implement a clause
 * similar to the 'default' one from the C language switch() command,
 * it must be the last one and its condition code be always true.
 * [Control structures]
 */
static int _filpf_switch(void)
/** [ @condition_code @execute_code ... ] switch */
{
    struct filp_val *code;
    struct filp_val *cond;
    struct filp_val *v;
    int ret = 0;
    int found_null = 0;
    int found = 0;

    code = NULL;

    while (!found_null) {
        code = filp_pop();

        if (code->type == FILP_NULL) {
            found_null = 1;
            break;
        }

        filp_ref_value(code);

        cond = filp_pop();
        if ((ret = filp_execv(cond)) < 0) {
            filp_unref_value(code);
            break;
        }

        v = filp_pop();
        if (filp_is_true(v)) {
            found = 1;
            break;
        }

        filp_unref_value(code);
    }

    /* clean stack until NULL */
    while (!found_null) {
        v = filp_pop();

        if (v->type == FILP_NULL)
            found_null = 1;
    }

    if (found) {
        ret = filp_execv(code);
        filp_unref_value(code);
    }

    /* switch *must not* propagate FILP_BREAK */
    if (ret == FILP_BREAK)
        ret = FILP_OK;

    return ret;
}


/**
 * reverse - Reverses a list.
 * @elem-1: elements of the list
 *
 * Reverses a list.
 * [List processing commands]
 */
static int _filpf_reverse(void)
/** [ @elem-1 @elem-2 ... @elem-n ] reverse [ @elem-n ... @elem-2 @elem-1 ] */
{
    struct filp_val *v;
    int n;

    /* the filp version was
       filp_exec("/reverse { [ # /_ 3 = { $_ rot } { /_ ++ } while } set");
       But the while test does not work if a 0 value is found.
     */

    filp_exec("[ #");

    for (n = 3;; n++) {
        filp_rot(n);

        v = filp_stack_value(1);

        if (v->type == FILP_NULL) {
            filp_pop();
            break;
        }
    }

    return FILP_OK;
}


/**
 * seek - Seeks an element in a list.
 * @elements_of_list: elements of the list
 * @value: value to be searched for
 *
 * Seeks an element in the list. If the element is found,
 * its offset is returned on the top of stack; in not, 0
 * is returned instead. Take note that list elements are
 * numbered from 1.
 * [List processing commands]
 */
static int _filpf_seek(void)
/** [ @elements_of_list ] @value seek %offset */
{
    struct filp_val *o;
    struct filp_val *v;
    int ret;

    o = filp_pop();

    for (ret = 1;; ret++) {
        v = filp_pop();
        if (v->type == FILP_NULL) {
            ret = 0;
            break;
        }

        if (filp_cmp(v, o) == 0)
            break;
    }

    while (v->type != FILP_NULL)
        v = filp_pop();

    filp_int_push(ret);

    return FILP_OK;
}


/**
 * index - Extract an element from a list by subscript.
 * @elements_of_list: the elements of the list
 * @offset: offset of the element to be extracted
 *
 * Extract the element number @offset from the list, and
 * puts it on the top of stack. If the @offset is out of
 * range (i.e. there are less elements in the list), NULL
 * is returned instead.
 * [List processing commands]
 */
static int _filpf_index(void)
/** [ @elements_of_list ] @offset index %value */
{
    struct filp_val *v;
    struct filp_val *c;
    int m, n;

    n = filp_int_pop();
    c = NULL;

    for (m = 1; n != 0 && m < n; m++)
        filp_pop();

    if (n)
        c = filp_pop();

    for (;;) {
        v = filp_pop();
        if (v->type == FILP_NULL)
            break;
    }

    if (c != NULL)
        filp_push(c);
    else
        filp_null_push();

    return FILP_OK;
}


/**
 * load - Loads a filp source code file and executes it.
 * @file_name: the name of the file to be executed.
 *
 * Loads a filp source code file and executes it.
 * [Code execution commands]
 */
static int _filpf_load(void)
/** @file_name load */
{
    struct filp_val *v;
    int ret;

    v = filp_pop();
    ret = filp_load_exec(v->value);

    return ret;
}


/**
 * strcat - Concatenates two strings.
 * @str1: the first string
 * @str2: the second string
 *
 * Concatenates two strings. The . command can be used as a synonym.
 * [String manipulation commands]
 */
static int _filpf_strcat(void)
/** @str1 @str2 strcat %str1str2 */
/** @str1 @str2 . %str1str2 */
{
    struct filp_val *first;
    struct filp_val *last;
    int l;
    char *str;

    last = filp_pop();
    first = filp_pop();

    if (first->type != FILP_SCALAR || last->type != FILP_SCALAR) {
        filp_null_push();
        _filp_error = FILPERR_SCALAR_EXPECTED;
        return FILP_ERROR;
    }

    l = strlen(first->value) + strlen(last->value) + 1;
    if ((str = malloc(l)) == NULL) {
        _filp_error = FILPERR_OUT_OF_MEMORY;
        return FILP_ERROR;
    }

    strcpy(str, first->value);
    strcat(str, last->value);

    filp_scalar_push(str);
    free(str);

    return FILP_OK;
}


/**
 * length - Returns the length of a string.
 * @string: the string
 *
 * Returns the length of the string.
 * [String manipulation commands]
 */
static int _filpf_length(void)
/** @string length %len */
{
    struct filp_val *v;
    int i;

    v = filp_pop();

    if (v->type != FILP_SCALAR) {
        filp_null_push();
        _filp_error = FILPERR_SCALAR_EXPECTED;
        return FILP_ERROR;
    }

    i = strlen(v->value);

    filp_int_push(i);

    return FILP_OK;
}


/**
 * substr - Extracts a substring.
 * @string: the string to be extracted from
 * @offset: offset of the first character in string
 * @size: number of characters to be extracted
 *
 * Extracts a substring from the @string, starting at @offset.
 * Take note that string subscripts start from 1.
 * [String manipulation commands]
 */
static int _filpf_substr(void)
/** @string @offset @size substr %substring */
{
    struct filp_val *s;
    int org, num, l;
    char *str;

    num = filp_int_pop();
    org = filp_int_pop();

    s = filp_pop();
    l = strlen(s->value);
    str = (char *) malloc(l + 1);

    if (num <= 0 || org <= 0) {
        num = org = 0;
    }

    org--;
    if (org + num > l)
        num = l - org;

    if (num <= 0)
        num = 0;

    strncpy(str, &s->value[org], num);
    str[num] = '\0';

    filp_scalar_push(str);

    free(str);

    return FILP_OK;
}


static int _filpf_splice(void)
/** @str @offset @size @new splice %new_string */
{
    struct filp_val *new;
    struct filp_val *str;
    int offset, size;
    char *ptr;

    new = filp_pop();
    size = filp_int_pop();
    offset = filp_int_pop();
    str = filp_pop();

    ptr = filp_splice(str->value, offset, size, new->value);

    filp_scalar_push(ptr);

    free(ptr);

    return FILP_OK;
}


/**
 * instr - Search a string inside another.
 * @string: the string to be searched into
 * @substring: the substring to be searched
 *
 * Search a @substring inside a @string. If it's found,
 * the offset is returned, or 0 instead. Remember that
 * string offsets start from 1.
 * [String manipulation commands]
 */
static int _filpf_instr(void)
/** @string @substring instr %offset */
{
    struct filp_val *ss;
    struct filp_val *s;
    int n, l;

    ss = filp_pop();
    s = filp_pop();

    if (ss->type != FILP_SCALAR || s->type != FILP_SCALAR) {
        _filp_error = FILPERR_SCALAR_EXPECTED;
        return FILP_ERROR;
    }

    l = strlen(ss->value);

    for (n = 0; s->value[n]; n++) {
        if (memcmp(&s->value[n], ss->value, l) == 0)
            break;
    }

    if (s->value[n] == '\0')
        n = -1;

    filp_int_push(n + 1);

    return FILP_OK;
}


/**
 * split - Splits a string into substrings.
 * @string: the string to be splitted
 * @separator: the separator
 *
 * Splits a @string into a list by the @separator. If @separator
 * is an empty string, @string is splitted by char.
 * [String manipulation commands]
 * [List processing commands]
 */
static int _filpf_split(void)
/** @string @separator split [ @string_slices ] */
{
    struct filp_val *s;
    struct filp_val *v;
    char *ptr;
    char *wrk;

    v = filp_pop();
    s = filp_pop();

    filp_null_push();

    if (s->type != FILP_SCALAR || v->type != FILP_SCALAR) {
        _filp_error = FILPERR_SCALAR_EXPECTED;
        return FILP_ERROR;
    }

    if (v->value[0] == '\0') {
        char tmp[2];

        tmp[1] = '\0';
        for (ptr = s->value; *ptr; ptr++) {
            tmp[0] = *ptr;
            filp_scalar_push(tmp);
        }
    }
    else {
        /* creates a working copy (as strtok destroys it) */
        wrk = (char *) malloc(strlen(s->value) + 1);
        strcpy(wrk, s->value);

        ptr = strtok(wrk, v->value);

        while (ptr != NULL) {
            filp_scalar_push(ptr);
            ptr = strtok(NULL, v->value);
        }

        free(wrk);
    }

    return FILP_OK;
}


/**
 * sprintf - Formats into a string.
 * @value: values to be inserted
 * @format_string: printf style formatting string
 *
 * Makes a printf() -like formatting into a string. As in that
 * function, the percent char is used as a placeholder for
 * a formatting command.
 * [String manipulation commands]
 */
static int _filpf_sprintf(void)
/** @value [ @value ... ] @format_string sprintf %result_string */
{
    struct filp_val *s;
    struct filp_val *v;
    int ivalue;
    double rvalue;
    char *str = NULL;
    int size, n, m, c, i;
    char tmp[2048];
    char tmp2[2048];

    s = filp_pop();

    for (n = m = size = 0; (c = s->value[n++]) != '\0';) {
        if (c == '%') {
            if ((c = s->value[n++]) == '%')
                str = filp_poke(str, &size, m++, '%');
            else {
                tmp[0] = '%';
                for (i = 1; c != '\0' && strchr("-.0123456789", c) != NULL;
                     i++) {
                    tmp[i] = c;
                    c = s->value[n++];
                }
                tmp[i++] = c;
                tmp[i] = '\0';

                switch (tmp[i - 1]) {
                case 'd':
                case 'i':
                case 'x':
                case 'X':
                case 'o':
                case 'c':
                    ivalue = filp_int_pop();
                    sprintf(tmp2, tmp, ivalue);
                    break;
                case 'f':
                    rvalue = filp_real_pop();
                    sprintf(tmp2, tmp, rvalue);
                    break;
                case 's':
                    v = filp_pop();
                    sprintf(tmp2, tmp, v->value);
                    break;
                default:
                    strcpy(tmp, tmp2);
                    break;
                }

                /* transfer */
                for (i = 0; tmp2[i]; i++)
                    str = filp_poke(str, &size, m++, tmp2[i]);
            }
        }
        else
            str = filp_poke(str, &size, m++, c);
    }

    str = filp_poke(str, &size, m++, '\0');
    filp_scalar_push(str);
    free(str);

    return FILP_OK;
}


/**
 * sscanf - Scans a string and extracts values.
 * @string: the string to be scanned
 * @format_string: the format string
 *
 * Scans a string and extracts values using a scanf() -like
 * format string.
 * [String manipulation commands]
 */
static int _filpf_sscanf(void)
/* funci?n 'sscanf' */
/** @string @format_string sscanf @value1 @value2 ... */
{
    struct filp_val *fv;
    struct filp_val *vv;
    char *f;
    char *v;
    char *str = NULL;
    char *ptr;
    int i, size, dsize, t;
    char cmd;

    fv = filp_pop();
    vv = filp_pop();

    f = fv->value;
    v = vv->value;

    size = dsize = 0;

    filp_null_push();

    t = _filp_stack_elems;

    while (*v && *f) {
        cmd = *f;
        if (cmd == '%') {
            int ignore = 0;

            /* which format? */
            f++;
            dsize = 0;

            /* if * follows, value will not be used */
            if (*f == '*') {
                ignore = 1;
                f++;
            }

            /* calculate size, if any */
            while (strchr("0123456789", *f) != NULL) {
                dsize *= 10;
                dsize += (*f) - '0';
                f++;
            }

            if (*f == 's') {
                f++;

                /* it's a string; store it */
                if (dsize) {
                    /* with size */
                    for (i = 0; i < dsize && *v; i++, v++)
                        str = filp_poke(str, &size, i, *v);
                    str = filp_poke(str, &size, i, '\0');
                }
                else {
                    /* no size: next char in format string
                       is used as a delimiter */
                    for (i = 0; *v != *f && *v; i++, v++)
                        str = filp_poke(str, &size, i, *v);
                    str = filp_poke(str, &size, i, '\0');
                }

                if (cmd == '%' && !ignore)
                    filp_scalar_push(str);
            }
            else if (*f == 'f' || *f == 'd' || *f == 'i' || *f == 'u' || *f == 'x') {
                if (*f == 'f')
                    ptr = "-.0123456789";
                else if (*f == 'x')
                    ptr = "-x0123456789abcdefABCDEF";
                else if (*f == 'u')
                    ptr = "0123456789";
                else
                    ptr = "-0123456789";

                /* take chars while valid */
                for (i = 0; strchr(ptr, *v) != NULL && *v; i++, v++)
                    str = filp_poke(str, &size, i, *v);
                str = filp_poke(str, &size, i, '\0');

                if (*str && !ignore) {
                    if (*f == 'd' || *f == 'i' || *f == 'u') {
                        int i;

                        i = atoi(str);
                        filp_int_push(i);
                    }
                    else
                        filp_scalar_push(str);
                }

                f++;
            }
            else if (*f == '%') {
                if (*f == *v)
                    f++;
                else
                    f--;
                v++;
            }
        }
        else {
            if (*f == *v)
                f++;
            v++;
        }
    }

    if (str)
        free(str);

    /* reverses and destroy the NULL list delimiter */
    if (t != _filp_stack_elems)
        filp_execf("reverse %d rot pop", _filp_stack_elems - t + 1);

    return FILP_OK;
}


/** arrays **/

/**
 * array - Creates an array.
 * @value: values that are part of the array.
 *
 * Creates an array. The ) character can be used as a synonym,
 * and it's preferred as it's more intuitive. The new array
 * value is left on the top of stack, ready to be asigned
 * or used.
 * [Array commands]
 */
static int _filpf_array(void)
/** ( @value @value ... ) %new_array */
/** ( @value @value array %new_array */
{
    struct filp_val *a;
    struct filp_val *e;
    int n;

    n = filp_list_size();

    a = filp_new_value(FILP_ARRAY, NULL, n);

    for (; n > 0; n--) {
        e = filp_pop();
        if (e->type == FILP_NULL)
            break;

        filp_array_set(a, e, n);
    }

    /* here there should be a NULL, so drop it */
    filp_pop();

    filp_push(a);

    return FILP_OK;
}


static struct filp_val *filp_array_pop(int *imm, int create)
{
    struct filp_sym *s;
    struct filp_val *a;
    struct filp_val *v;

    v = filp_pop();

    a = NULL;

    if (v->type == FILP_SCALAR) {
        /* it's not an immediate value */
        *imm = 0;

        /* is it a symbol name? */
        if ((s = filp_find_symbol(v->value)) != NULL) {
            if (s->value->type == FILP_ARRAY)
                a = s->value;
        }
        else if (create) {
            a = filp_new_value(FILP_ARRAY, NULL, 0);

            s = filp_new_symbol(FILP_ARRAY, v->value);
            filp_set_symbol(s, a);
        }
    }
    else if (v->type == FILP_ARRAY) {
        *imm = 1;
        a = v;
    }

    if (a == NULL)
        _filp_error = FILPERR_ARRAY_EXPECTED;

    return a;
}


/**
 * aget - Gets an element from an array.
 * @array: the array
 * @subscript: the number of the element to be taken
 *
 * Gets the element number @subscript from the @array. If
 * @subscript is out of range, NULL is returned instead.
 * The array subscripts in filp starts at 1; the 0 element
 * contains the total number of elements in the array.
 * [Array commands]
 */
static int _filpf_array_get(void)
/** @array @subscript @ %element */
/** @array @subscript aget %element */
{
    int n, i;
    struct filp_val *a;
    struct filp_val *v;

    n = filp_int_pop();

    if ((a = filp_array_pop(&i, 0)) == NULL)
        return FILP_ERROR;

    if (n == 0)
        filp_int_push(filp_array_size(a));
    else if ((v = filp_array_get(a, n)) != NULL)
        filp_push(v);
    else
        filp_null_push();

    return FILP_OK;
}


static int _filpf_array_setins(char *token)
{
    struct filp_val *v;
    struct filp_val *a;
    int n, i;
    int ret = 0;

    v = filp_pop();
    n = filp_int_pop();

    if ((a = filp_array_pop(&i, 1)) == NULL)
        return FILP_ERROR;

    if (strcmp(token, "ains") == 0)
        filp_array_ins(a, v, n);
    else
        filp_array_set(a, v, n);

    if (i)
        filp_push(a);

    return ret;
}


/**
 * aset - Assigns a value to an element of an array.
 * @array: the array or array symbol
 * @subscript: the subscript number to be asigned
 * @value: the value to be set
 *
 * Assigns a @value to the element @subscript of an @array.
 * Remember that the array subscripts in filp start at 1.
  * The @= command can be used as a synonym.
 * [Array commands]
 */
/** @array_symbol @subscript @value aset */
/** @array @subscript @value aset %modified_array */
static int _filpf_array_set(void)
{
    return _filpf_array_setins("aset");
}

/**
 * ains - Inserts a value into an array.
 * @array: the array or array symbol
 * @subscript: the subscript number of the insertion position
 * @value: the value to be inserted
 *
 * Inserts a @value into the @subscript position of an @array.
 * The elements with greater subscripts move up one position,
 * incrementing the total size of the array by 1.
 * Remember that the array subscripts in filp start at 1. If the
 * subscript is 0 (array size), the element is added to the end.
 * [Array commands]
 */
/** @array_symbol @subscript @value ains */
/** @array @subscript @value ains %modified_array */
static int _filpf_array_ins(void)
{
    return _filpf_array_setins("ains");
}


/**
 * adump - Dumps an array as a list.
 * @array: the array to be dumped.
 *
 * Dumps an array as a list.
 * [Array commands]
 */
static int _filpf_array_dump(void)
/** @array adump [ %array_elements ] */
{
    int i, n;
    struct filp_val *a;

    if ((a = filp_array_pop(&i, 0)) == NULL)
        return FILP_ERROR;

    filp_null_push();

    for (n = 1; n <= a->size; n++)
        filp_push(filp_array_get(a, n));

    return FILP_OK;
}


/**
 * adel - Deletes an element from an array.
 * @array: the array or array symbol
 * @subscript: subscript of the element to be deleted
 *
 * Deletes an element from an @array. The array shrinks.
 * If @subscript is zero, the last element is deleted and
 * if it's out of range, nothing happens.
 * [Array commands]
 */
static int _filpf_array_del(void)
/** @array_symbol @subscript adel */
/** @array @subscript adel %modified_array */
{
    int n, i;
    struct filp_val *a;

    n = filp_int_pop();

    if ((a = filp_array_pop(&i, 0)) == NULL)
        return FILP_ERROR;

    filp_array_del(a, n);

    if (i)
        filp_push(a);

    return FILP_OK;
}


/**
 * aseek - Seeks an array for a scalar.
 * @array: the array to be searched into
 * @scalar: the value to be found
 *
 * Seeks an array in search of a string.
 * Returns the subscript where the scalar is, or 0 if not found.
 * [Array commands]
 */
/** @array @scalar aseek %subscript */
int _filpf_array_seek(void)
{
    int i;
    struct filp_val *v;
    struct filp_val *a;

    v = filp_pop();

    if ((a = filp_array_pop(&i, 0)) == NULL)
        return FILP_ERROR;

    filp_int_push(filp_array_seek(a, v->value, 1));

    return FILP_OK;
}


/**
 * abseek - Seeks a sorted array for a scalar.
 * @array: the array to be searched into
 * @scalar: the value to be found
 *
 * Seeks a sorted array in search of a string.
 * Returns the subscript where the scalar is if positive,
 * or the position where it should be if negative.
 * [Array commands]
 */
/** @array @scalar abseek %subscript */
int _filpf_array_binary_seek(void)
{
    int i;
    struct filp_val *v;
    struct filp_val *a;

    v = filp_pop();

    if ((a = filp_array_pop(&i, 0)) == NULL)
        return FILP_ERROR;

    filp_int_push(filp_array_binary_seek(a, v->value, 1));

    return FILP_OK;
}


/**
 * asort - Sorts an array.
 * @array: array or array symbol to be sorted
 *
 * Sorts (alfabetically) an array.
 * [Array commands]
 */
/** @array_symbol asort */
/** @array asort %sorted_array */
int _filpf_array_sort(void)
{
    int i;
    struct filp_val *a;

    if ((a = filp_array_pop(&i, 0)) == NULL)
        return FILP_ERROR;

    filp_array_sort(a, 1);

    if (i)
        filp_push(a);

    return FILP_OK;
}


static int _filpf_forall_map(int reassign)
{
    int i, n;
    struct filp_val *a;
    struct filp_val *c;
    struct filp_val *v;

    c = filp_pop();
    if ((a = filp_array_pop(&i, 0)) == NULL)
        return FILP_ERROR;

    filp_ref_value(c);
    filp_ref_value(a);

    for (n = 1; n <= filp_array_size(a); n++) {
        if ((v = filp_array_get(a, n)) != NULL) {
            filp_push(v);
            filp_execv(c);

            if (reassign) {
                v = filp_pop();
                filp_array_set(a, v, n);
            }
        }
    }

    filp_unref_value(a);
    filp_unref_value(c);

    if (reassign && i)
        filp_push(a);

    return FILP_OK;
}


/**
 * forall - Executes a block of code for all elements of the array
 * @array: the array
 * @code: the code to be executed
 *
 * Executes a block of @code for each element of @array after pushing each
 * one to the stack. The code block must take the values from the top of
 * stack on each iteration.
 * [Control structures]
 * [Array commands]
 */
static int _filpf_forall(void)
/** @array { @code } forall */
{
    return _filpf_forall_map(0);
}


/**
 * map - Executes a block of code for all elements of the array modifying
 * @array: the array or array symbol
 * @code: the code to be executed
 *
 * Executes a block of @code for each element of @array after pushing each
 * one to the stack. At the end of the block, the value in the top of stack
 * will be assigned to the element. If @array is an immediate value (i.e.
 * not a symbol), the modified array is left on the stack.
 * [Control structures]
 * [Array commands]
 */
static int _filpf_map(void)
/** @array_symbol { @code } map */
/** @array { @code } map %modified_array */
{
    return _filpf_forall_map(1);
}


/**
 * license - Returns the filp license.
 *
 * Returns a text string containing the license of the filp library
 * [Special constants]
 */
static int _filpf_license(void)
/** license %license_text */
{
    filp_scalar_push(_filp_license);
    return FILP_OK;
}



/**
 * sweep - Collects garbage.
 *
 * Calls the internal garbage collector.
 * [Special commands]
 */
static int _filpf_sweep(void)
/** sweep */
{
    filp_sweeper(1);
    return FILP_OK;
}


/**
 * dumper - Dumps a value (probably an array) as a tree.
 * @value: the value to be dumped
 *
 * Dumps a value (probably an array) as a filp-parseable tree.
 */
static int _filpf_dumper(void)
/** @value dumper %value_tree */
{
    char *ptr;

    ptr = filp_dumper(filp_pop(), 3);
    filp_scalar_push(ptr);
    free(ptr);

    return FILP_OK;
}


/**
 * safe - Enters isolate mode
 *
 * Enters isolate mode, so all potentially dangerous commands
 * as file access, environment variable definitions and external
 * command executions are disabled. There is no turning back.
 * When executed inside an 'eval' code, isolate mode operates
 * only inside it.
 */
static int _filpf_safe(void)
/** safe */
{
    _filp_isolate = 1;
    return FILP_OK;
}


static int _filpf_hash(void)
/** [ @key @value ... ] hash @hash */
{
    struct filp_val *h;
    struct filp_val *v;
    struct filp_val *k;

    h = filp_new_hash(31);

    for (;;) {
        v = filp_pop();

        if (v->type == FILP_NULL)
            break;

        k = filp_pop();

        if (k->type == FILP_NULL)
            break;

        filp_hash_set(h, k->value, v);
    }

    filp_push(h);

    return FILP_OK;
}


static int _filpf_hget(void)
/** @hash @key hget %value */
{
    int i;
    struct filp_val *k;
    struct filp_val *h;
    struct filp_val *v;


    k = filp_pop();

    if ((h = filp_array_pop(&i, 0)) == NULL)
        return FILP_ERROR;

    v = filp_hash_get(h, k->value);
    filp_push(v == NULL ? _filp_null_value : v);

    return FILP_OK;
}


static int _filpf_hset(void)
/** @hash_symbol @key @value hset */
/** @hash @key @value hset %modified_hash */
{
    int i;
    struct filp_val *v;
    struct filp_val *k;
    struct filp_val *h;

    v = filp_pop();
    k = filp_pop();
    if ((h = filp_array_pop(&i, 0)) == NULL)
        return FILP_ERROR;

    filp_hash_set(h, k->value, v);
    if (i)
        filp_push(h);

    return FILP_OK;
}


static int _filpf_hdel(void)
/** @hash_symbol @key hdel */
/** @hash @key hdel %modified_hash */
{
    int i;
    struct filp_val *k;
    struct filp_val *h;

    k = filp_pop();

    if ((h = filp_array_pop(&i, 0)) == NULL)
        return FILP_ERROR;

    filp_hash_del(h, k->value);
    if (i)
        filp_push(h);

    return FILP_OK;
}


static int _filpf_hdump_keys_values(int keys, int values)
{
    int i, n;
    struct filp_val *h;
    struct filp_val *k;
    struct filp_val *v;

    if ((h = filp_array_pop(&i, 0)) == NULL)
        return FILP_ERROR;

    filp_null_push();

    for (n = 1; n <= filp_hash_size(h); n++) {
        if (!filp_hash_get_pair(h, n, &k, &v))
            break;

        if (keys)
            filp_push(k);
        if (values)
            filp_push(v);
    }

    return FILP_OK;
}


static int _filpf_hdump(void)
/** @hash hdump [ %hash_elements ] */
{
    return _filpf_hdump_keys_values(1, 1);
}


static int _filpf_keys(void)
/** @hash keys [ %hash_keys ] */
{
    return _filpf_hdump_keys_values(1, 0);
}


static int _filpf_values(void)
/** @hash values [ %hash_values ] */
{
    return _filpf_hdump_keys_values(0, 1);
}


static int _filpf_hsize(void)
/** @hash hsize %size */
{
    int i;
    struct filp_val *h;

    if ((h = filp_array_pop(&i, 0)) == NULL)
        return FILP_ERROR;

    filp_push(filp_new_int_value(filp_hash_size(h)));

    return FILP_OK;
}


void filp_lib_startup(void)
/* inits the basic library. All these functions are purely filp or use
   just the standard C library. May be suitable for embedded systems */
{
    _filp_null_value = filp_new_value(FILP_NULL, NULL, 0);
    filp_ref_value(_filp_null_value);
    _filp_true_value = filp_new_value(FILP_SCALAR, "1", -1);
    filp_ref_value(_filp_true_value);

    filp_bin_code("license", _filpf_license);
    filp_bin_code("licence", _filpf_license);
    filp_bin_code("warranty", _filpf_license);

    filp_bin_code("nop", NULL);
    filp_bin_code("]", NULL);
    filp_bin_code("size", _filpf_size);
    filp_bin_code("lsize", _filpf_lsize);
    filp_bin_code("dup", _filpf_dup);
    filp_bin_code("dupnz", _filpf_dupnz);
    filp_bin_code("idup", _filpf_idup);
    filp_bin_code("swap", _filpf_swap);
    filp_bin_code("xchg", _filpf_swap);
    filp_bin_code("#", _filpf_swap);
    filp_bin_code("rot", _filpf_rot);
    filp_bin_code("pop", _filpf_pop);
    filp_bin_code("swstack", _filpf_swstack);

    filp_bin_code("set", _filpf_set);
    filp_bin_code("def", _filpf_set);
    filp_bin_code("=", _filpf_set);
    filp_bin_code("unset", _filpf_unset);
    filp_bin_code("undef", _filpf_unset);
    filp_bin_code("defined", _filpf_defined);
    filp_bin_code("type", _filpf_type);
    filp_bin_code("val", _filpf_val);
    filp_bin_code("exec", _filpf_exec);
    filp_bin_code("eval", _filpf_eval);
    filp_bin_code("symbol", _filpf_symbol);

    filp_bin_code("true", _filpf_true);
    filp_bin_code("false", _filpf_false);
    filp_bin_code("NULL", _filpf_NULL);
    filp_bin_code("[", _filpf_NULL);
    filp_bin_code("(", _filpf_NULL);

    filp_bin_code("add", _filpf_bmath_add);
    filp_bin_code("sub", _filpf_bmath_sub);
    filp_bin_code("mul", _filpf_bmath_mul);
    filp_bin_code("div", _filpf_bmath_div);
    filp_bin_code("mod", _filpf_bmath_mod);
    filp_bin_code("+", _filpf_bmath_add);
    filp_bin_code("-", _filpf_bmath_sub);
    filp_bin_code("*", _filpf_bmath_mul);
    filp_bin_code("/", _filpf_bmath_div);
    filp_bin_code("%", _filpf_bmath_mod);
    filp_bin_code("+=", _filpf_bimath_add);
    filp_bin_code("-=", _filpf_bimath_sub);
    filp_bin_code("*=", _filpf_bimath_mul);
    filp_bin_code("/=", _filpf_bimath_div);

    filp_bin_code("eq", _filpf_cmp_eq);
    filp_bin_code("gt", _filpf_cmp_gt);
    filp_bin_code("lt", _filpf_cmp_lt);
    filp_bin_code("==", _filpf_mcmp_eq);
    filp_bin_code(">", _filpf_mcmp_gt);
    filp_bin_code("<", _filpf_mcmp_lt);
    filp_bin_code("and", _filpf_bool_and);
    filp_bin_code("&&", _filpf_bool_and);
    filp_bin_code("or", _filpf_bool_or);
    filp_bin_code("||", _filpf_bool_or);

    filp_bin_code("if", _filpf_if);
    filp_bin_code("unless", _filpf_unless);
    filp_bin_code("ifelse", _filpf_ifelse);
    filp_bin_code("repeat", _filpf_repeat);
    filp_bin_code("loop", _filpf_loop);
    filp_bin_code("foreach", _filpf_foreach);
    filp_bin_code("for", _filpf_for);
    filp_bin_code("while", _filpf_while);
    filp_bin_code("switch", _filpf_switch);

    filp_bin_code("reverse", _filpf_reverse);
    filp_bin_code("seek", _filpf_seek);
    filp_bin_code("index", _filpf_index);
    filp_bin_code("load", _filpf_load);

    filp_bin_code("strcat", _filpf_strcat);
    filp_bin_code(".", _filpf_strcat);
    filp_bin_code("length", _filpf_length);
    filp_bin_code("strlen", _filpf_length);
    filp_bin_code("substr", _filpf_substr);
    filp_bin_code("splice", _filpf_splice);
    filp_bin_code("instr", _filpf_instr);
    filp_bin_code("sprintf", _filpf_sprintf);
    filp_bin_code("sscanf", _filpf_sscanf);
    filp_bin_code("split", _filpf_split);

    filp_bin_code(")", _filpf_array);
    filp_bin_code("array", _filpf_array);
    filp_bin_code("aget", _filpf_array_get);
    filp_bin_code("@", _filpf_array_get);
    filp_bin_code("aset", _filpf_array_set);
    filp_bin_code("@=", _filpf_array_set);
    filp_bin_code("ains", _filpf_array_ins);
    filp_bin_code("adump", _filpf_array_dump);
    filp_bin_code("adel", _filpf_array_del);
    filp_bin_code("aseek", _filpf_array_seek);
    filp_bin_code("abseek", _filpf_array_binary_seek);
    filp_bin_code("asort", _filpf_array_sort);
    filp_bin_code("forall", _filpf_forall);
    filp_bin_code("map", _filpf_map);

    filp_bin_code("sweep", _filpf_sweep);
    filp_bin_code("dumper", _filpf_dumper);

    filp_bin_code("safe", _filpf_safe);

    filp_bin_code("hash", _filpf_hash);
    filp_bin_code("hget", _filpf_hget);
    filp_bin_code("hset", _filpf_hset);
    filp_bin_code("hdel", _filpf_hdel);
    filp_bin_code("hdump", _filpf_hdump);
    filp_bin_code("keys", _filpf_keys);
    filp_bin_code("values", _filpf_values);
    filp_bin_code("hsize", _filpf_hsize);

    /**
     * tpop - Stores the top of stack into the temporal variable.
     * @value: value to be stored
     *
     * Stores the top of stack into the temporal variable ($_).
     * [Stack manipulation commands]
     */
    /** @value tpop */
    filp_exec("/tpop { /_ # = } set");

    /**
     * tpush - Pushes the temporal variable to the stack.
     *
     * Pushes the temporal variable ($_) to the stack.
     * [Stack manipulation commands]
     */
    /** tpush %value */
    filp_exec("/tpush { $_ } set");

    filp_exec("/clean { { pop } foreach } set");

    /**
     * chop - Chops the last character of a string.
     * @string: the string to be chopped
     *
     * Chops the last character of a string. Mainly to be used
     * to cut the trailing \n character of a line read from a
     * file.
     * [String manipulation commands]
     */
    /** @string chop %strin */
    filp_exec("/chop { dup length 1 - 1 # substr } set");

    /**
     * join - Joins a list into a string.
     * @joiner: the joiner string
     * @list_elements: the elements of the list
     *
     * Joins a list into a string, using the string @joiner
     * as a glue.
     * [String manipulation commands]
     * [List processing commands]
     */
    /** [ @list_elements ] @joiner join %string */
    filp_exec("/join { tpop # { # $_ # . . # } foreach } set");

    /**
     * sort - Sorts a list.
     * @list: the list
     *
     * Sorts a list.
     * [List processing commands]
     */
    /** [ @list_elements ] sort [ @sorted_elements ] */
    filp_exec("/sort { array asort adump } set");

    /**
     * enum - Defines a group of constants.
     * @list: the list of constants to set
     * @first: the value of the first element
     *
     * Defines a group of constants. The first (deeper in the stack)
     * will have a value of @first and so on, increasingly.
     * [Symbol management commands]
     */
    /** [ @list_elements ] @first enum */
    filp_exec("/enum { lsize 1 - + /_ #= { /_ -- $_ = } foreach } set");

    filp_exec("/filp_doc [ hash =");

    /**
     * setdoc - Set documentation for a symbol.
     * @documentation_string: String to be associated to the symbol
     * @symbol: the symbol
     *
     * Stores documentation for a symbol.
     * [Documentation commands]
     */
    /** @documentation_string @symbol setdoc */
    filp_exec("/setdoc { /filp_doc 2 rot 3 rot hset } set");

    /**
     * getdoc - Gets documentation from a symbol.
     * @symbol: the symbol
     *
     * Gets documentation from a @symbol. If it hasn't any,
     * it's name is returned instead.
     * [Documentation commands]
     */
    /** @symbol getdoc %documentation_string */
    filp_exec("/getdoc { /filp_doc swap hget } set");

    /**
     * ARGV - Array of command-line arguments.
     *
     * This array contains the command-line arguments when used from
     * the filp interpreter, or an empty array if used from an
     * embedded version of the filp library.
     * [Special variables]
     */
    /** ARGV */
    filp_exec("/ARGV ( ) =");

    /**
     * filp_stack_size - Maximum size of the stack.
     *
     * The maximum number of values the stack is allowed to store.
     * By default is 16384. Just to avoid mad code from
     * devouring all the available memory.
     * [Special variables]
     */
    /** filp_stack_size */
    filp_ext_int("filp_stack_size", &_filp_stack_size);

    /**
     * filp_stack_elems - Number of elements currently in the stack.
     *
     * The number of elements currently stored in the stack.
     * [Special variables]
     */
    /** filp_stack_elems */
    filp_ext_int("filp_stack_elems", &_filp_stack_elems);

    filp_ext_int("filp_val_account", &_filp_val_account);
    filp_ext_int("filp_sym_account", &_filp_sym_account);

    /**
     * filp_version - Version of filp.
     *
     * This string holds the current version of filp.
     * [Special constants]
     */
    /** filp_version */
    filp_ext_string("filp_version", _filp_version, sizeof(VERSION));

    /**
     * filp_real - Use of real numbers instead of integers flag.
     *
     * This flag tells filp if it must perform mathematical operations
     * using integers (by default) or real numbers.
     * [Special variables]
     */
    /** filp_real */
    filp_ext_int("filp_real", &_filp_real);

    /**
     * filp_bareword - Use of barewords flag.
     *
     * If this flag is set, barewords (not-quoted words that are
     * not recognized as commands) are treated as literal strings.
     * Barewords are dangerous and hard to debug. Don't use it.
     * [Special variables]
     */
    /** filp_bareword */
    filp_ext_int("filp_bareword", &_filp_bareword);

    /**
     * filp_error - Last error code.
     *
     * This variable contains the last error code. Can be
     * used as an offset to the filp_error_strings filp array.
     * [Special variables]
     */
    /** filp_error */
    filp_ext_int("filp_error", &_filp_error);

    /**
     * filp_error_info - Text info about the last error.
     *
     * This string contain additional info about the last error.
     * [Special variables]
     */
    /** filp_error_info */
    filp_ext_string("filp_error_info", _filp_error_info, sizeof(_filp_error_info));

    /**
     * filp_compile_date - Compilation date.
     *
     * This string contains the date of compilation of the filp library.
     * [Special constants]
     */
    /** filp_compile_date */
    filp_exec("/filp_compile_date '" __DATE__ "' set");

    /**
     * filp_compile_time - Compilation time.
     *
     * This string contains the time of compilation of the filp library.
     * [Special constants]
     */
    /** filp_compile_time */
    filp_exec("/filp_compile_time '" __TIME__ "' set");

    /**
     * filp_error_strings - Array of error strings.
     *
     * This array contains the message associated to an error message
     * contained in filp_error.
     * [Special variables]
     */
    /** filp_error_strings */
    filp_exec
        ("/filp_error_strings ( 'token not found' 'scalar expected' 'internal error' 'out of memory' 'array expected' 'file expected' 'file not found' 'permission denied' 'not implemented' 'syntax error') =");

    filp_exec("/#= { # = } set");
    filp_exec("/not { { false } { true } ifelse } set");
    filp_exec("/ne { eq not } set");
    filp_exec("/ge { lt not } set");
    filp_exec("/le { gt not } set");
    filp_exec("/!= { == not } set");
    filp_exec("/<= { > not } set");
    filp_exec("/>= { < not } set");
    filp_exec("/++ { 1 += } set");
    filp_exec("/-- { 1 -= } set");
    filp_exec("/abs { dup 0 < { -1 * } if } set");
}


int filp_startup(void)
/* main startup point */
{
    filp_lib_startup();
    filp_slib_startup();

    return 1;
}

void filp_shutdown(void)
/* main shutdown point */
{
}
