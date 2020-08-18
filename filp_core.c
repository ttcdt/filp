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

#include "filp.h"

/*******************
    Data
********************/

/* queue of values (needed for the garbage collector) */
static struct filp_val *_filp_val_head = NULL;
static struct filp_val *_filp_val_tail = NULL;

/**
 * _filp_stack - Pointer to the stack.
 *
 * This variable stores the stack in use.
 */
static struct filp_stack *_filp_stack = NULL;

/**
 * _filp_swap_stack - Pointer to the swapped stack.
 *
 * This variable points to the alternative (swapped) stack.
 * The content of this variable is swapped with _filp_stack
 * by the filp_swap_stack() function.
 */
static struct filp_stack *_filp_swap_stack = NULL;

/**
 * _filp_stack_size - Maximum size of the stack.
 *
 * The maximum number of values the stack is allowed to store.
 * By default is 16384. Just to avoid mad code from
 * devouring all the available memory.
 */
int _filp_stack_size = 16384;

/**
 * _filp_stack_elems - Number of elements currently in the stack.
 *
 * The number of elements currently stored in the stack.
 */
int _filp_stack_elems = 0;


/* dictionary */

#define FILP_DICT_HASH_SIZE 67

/**
 * _filp_dict - Pointer to the symbol table.
 *
 * This variable holds the symbol table, or dictionary.
 */
static struct filp_sym *_filp_dict[FILP_DICT_HASH_SIZE];

/* accounting */
int _filp_val_account = 0;
int _filp_sym_account = 0;

/**
 * _filp_version - Version of filp.
 *
 * This string holds the current version of filp.
 */
char _filp_version[] = VERSION;

/**
 * _filp_real - Use of real numbers instead of integers flag.
 *
 * This flag tells filp if it must perform mathematical operations
 * using integers (by default) or real numbers.
 */
int _filp_real = 0;

/* increment block size for filp_poke() */
int _filp_block_size = 1024;

/**
 * _filp_bareword - Use of barewords flag.
 *
 * If this flag is set, barewords (not-quoted words that are
 * not recognized as commands) are treated as literal strings.
 * Barewords are dangerous and hard to debug. Don't use it.
 */
int _filp_bareword = 0;

/**
 * _filp_error - Last error code.
 *
 * This variable contains the last error code. Can be
 * used as an offset to the filp_error_strings filp array.
 */
int _filp_error = FILPERR_NONE;

/**
 * _filp_error_info - Text info about the last error.
 *
 * This string contain additional info about the last error.
 */
char _filp_error_info[80] = "";

/**
 * _filp_isolate - Flag to isolate potentially dangerous commands.
 *
 * If this flag is set to 1, potentially dangerous commands
 * are disabled and generate errors. Can be used on sensible
 * embedded systems exposed to untested input code. Basicly,
 * denies access to file functions, putenv and shell execution.
 */
int _filp_isolate = 0;

/* license */
char *_filp_license =
    "\nfilp " VERSION " - Embeddable, Reverse Polish Notation Programming Language\n\n\
This is free and unencumbered software released into the public domain.\n\
\n\
Anyone is free to copy, modify, publish, use, compile, sell, or\n\
distribute this software, either in source code form or as a compiled\n\
binary, for any purpose, commercial or non-commercial, and by any\n\
means.\n\
\n\
THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND,\n\
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF\n\
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.\n\
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR\n\
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,\n\
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR\n\
OTHER DEALINGS IN THE SOFTWARE.\n\
\n\
https://triptico.com/software/filp.html\n";

/* > 0 if filp code is in execution */
int _in_filp = 0;

/* frequently used values */
struct filp_val *_filp_null_value = NULL;
struct filp_val *_filp_true_value = NULL;


/******************
    Code
*******************/

/**
 * filp_poke - Stores a byte in a dynamic string.
 * @ptr: the string
 * @size: size of the string
 * @offset: offset where the byte should be stored
 * @c: the byte to be stored
 *
 * Stores a byte in a dynamic string at @offset. If the @offset
 * is higher than the @size of the string, the latter is resized
 * using realloc(). The string @ptr can be NULL; in that case,
 * a new string is created.
 * Returns a pointer to the new string (the original @ptr could
 * have changed).
 */
