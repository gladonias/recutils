# src/ Makefile.am
# GNU recutils

# Copyright (C) 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017,
# 2018, 2019, 2020, 2022 Jose E. Marchesi

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

BUILT_SOURCES=
MOSTLYCLEANFILES=
MAINTAINERCLEANFILES=
EXTRA_DIST=

lib_LTLIBRARIES = librec.la

include_HEADERS = rec.h

librec_la_SOURCES = rec.c \
                    rec-mset.c \
                    rec-utils.h \
                    rec-utils.c \
                    rec-comment.c \
                    rec-field-name.c \
                    rec-field.c \
                    rec-record.c \
                    rec-rset.c \
                    rec-db.c \
                    rec-parser.c \
                    rec-writer.c \
                    rec-int.c \
                    rec-sex-parser.h \
                    rec-sex-parser.c \
                    rec-sex-ast.h \
                    rec-sex-ast.c \
                    rec-sex-tab.y \
                    rec-sex-lex.l \
                    rec-sex.c \
                    rec-fex.c \
                    rec-types.c \
                    rec-buf.c \
                    rec-aggregate.c

if CRYPT
   librec_la_SOURCES += rec-crypt.c
else
   librec_la_SOURCES += rec-crypt-dummy.c
endif

AM_LFLAGS = -d
# The Automake generated .l.c rule is broken: When executed in a VPATH build,
#   - The .c file gets generated in the build directory. But since it requires
#     special tools to rebuild it, we need to distribute it in the tarballs,
#     and by the GNU Coding Standards
#     <https://www.gnu.org/prep/standards/html_node/Makefile-Basics.html>
#     the file should be generated in the source directory.
#   - The #line directives in the .c file refer to a nonexistent file once it
#     has been moved from the build directory to the source directory. This
#     leads to error if 'lcov' is used later.
# Additionally, here we assume Flex and therefore don't need the ylwrap script.
# Therefore we override this rule.
# Since this is a rule that produces multiple files, we apply the idiom from
# <https://lists.gnu.org/archive/html/bug-make/2020-09/msg00008.html>, so that
# it works also in parallel 'make'.
generate-rec-sex-lex:
	$(AM_V_LEX)$(LEX) $(LFLAGS) $(AM_LFLAGS) -t $(srcdir)/rec-sex-lex.l > rec-sex-lex.c \
	&& test ':' = '$(LEX)' || { \
	  sed -e 's|".*/rec-sex-lex\.l"|"rec-sex-lex.l"|' \
	      -e 's|"lex\.yy\.c"|"rec-sex-lex.c"|' \
	      < rec-sex-lex.c > rec-sex-lex.c-tmp \
	  && sed -e 's|".*/rec-sex-lex\.l"|"rec-sex-lex.l"|' \
	         < rec-sex-lex.h > rec-sex-lex.h-tmp \
	  && rm -f rec-sex-lex.c rec-sex-lex.h \
	  && mv rec-sex-lex.c-tmp $(srcdir)/rec-sex-lex.c \
	  && mv rec-sex-lex.h-tmp $(srcdir)/rec-sex-lex.h; \
	}
.PHONY: generate-rec-sex-lex
# The above rule will generate files with time-stamp order
# rec-sex-lex.l <= rec-sex-lex.c <= rec-sex-lex.h.
rec-sex-lex.c: rec-sex-lex.l
	@{ test -f $(srcdir)/rec-sex-lex.c && test ! $(srcdir)/rec-sex-lex.c -ot $(srcdir)/rec-sex-lex.l; } || $(MAKE) generate-rec-sex-lex
rec-sex-lex.h: rec-sex-lex.c
	@{ test -f $(srcdir)/rec-sex-lex.h && test ! $(srcdir)/rec-sex-lex.h -ot $(srcdir)/rec-sex-lex.c; } || $(MAKE) generate-rec-sex-lex
BUILT_SOURCES += rec-sex-lex.c rec-sex-lex.h
MOSTLYCLEANFILES += rec-sex-lex.c-tmp rec-sex-lex.h-tmp
MAINTAINERCLEANFILES += $(srcdir)/rec-sex-lex.c $(srcdir)/rec-sex-lex.h
EXTRA_DIST += rec-sex-lex.l rec-sex-lex.c rec-sex-lex.h

