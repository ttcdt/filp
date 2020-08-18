/*

    filp - Embeddable, Reverse Polish Notation Programming Language

    Angel Ortega <angel@triptico.com>

    This software is released into the public domain.
    NO WARRANTY. See file LICENSE for details.

*/

#include <stdio.h>

/* data types */

typedef enum {
    FILP_SCALAR,        /* scalar */
    FILP_CODE,          /* filp source code */
    FILP_BIN_CODE,      /* binary code (C function) */
    FILP_EXT_INT,       /* external integer */
    FILP_EXT_REAL,      /* external real (double) */
    FILP_EXT_STRING,    /* external string */
    FILP_NULL,          /* NULL value */
    FILP_FILE,          /* file descriptor (FILE *) */
    FILP_ARRAY          /* array */
} filp_type;

/* errors */

typedef enum {
    FILPERR_NONE,
    FILPERR_TOKEN_NOT_FOUND,
    FILPERR_SCALAR_EXPECTED,
    FILPERR_INTERNAL_ERROR,
    FILPERR_OUT_OF_MEMORY,
    FILPERR_ARRAY_EXPECTED,
    FILPERR_FILE_EXPECTED,
    FILPERR_FILE_NOT_FOUND,
    FILPERR_PERMISSION_DENIED,
    FILPERR_NOT_IMPLEMENTED,
    FILPERR_SYNTAX_ERROR
} filp_error;

/* status codes */

typedef enum {
    FILP_OK = 0,
    FILP_ERROR = -1,
    FILP_END = -2,
    FILP_BREAK = 1
} filp_status;


struct filp_val {
    filp_type type;             /* type */
    char *value;                /* pointer to content */
    int size;                   /* size */
    int count;                  /* usage count */
    struct filp_val **array;    /* array (if type == FILP_ARRAY) */
    struct filp_val *next;      /* next in values chain */
    int pipe:1;                 /* 1 if file is a pipe */
};

struct filp_stack {
    struct filp_val *value;     /* value */
    struct filp_stack *next;    /* next in chain */
};

struct filp_sym {
    filp_type type;             /* type */
    int size;                   /* size (for FILP_EXT_STRING) */
    char *name;                 /* symbol name */
    struct filp_val *value;     /* value */
    struct filp_sym *next;      /* next in chain */
};

/* externals */

extern int _filp_stack_size;
extern int _filp_stack_elems;
extern int _filp_val_account;
extern int _filp_sym_account;
extern char _filp_version[];
extern int _filp_real;
extern int _filp_bareword;
extern int _filp_error;
extern char _filp_error_info[80];
extern int _filp_isolate;
extern char *_filp_license;
extern int _in_filp;
extern struct filp_val *_filp_null_value;
extern struct filp_val *_filp_true_value;

/* macros */

/* is separator? */
#define filp_issep(c) ((c)==' ' || (c)=='\t' || (c)=='\n')

/* exports a C constant into Filp */
#define FILP_C_CONSTANT(c) filp_execf("/%s { %d } set", #c, c)

/* protos */

char *filp_poke(char *ptr, int *size, int offset, int c);
char *filp_splice(char *src, int offset, int size, char *new);
int filp_hashfunc(unsigned char *string, int mod);

struct filp_val *filp_new_value(filp_type type, void *value, int size);
void filp_ref_value(struct filp_val *v);
void filp_unref_value(struct filp_val *v);
int filp_cmp(struct filp_val *v1, struct filp_val *v2);
int filp_is_true(struct filp_val *v);

struct filp_val *filp_new_int_value(int value);
int filp_val_to_int(struct filp_val *v);
struct filp_val *filp_new_real_value(double value);
double filp_val_to_real(struct filp_val *v);
struct filp_val *filp_new_bin_code_value(int (*func) (void));

int filp_push(struct filp_val *v);
int filp_scalar_push(char *value);
int filp_int_push(int value);
int filp_real_push(double value);
int filp_null_push(void);
int filp_code_push(char *code);
int filp_bool_push(int value);

struct filp_val *filp_pop(void);
int filp_int_pop(void);
double filp_real_pop(void);
struct filp_val *filp_stack_value(int pos);
void filp_rot(int pos);
void filp_swap_stack(void);

struct filp_sym *filp_find_symbol(char *name);
struct filp_sym *filp_new_symbol(filp_type type, char *name);
struct filp_val *filp_get_symbol(struct filp_sym *s);
void filp_set_symbol(struct filp_sym *s, struct filp_val *v);
void filp_destroy_symbol(struct filp_sym *s);
int filp_bin_code(char *name, int (*func) (void));
int filp_ext_int(char *name, int *val);
int filp_ext_real(char *name, double *val);
int filp_ext_string(char *name, char *val, int size);
int filp_ext_file(char *name, FILE * f);

int filp_push_dict(char *mask);

extern FILE *(*filp_external_fopen) (char *filename, char *mode);

FILE *filp_fopen(char *filename, char *mode);
char *filp_load_file(char *filename);
int filp_list_size(void);

struct filp_val **filp_array_dim(int asize);
void filp_array_destroy(struct filp_val *array);
void filp_array_ins(struct filp_val *array, struct filp_val *val, int e);
int filp_array_size(struct filp_val *array);
struct filp_val *filp_array_get(struct filp_val *a, int e);
struct filp_val *filp_array_set(struct filp_val *a, struct filp_val *v, int e);
struct filp_val *filp_array_dup(struct filp_val *a);
struct filp_val *filp_array_del(struct filp_val *array, int e);
int filp_array_seek(struct filp_val *array, char *str, int inc);
int filp_array_binary_seek(struct filp_val *a, char *str, int inc);
void filp_array_sort(struct filp_val *value, int inc);

struct filp_val *filp_new_hash(int slots);
struct filp_val *filp_hash_get(struct filp_val *h, char *key);
struct filp_val *filp_hash_set(struct filp_val *h, char *key, struct filp_val *value);
struct filp_val *filp_hash_del(struct filp_val *h, char *key);
int filp_hash_size(struct filp_val *h);
int filp_hash_get_pair(struct filp_val *h, int i,
               struct filp_val **key, struct filp_val **value);

void filp_push_symbol_value(char *symbol);
int filp_exec(char *code);
int filp_execf(char *code, ...);
int filp_execv(struct filp_val *v);
int filp_load_exec(char *filename);
char *filp_dumper(struct filp_val *v, int max);
int filp_array_to_doubles(struct filp_val *v, double *d, int max);
struct filp_val *filp_doubles_to_array(double *d, int num);

char *filp_readline(char *prompt);
void filp_console(void);

int filp_sweep_head(void);
void filp_sweeper(int full);

void filp_lib_startup(void);
void filp_slib_startup(void);

int filp_startup(void);
void filp_shutdown(void);