char *filp_poke(char *ptr, int *size, int offset, int c)
{
    if (offset >= *size) {
        *size = offset + _filp_block_size;
        ptr = realloc(ptr, *size);
    }

    ptr[offset] = c;

    return ptr;
}


/**
 * filp_splice - Deletes and inserts text in an string
 * @src: the source string
 * @offset: offset
 * @size: number of chars to be substituted
 * @new: new string to be inserted
 *
 * Removes @size elements from the @offset of the @src string
 * and substitutes them by @new. @size can be 0 if no chars
 * are to be deleted, of @new can be "" if no string is to
 * be inserted. A new allocated string is returned.
 */
char *filp_splice(char *src, int offset, int size, char *new)
{
    int n, m, i;
    char *ptr = NULL;
    int s = 0;

    /* new string is inserted BEFORE offset */
    offset--;

    /* copy source until offset */
    for (n = i = 0; n < offset && src[n]; n++, i++)
        ptr = filp_poke(ptr, &s, i, src[n]);

    /* skip size chars */
    for (; n < offset + size && src[n]; n++);

    /* insert new */
    for (m = 0; new[m]; m++, i++)
        ptr = filp_poke(ptr, &s, i, new[m]);

    /* continue adding */
    for (; src[n]; n++, i++)
        ptr = filp_poke(ptr, &s, i, src[n]);

    /* end */
    ptr = filp_poke(ptr, &s, i, '\0');

    return ptr;
}


/**
 * filp_hashfunc - Hash function for the dictionary.
 * @string: the string to be hashed
 * @mod: modulus
 *
 * Calculates a hash for the @string argument,
 * to be used to search the dictionary.
 * Returns the computed hash.
 */
int filp_hashfunc(unsigned char *string, int mod)
{
    int c;

    for (c = 0; *string != '\0'; string++)
        c = (128 * c + *string) % mod;

    return c;
}


/* values */

/**
 * filp_new_value - Creates a new filp value.
 * @type: value type
 * @value: the value
 * @size: size of the value
 *
 * Creates a new filp value. The @type must be a valid filp
 * value type, and the @value a pointer to the specific
 * data to be stored inside (can be NULL for some types).
 * The @size argument can be -1 for the FILP_SCALAR and
 * FILP_CODE types to make it automatically calculated.
 * Returns a pointer to a struct filp_val, containing the
 * new value, or NULL if it could not be done.
 */
struct filp_val *filp_new_value(filp_type type, void *value, int size)
{
    struct filp_val *v;
    char *cp_value;

    if ((v = (struct filp_val *) malloc(sizeof(struct filp_val))) == NULL)
        return NULL;

    memset(v, '\0', sizeof(struct filp_val));

    if (type == FILP_NULL)
        value = (char *) "[NULL]";

    if (type == FILP_SCALAR || type == FILP_CODE) {
        /* duplicates */
        if (value != NULL) {
            if (size == -1)
                size = strlen((char *) value) + 1;

            if ((cp_value = (char *) malloc(size)) == NULL)
                return NULL;

            memcpy(cp_value, (char *) value, size);
        }
        else
            cp_value = NULL;
    }
    else if (type == FILP_ARRAY) {
        v->array = filp_array_dim(size);
        cp_value = (char *) "[ARRAY]";
    }
    else
        cp_value = value;

    v->type = type;
    v->value = cp_value;
    v->size = size;
    v->next = NULL;

    /* add to the chain of values */

    if (_filp_val_head == NULL)
        _filp_val_head = v;

    if (_filp_val_tail != NULL)
        _filp_val_tail->next = v;

    _filp_val_tail = v;

    /* count one more */
    _filp_val_account++;

    return v;
}


/**
 * filp_ref_value - Increments the reference to a value
 * @v: the value
 *
 * Increments the reference count of @v. Values with
 * a reference count > 0 will never be destroyed by
 * the garbage collector.
 */
void filp_ref_value(struct filp_val *v)
{
    v->count++;
}


