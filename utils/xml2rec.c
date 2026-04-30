/* xml2rec.c - XML to rec converter.  */

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
#include <stdio.h>
#include <ctype.h>
#include <xalloc.h>
#include <gettext.h>
#define _(str) gettext (str)

#include <rec.h>
#include <recutl.h>

/*
 * Global variables
 */

char *xml2rec_record_type = NULL;
char *xml2rec_xml_file = NULL;

/*
 * Command line options management
 */

enum
  {
    COMMON_ARGS,
    RECORD_TYPE_ARG
  };

static const struct option GNU_longOptions[] =
  {
    COMMON_LONG_ARGS,
    {"type", required_argument, NULL, RECORD_TYPE_ARG},
    {NULL, 0, NULL, 0}
  };

/*
 * Simple XML parser for the recdata format produced by rec2xml.
 *
 * Expected structure:
 *   <?xml ...?>
 *   <recdata>
 *     <recordset [type="..."]>
 *       <record>
 *         <field name="...">value</field>
 *       </record>
 *     </recordset>
 *   </recdata>
 */

static const char *xp;

static void
xml_skip_ws (void)
{
  while (*xp == ' ' || *xp == '\t' || *xp == '\n' || *xp == '\r')
    xp++;
}

static bool
xml_match (const char *str)
{
  size_t len = strlen (str);
  if (strncmp (xp, str, len) == 0)
    {
      xp += len;
      return true;
    }
  return false;
}

static void
xml_expect (const char *str)
{
  if (!xml_match (str))
    recutl_fatal (_("expected '%s' in XML input\n"), str);
}

/* Unescape XML entities in-place and return the string.  */

static char *
xml_unescape (const char *src, size_t len)
{
  char *buf = xmalloc (len + 1);
  size_t j = 0;
  size_t i;

  for (i = 0; i < len; i++)
    {
      if (src[i] == '&')
        {
          if (strncmp (src + i, "&amp;", 5) == 0)
            { buf[j++] = '&'; i += 4; }
          else if (strncmp (src + i, "&lt;", 4) == 0)
            { buf[j++] = '<'; i += 3; }
          else if (strncmp (src + i, "&gt;", 4) == 0)
            { buf[j++] = '>'; i += 3; }
          else if (strncmp (src + i, "&quot;", 6) == 0)
            { buf[j++] = '"'; i += 5; }
          else if (strncmp (src + i, "&apos;", 6) == 0)
            { buf[j++] = '\''; i += 5; }
          else
            buf[j++] = src[i];
        }
      else
        buf[j++] = src[i];
    }

  buf[j] = '\0';
  return buf;
}

/* Parse a quoted attribute value and return the unescaped content.  */

static char *
xml_parse_attr_value (void)
{
  const char *start;
  const char *end;

  if (*xp != '"')
    recutl_fatal (_("expected '\"' in XML attribute\n"));
  xp++;

  start = xp;
  end = strchr (xp, '"');
  if (!end)
    recutl_fatal (_("unterminated attribute value in XML\n"));

  xp = end + 1;
  return xml_unescape (start, end - start);
}

/* Parse text content up to '<' and return the unescaped content.  */

static char *
xml_parse_text (void)
{
  const char *start = xp;

  while (*xp && *xp != '<')
    xp++;

  return xml_unescape (start, xp - start);
}

/* Skip the XML declaration (<?xml ...?>).  */

static void
xml_skip_declaration (void)
{
  xml_skip_ws ();
  if (strncmp (xp, "<?", 2) == 0)
    {
      const char *end = strstr (xp, "?>");
      if (!end)
        recutl_fatal (_("unterminated XML declaration\n"));
      xp = end + 2;
    }
}

/* Skip any <!DOCTYPE ...> declaration.  */

static void
xml_skip_doctype (void)
{
  xml_skip_ws ();
  if (strncmp (xp, "<!DOCTYPE", 9) == 0)
    {
      const char *end = strchr (xp, '>');
      if (!end)
        recutl_fatal (_("unterminated DOCTYPE declaration\n"));
      xp = end + 1;
    }
}

/* Skip XML comments <!-- ... -->.  */

static void
xml_skip_comments (void)
{
  while (true)
    {
      xml_skip_ws ();
      if (strncmp (xp, "<!--", 4) != 0)
        break;
      const char *end = strstr (xp, "-->");
      if (!end)
        recutl_fatal (_("unterminated XML comment\n"));
      xp = end + 3;
    }
}

/* Parse a <field name="...">value</field> element.  */

static rec_field_t
xml_parse_field (void)
{
  char *name;
  char *value;
  rec_field_t field;

  xml_skip_ws ();
  xml_skip_comments ();
  xml_expect ("<field");

  xml_skip_ws ();
  xml_expect ("name=");
  name = xml_parse_attr_value ();

  xml_skip_ws ();
  xml_expect (">");

  value = xml_parse_text ();

  xml_expect ("</field>");

  if (!rec_field_name_p (name))
    {
      recutl_warning (_("skipping invalid field name '%s'\n"), name);
      free (name);
      free (value);
      return NULL;
    }

  field = rec_field_new (name, value);
  free (name);
  free (value);

  return field;
}

/* Parse a <record>...</record> element.  */

