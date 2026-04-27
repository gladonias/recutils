dnl configure.ac for GNU rec
dnl
dnl Please process this file with autoconf to get a 'configure'
dnl script.

dnl Copyright (C) 2009-2022 Jose E. Marchesi

dnl This program is free software: you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation, either version 3 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program.  If not, see <http://www.gnu.org/licenses/>.

AC_INIT([GNU recutils], [1.9.1], [bug-recutils@gnu.org])

dnl Must come before AM_INIT_AUTOMAKE
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_MACRO_DIR([m4])

dnl Autobuild
AB_INIT

AC_PROG_CC
gl_EARLY

LT_INIT

dnl Both lex and yacc are required to generate the lexer/parser source
dnl files.

: ${LEX='flex'}
: ${LEXLIB='-lfl'}
: ${LEX_OUTPUT_ROOT='lex.yy'}
AC_ARG_VAR([LEX], [The flex implementation to use.])
AC_ARG_VAR([LEXLIB], [Options for linking with the flex runtime library.])
AC_ARG_VAR([LEX_OUTPUT_ROOT], [Base of the file name that the lexer generates.])

gl_PROG_BISON([RECUTILS_BISON],[3.6])

AS_IF([test "x$RECUTILS_BISON" = "x:"], [
    AC_MSG_ERROR([GNU Bison >= 3.6 is required but was not found.
  On macOS, install via: brew install bison
  Then ensure Homebrew bison is in your PATH:
    export PATH="/opt/homebrew/opt/bison/bin:\$PATH"  (Apple Silicon)
    export PATH="/usr/local/opt/bison/bin:\$PATH"     (Intel)])
])

dnl System
AC_CANONICAL_HOST
canonical=$host

gl_INIT

# GNU help2man creates man pages from --help output; in many cases, this
# is sufficient, and obviates the need to maintain man pages separately.
# However, this means invoking executables, which we generally cannot do
# when cross-compiling, so we test to avoid that (the variable
# "cross_compiling" is set by AC_PROG_CC).
AS_IF([test "x$cross_compiling" = "xno"], [
  AM_MISSING_PROG([HELP2MAN], [help2man])
], [
  HELP2MAN=:
])

dnl Require makeinfo (GNU Texinfo) for documentation
AC_CHECK_PROG([MAKEINFO_CHECK], [makeinfo], [yes], [no])
AS_IF([test "x$MAKEINFO_CHECK" = "xno"], [
    AC_MSG_ERROR([makeinfo is required (part of GNU Texinfo).
  On macOS, install via: brew install texinfo
  Then ensure Homebrew texinfo is in your PATH:
    export PATH="/opt/homebrew/opt/texinfo/bin:\$PATH"  (Apple Silicon)
    export PATH="/usr/local/opt/texinfo/bin:\$PATH"     (Intel)])
])

dnl Seach for headers
AC_CHECK_HEADERS([malloc.h string.h])

dnl Search for data types
AC_CHECK_TYPE([size_t], [unsigned])
AC_CHECK_TYPE([off_t], [long])

dnl Search for functions
AC_FUNC_FSEEKO

dnl Search for required libraries
PKG_PROG_PKG_CONFIG

have_check=no
PKG_CHECK_MODULES([CHECK], [check],
                  [have_check=yes], [have_check=no])
AM_CONDITIONAL([CHECK], [test "x$have_check" != "xno"])
AC_SUBST([CHECK_CFLAGS])
AC_SUBST([CHECK_LIBS])

AC_ARG_ENABLE([curl],
              AS_HELP_STRING([--enable-curl],
                             [Enable support for remote descriptors using libcurl (default is YES)]),
              [curl_enabled=$enableval], [curl_enabled=yes])
have_curl=no
AS_IF([test "x$curl_enabled" = "xyes"], [
    PKG_CHECK_MODULES([CURL], [libcurl], [have_curl=yes], [
        AC_MSG_ERROR([curl support enabled but libcurl not found])
    ])
], [])

AC_ARG_ENABLE([uuid],
              AS_HELP_STRING([--enable-uuid],
                             [Compile recutils with support for uuid types (default is YES)]),
              [uuid_enabled=$enableval], [uuid_enabled=yes])