/**
 * filp_unref_value - Decrements the reference to a value
 * @v: the value
 *
 * Decrements the reference count of @v. Values with
 * a reference count > 0 will never be destroyed by
 * the garbage collector.
 */
void filp_unref_value(struct filp_val *v)
{
    v->count--;
}


/**
 * filp_cmp - Compares two filp values.
 * @v1: first value
 * @v2: second value
 *
 * Compares two filp values. Only must be used compare
 * scalar values.
 * Returns a value similar to that of strcmp().
 */
int filp_cmp(struct filp_val *v1, struct filp_val *v2)
{
    /* only scalars can really be compared */
    if (v1->type == FILP_SCALAR || v2->type == FILP_SCALAR)
        return strcmp(v1->value, v2->value);

    /* if they are pointers, try a simple comparison,
       just to test if both pointers are the same */
    if (v1->type == v2->type && (v1->type == FILP_BIN_CODE ||
                     v1->type == FILP_EXT_INT ||
                     v1->type == FILP_EXT_REAL || v1->type == FILP_EXT_STRING))
        return v1->value - v2->value;

    return v1->type - v2->type;
}


/**
 * filp_is_true - True condition tester.
 * @v: the value to be tested
 *
 * Tests if the @v value is evaluated as a boolean 'true'
 * filp expression. A filp value is true if it's not
 * FILP_NULL and it's not a scalar containing 0 or "0".
 */
int filp_is_true(struct filp_val *v)
{
    if (v->type == FILP_NULL)
        return 0;

    if (v->type == FILP_SCALAR) {
        if (strcmp(v->value, "0") == 0)
            return 0;
    }

    return 1;
}


/* stack */

/**
 * filp_push - Pushes a value into the stack, duplicating it.
 * @v: the value to be pushed
 *
 * Pushes a copy of the value into the stack. The value can
 * still be used, and must be destroyed when not useful anymore,
 * as usual.
 */
int filp_push(struct filp_val *v)
{
    struct filp_stack *s;

    /* stack overflow? */
    if (_filp_stack_elems == _filp_stack_size)
        return 0;

    if ((s = (struct filp_stack *) malloc(sizeof(struct filp_stack))) == NULL)
        return 0;

    memset(s, '\0', sizeof(struct filp_stack));

    s->next = _filp_stack;
    _filp_stack = s;

    _filp_stack_elems++;

    /* if value is an array, it must be duplicated */
    if (v->type == FILP_ARRAY)
        v = filp_array_dup(v);

    s->value = v;

    /* the value is now referenced in the stack */
    filp_ref_value(v);

    return 1;
}


/**
 * filp_pop - Pops a value from the stack.
 *
 * Pops a value from the stack. Returns a special NULL filp
 * value on stack underflow condition.
 */
struct filp_val *filp_pop(void)
{
    struct filp_stack *s;
    struct filp_val *v;

    if (_filp_stack == NULL)
        return _filp_null_value;

    s = _filp_stack;
    _filp_stack = s->next;
    _filp_stack_elems--;

    v = s->value;

    free(s);

    /* value is not referenced here anymore */
    filp_unref_value(v);

    return v;
}


/**
 * filp_stack_value - Returns the nth element of the stack.
 * @pos: offset in the stack
 *
 * Returns the nth element of the stack, counting from 1.
 * The stack is left untouched. If no value is found,
 * a FILP_NULL value is returned.
 */
struct filp_val *filp_stack_value(int pos)
{
    struct filp_stack *s;

    for (s = _filp_stack, pos--; s != NULL && pos; s = s->next, pos--);

    if (s == NULL)
        return _filp_null_value;

    return s->value;
}


/**
 * filp_rot - Rotates the stack.
 * @pos: offset of the element to be rotated
 *
 * Move the element number @pos from the stack to the top.
 */
void filp_rot(int pos)
{
    struct filp_stack *s;
    struct filp_stack *r;

    if (--pos == 0)
        return;

    for (s = _filp_stack, --pos; s != NULL && pos; s = s->next, pos--);

    if (s == NULL || s->next == NULL) {
        filp_null_push();
        return;
    }

    r = s->next;
    s->next = r->next;
    r->next = _filp_stack;
    _filp_stack = r;
}


