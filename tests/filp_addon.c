/*

    filp_addon.c - Filp addon example

    Angel Ortega <angel@triptico.com>

    This software is released into the public domain.
    NO WARRANTY. See file LICENSE for details.

*/

/*

    To build addons to the Filp interpreter:

    - Write code (like this one) and compile it as an object or library:

	gcc -I.. filp_addon.c -c

    - Add the ADDON_LIBRARIES and ADDON_ENTRY_POINTS to the make command line:

	make ADDON_LIBRARIES=tests/filp_addon.o ADDON_ENTRY_POINTS="filp_addon();"

    ADDON_LIBRARIES can be multiple, objects or libraries; ADDON_ENTRY_POINTS
    can also be several calls to C functions. You can safely ignore any warning
    about 'implicit declaration of function'.

*/

#include "filp.h"

/*******************
	Data
********************/

/*******************
	Code
********************/

void filp_addon(void)
{
	filp_exec("/addon_test { \"It works!!!\" } set");
}