have_uuid=no
UUIDLIBS=
AS_IF([test "x$uuid_enabled" = "xyes"], [
    dnl Try libuuid first (Linux)
    AC_CHECK_LIB([uuid], [uuid_generate], [have_uuid=yes; UUIDLIBS=-luuid], [])
    dnl If that failed, try without -luuid (macOS provides uuid in system libs)
    AS_IF([test "x$have_uuid" = "xno"], [
        AC_MSG_CHECKING([for uuid_generate without -luuid (macOS)])
        AC_LINK_IFELSE(
            [AC_LANG_PROGRAM([[#include <uuid/uuid.h>]],
                             [[uuid_t uu; uuid_generate(uu);]])],
            [have_uuid=yes; AC_MSG_RESULT([yes])],
            [AC_MSG_RESULT([no])])
    ])
    AS_IF([test "x$have_uuid" = "xno"], [
        AC_MSG_ERROR([uuid support enabled but no uuid implementation found.
  On Linux, install libuuid-dev (Debian/Ubuntu) or libuuid-devel (Fedora).
  On macOS, uuid is provided natively and should be detected automatically.])
    ])
], [])
AC_SUBST([UUIDLIBS])
AC_SUBST([have_uuid])

AC_ARG_ENABLE([encryption],
              AS_HELP_STRING([--enable-encryption],
                             [Compile recutils with encryption support (default is YES)]),
              [crypt_enabled=$enableval], [crypt_enabled=yes])

crypt_support=no
AS_IF([test "x$crypt_enabled" = "xyes"], [
    AC_LIB_HAVE_LINKFLAGS([gcrypt], [gpg-error], [#include <gpg-error.h>])
    crypt_support=$HAVE_LIBGCRYPT

    AS_IF([test "x$crypt_support" = "xyes"], [
        AC_DEFINE([REC_CRYPT_SUPPORT], [1], [Compile encryption support])
    ], [])
], [])

AM_CONDITIONAL([CRYPT], [test "x$crypt_support" = "xyes"])
AC_SUBST([crypt_support])

mdb_default=yes
case "${host_os}" in
    darwin*) mdb_default=no ;;
esac

AC_ARG_ENABLE([mdb],
              AS_HELP_STRING([--enable-mdb],
                             [Build the mdb2rec utility (default is YES on Linux, NO on macOS)]),
              [mdb_enabled=$enableval], [mdb_enabled=$mdb_default])
have_mdb=no
AS_IF([test "x$mdb_enabled" = "xyes"], [
    AC_CHECK_LIB([mdb], [mdb_init_backends], [have_mdb=yes], [])
    AS_IF([test "x$have_mdb" = "xyes"], [
        MDBLIBS=-lmdb

        OLD_CFLAGS=$CFLAGS
        CFLAGS="$CFLAGS $pkg_cv_GLIB_CFLAGS"
        mdb_uses_sdatetime=no
        AC_CHECK_DECL([MDB_SDATETIME], [mdb_uses_sdatetime=yes], [], [
            #include <glib.h>
            #include <mdbtools.h>
        ])
        CFLAGS=$OLD_CFLAGS

        AS_IF([test "x$mdb_uses_sdatetime" = "xyes"], [
           MDB_DATETIME="MDB_SDATETIME"
        ], [
         MDB_DATETIME="MDB_DATETIME"
       ])
    ], [
        AC_MSG_ERROR([mdb support enabled but libmdb not found])
    ])

    AC_SUBST([MDB_DATETIME])
], [])
AC_SUBST([MDBLIBS])

have_glib=no
AS_IF([test "x$mdb_enabled" = "xyes"], [
    PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.0.0],
                      [have_glib=yes], [have_glib=no])
], [])

AM_CONDITIONAL([COMPILE_MDB2REC],
               [test "x$have_glib" = "xyes" && test "x$have_mdb" = "xyes"])
AM_CONDITIONAL([REMOTE_DESCRIPTORS], [test "x$have_curl" = "xyes"])
AM_CONDITIONAL([UUID_TYPE], [test "x$have_uuid" = "xyes"])

dnl Bash builtins

AC_ARG_WITH([bash-headers],
            AS_HELP_STRING([--with-bash-headers],
                           [location of the bash header files (auto-detected by default)]),
            [BASH_HEADERS=$withval],
            [BASH_HEADERS=auto])

AS_IF([test "x$BASH_HEADERS" = "xauto"], [
    AS_IF([test -f /usr/include/bash/config.h], [
        BASH_HEADERS=/usr/include/bash
    ], [test -f /opt/homebrew/opt/bash/include/bash/config.h], [
        BASH_HEADERS=/opt/homebrew/opt/bash/include/bash
    ], [test -f /usr/local/opt/bash/include/bash/config.h], [
        BASH_HEADERS=/usr/local/opt/bash/include/bash
    ], [
        BASH_HEADERS=
    ])
])

AS_IF([test -n "$BASH_HEADERS" && test -f "${BASH_HEADERS}/config.h"], [
   AC_SUBST([BASH_HEADERS])
   bash_headers_available=yes
], [
   bash_headers_available=no
])

AC_ARG_ENABLE([bash-builtins],
              AS_HELP_STRING([--enable-bash-builtins],
                             [Build the recutils bash builtins (default is YES)]),
              [bash_builtins_enabled=$enableval], [bash_builtins_enabled=yes])

AM_CONDITIONAL([BASH_BUILTINS],
               [test "x$bash_headers_available" = "xyes" && test "x$bash_builtins_enabled" = "xyes"])

dnl Platform-based compilation options
compile_w32_system=no
case "${host}" in
     *-mingw32*)
          compile_w32_system=yes
          ;;
     *)
          ;;
esac

dnl i18n with gettext
AM_GNU_GETTEXT_VERSION([0.19.8])
AM_GNU_GETTEXT([external])

dnl gcov compilation
AC_ARG_ENABLE([coverage],
              AS_HELP_STRING([--enable-coverage],
                             [Compile the library with code coverage support (default is NO)]),
              [use_gcov=$enableval], [use_gcov=no])
AM_CONDITIONAL([USE_COVERAGE], [test "x$use_gcov" = "xyes"])

dnl Generate output files
AC_CONFIG_FILES(Makefile
                lib/Makefile
                libcsv/Makefile
                src/Makefile
                utils/Makefile
                bash/Makefile
                doc/Makefile
                po/Makefile.in
                torture/Makefile
                torture/utils/Makefile
                torture/utils/config.sh
                man/Makefile
                etc/Makefile)
AC_OUTPUT

dnl Report warnings

AS_IF([test "x$bash_headers_available" = "xno" || test "x$bash_builtins_enabled" = "xno"], [
    AC_MSG_WARN([Not building the recutils bash builtins])
], [])

dnl End of configure.ac
