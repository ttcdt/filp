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


/* arrays */

/**
 * filp_array_dim - Gives dimension to an array.
 * @asize: the new size of the array
 *
 * Gives dimension to an array.
 */
struct filp_val **filp_array_dim(int asize)
{
    struct filp_val **a;

    if (asize == 0)
        return NULL;

    a = (struct filp_val **) malloc(asize * sizeof(struct filp_val *));
    memset(a, '\0', asize * sizeof(struct filp_val *));

    return a;
}


/**
 * filp_array_expand - Inserts room in an array.
 * @a: the array
 * @offset: offset where the room is to be open
 * @num: number of elements to be inserted
 *
 * Inserts @num empty elements in the @offset of the @a array.
 */
void filp_array_expand(struct filp_val *a, int offset, int num)
{
    int n;
    struct filp_val **na;

    /* ignore stupid situations */
    if (offset < 0)
        return;

    /* offset 0: the end of the array */
    if (offset == 0)
        offset = a->size;
    else
        offset--;

    /* array is longer */
    a->size += num;
    na = realloc(a->array, a->size * sizeof(struct filp_val *));

    /* moves up from top of the array */
    for (n = a->size - 1; n >= offset + num; n--)
        na[n] = na[n - num];

    /* fills the new space with blanks */
    for (; n >= offset; n--)
        na[n] = NULL;

    a->array = na;
}


/**
 * filp_array_collapse - Shrinks an array.
 * @a: the array
 * @offset: offset to the first element to be deleted
 * @num: number of elements to be deleted
 *
 * Deletes @num empty elements at the @offset of the @a array.
 */
void filp_array_collapse(struct filp_val *a, int offset, int num)
{
    int n;

    /* bound tests */
    if (offset < 0 || a->size == 0 || offset > a->size)
        return;

    /* don't try to delete beyond the limit */
    if (offset + num > a->size)
        num = a->size - offset;

    /* array is shorter */
    a->size -= num;

    /* offset 0: last element */
    if (offset == 0)
        offset = a->size;
    else
        offset--;

    /* moves down the elements */
    for (n = offset; n < a->size; n++)
        a->array[n] = a->array[n + num];

    /* finally shrinks the memory block */
    a->array = realloc(a->array, a->size * sizeof(struct filp_val *));
}


/**
 * filp_array_size - Gets the size of an array.
 * @value: the array
 *
 * Returns the number of elements of the array.
 */
int filp_array_size(struct filp_val *value)
{
    return value->size;
}


/**
 * filp_array_set - Sets the value of an array's element.
 * @value: the array
 * @e: the element to be assigned
 * @i: the subscript of the element
 *
 * Sets the element @i of the array @value to be the @e value. If @i
 * is 0, the element set is the last of the array.
 * Returns the previous element.
 */
struct filp_val *filp_array_set(struct filp_val *value, struct filp_val *e, int i)
{
    struct filp_val *v;

    if (i == 0)
        i = value->size - 1;
    else
        i--;

    if (i < 0 || i >= value->size)
        return NULL;

    v = value->array[i];
    value->array[i] = e;

    if (v != NULL)
        filp_unref_value(v);
    if (e != NULL)
        filp_ref_value(e);

    return v;
}


/**
 * filp_array_destroy - Destroys an array.
 * @value: the array
 *
 * Destroys an array.
 */
void filp_array_destroy(struct filp_val *value)
{
    int n;

    /* sets all elements to NULL (unreferencing them) */
    for (n = 1; n <= filp_array_size(value); n++)
        filp_array_set(value, NULL, n);

    /* destroy the array itself now */
    free(value->array);
    value->array = NULL;
}


/**
 * filp_array_get - Gets an element of an array.
 * @value: the array
 * @i: the subscript of the element
 *
 * Returns the element number @i of the array @value.
 */
struct filp_val *filp_array_get(struct filp_val *value, int i)
{
    i--;

    if (i < 0 || i >= value->size)
        return NULL;

    return value->array[i];
}


/**
 * filp_array_ins - Inserts a value in an array.
 * @value: the array
 * @e: the element to be inserted
 * @i: subscript where the element is going to be inserted
 *
 * Inserts the @e value in the @value array in the @i subscript.
 * Further elements are pushed up, so the array increases its size
 * by one. If @i is 0, the element is inserted at the end of the array.
 */