static rec_record_t
xml_parse_record (void)
{
  rec_record_t record;

  xml_skip_ws ();
  xml_skip_comments ();
  xml_expect ("<record>");

  record = rec_record_new ();
  if (!record)
    recutl_out_of_memory ();

  while (true)
    {
      xml_skip_ws ();
      xml_skip_comments ();
      if (strncmp (xp, "</record>", 9) == 0)
        break;
      if (strncmp (xp, "<field", 6) != 0)
        recutl_fatal (_("expected <field> or </record> in XML input\n"));

      rec_field_t field = xml_parse_field ();
      if (field)
        rec_mset_append (rec_record_mset (record), MSET_FIELD,
                         (void *) field, MSET_ANY);
    }

  xml_expect ("</record>");
  return record;
}

/* Parse a <recordset [type="..."]>...</recordset> element.  */

static rec_rset_t
xml_parse_recordset (void)
{
  rec_rset_t rset;
  char *type = NULL;

  xml_skip_ws ();
  xml_skip_comments ();
  xml_expect ("<recordset");

  xml_skip_ws ();
  if (strncmp (xp, "type=", 5) == 0)
    {
      xp += 5;
      type = xml_parse_attr_value ();
    }

  xml_skip_ws ();
  xml_expect (">");

  rset = rec_rset_new ();
  if (!rset)
    recutl_out_of_memory ();

  if (xml2rec_record_type)
    rec_rset_set_type (rset, xml2rec_record_type);
  else if (type && strlen (type) > 0)
    rec_rset_set_type (rset, type);

  while (true)
    {
      xml_skip_ws ();
      xml_skip_comments ();
      if (strncmp (xp, "</recordset>", 12) == 0)
        break;
      if (strncmp (xp, "<record>", 8) != 0)
        recutl_fatal (_("expected <record> or </recordset> in XML input\n"));

      rec_record_t record = xml_parse_record ();
      rec_mset_append (rec_rset_mset (rset), MSET_RECORD,
                       (void *) record, MSET_ANY);
    }

  xml_expect ("</recordset>");
  free (type);
  return rset;
}

/*
 * Functions.
 */

void
recutl_print_help (void)
{
  printf (_("\
Usage: xml2rec [OPTIONS]... [XML_FILE]\n"));

  fputs (_("\
Convert XML data (in recdata format) into rec data.\n"), stdout);

  puts ("");
  fputs (_("\
  -t, --type=TYPE                     type name for the converted records; if this\n\
                                        parameter is omitted then the type is inferred\n\
                                        from the XML structure.\n"),
         stdout);

  recutl_print_help_common ();
  puts ("");
  recutl_print_help_footer ();
}

static void
xml2rec_parse_args (int argc, char **argv)
{
  int ret;
  char c;

  while ((ret = getopt_long (argc,
                             argv,
                             "t:",
                             GNU_longOptions,
                             NULL)) != -1)
    {
      c = ret;
      switch (c)
        {
          COMMON_ARGS_CASES
        case RECORD_TYPE_ARG:
        case 't':
          xml2rec_record_type = xstrdup (optarg);
          break;
        default:
          exit (EXIT_FAILURE);
        }
    }

  if (optind < argc)
    {
      if ((argc - optind) != 1)
        {
          recutl_print_help ();
          exit (EXIT_FAILURE);
        }

      xml2rec_xml_file = argv[optind++];
    }
}

static char *
xml2rec_read_input (void)
{
  FILE *in;
  size_t cap = 4096;
  size_t len = 0;
  char *buf;
  size_t n;

  if (xml2rec_xml_file)
    {
      if (!(in = fopen (xml2rec_xml_file, "r")))
        recutl_fatal (_("cannot read file %s\n"), xml2rec_xml_file);
    }
  else
    in = stdin;

  buf = xmalloc (cap);
  while ((n = fread (buf + len, 1, cap - len - 1, in)) > 0)
    {
      len += n;
      if (len + 1 >= cap)
        {
          cap *= 2;
          buf = xrealloc (buf, cap);
        }
    }
  buf[len] = '\0';

  if (xml2rec_xml_file)
    fclose (in);

  return buf;
}

static rec_db_t
xml2rec_process_input (void)
{
  char *input;
  rec_db_t db;

  input = xml2rec_read_input ();
  xp = input;

  xml_skip_ws ();
  if (*xp == '\0')
    {
      free (input);
      return NULL;
    }

  xml_skip_declaration ();
  xml_skip_doctype ();
  xml_skip_ws ();
  xml_skip_comments ();

  xml_expect ("<recdata>");

  db = rec_db_new ();
  if (!db)
    recutl_out_of_memory ();

  while (true)
    {
      xml_skip_ws ();
      xml_skip_comments ();
      if (strncmp (xp, "</recdata>", 10) == 0)
        break;
      if (strncmp (xp, "<recordset", 10) != 0)
        recutl_fatal (_("expected <recordset> or </recdata> in XML input\n"));

      rec_rset_t rset = xml_parse_recordset ();
      rec_db_insert_rset (db, rset, rec_db_size (db));
    }

  xml_expect ("</recdata>");
  free (input);

  return db;
}

int
main (int argc, char *argv[])
{
  int ret;
  rec_db_t db;
  rec_writer_t writer;

  recutl_init ("xml2rec");

  xml2rec_parse_args (argc, argv);
  db = xml2rec_process_input ();
  ret = EXIT_SUCCESS;

  if (db)
    {
      writer = rec_writer_new (stdout);
      rec_write_db (writer, db);

      rec_writer_destroy (writer);
      rec_db_destroy (db);
    }

  return ret;
}
