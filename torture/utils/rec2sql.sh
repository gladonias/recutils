#!/bin/sh
#
# rec2sql.sh - System tests for rec2sql.
#
# Copyright (C) 2026 Jose E. Marchesi.
#
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

#
# Initialization
#

: ${srcdir=.}
. $srcdir/testutils.sh
test_init "rec2sql"

#
# Create input files.
#

test_declare_input_file simple \
'Name: John
Email: john@example.com

Name: Jane
Email: jane@example.com
'

test_declare_input_file typed \
'%rec: Contact

Name: John
Email: john@example.com
'

test_declare_input_file multi-value \
'Name: John
Email: john@home.com
Email: john@work.com
'

test_declare_input_file escape \
'Name: O'\''Brien
City: New York
'

test_declare_input_file empty \
''

test_declare_input_file multi-type \
'%rec: Contact

Name: John

%rec: Task

Title: Fix bug
'

#
# Declare tests.
#

test_tool rec2sql-simple ok \
          rec2sql \
          '' \
          simple \
'INSERT INTO "data" ("Name", "Email") VALUES ('\''John'\'', '\''john@example.com'\'');
INSERT INTO "data" ("Name", "Email") VALUES ('\''Jane'\'', '\''jane@example.com'\'');
'

test_tool rec2sql-typed ok \
          rec2sql \
          '' \
          typed \
'INSERT INTO "Contact" ("Name", "Email") VALUES ('\''John'\'', '\''john@example.com'\'');
'

test_tool rec2sql-table-override ok \
          rec2sql \
          '-T my_table' \
          typed \
'INSERT INTO "my_table" ("Name", "Email") VALUES ('\''John'\'', '\''john@example.com'\'');
'

test_tool rec2sql-multi-value ok \
          rec2sql \
          '' \
          multi-value \
'INSERT INTO "data" ("Name", "Email") VALUES ('\''John'\'', '\''john@home.com\njohn@work.com'\'');
'

test_tool rec2sql-escape ok \
          rec2sql \
          '' \
          escape \
'INSERT INTO "data" ("Name", "City") VALUES ('\''O'\'''\''Brien'\'', '\''New York'\'');
'

test_tool rec2sql-empty ok \
          rec2sql \
          '' \
          empty \
''

test_tool rec2sql-with-type ok \
          rec2sql \
          '-t Task' \
          multi-type \
'INSERT INTO "Task" ("Title") VALUES ('\''Fix bug'\'');
'

#
# Cleanup
#

test_cleanup
exit $?

# End of rec2sql.sh
