/* rec2json.c - rec to JSON converter.  */

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

char *rec2json_record_type = NULL;
rec_fex_t rec2json_sort_by_fields = NULL;

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
Usage: rec2json [OPTIONS]... [REC_FILE]\n"));

  fputs (_("\
Convert rec data into JSON data.\n"), stdout);

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
rec2json_parse_args (int argc, char **argv)
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
          rec2json_record_type = xstrdup (optarg);
          break;
        case SORT_ARG:
        case 'S':
          if (rec2json_sort_by_fields)
            recutl_fatal (_("only one list of fields can be specified\
 as a sorting criteria.\n"));

          if (!rec_fex_check (optarg, REC_FEX_CSV))
            recutl_fatal (_("invalid field name list in -S.\n"));

          rec2json_sort_by_fields = rec_fex_new (optarg, REC_FEX_CSV);
          if (!rec2json_sort_by_fields)
            recutl_fatal (_("internal error creating fex.\n"));

          break;
        default:
          exit (EXIT_FAILURE);
        }
    }
}

static void
rec2json_write_str (const char *str)
{
  const unsigned char *p;

  putchar ('"');
  for (p = (const unsigned char *) str; *p; p++)
    {
      switch (*p)
        {
        case '"':  fputs ("\\\"", stdout); break;
        case '\\': fputs ("\\\\", stdout); break;
        case '\b': fputs ("\\b", stdout); break;
        case '\f': fputs ("\\f", stdout); break;
        case '\n': fputs ("\\n", stdout); break;
        case '\r': fputs ("\\r", stdout); break;
        case '\t': fputs ("\\t", stdout); break;
        default:
          if (*p < 0x20)
            printf ("\\u%04x", *p);
          else
            putchar (*p);
          break;
        }
    }
  putchar ('"');
}

static void
rec2json_generate_record (rec_record_t record)
{
  rec_mset_iterator_t iter;
  rec_field_t field;
  const char *prev_name = NULL;
  int array_depth = 0;
  bool first_field = true;

  putchar ('{');

  iter = rec_mset_iterator (rec_record_mset (record));
  while (rec_mset_iterator_next (&iter, MSET_FIELD,
                                 (const void **) &field, NULL))
    {
      const char *name = rec_field_name (field);
      const char *value = rec_field_value (field);

      if (prev_name && rec_field_name_equal_p (prev_name, name))
        {
          /* Same field name repeated — continue the JSON array.  */
          fputs (", ", stdout);
          rec2json_write_str (value);
        }
      else
        {
          /* Close the previous array if one was open.  */
          if (array_depth > 0)
            {
              putchar (']');
              array_depth = 0;
            }

          if (!first_field)
            fputs (", ", stdout);
          first_field = false;

          rec2json_write_str (name);
          fputs (": ", stdout);

          /* Check if the next field has the same name (look ahead by
             counting occurrences).  If so, start a JSON array.  */
          if (rec_record_get_num_fields_by_name (record, name) > 1)
            {
              putchar ('[');
              array_depth = 1;
            }

          rec2json_write_str (value);
        }

      prev_name = name;
    }

  if (array_depth > 0)
    putchar (']');

  rec_mset_iterator_free (&iter);

  putchar ('}');
}

static void
rec2json_generate_rset (rec_rset_t rset)
{
  rec_mset_iterator_t iter;
  rec_record_t record;
  bool first = true;

  putchar ('[');

  iter = rec_mset_iterator (rec_rset_mset (rset));
  while (rec_mset_iterator_next (&iter, MSET_RECORD,
                                 (const void **) &record, NULL))
    {
      if (!first)
        fputs (", ", stdout);
      first = false;

      rec2json_generate_record (record);
    }

  rec_mset_iterator_free (&iter);

  putchar (']');
}

static bool
rec2json_process_data (rec_db_t db)
{
  bool ret = true;
  size_t i;
  rec_rset_t rset;
  bool multi = false;
  bool first = true;

  /* Determine whether we are outputting multiple record sets.  */
  if (!rec2json_record_type && rec_db_size (db) > 1)
    {
      size_t typed_count = 0;
      for (i = 0; i < rec_db_size (db); i++)
        if (rec_rset_type (rec_db_get_rset (db, i)))
          typed_count++;
      multi = (typed_count > 1)
        || (typed_count == 1 && rec_db_size (db) > 1);
    }

  if (rec_db_size (db) == 0)
    {
      fputs ("[]", stdout);
    }
  else
    {
      if (multi)
        putchar ('{');

      for (i = 0; i < rec_db_size (db); i++)
        {
          rset = rec_db_get_rset (db, i);

          if (rec2json_record_type)
            {
              if (!rec_rset_type (rset)
                  || strcmp (rec_rset_type (rset), rec2json_record_type) != 0)
                continue;
            }

          if (!rec_rset_sort (rset, rec2json_sort_by_fields))
            recutl_out_of_memory ();

          if (multi)
            {
              if (!first)
                fputs (", ", stdout);
              first = false;

              if (rec_rset_type (rset))
                rec2json_write_str (rec_rset_type (rset));
              else
                fputs ("\"\"", stdout);

              fputs (": ", stdout);
            }

          rec2json_generate_rset (rset);
        }

      if (multi)
        putchar ('}');
    }

  putchar ('\n');
  return ret;
}

int
main (int argc, char *argv[])
{
  int res = 0;
  rec_db_t db;

  recutl_init ("rec2json");

  rec2json_parse_args (argc, argv);

  db = recutl_build_db (argc, argv);
  if (!db)
    res = 1;
  else
    {
      if (!rec2json_process_data (db))
        res = 1;
    }

  rec_db_destroy (db);

  return res;
}