/**
 * filp_swap_stack - Commutes between the two stacks.
 *
 * Commutes between the two stacks.
 */
void filp_swap_stack(void)
{
    struct filp_stack *s;

    s = _filp_swap_stack;
    _filp_swap_stack = _filp_stack;
    _filp_stack = s;

    for (_filp_stack_elems = 0; s != NULL; s = s->next)
        _filp_stack_elems++;
}


/**
 * filp_list_size - Computes the size of a list.
 *
 * Computes the size of a list.
 */
int filp_list_size(void)
{
    int n;
    struct filp_stack *s;

    for (n = 0, s = _filp_stack; s != NULL && s->value->type != FILP_NULL;
         s = s->next, n++);

    return n;
}


/** dictionary **/

/**
 * filp_find_symbol - Finds a symbol by name.
 * @name: name of the symbol to be found
 *
 * Finds a symbol by name. Returns the symbol, or a NULL
 * pointer if not found.
 */
struct filp_sym *filp_find_symbol(char *name)
{
    struct filp_sym *s;
    struct filp_sym *p;
    int h;

    if (name == NULL || *name == '\0')
        return NULL;

    h = filp_hashfunc((unsigned char *) name, FILP_DICT_HASH_SIZE);

    for (p = NULL, s = _filp_dict[h]; s != NULL; p = s, s = s->next) {
        if (strcmp(name, s->name) == 0)
            break;
    }

    /* move the found symbol to the beginning of the list,
       hoping it will be reused soon, as in loops */

    if (s != NULL && p != NULL) {
        p->next = s->next;

        s->next = _filp_dict[h];
        _filp_dict[h] = s;
    }

    return s;
}


/**
 * filp_new_symbol - Creates a new symbol.
 * @type: type of the new symbol
 * @name: name of the new symbol
 *
 * Creates a new symbol. A pointer to it is returned.
 */
struct filp_sym *filp_new_symbol(filp_type type, char *name)
{
    struct filp_sym *s;
    char *cp_name;
    int l, h;

    if (name == NULL)
        return NULL;

    if ((s = (struct filp_sym *) malloc(sizeof(struct filp_sym))) == NULL)
        return NULL;

    memset(s, '\0', sizeof(struct filp_sym));

    l = strlen(name) + 1;
    if ((cp_name = (char *) malloc(l)) == NULL)
        return NULL;

    strcpy(cp_name, name);
    s->name = cp_name;
    s->type = type;

    h = filp_hashfunc((unsigned char *) name, FILP_DICT_HASH_SIZE);
    s->next = _filp_dict[h];
    _filp_dict[h] = s;

    _filp_sym_account++;

    return s;
}


/**
 * filp_get_symbol - Gets the value of a symbol.
 * @s: the symbol
 *
 * Gets the value of a symbol.
 */
struct filp_val *filp_get_symbol(struct filp_sym *s)
{
    struct filp_val *v;
    int *i;
    char *ptr;
    double *d;
    FILE *f;

    if (s->value == NULL)
        return NULL;

    if (s->type == FILP_EXT_INT) {
        i = (int *) s->value;
        v = filp_new_int_value(*i);
    }
    else if (s->type == FILP_EXT_REAL) {
        d = (double *) s->value;
        v = filp_new_real_value(*d);
    }
    else if (s->type == FILP_EXT_STRING) {
        ptr = (char *) s->value;
        v = filp_new_value(FILP_SCALAR, ptr, s->size);
    }
    else if (s->type == FILP_FILE) {
        f = (FILE *) s->value->value;
        v = filp_new_value(FILP_FILE, f, 0);
    }
    else if (s->type == FILP_ARRAY)
        v = filp_array_dup(s->value);
    else
        v = s->value;

    return v;
}


/**
 * filp_set_symbol - Sets a symbol to a value.
 * @s: the symbol to be set.
 * @v: the value.
 *
 * Assigns the value @v to the symbol @s.
 */
