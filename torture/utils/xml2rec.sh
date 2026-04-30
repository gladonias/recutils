#!/bin/sh
#
# xml2rec.sh - System tests for xml2rec.
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
test_init "xml2rec"

#
# Create input files.
#

test_declare_input_file simple \
'<?xml version="1.0" encoding="UTF-8"?>
<recdata>
  <recordset>
    <record>
      <field name="Name">John</field>
      <field name="Email">john@example.com</field>
    </record>
    <record>
      <field name="Name">Jane</field>
      <field name="Email">jane@example.com</field>
    </record>
  </recordset>
</recdata>
'

test_declare_input_file typed \
'<?xml version="1.0" encoding="UTF-8"?>
<recdata>
  <recordset type="Contact">
    <record>
      <field name="Name">John</field>
    </record>
  </recordset>
  <recordset type="Task">
    <record>
      <field name="Title">Fix bug</field>
    </record>
  </recordset>
</recdata>
'

test_declare_input_file multi-value \
'<?xml version="1.0" encoding="UTF-8"?>
<recdata>
  <recordset>
    <record>
      <field name="Name">John</field>
      <field name="Email">john@home.com</field>
      <field name="Email">john@work.com</field>
    </record>
  </recordset>
</recdata>
'

test_declare_input_file escape \
'<?xml version="1.0" encoding="UTF-8"?>
<recdata>
  <recordset>
    <record>
      <field name="Name">A &amp; B</field>
      <field name="Note">&lt;tag&gt;</field>
      <field name="Quote">&quot;hello&quot; &amp; &apos;world&apos;</field>
    </record>
  </recordset>
</recdata>
'

test_declare_input_file empty \
''

test_declare_input_file with-type \
'<?xml version="1.0" encoding="UTF-8"?>
<recdata>
  <recordset>
    <record>
      <field name="A">a1</field>
    </record>
  </recordset>
</recdata>
'

#
# Declare tests.
#

test_tool xml2rec-simple ok \
          xml2rec \
          '' \
          simple \
'Name: John
Email: john@example.com

Name: Jane
Email: jane@example.com
'

test_tool xml2rec-typed ok \
          xml2rec \
          '' \
          typed \
'%rec: Contact

Name: John

%rec: Task

Title: Fix bug
'

test_tool xml2rec-multi-value ok \
          xml2rec \
          '' \
          multi-value \
'Name: John
Email: john@home.com
Email: john@work.com
'

test_tool xml2rec-escape ok \
          xml2rec \
          '' \
          escape \
'Name: A & B
Note: <tag>
Quote: "hello" & '"'"'world'"'"'
'

test_tool xml2rec-empty ok \
          xml2rec \
          '' \
          empty \
''

test_tool xml2rec-with-type ok \
          xml2rec \
          '-t Foo' \
          with-type \
'%rec: Foo

A: a1
'

#
# Cleanup
#

test_cleanup
exit $?

# End of xml2rec.sh
