REM filp - Borland C build

echo /* automatically created by buildbcc.bat - do not modify */ > config.h
type VERSION >> config.h
echo /* */ >> config.h
echo #define UNAME "MS Windows" >> config.h
echo #define WITHOUT_REGEX >> config.h

bcc32 -c filp_core.c
bcc32 -c filp_util.c
bcc32 -c filp_array.c
bcc32 -c filp_parse.c
bcc32 -c filp_lib.c
bcc32 -c filp_slib.c
bcc32 -c gnu_regex.c
tlib filp.lib -+filp_core.obj
tlib filp.lib -+filp_util.obj
tlib filp.lib -+filp_array.obj
tlib filp.lib -+filp_parse.obj
tlib filp.lib -+filp_lib.obj
tlib filp.lib -+filp_slib.obj
tlib filp.lib -+gnu_regex.obj
bcc32 -efilp.exe filp_interp.c filp.lib