AM_YFLAGS = -t --report=state
# The Automake generated .y.c rule is broken: When executed in a VPATH build,
#   - The .c file gets generated in the build directory. But since it requires
#     special tools to rebuild it, we need to distribute it in the tarballs,
#     and by the GNU Coding Standards
#     <https://www.gnu.org/prep/standards/html_node/Makefile-Basics.html>
#     the file should be generated in the source directory.
#   - The #line directives in the .c file refer to a nonexistent file once it
#     has been moved from the build directory to the source directory. This
#     leads to error if 'lcov' is used later.
# Additionally, here we assume GNU Bison and therefore don't need the ylwrap
# script.
# Therefore we override this rule.
# Since this is a rule that produces multiple files, we apply the idiom from
# <https://lists.gnu.org/archive/html/bug-make/2020-09/msg00008.html>, so that
# it works also in parallel 'make'.
generate-rec-sex-tab:
	$(AM_V_YACC)$(RECUTILS_BISON) -d $(YFLAGS) $(AM_YFLAGS) $(srcdir)/rec-sex-tab.y \
	&& test ':' = '$(POKE_BISON)' || { \
	  sed -e 's|".*/rec-sex-tab\.y"|"rec-sex-tab.y"|' \
	      -e 's|"rec-sex-tab\.tab\.c"|"rec-sex-tab.c"|' \
	      -e 's|"rec-sex-tab\.tab\.h"|"rec-sex-tab.h"|' \
	      < rec-sex-tab.tab.c > rec-sex-tab.c-tmp \
	  && sed -e 's|".*/rec-sex-tab\.y"|"rec-sex-tab.y"|' \
	         -e 's|"rec-sex-tab\.tab\.h"|"rec-sex-tab.h"|' \
	         < rec-sex-tab.tab.h > rec-sex-tab.h-tmp \
	  && rm -f rec-sex-tab.tab.c rec-sex-tab.tab.h \
	  && mv rec-sex-tab.c-tmp $(srcdir)/rec-sex-tab.c \
	  && mv rec-sex-tab.h-tmp $(srcdir)/rec-sex-tab.h; \
	}
.PHONY: generate-rec-sex-tab
# The above rule will generate files with time-stamp order
# rec-sex-tab.y <= rec-sex-tab.c <= rec-sex-tab.h.
rec-sex-tab.c: rec-sex-tab.y
	@{ test -f $(srcdir)/rec-sex-tab.c && test ! $(srcdir)/rec-sex-tab.c -ot $(srcdir)/rec-sex-tab.y; } || $(MAKE) generate-rec-sex-tab
rec-sex-tab.h: rec-sex-tab.c
	@{ test -f $(srcdir)/rec-sex-tab.h && test ! $(srcdir)/rec-sex-tab.h -ot $(srcdir)/rec-sex-tab.c; } || $(MAKE) generate-rec-sex-tab
BUILT_SOURCES += rec-sex-tab.c rec-sex-tab.h
MOSTLYCLEANFILES += \
  rec-sex-tab.tab.c rec-sex-tab.tab.h \
  rec-sex-tab.c-tmp rec-sex-tab.h-tmp \
  rec-sex-tab.output
MAINTAINERCLEANFILES += $(srcdir)/rec-sex-tab.c $(srcdir)/rec-sex-tab.h
EXTRA_DIST += rec-sex-tab.y rec-sex-tab.c rec-sex-tab.h

AM_CFLAGS = -Wall
if USE_COVERAGE
  AM_CFLAGS += -fprofile-arcs -ftest-coverage
endif

AM_LDFLAGS = -no-undefined
AM_CPPFLAGS = -I$(top_builddir)/lib -I$(top_srcdir)/lib \
              -DLOCALEDIR=\"$(localedir)\"
librec_la_LDFLAGS = -version-info 1:0:0
librec_la_LIBADD = $(top_builddir)/lib/librecutils.la \
                   $(LIB_CLOCK_GETTIME)

if CRYPT
   librec_la_LIBADD += $(LTLIBGCRYPT)
endif

if REMOTE_DESCRIPTORS
  AM_CPPFLAGS += -DREMOTE_DESCRIPTORS
  librec_la_LIBADD += $(CURL_LIBS)
endif

if UUID_TYPE
  AM_CPPFLAGS += -DUUID_TYPE
  librec_la_LIBADD += $(UUIDLIBS)
endif

check-syntax: all

# End of Makefile.am