void filp_array_ins(struct filp_val *value, struct filp_val *e, int i)
{
    /* open room */
    filp_array_expand(value, i, 1);

    /* set value */
    filp_array_set(value, e, i);
}


/**
 * filp_array_dup - Duplicates an array.
 * @value: the array
 *
 * Duplicates the @value array. A new value containing the
 * same elements is returned.
 */
struct filp_val *filp_array_dup(struct filp_val *value)
{
    int n;
    struct filp_val *v;

    /* creates a new array with the same size */
    v = filp_new_value(FILP_ARRAY, NULL, value->size);

    /* copies the elements */
    for (n = 1; n <= filp_array_size(value); n++)
        filp_array_set(v, filp_array_get(value, n), n);

    return v;
}


/**
 * filp_array_del - Deletes an element of an array.
 * @value: the value
 * @i: subscript of the element to be deleted.
 *
 * Deleted the @i element from the @value array. The array
 * is shrinked by one.
 * Returns the deleted element.
 */
struct filp_val *filp_array_del(struct filp_val *value, int i)
{
    struct filp_val *v;

    /* unrefs the value */
    v = filp_array_set(value, NULL, i);

    /* shrinks the array */
    filp_array_collapse(value, i, 1);

    return v;
}


/**
 * filp_array_seek - Seeks a string in an array.
 * @value: the array
 * @str: the string to be found
 * @inc: element increment
 *
 * Seeks the string @str in the @value array, incrementing the
 * element to be seek by @inc (to seek all the array, use an
 * increment of 1). Returns the subscript of the first element
 * with a value equal to @str or 0 if none has it.
 */
int filp_array_seek(struct filp_val *value, char *str, int inc)
{
    int n;
    struct filp_val *v;
    struct filp_val *k;

    k = filp_new_value(FILP_SCALAR, str, -1);

    /* searches sequentially until found */
    for (n = 1; n <= filp_array_size(value); n += inc) {
        v = filp_array_get(value, n);

        if (filp_cmp(k, v) == 0)
            return n;
    }

    return 0;
}


/**
 * filp_array_binary_seek - Seeks a string in a sorted array.
 * @a: the array
 * @str: the string to be found
 * @inc: element increment
 *
 * Seeks the string @str in the @a sorted array, incrementing the
 * element to be seek by @inc (to seek all the array, use an
 * increment of 1). If the element is found, its subscript is
 * returned as a positive number; otherwise, the subscript
 * of the position where the value should be is returned as a
 * negative number.
 */
int filp_array_binary_seek(struct filp_val *a, char *str, int inc)
{
    int b, t, n, c;
    struct filp_val *k;
    struct filp_val *v;

    b = n = 0;
    t = (filp_array_size(a) - 1) / inc;
    k = filp_new_value(FILP_SCALAR, str, -1);

    while (t >= b) {
        n = (b + t) / 2;
        if ((v = filp_array_get(a, (n * inc) + 1)) == NULL)
            break;

        c = filp_cmp(k, v);

        if (c == 0)
            return (n * inc) + 1;
        else if (c < 0)
            t = n - 1;
        else
            b = n + 1;
    }

    return -((b * inc) + 1);
}


static int _filp_sort_cmp(const void *s1, const void *s2)
{
    return filp_cmp(*(struct filp_val **) s1, *(struct filp_val **) s2);
}


/**
 * filp_array_sort - Sorts an array.
 * @value: the value
 * @inc: increment
 *
 * Sorts alphabetically the elements of the array. If @inc is greater than 1,
 * the elements are ordered in groups of that quantity.
 */
void filp_array_sort(struct filp_val *value, int inc)
{
    if (inc == 0)
        return;

    qsort(value->array, value->size / inc,
          sizeof(struct filp_val *) * inc, _filp_sort_cmp);
}



/* hashes */


/**
 * filp_new_hash - Creates a new hash.
 * @buckets: number of buckets in the hash
 *
 * Creates a new hash. A filp hash is, internally, a filp array of @buckets arrays.
 */
struct filp_val *filp_new_hash(int buckets)
{
    struct filp_val *h;
    int n;

