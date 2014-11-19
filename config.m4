dnl $Id$
dnl config.m4 for extension phrasea2

PHP_ARG_ENABLE(phrasea2, whether to enable phrasea2 support,
[  --enable-phrasea2           Enable phrasea2 support])

PHP_ARG_ENABLE(phrasea2-log,  enable extension to log in /tmp/phraseanet-extension.log,
[  --enable-phrasea2-log      enable extension to log in /tmp/phraseanet-extension.log], no, no)

dnl PHP_ARG_WITH(phrasea2, for phrasea2 support,
dnl [  --with-phrasea2[=DIR]      Include phrasea2 support. DIR is the phrasea2 base directory])

if test "$PHP_PHRASEA2" != "no"; then
    dnl  PHP_EXPAND_PATH($MYSQL_INCLUDE, MYSQL_INCLUDE)
    
    dnl *************** get version ****************
    AC_MSG_CHECKING([====== phrasea extension version number])
    PHRASEAVERSION=
    if test -r php_phrasea2/_VERSION.h; then
        PHRASEAVERSION=`cat php_phrasea2/_VERSION.h | egrep -e "#define[[:blank:]]*PHDOTVERSION" | tr -d '\r' | sed  -e "s/#define[[:blank:]]*PHDOTVERSION[[:blank:]]*\(.*\)[[:space:]]*$/\1/"`
        AC_MSG_RESULT([$PHRASEAVERSION])
    else
        AC_MSG_ERROR([Cannot find file _VERSION.h])
    fi


    dnl ****************** tell we will compile .cpp files *****************
    PHP_REQUIRE_CXX()
    # CPPFLAGS="$CPPFLAGS -Wno-write-strings"
    # CPPFLAGS="$CPPFLAGS -g3"
    # CPPFLAGS="$CPPFLAGS -gdwarf-2"

    dnl ****************** tell we will link with g++ *****************
    CC="g++"
    # CC="$CC -g3"
    # CC="$CC -gdwarf-2"

dnl Discard optimization flags when debugging is enabled
if test "$PHP_DEBUG" = "yes"; then
  PHP_DEBUG=1
  ZEND_DEBUG=yes
  changequote({,})
  CFLAGS=`echo "$CFLAGS" | $SED -e 's/-O[0-9s]*//g'`
  CXXFLAGS=`echo "$CXXFLAGS" | $SED -e 's/-O[0-9s]*//g'`
  changequote([,])
  dnl add -O0 only if GCC or ICC is used
  if test "$GCC" = "yes" || test "$ICC" = "yes"; then
    CFLAGS="$CFLAGS -O0"
    CXXFLAGS="$CXXFLAGS -g -O0"
  fi
  if test "$SUNCC" = "yes"; then
    if test -n "$auto_cflags"; then
      CFLAGS="-g"
      CXXFLAGS="-g"
    else
      CFLAGS="$CFLAGS -g"
      CXXFLAGS="$CFLAGS -g"
    fi
  fi
else
  PHP_DEBUG=0
  ZEND_DEBUG=no
fi

    dnl Check whether to enable loging
    if test "$PHP_PHRASEA2_LOG" == "yes"; then
        dnl Yes, so set the C macro
        AC_DEFINE(USE_PHRASEA2_LOG,1,[Include loging support in phrasea2])
    fi


    # PHP_ADD_LIBRARY_WITH_PATH($MYSQL_LIBNAME, $MYSQL_LIB_DIR, PHRASEA2_SHARED_LIBADD)
    PHP_SUBST(PHRASEA2_SHARED_LIBADD)
    PHP_ADD_LIBRARY(stdc++, 1, PHRASEA2_SHARED_LIBADD)

    dnl *************** search mysql inlude/lib path ****************
    AC_PATH_PROG([MYSQL_CONFIG], [mysql_config], , $PATH:/usr/bin:/usr/local/mysql/bin)
    if test "x$MYSQL_CONFIG" = "x"; then
        AC_MSG_ERROR([mysql_config program not found])
        exit 3
    else
        MYSQLCFLAGS=`$MYSQL_CONFIG --cflags`
        CPPFLAGS="${CPPFLAGS} $MYSQLCFLAGS"

        if test "$enable_maintainer_zts" = "yes"; then
            LBASE=`$MYSQL_CONFIG --libs_r`
        else
            LBASE=`$MYSQL_CONFIG --libs`
        fi
        LDFLAGS="${LDFLAGS} $LBASE"
    fi





#    # Determine XML2 include path
#    AC_MSG_CHECKING(for libxml/xmlmemory.h)
#
#    # Can we include headers using system include dirs?
#    AC_TRY_COMPILE([#include <libxml/xmlmemory.h>], [int a = 1;],
#        XML2_INCLUDE=" ",
#        XML2_INCLUDE=
#    )
#
#    # Hunt through several possible directories to find the includes for libxml2
#    if test "x$XML2_INCLUDE" = "x"; then
#        old_CPPFLAGS="$CPPFLAGS"
#        for i in $xml2_include_dir /usr/include /usr/local/include /usr/include/libxml2 /usr/local/include/libxml2 ; do
#            CPPFLAGS="$old_CPPFLAGS -I$i"
#            AC_TRY_COMPILE([#include <libxml/xmlmemory.h>], [int a = 1;],
#                XML2_INCLUDE="-I$i",
#                XML2_INCLUDE=
#            )
#            if test "x$XML2_INCLUDE" != "x"; then
#                break;
#            fi
#        done
#        CPPFLAGS="$old_CPPFLAGS $XML2_INCLUDE"
#    fi
#    AC_MSG_RESULT([$XML2_INCLUDE])



    AC_CHECK_LIB([mysqlclient], [my_init], [LIBMYSQL="OK"], [LIBMYSQL="BAD"])
#    AC_CHECK_LIB([xml2], [xmlInitParser], [LIBXML="OK"], [LIBXML="BAD"])

    STEF=`ls libstemmer_c/src_c/stem_UTF_8_*.c`


    PHP_NEW_EXTENSION(phrasea2, \
                        php_phrasea2/phrasea2.cpp \
                        ${STEF} \
                        libstemmer_c/runtime/api.c\
                        libstemmer_c/runtime/utilities.c\
                        libstemmer_c/libstemmer/libstemmer_utf8.c\
                        phrasea_engine/phrasea_types.cpp \
                        phrasea_engine/cache.cpp\
                        phrasea_engine/fetchresults.cpp \
                        phrasea_engine/phrasea_clock_t.cpp \
                        phrasea_engine/qtree.cpp \
                        phrasea_engine/qtree3.cpp \
                        phrasea_engine/query.cpp \
                        phrasea_engine/query3.cpp \
                        phrasea_engine/session.cpp \
                        phrasea_engine/sql.cpp \
                        phrasea_engine/cquerytree2parm.cpp \
                        phrasea_engine/mutex.cpp \
                        phrasea_engine/unicode.cpp \
                        , $ext_shared, [], -DCOMPILE_DL_PHRASEA2 -DMYSQLENCODE=utf8)
fi
