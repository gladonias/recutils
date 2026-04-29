#!/bin/sh
#
# rec2xml.sh - System tests for rec2xml.
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
test_init "rec2xml"

#
# Create input files.
#

test_declare_input_file simple \
'Name: John
Email: john@example.com
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
'Name: A&B <Corp>
Note: "quoted" & '\''apos'\''
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

test_tool rec2xml-simple ok \
          rec2xml \
          '' \
          simple \
'<?xml version="1.0" encoding="UTF-8"?>
<recdata>
  <recordset>
    <record>
      <field name="Name">John</field>
      <field name="Email">john@example.com</field>
    </record>
  </recordset>
</recdata>
'

test_tool rec2xml-typed ok \
          rec2xml \
          '' \
          typed \
'<?xml version="1.0" encoding="UTF-8"?>
<recdata>
  <recordset type="Contact">
    <record>
      <field name="Name">John</field>
      <field name="Email">john@example.com</field>
    </record>
  </recordset>
</recdata>
'

test_tool rec2xml-multi-value ok \
          rec2xml \
          '' \
          multi-value \
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

test_tool rec2xml-escape ok \
          rec2xml \
          '' \
          escape \
'<?xml version="1.0" encoding="UTF-8"?>
<recdata>
  <recordset>
    <record>
      <field name="Name">A&amp;B &lt;Corp&gt;</field>
      <field name="Note">&quot;quoted&quot; &amp; &apos;apos&apos;</field>
    </record>
  </recordset>
</recdata>
'

test_tool rec2xml-empty ok \
          rec2xml \
          '' \
          empty \
'<?xml version="1.0" encoding="UTF-8"?>
<recdata>
</recdata>
'

test_tool rec2xml-with-type ok \
          rec2xml \
          '-t Task' \
          multi-type \
'<?xml version="1.0" encoding="UTF-8"?>
<recdata>
  <recordset type="Task">
    <record>
      <field name="Title">Fix bug</field>
    </record>
  </recordset>
</recdata>
'

#
# Cleanup
#

test_cleanup
exit $?

# End of rec2xml.sh
