/* rec2xml.c - rec to XML converter.  */

/* Copyright (C) 2026 Jose E. Marchesi */

/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <xalloc.h>
#include <gettext.h>
#define _(str) gettext (str)

#include <rec.h>
#include <recutl.h>

/*
 * Global variables
 */

char *rec2xml_record_type = NULL;
rec_fex_t rec2xml_sort_by_fields = NULL;

/*
 * Command line options management
 */

enum
  {
    COMMON_ARGS,
    RECORD_TYPE_ARG,
    SORT_ARG
  };

static const struct option GNU_longOptions[] =
  {
    COMMON_LONG_ARGS,
    {"type", required_argument, NULL, RECORD_TYPE_ARG},
    {"sort", required_argument, NULL, SORT_ARG},
    {NULL, 0, NULL, 0}
  };

/*
 * Functions.
 */

void
recutl_print_help (void)
{
  printf (_("\
Usage: rec2xml [OPTIONS]... [REC_FILE]\n"));

  fputs (_("\
Convert rec data into XML data.\n"), stdout);

  puts ("");
  fputs (_("\
  -t, --type=TYPE                     record set to convert; if this parameter\n\
                                        is omitted then the default record set is used\n\
  -S, --sort=FIELDS                   sort the output by the specified fields.\n"),
         stdout);

  recutl_print_help_common ();
  puts ("");
  recutl_print_help_footer ();
}

static void
rec2xml_parse_args (int argc, char **argv)
{
  int ret;
  char c;

  while ((ret = getopt_long (argc,
                             argv,
                             "t:S:",
                             GNU_longOptions,
                             NULL)) != -1)
    {
      c = ret;
      switch (c)
        {
          COMMON_ARGS_CASES
        case RECORD_TYPE_ARG:
        case 't':
          rec2xml_record_type = xstrdup (optarg);
          break;
        case SORT_ARG:
        case 'S':
          if (rec2xml_sort_by_fields)
            recutl_fatal (_("only one list of fields can be specified\
 as a sorting criteria.\n"));

          if (!rec_fex_check (optarg, REC_FEX_CSV))
            recutl_fatal (_("invalid field name list in -S.\n"));

          rec2xml_sort_by_fields = rec_fex_new (optarg, REC_FEX_CSV);
          if (!rec2xml_sort_by_fields)
            recutl_fatal (_("internal error creating fex.\n"));

          break;
        default:
          exit (EXIT_FAILURE);
        }
    }
}

static void
rec2xml_write_escaped (const char *str)
{
  const char *p;

  for (p = str; *p; p++)
    {
      switch (*p)
        {
        case '&':  fputs ("&amp;", stdout); break;
        case '<':  fputs ("&lt;", stdout); break;
        case '>':  fputs ("&gt;", stdout); break;
        case '"':  fputs ("&quot;", stdout); break;
        case '\'': fputs ("&apos;", stdout); break;
        default:
          putchar (*p);
          break;
        }
    }
}

static void
rec2xml_generate_record (rec_record_t record, const char *indent)
{
  rec_mset_iterator_t iter;
  rec_field_t field;

  printf ("%s<record>\n", indent);

  iter = rec_mset_iterator (rec_record_mset (record));
  while (rec_mset_iterator_next (&iter, MSET_FIELD,
                                 (const void **) &field, NULL))
    {
      const char *name = rec_field_name (field);
      const char *value = rec_field_value (field);

      printf ("%s  <field name=\"", indent);
      rec2xml_write_escaped (name);
      fputs ("\">", stdout);
      rec2xml_write_escaped (value);
      fputs ("</field>\n", stdout);
    }
  rec_mset_iterator_free (&iter);

  printf ("%s</record>\n", indent);
}

static void
rec2xml_generate_rset (rec_rset_t rset, const char *indent)
{
  rec_mset_iterator_t iter;
  rec_record_t record;

  iter = rec_mset_iterator (rec_rset_mset (rset));
  while (rec_mset_iterator_next (&iter, MSET_RECORD,
                                 (const void **) &record, NULL))
    rec2xml_generate_record (record, indent);

  rec_mset_iterator_free (&iter);
}

static bool
rec2xml_process_data (rec_db_t db)
{
  bool ret = true;
  size_t i;
  rec_rset_t rset;

  puts ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
  puts ("<recdata>");

  for (i = 0; i < rec_db_size (db); i++)
    {
      rset = rec_db_get_rset (db, i);

      if (rec2xml_record_type)
        {
          if (!rec_rset_type (rset)
              || strcmp (rec_rset_type (rset), rec2xml_record_type) != 0)
            continue;
        }

      if (!rec_rset_sort (rset, rec2xml_sort_by_fields))
        recutl_out_of_memory ();

      if (rec_rset_type (rset))
        {
          printf ("  <recordset type=\"");
          rec2xml_write_escaped (rec_rset_type (rset));
          puts ("\">");
          rec2xml_generate_rset (rset, "    ");
          puts ("  </recordset>");
        }
      else
        {
          puts ("  <recordset>");
          rec2xml_generate_rset (rset, "    ");
          puts ("  </recordset>");
        }
    }

  puts ("</recdata>");
  return ret;
}

int
main (int argc, char *argv[])
{
  int res = 0;
  rec_db_t db;

  recutl_init ("rec2xml");

  rec2xml_parse_args (argc, argv);

  db = recutl_build_db (argc, argv);
  if (!db)
    res = 1;
  else
    {
      if (!rec2xml_process_data (db))
        res = 1;
    }

  rec_db_destroy (db);

  return res;
}
