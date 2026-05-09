/* rec2sql.c - rec to SQL converter.  */

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

char *rec2sql_record_type = NULL;
rec_fex_t rec2sql_sort_by_fields = NULL;
char *rec2sql_table_name = NULL;
bool rec2sql_create_table = false;
const char *rec2sql_multi_separator = "\n";

/*
 * Command line options management
 */

enum
  {
    COMMON_ARGS,
    RECORD_TYPE_ARG,
    SORT_ARG,
    TABLE_NAME_ARG,
    CREATE_TABLE_ARG,
    SEPARATOR_ARG
  };

static const struct option GNU_longOptions[] =
  {
    COMMON_LONG_ARGS,
    {"type", required_argument, NULL, RECORD_TYPE_ARG},
    {"sort", required_argument, NULL, SORT_ARG},
    {"table", required_argument, NULL, TABLE_NAME_ARG},
    {"create-table", no_argument, NULL, CREATE_TABLE_ARG},
    {"separator", required_argument, NULL, SEPARATOR_ARG},
    {NULL, 0, NULL, 0}
  };

/*
 * Functions.
 */

void
recutl_print_help (void)
{
  printf (_("\
Usage: rec2sql [OPTIONS]... [REC_FILE]\n"));

  fputs (_("\
Convert rec data into SQL INSERT statements.\n"), stdout);

  puts ("");
  fputs (_("\
  -t, --type=TYPE                     record set to convert; if this parameter\n\
                                        is omitted then the default record set is used\n\
  -T, --table=NAME                    use NAME as the SQL table name instead of\n\
                                        the record type\n\
  -S, --sort=FIELDS                   sort the output by the specified fields.\n\
  -c, --create-table                  emit CREATE TABLE statements before INSERTs.\n\
      --separator=STR                 string used to join multi-valued fields in SQL\n\
                                        values (default is a newline character).\n"),
         stdout);

  recutl_print_help_common ();
  puts ("");
  recutl_print_help_footer ();
}

static void
rec2sql_parse_args (int argc, char **argv)
{
  int ret;
  char c;

  while ((ret = getopt_long (argc,
                             argv,
                             "t:S:T:c",
                             GNU_longOptions,
                             NULL)) != -1)
    {
      c = ret;
      switch (c)
        {
          COMMON_ARGS_CASES
        case RECORD_TYPE_ARG:
        case 't':
          rec2sql_record_type = xstrdup (optarg);
          break;
        case SORT_ARG:
        case 'S':
          if (rec2sql_sort_by_fields)
            recutl_fatal (_("only one list of fields can be specified\
 as a sorting criteria.\n"));

          if (!rec_fex_check (optarg, REC_FEX_CSV))
            recutl_fatal (_("invalid field name list in -S.\n"));

          rec2sql_sort_by_fields = rec_fex_new (optarg, REC_FEX_CSV);
          if (!rec2sql_sort_by_fields)
            recutl_fatal (_("internal error creating fex.\n"));

          break;
        case TABLE_NAME_ARG:
        case 'T':
          rec2sql_table_name = xstrdup (optarg);
          break;
        case CREATE_TABLE_ARG:
        case 'c':
          rec2sql_create_table = true;
          break;
        case SEPARATOR_ARG:
          rec2sql_multi_separator = optarg;
          break;
        default:
          exit (EXIT_FAILURE);
        }
    }
}

static void
rec2sql_write_sql_str (const char *str)
{
  const char *p;

  putchar ('\'');
  for (p = str; *p; p++)
    {
      if (*p == '\'')
        fputs ("''", stdout);
      else
        putchar (*p);
    }
  putchar ('\'');
}

static void
rec2sql_write_identifier (const char *str)
{
  const char *p;

  putchar ('"');
  for (p = str; *p; p++)
    {
      if (*p == '"')
        fputs ("\"\"", stdout);
      else
        putchar (*p);
    }
  putchar ('"');
}

static void
rec2sql_generate_record (rec_record_t record, const char *table)
{
  rec_mset_iterator_t iter;
  rec_field_t field;
  bool first;

  fputs ("INSERT INTO ", stdout);
  rec2sql_write_identifier (table);
  fputs (" (", stdout);

  /* Columns.  Skip duplicate field names (multi-valued fields get
     concatenated with newlines into one SQL value).  */
  first = true;
  iter = rec_mset_iterator (rec_record_mset (record));
  while (rec_mset_iterator_next (&iter, MSET_FIELD,
                                 (const void **) &field, NULL))
    {
      const char *name = rec_field_name (field);
      size_t idx = rec_record_get_field_index_by_name (record, field);

      if (idx != 0)
        continue;

      if (!first)
        fputs (", ", stdout);
      first = false;

      rec2sql_write_identifier (name);
    }
  rec_mset_iterator_free (&iter);

  fputs (") VALUES (", stdout);

  /* Values.  For multi-valued fields, concatenate all values with
     newline separators.  */
  first = true;
  iter = rec_mset_iterator (rec_record_mset (record));
  while (rec_mset_iterator_next (&iter, MSET_FIELD,
                                 (const void **) &field, NULL))
    {
      const char *name = rec_field_name (field);
      size_t idx = rec_record_get_field_index_by_name (record, field);

      if (idx != 0)
        continue;

      if (!first)
        fputs (", ", stdout);
      first = false;

      size_t count = rec_record_get_num_fields_by_name (record, name);
      if (count == 1)
        {
          rec2sql_write_sql_str (rec_field_value (field));
        }
      else
        {
          putchar ('\'');
          for (size_t i = 0; i < count; i++)
            {
              rec_field_t f = rec_record_get_field_by_name (record,
                                                            name, i);
              const char *val = rec_field_value (f);
              const char *p;

              if (i > 0)
                {
                  for (p = rec2sql_multi_separator; *p; p++)
                    {
                      if (*p == '\'')
                        fputs ("''", stdout);
                      else
                        putchar (*p);
                    }
                }
              for (p = val; *p; p++)
                {
                  if (*p == '\'')
                    fputs ("''", stdout);
                  else
                    putchar (*p);
                }
            }
          putchar ('\'');
        }
    }
  rec_mset_iterator_free (&iter);

  fputs (");\n", stdout);
}

