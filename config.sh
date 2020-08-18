#!/bin/sh

# Configuration shell script

# gets program version
VERSION=`cut -f2 -d\" VERSION`

# store command line args for configuring the libraries
CONF_ARGS="$*"

# default installation prefix
PREFIX=/usr/local

# installation directory for documents
DOCDIR=""
DOCS="\$(ADD_DOCS)"

TARGET=filp

MINGW32_PREFIX=x86_64-w64-mingw32

# parse arguments
while [ $# -gt 0 ] ; do

    case $1 in
    --without-win32)        WITHOUT_WIN32=1 ;;
    --without-unix-glob)    WITHOUT_UNIX_GLOB=1 ;;
    --without-regex)        WITHOUT_REGEX=1 ;;
    --with-included-regex)  WITH_INCLUDED_REGEX=1 ;;
    --with-pcre)            WITH_PCRE=1 ;;
    --help)                 CONFIG_HELP=1 ;;

    --mingw32-prefix=*)     MINGW32_PREFIX=`echo $1 | sed -e 's/--mingw32-prefix=//'`
                            ;;

    --mingw32)          CC=${MINGW32_PREFIX}-gcc
                        WINDRES=${MINGW32_PREFIX}-windres
                        AR=${MINGW32_PREFIX}-ar
                        LD=${MINGW32_PREFIX}-ld
                        CPP=${MINGW32_PREFIX}-g++
                        ;;

    --prefix)       PREFIX=$2 ; shift ;;
    --prefix=*)     PREFIX=`echo $1 | sed -e 's/--prefix=//'` ;;
    esac

    shift
done

if [ "$CONFIG_HELP" = "1" ] ; then

    echo "Available options:"
    echo "--prefix=PREFIX       Installation prefix ($PREFIX)."
    echo "--without-win32       Disable win32 interface detection."
    echo "--without-unix-glob   Disable glob.h usage (use workaround)."
    echo "--with-included-regex Use included regex code (gnu_regex.c)."
    echo "--with-pcre           Enable PCRE library detection."
    echo "--mingw32             Build using the mingw32 compiler."

    echo
    echo "Environment variables:"
    echo "CC                    C Compiler."
    echo "CFLAGS                Compile flags (i.e., -O3)."
    echo "AR                    Library Archiver."

    exit 1
fi

echo "Configuring Filp..."

echo "/* automatically created by config.sh - do not modify */" > config.h
echo "# automatically created by config.sh - do not modify" > makefile.opts
> config.ldflags
> config.cflags
> .config.log

# set compiler
if [ "$CC" = "" ] ; then
    CC=cc
    # if CC is unset, try if gcc is available
    which gcc > /dev/null

    if [ $? = 0 ] ; then
        CC=gcc
    fi
fi

echo "CC=$CC" >> makefile.opts

# set cflags
if [ "$CFLAGS" = "" -a "$CC" = "gcc" ] ; then
    CFLAGS="-g -Wall"
fi

echo "CFLAGS=$CFLAGS" >> makefile.opts

# Add CFLAGS to CC
CC="$CC $CFLAGS"

# set archiver
if [ "$AR" = "" ] ; then
    AR=ar
fi

echo "AR=$AR" >> makefile.opts

# add version
cat VERSION >> config.h

# add installation prefix
echo "#define CONFOPT_PREFIX \"$PREFIX\"" >> config.h

# get uname
CONFOPT_UNAME=`uname -mrs`

#########################################################

# configuration directives

# Win32
echo -n "Testing for win32... "
if [ "$WITHOUT_WIN32" = "1" ] ; then
    echo "Disabled by user"
else
    echo "#include <windows.h>" > .tmp.c
    echo "#include <commctrl.h>" >> .tmp.c
    echo "int STDCALL WinMain(HINSTANCE h, HINSTANCE p, LPSTR c, int m)" >> .tmp.c
    echo "{ return 0; }" >> .tmp.c

    TMP_LDFLAGS=""
    $CC .tmp.c $TMP_LDFLAGS -o .tmp.o 2>> .config.log

    if [ $? = 0 ] ; then
        CONFOPT_UNAME="MS Windows"
        echo "#define CONFOPT_WIN32 1" >> config.h
        echo "$TMP_LDFLAGS " >> config.ldflags
        echo "OK"
        WITHOUT_UNIX_GLOB=1
    else
        echo "No"
    fi
fi

# glob.h support
if [ "$WITHOUT_UNIX_GLOB" != 1 ] ; then
    echo -n "Testing for unix-like glob.h... "
    echo "#include <stdio.h>" > .tmp.c
    echo "#include <glob.h>" >> .tmp.c
    echo "int main(void) { glob_t g; g.gl_offs=1; glob(\"*\",GLOB_MARK,NULL,&g); return 0; }" >> .tmp.c

    $CC .tmp.c -o .tmp.o 2>> .config.log

    if [ $? = 0 ] ; then
        echo "#define CONFOPT_GLOB_H 1" >> config.h
        echo "OK"
    else
        echo "No"
    fi
fi

# regex
echo -n "Testing for regular expressions... "

if [ "$WITH_PCRE" = 1 ] ; then
    # try first the pcre library
    TMP_CFLAGS="-I/usr/local/include"
    TMP_LDFLAGS="-L/usr/local/lib -lpcre -lpcreposix"
    echo "#include <pcreposix.h>" > .tmp.c
    echo "int main(void) { regex_t r; regmatch_t m; regcomp(&r,\".*\",REG_EXTENDED|REG_ICASE); return 0; }" >> .tmp.c

    $CC $TMP_CFLAGS .tmp.c $TMP_LDFLAGS -o .tmp.o 2>> .config.log

    if [ $? = 0 ] ; then
        echo "OK (using pcre library)"
        echo "#define CONFOPT_PCRE 1" >> config.h
        echo "$TMP_CFLAGS " >> config.cflags
        echo "$TMP_LDFLAGS " >> config.ldflags
        REGEX_YET=1
    fi
