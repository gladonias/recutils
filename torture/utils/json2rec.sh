#!/bin/sh
#
# json2rec.sh - System tests for json2rec.
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
test_init "json2rec"

#
# Create input files.
#

test_declare_input_file simple \
'[{"Name": "John", "Email": "john@example.com"}, {"Name": "Jane", "Email": "jane@example.com"}]
'

test_declare_input_file multi-value \
'[{"Name": "John", "Email": ["john@home.com", "john@work.com"]}]
'

test_declare_input_file multi-type \
'{"Contact": [{"Name": "John"}], "Task": [{"Title": "Fix bug"}]}
'

test_declare_input_file escape \
'[{"Name": "He said \"hello\"", "Note": "back\\\\slash", "Tab": "col1\tcol2"}]
'

test_declare_input_file empty \
''

test_declare_input_file with-type \
'[{"A": "a1"}]
'

test_declare_input_file numbers \
'[{"Name": "John", "Age": 30, "Score": 9.5}]
'

test_declare_input_file booleans \
'[{"Name": "John", "Active": true, "Deleted": false}]
'

#
# Declare tests.
#

test_tool json2rec-simple ok \
          json2rec \
          '' \
          simple \
'Name: John
Email: john@example.com

Name: Jane
Email: jane@example.com
'

test_tool json2rec-multi-value ok \
          json2rec \
          '' \
          multi-value \
'Name: John
Email: john@home.com
Email: john@work.com
'

test_tool json2rec-multi-type ok \
          json2rec \
          '' \
          multi-type \
'%rec: Contact

Name: John

%rec: Task

Title: Fix bug
'

test_tool json2rec-escape ok \
          json2rec \
          '' \
          escape \
'Name: He said "hello"
Note: back\\slash
Tab: col1	col2
'

test_tool json2rec-empty ok \
          json2rec \
          '' \
          empty \
''

test_tool json2rec-with-type ok \
          json2rec \
          '-t Foo' \
          with-type \
'%rec: Foo

A: a1
'

test_tool json2rec-numbers ok \
          json2rec \
          '' \
          numbers \
'Name: John
Age: 30
Score: 9.5
'

test_tool json2rec-booleans ok \
          json2rec \
          '' \
          booleans \
'Name: John
Active: true
Deleted: false
'

#
# Cleanup
#

test_cleanup
exit $?

# End of json2rec.sh
