#!/bin/sh
#
# rec2json.sh - System tests for rec2json.
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
test_init "rec2json"

#
# Create input files.
#

test_declare_input_file simple \
'Name: John
Email: john@example.com

Name: Jane
Email: jane@example.com
'

test_declare_input_file multi-value \
'Name: John
Email: john@home.com
Email: john@work.com
'

test_declare_input_file multi-type \
'%rec: Contact

Name: John

%rec: Task

Title: Fix bug
'

test_declare_input_file escape \
'Name: He said "hello"
Note: back\\slash
Tab: col1	col2
'

test_declare_input_file empty \
''

test_declare_input_file with-type \
'%rec: Foo

A: a1

%rec: Bar

B: b1
'

test_declare_input_file sort \
'%rec: Item
%type: Priority int

Name: Beta
Priority: 2

Name: Alpha
Priority: 1
'

#
# Declare tests.
#

test_tool rec2json-simple ok \
          rec2json \
          '' \
          simple \
'[{"Name": "John", "Email": "john@example.com"}, {"Name": "Jane", "Email": "jane@example.com"}]
'

test_tool rec2json-multi-value ok \
          rec2json \
          '' \
          multi-value \
'[{"Name": "John", "Email": ["john@home.com", "john@work.com"]}]
'

test_tool rec2json-multi-type ok \
          rec2json \
          '' \
          multi-type \
'{"Contact": [{"Name": "John"}], "Task": [{"Title": "Fix bug"}]}
'

test_tool rec2json-escape ok \
          rec2json \
          '' \
          escape \
'[{"Name": "He said \"hello\"", "Note": "back\\\\slash", "Tab": "col1\tcol2"}]
'

test_tool rec2json-empty ok \
          rec2json \
          '' \
          empty \
'[]
'

test_tool rec2json-with-type ok \
          rec2json \
          '-t Bar' \
          with-type \
'[{"B": "b1"}]
'

test_tool rec2json-sort ok \
          rec2json \
          '-S Priority' \
          sort \
'[{"Name": "Alpha", "Priority": "1"}, {"Name": "Beta", "Priority": "2"}]
'

#
# Cleanup
#

test_cleanup
exit $?

# End of rec2json.sh