static void
rec2sql_generate_rset (rec_rset_t rset, const char *table)
{
  rec_mset_iterator_t iter;
  rec_record_t record;

  iter = rec_mset_iterator (rec_rset_mset (rset));
  while (rec_mset_iterator_next (&iter, MSET_RECORD,
                                 (const void **) &record, NULL))
    rec2sql_generate_record (record, table);

  rec_mset_iterator_free (&iter);
}

static const char *
rec2sql_sql_type_for (rec_rset_t rset, const char *field_name)
{
  rec_type_t type = rec_rset_get_field_type (rset, field_name);
  if (!type)
    return "TEXT";

  switch (rec_type_kind (type))
    {
    case REC_TYPE_INT:
    case REC_TYPE_RANGE:
    case REC_TYPE_SIZE:
      return "INTEGER";
    case REC_TYPE_REAL:
      return "REAL";
    case REC_TYPE_BOOL:
      return "BOOLEAN";
    case REC_TYPE_DATE:
      return "TIMESTAMP";
    default:
      return "TEXT";
    }
}

static void
rec2sql_generate_create_table (rec_rset_t rset, const char *table)
{
  rec_mset_iterator_t iter_rset;
  rec_mset_iterator_t iter_record;
  rec_record_t record;
  rec_field_t field;
  rec_fex_t columns;
  size_t i;

  columns = rec_fex_new (NULL, REC_FEX_SIMPLE);

  iter_rset = rec_mset_iterator (rec_rset_mset (rset));
  while (rec_mset_iterator_next (&iter_rset, MSET_RECORD,
                                 (const void **) &record, NULL))
    {
      iter_record = rec_mset_iterator (rec_record_mset (record));
      while (rec_mset_iterator_next (&iter_record, MSET_FIELD,
                                     (const void **) &field, NULL))
        {
          if (!rec_fex_member_p (columns, rec_field_name (field), 0, 0))
            rec_fex_append (columns, rec_field_name (field), 0, 0);
        }
      rec_mset_iterator_free (&iter_record);
    }
  rec_mset_iterator_free (&iter_rset);

  if (rec_fex_size (columns) == 0)
    {
      rec_fex_destroy (columns);
      return;
    }

  fputs ("CREATE TABLE IF NOT EXISTS ", stdout);
  rec2sql_write_identifier (table);
  fputs (" (\n", stdout);

  for (i = 0; i < rec_fex_size (columns); i++)
    {
      rec_fex_elem_t elem = rec_fex_get (columns, i);
      const char *name = rec_fex_elem_field_name (elem);

      if (i > 0)
        fputs (",\n", stdout);
      fputs ("  ", stdout);
      rec2sql_write_identifier (name);
      putchar (' ');
      fputs (rec2sql_sql_type_for (rset, name), stdout);
    }

  fputs ("\n);\n\n", stdout);
  rec_fex_destroy (columns);
}

static bool
rec2sql_process_data (rec_db_t db)
{
  bool ret = true;
  size_t i;
  rec_rset_t rset;

  for (i = 0; i < rec_db_size (db); i++)
    {
      rset = rec_db_get_rset (db, i);

      if (rec2sql_record_type)
        {
          if (!rec_rset_type (rset)
              || strcmp (rec_rset_type (rset), rec2sql_record_type) != 0)
            continue;
        }

      if (!rec_rset_sort (rset, rec2sql_sort_by_fields))
        recutl_out_of_memory ();

      const char *table;
      if (rec2sql_table_name)
        table = rec2sql_table_name;
      else if (rec_rset_type (rset))
        table = rec_rset_type (rset);
      else
        table = "data";

      if (rec2sql_create_table)
        rec2sql_generate_create_table (rset, table);

      rec2sql_generate_rset (rset, table);
    }

  return ret;
}

int
main (int argc, char *argv[])
{
  int res = 0;
  rec_db_t db;

  recutl_init ("rec2sql");

  rec2sql_parse_args (argc, argv);

  db = recutl_build_db (argc, argv);
  if (!db)
    res = 1;
  else
    {
      if (!rec2sql_process_data (db))
        res = 1;
    }

  rec_db_destroy (db);

  return res;
}