void filp_set_symbol(struct filp_sym *s, struct filp_val *v)
{
    int i;
    double d;
    int *iptr;
    double *dptr;
    char *ptr;

    if (s == NULL || v == NULL)
        return;

    if (s->type == FILP_EXT_INT) {
        if ((iptr = (int *) s->value) != NULL) {
            i = filp_val_to_int(v);
            *iptr = i;
        }
    }
    else if (s->type == FILP_EXT_REAL) {
        if ((dptr = (double *) s->value) != NULL) {
            d = filp_val_to_real(v);
            *dptr = d;
        }
    }
    else if (s->type == FILP_EXT_STRING) {
        if ((ptr = (char *) s->value) != NULL)
            strncpy(ptr, v->value, s->size);
    }
    else {
        /* previous value is no longer referenced */
        if (s->value != NULL)
            filp_unref_value(s->value);

        s->value = v;

        /* value is now referenced inside the symbol */
        filp_ref_value(s->value);

        /* 0.9.26d: fixed segfault on reassigning bin_code */
        s->type = v->type;
    }
}


/**
 * filp_destroy_symbol - Destroys a symbol.
 * @s: the symbol to be destroyed
 *
 * Destroys a symbol and deletes it from the symbol table.
 */
void filp_destroy_symbol(struct filp_sym *s)
{
#ifdef REALLY_DESTROY_SYMBOL
    struct filp_sym *s2;
    int h;

    h = filp_hashfunc(s->name, FILP_DICT_HASH_SIZE);

    if (_filp_dict[h] == s)
        _filp_dict[h] = s->next;
    else {
        for (s2 = _filp_dict[h]; s2 != NULL && s2->next != s; s2 = s2->next);

        if (s2 == NULL)
            return;

        s2->next = s->next;
    }

    if (s->value != NULL) {
        if (s->type == FILP_SCALAR || s->type == FILP_CODE || s->type == FILP_ARRAY)
            filp_unref_value(s->value);
    }

    free(s);

    _filp_sym_account--;
#else
    filp_set_symbol(s, _filp_null_value);
#endif
}


/**
 * filp_push_dict - Pushes to the stack names of symbols.
 * mask: prefix
 *
 * Pushes to the stack all the symbol names that start with @mask
 * as a list.
 */
int filp_push_dict(char *mask)
{
    struct filp_sym *s;
    int i, n, cnt = 0;

    i = strlen(mask);

    filp_null_push();

    for (n = 0; n < FILP_DICT_HASH_SIZE; n++) {
        for (s = _filp_dict[n]; s != NULL; s = s->next) {
            if (i == 0 || memcmp(s->name, mask, i) == 0) {
                filp_scalar_push(s->name);
                cnt++;
            }
        }
    }

    return cnt;
}


/** garbage collection **/

int filp_sweep_head(void)
{
    struct filp_val *v;

    /* if value in head is still referenced... */
    if (_filp_val_head->count) {
        /* move to tail */
        _filp_val_tail->next = _filp_val_head;
        _filp_val_head = _filp_val_head->next;
        _filp_val_tail = _filp_val_tail->next;
        _filp_val_tail->next = NULL;

        return 0;
    }

    /* destroy value */

    v = _filp_val_head;
    _filp_val_head = _filp_val_head->next;

    /* free memory blocks */
    if (v->type == FILP_SCALAR || v->type == FILP_CODE) {
        if (v->value != NULL)
            free(v->value);
    }
    else if (v->type == FILP_ARRAY)
        filp_array_destroy(v);

    free(v);

    /* one value less */
    _filp_val_account--;

    return 1;
}


/**
 * filp_sweeper - Forces a garbage collection
 * @full: full sweeping flag
 *
 * Forces a garbage collection. If @full is nonzero, the complete
 * value poll is checked; otherwise, only a small set of them
 * are checked.
 */
void filp_sweeper(int full)
{
    static int _last_val_account = 0;
    int n;

    /* do nothing if queue value is empty or has one element */
    if (_filp_val_head == NULL ||
        _filp_val_tail == NULL || _filp_val_head == _filp_val_tail)
        return;

    n = _filp_val_account;

    /* if not a full garbage collection, rotate at least
       the difference from the last time + 2 */
    if (!full)
        n -= (_last_val_account - 2);

    for (; n > 0; n--)
        filp_sweep_head();

    _last_val_account = _filp_val_account;
}