fi

if [ "$REGEX_YET" != 1 -a "$WITH_INCLUDED_REGEX" != 1 ] ; then
    echo "#include <sys/types.h>" > .tmp.c
    echo "#include <regex.h>" >> .tmp.c
    echo "int main(void) { regex_t r; regmatch_t m; regcomp(&r,\".*\",REG_EXTENDED|REG_ICASE); return 0; }" >> .tmp.c

    $CC .tmp.c -o .tmp.o 2>> .config.log

    if [ $? = 0 ] ; then
        echo "OK (using system one)"
        echo "#define CONFOPT_SYSTEM_REGEX 1" >> config.h
        REGEX_YET=1
    fi
fi

if [ "$REGEX_YET" != 1 ] ; then
    # if system libraries lack regex, try compiling the
    # included gnu_regex.c

    $CC -c -DSTD_HEADERS -DREGEX gnu_regex.c -o .tmp.o 2>> .config.log

    if [ $? = 0 ] ; then
        echo "OK (using included gnu_regex.c)"
        echo "#define HAVE_STRING_H 1" >> config.h
        echo "#define REGEX 1" >> config.h
        echo "#define CONFOPT_INCLUDED_REGEX 1" >> config.h
    else
        echo "#define CONFOPT_NO_REGEX 1" >> config.h
        echo "No (No usable regex library)"
    fi
fi

# unistd.h detection
echo -n "Testing for unistd.h... "
echo "#include <unistd.h>" > .tmp.c
echo "int main(void) { return(0); }" >> .tmp.c

$CC .tmp.c -o .tmp.o 2>> .config.log

if [ $? = 0 ] ; then
    echo "#define CONFOPT_UNISTD_H 1" >> config.h
    echo "OK"
else
    echo "No"
fi

# test for gettimeofday()
echo -n "Testing for gettimeofday()... "
echo "#include <stdio.h>" > .tmp.c
echo "#include <sys/time.h>" >> .tmp.c
echo "#include <time.h>" >> .tmp.c
echo "int main(void) { struct timeval tv; gettimeofday(&tv, NULL); return 0; }" >> .tmp.c

$CC .tmp.c -o .tmp.o 2>> .config.log
if [ $? = 0 ] ; then
    echo "#define CONFOPT_GETTIMEOFDAY 1" >> config.h
    echo "OK"
else
    echo "No"
fi


# test for termios.h
echo -n "Testing for termios.h... "

echo "#include <termios.h>" > .tmp.c
echo "int main(void) { struct termios t; tcgetattr(0,&t); return 0; }" >> .tmp.c

$CC .tmp.c -o .tmp.o 2>> .config.log
if [ $? = 0 ] ; then
    echo "#define CONFOPT_TERMIOS 1" >> config.h
    echo "OK"
else
    echo "No"
fi


# test for readline()
echo -n "Testing for readline()... "

echo "#include <readline/readline.h>" > .tmp.c
echo "#include <readline/history.h>" >> .tmp.c
echo "int main(void) { readline(\"prompt:\"); return 0; }" >> .tmp.c

$CC .tmp.c -lreadline -o .tmp.o 2>> .config.log

if [ $? = 0 ] ; then
    echo "#define CONFOPT_READLINE 1" >> config.h
    echo "-lreadline" >> config.ldflags
    echo "OK"
else
    echo "No"
fi

# test for Grutatxt
echo -n "Testing if Grutatxt is installed... "

DOCS="\$(ADD_DOCS)"

if which grutatxt > /dev/null ; then
    echo "OK"
    echo "GRUTATXT=yes" >> makefile.opts
    DOCS="$DOCS \$(GRUTATXT_DOCS)"
else
    echo "No"
    echo "GRUTATXT=no" >> makefile.opts
fi

# test for mp_doccer
echo -n "Testing if mp_doccer is installed... "
MP_DOCCER=$(which mp_doccer || which mp-doccer)

if [ $? = 0 ] ; then

    if ${MP_DOCCER} --help | grep grutatxt > /dev/null ; then

        echo "OK"

        echo "MP_DOCCER=yes" >> makefile.opts
        DOCS="$DOCS \$(MP_DOCCER_DOCS)"

        grep GRUTATXT=yes makefile.opts > /dev/null && DOCS="$DOCS \$(G_AND_MP_DOCS)"
    else
        echo "Outdated (No)"
        echo "MP_DOCCER=no" >> makefile.opts
    fi
else
    echo "No"
    echo "MP_DOCCER=no" >> makefile.opts
fi

# store uname
echo "#define CONFOPT_UNAME \"$CONFOPT_UNAME\"" >> config.h

# if win32, the interpreter is called filp.exe
grep CONFOPT_WIN32 config.h >/dev/null && TARGET=filp.exe

#########################################################

# final setup

echo "DOCS=$DOCS" >> makefile.opts
echo "VERSION=$VERSION" >> makefile.opts
echo "PREFIX=\$(DESTDIR)$PREFIX" >> makefile.opts
echo "CONF_ARGS=$CONF_ARGS" >> makefile.opts
echo "TARGET=$TARGET" >> makefile.opts
echo >> makefile.opts

cat makefile.opts makefile.in makefile.depend > Makefile

#########################################################

# cleanup

rm -f .tmp.c .tmp.o

exit 0
