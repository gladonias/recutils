# Makefile.am for recutils/man

# Copyright (C) 2010, 2020, 2022 Jose E. Marchesi

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

man_MANS = recdel.1 \
           recfix.1 \
           recinf.1 \
           recins.1 \
           recsel.1 \
           recset.1 \
           recfmt.1 \
           csv2rec.1 \
           rec2csv.1 \
           rec2json.1 \
           rec2sql.1

if COMPILE_MDB2REC
man_MANS += mdb2rec.1
endif

H2M_FILES = recsel.h2m recins.h2m recdel.h2m recset.h2m recinf.h2m \
            recfix.h2m recfmt.h2m rec2csv.h2m csv2rec.h2m mdb2rec.h2m \
            rec2json.h2m \
            rec2sql.h2m

EXTRA_DIST = $(man_MANS) $(H2M_FILES)
MAINTAINERCLEANFILES = $(man_MANS)

# The man pages depend on the --help strings and the version number.
common_mandeps = $(top_srcdir)/configure.ac \
                 $(top_srcdir)/utils/recutl.c

# Depend on the source, not the binary; we don't need to regenerate
# the binary when any source file changes, only the main one.
# Use -o so that the `missing' program can infer the output file.
# Use --include to add EXAMPLES and SEE ALSO sections from .h2m files.

rec2csv.1: $(top_srcdir)/utils/rec2csv.c $(common_mandeps) $(srcdir)/rec2csv.h2m
	$(HELP2MAN) -p recutils --name="rec to csv converter" --include=$(srcdir)/rec2csv.h2m $(builddir)/../utils/rec2csv -o $@

rec2json.1: $(top_srcdir)/utils/rec2json.c $(common_mandeps) $(srcdir)/rec2json.h2m
	$(HELP2MAN) -p recutils --name="rec to json converter" --include=$(srcdir)/rec2json.h2m $(builddir)/../utils/rec2json -o $@

rec2sql.1: $(top_srcdir)/utils/rec2sql.c $(common_mandeps) $(srcdir)/rec2sql.h2m
	$(HELP2MAN) -p recutils --name="rec to sql converter" --include=$(srcdir)/rec2sql.h2m $(builddir)/../utils/rec2sql -o $@

csv2rec.1: $(top_srcdir)/utils/csv2rec.c $(common_mandeps) $(srcdir)/csv2rec.h2m
	$(HELP2MAN) -p recutils --name="csv to rec converter" --include=$(srcdir)/csv2rec.h2m $(builddir)/../utils/csv2rec -o $@

mdb2rec.1: $(top_srcdir)/utils/mdb2rec.c $(common_mandeps) $(srcdir)/mdb2rec.h2m
	$(HELP2MAN) -p recutils --name="mdb to rec converter" --include=$(srcdir)/mdb2rec.h2m $(builddir)/../utils/mdb2rec -o $@

recdel.1: $(top_srcdir)/utils/recdel.c $(common_mandeps) $(srcdir)/recdel.h2m
	$(HELP2MAN) -p recutils --name="remove records from a recfile" --include=$(srcdir)/recdel.h2m $(builddir)/../utils/recdel -o $@

recfix.1: $(top_srcdir)/utils/recfix.c $(common_mandeps) $(srcdir)/recfix.h2m
	$(HELP2MAN) -p recutils --name="check a recfile for errors" --include=$(srcdir)/recfix.h2m $(builddir)/../utils/recfix -o $@

recinf.1: $(top_srcdir)/utils/recinf.c $(common_mandeps) $(srcdir)/recinf.h2m
	$(HELP2MAN) -p recutils --name="print information about a recfile" --include=$(srcdir)/recinf.h2m $(builddir)/../utils/recinf -o $@

recins.1: $(top_srcdir)/utils/recins.c $(common_mandeps) $(srcdir)/recins.h2m
	$(HELP2MAN) -p recutils --name="insert records in a recfile" --include=$(srcdir)/recins.h2m $(builddir)/../utils/recins -o $@

recsel.1: $(top_srcdir)/utils/recsel.c $(common_mandeps) $(srcdir)/recsel.h2m
	$(HELP2MAN) -p recutils --name="print records from a recfile" --include=$(srcdir)/recsel.h2m $(builddir)/../utils/recsel -o $@

recset.1: $(top_srcdir)/utils/recset.c $(common_mandeps) $(srcdir)/recset.h2m
	$(HELP2MAN) -p recutils --name="manipulate fields in a recfile" --include=$(srcdir)/recset.h2m $(builddir)/../utils/recset -o $@

recfmt.1: $(top_srcdir)/utils/recfmt.c $(common_mandeps) $(srcdir)/recfmt.h2m
	$(HELP2MAN) -p recutils --name="apply a template to records" --include=$(srcdir)/recfmt.h2m $(builddir)/../utils/recfmt -o $@

# End of Makefile.am
