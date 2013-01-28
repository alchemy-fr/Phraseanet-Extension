dnl $Id$
dnl config.m4 for extension phrasea2

PHP_ARG_ENABLE(phrasea2, whether to enable phrasea2 support,
[  --enable-phrasea2           Enable phrasea2 support])

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

    # PHP_ADD_LIBRARY_WITH_PATH($MYSQL_LIBNAME, $MYSQL_LIB_DIR, PHRASEA2_SHARED_LIBADD)
    PHP_SUBST(PHRASEA2_SHARED_LIBADD)
    PHP_ADD_LIBRARY(stdc++, 1, PHRASEA2_SHARED_LIBADD)

    dnl *************** search mysql inlude/lib path ****************
    AC_PATH_PROG([MYSQL_CONFIG], [mysql_config], , $PATH/usr/bin:/usr/local/mysql/bin)
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

    AC_CHECK_LIB([mysqlclient], [my_init], [LIBMYSQL="OK"], [LIBMYSQL="BAD"])

    STEF=`ls libstemmer_c/src_c/stem_UTF_8_*.c`


    PHP_NEW_EXTENSION(phrasea2, \
                        php_phrasea2/phrasea2.cpp \
                        ${STEF} \
                        libstemmer_c/runtime/api.c\
                        libstemmer_c/runtime/utilities.c\
                        libstemmer_c/libstemmer/libstemmer_utf8.c\
                        phrasea_engine/cache.cpp\
                        phrasea_engine/fetchresults.cpp \
                        phrasea_engine/phrasea_clock_t.cpp \
                        phrasea_engine/qtree.cpp \
                        phrasea_engine/query.cpp \
                        phrasea_engine/session.cpp \
                        phrasea_engine/sql.cpp \
                        phrasea_engine/cquerytree2parm.cpp \
                        phrasea_engine/mutex.cpp \
                        , $ext_shared, [], -DCOMPILE_DL_PHRASEA2 -DMYSQLENCODE=utf8)
fi