    h = filp_new_value(FILP_ARRAY, NULL, buckets);

    for (n = 1; n <= buckets; n++)
        filp_array_set(h, filp_new_value(FILP_ARRAY, NULL, 0), n);

    return h;
}


/**
 * filp_hash_size - Returns the number of pairs of a hash.
 * @h: the hash
 *
 * Returns the number of key-value pairs stored in the hash.
 */
int filp_hash_size(struct filp_val *h)
{
    int n, s;
    struct filp_val *v;

    for (s = 0, n = 1; n <= filp_array_size(h); n++) {
        if ((v = filp_array_get(h, n)) != NULL)
            s += filp_array_size(v);
    }

    return s / 2;
}


#define HASH_BUCKET(h, key) (filp_hashfunc((unsigned char *)key, filp_array_size(h)) + 1)

/**
 * filp_hash_get - Gets an element from a hash.
 * @h: the hash
 * @key: the key which value is wanted
 *
 * Returns the value associated with @key form the hash @h, or NULL
 * if no one exists.
 */
struct filp_val *filp_hash_get(struct filp_val *h, char *key)
{
    int e;
    struct filp_val *v;

    /* takes the bucket */
    e = HASH_BUCKET(h, key);
    v = filp_array_get(h, e);

    /* seeks in the bucket */
    if ((e = filp_array_binary_seek(v, key, 2)) > 0)
        v = filp_array_get(v, e + 1);
    else
        v = NULL;

    return v;
}


/**
 * filp_hash_set - Stores a key-value pair into a hash.
 * @h: the hash
 * @key: the key
 * @value: the value
 *
 * Stores a key-value pair into the hash. Returns the previously
 * stored value under that key if one exists, or NULL otherwise.
 */
struct filp_val *filp_hash_set(struct filp_val *h, char *key, struct filp_val *value)
{
    int e;
    struct filp_val *s;
    struct filp_val *v;

    /* takes the bucket */
    e = HASH_BUCKET(h, key);
    s = filp_array_get(h, e);

    /* seeks in the bucket */
    if ((e = filp_array_binary_seek(s, key, 2)) < 0) {
        e *= -1;
        filp_array_expand(s, e, 2);
        filp_array_set(s, filp_new_value(FILP_SCALAR, key, -1), e);
    }

    v = filp_array_set(s, value, e + 1);
    return v;
}


/**
 * filp_hash_del - Deletes a key-value pair from a hash.
 * @h: the hash
 * @key: the key
 *
 * Deletes a key-value pair from the hash given its @key. If the
 * pair exists, the value is returned, or NULL otherwise.
 */
struct filp_val *filp_hash_del(struct filp_val *h, char *key)
{
    int e;
    struct filp_val *s;
    struct filp_val *v = NULL;

    /* takes the bucket */
    e = HASH_BUCKET(h, key);
    s = filp_array_get(h, e);

    /* seeks in the bucket */
    if ((e = filp_array_binary_seek(s, key, 2)) > 0) {
        /* sets key and value to NULL */
        filp_array_set(s, NULL, e);
        v = filp_array_set(s, NULL, e + 1);

        /* finally shrink the array */
        filp_array_collapse(s, e, 2);
    }

    return v;
}


/**
 * filp_hash_get_pair - Gets a key-value pair from the hash.
 * @h: the hash
 * @i: the pair number
 * @key: a pointer to store the key
 * @value: a pointer to store the value 
 *
 * Gets the key-value pair number @i from the hash @h. A hash
 * contains key-value pairs numbered from 1 to filp_hash_size().
 * @key and @value can be pointers to struct filp_val or NULL.
 */
int filp_hash_get_pair(struct filp_val *h, int i,
               struct filp_val **key, struct filp_val **value)
{
    int n;
    struct filp_val *v;

    i *= 2;

    for (n = 1; n <= filp_array_size(h); n++) {
        if ((v = filp_array_get(h, n)) != NULL) {
            if (i > filp_array_size(v))
                i -= filp_array_size(v);
            else {
                if (key != NULL)
                    *key = filp_array_get(v, i - 1);

                if (value != NULL)
                    *value = filp_array_get(v, i);

                return 1;
            }
        }
    }

    return 0;
}
