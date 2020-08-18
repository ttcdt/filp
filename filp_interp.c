/*

    filp - Embeddable, Reverse Polish Notation Programming Language

    Angel Ortega <angel@triptico.com>

    This software is released into the public domain.
    NO WARRANTY. See file LICENSE for details.

    Filp Interpreter.

*/

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "filp.h"

#ifndef FILP_ADDON_FUNC
#define FILP_ADDON_FUNC (void)0
#endif


int main(int argc, char *argv[])
{
    int n;
    char *prg = NULL;
    char *iprg = NULL;

    if (argc > 1 && strcmp(argv[1], "--version") == 0) {
        printf("filp " VERSION
               " - Embeddable, Reverse Polish Notation Programming Language\n");
        printf("Angel Ortega <angel@triptico.com>\n");
        exit(0);
    }

    filp_startup();

    /* awesome help function */
    filp_exec("/help { "
          "tpop \"filp_lib.c\" open dup "
          "{ /f #= } { \"Cannot open 'filp_lib.c'\" ? break } ifelse "
          "{ $f read } "
          "{ chop dup dup \"/**\" instr # tpush instr && "
          "{ ' (filp_lib)' . ? break } { pop } ifelse } while "
          "$f close "
          "\"filp_slib.c\" open dup "
          "{ /f #= } { \"Cannot open 'filp_slib.c'\" ? break } ifelse "
          "{ $f read } "
          "{ chop dup dup \"/**\" instr # tpush instr && "
          "{ ' (filp_slib)' . ? break } { pop } ifelse } while " "$f close } set ");

    /* awesome benchmark function */
    filp_exec("/_filp_benchmark { "
          "/_ 0 = "
          "time { dup time != { break } if } loop "
          "time { dup time != { break } if /_ ++ } loop " "pop pop $_ } set");

    filp_exec("{ './config.filp' load } eval pop");

    FILP_ADDON_FUNC;

    n = 1;

    if (argc > n && strcmp(argv[n], "-r") == 0) {
        /* -r activates real numbers */
        _filp_real = 1;
        n++;
    }

    if (argc > n) {
        if (strcmp(argv[n], "-e") == 0) {
            n++;
            if (argc > n)
                iprg = argv[n];
        }
        else
            prg = argv[n];

        n++;
    }

    /* store the rest as ARGV parameters */
    filp_exec("/ARGV (");

    for (; n < argc; n++)
        filp_scalar_push(argv[n]);
    filp_exec(") =");

    if (iprg)
        filp_exec(iprg);
    else if (prg)
        filp_load_exec(prg);
    else
        filp_console();

    filp_shutdown();

    return 0;
}
