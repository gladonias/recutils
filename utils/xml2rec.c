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
static const char *xp_start;

static void
xml_get_position (size_t *line, size_t *col)
{
  const char *p;
  *line = 1;
  *col = 1;

  for (p = xp_start; p < xp; p++)
    {
      if (*p == '\n')
        {
          (*line)++;
          *col = 1;
        }
      else
        (*col)++;
    }
}

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
    {
      size_t line, col;
      xml_get_position (&line, &col);
      recutl_fatal (_("expected '%s' in XML input at line %zu, column %zu\n"),
                    str, line, col);
    }
}

/* Encode a Unicode codepoint as UTF-8 into buf at offset *j, growing
   the offset.  Caller must ensure at least 4 bytes of headroom.  */

static void
xml_emit_utf8 (char *buf, size_t *j, unsigned int cp)
{
  if (cp < 0x80)
    buf[(*j)++] = (char) cp;
  else if (cp < 0x800)
    {
      buf[(*j)++] = (char) (0xC0 | (cp >> 6));
      buf[(*j)++] = (char) (0x80 | (cp & 0x3F));
    }
  else if (cp < 0x10000)
    {
      buf[(*j)++] = (char) (0xE0 | (cp >> 12));
      buf[(*j)++] = (char) (0x80 | ((cp >> 6) & 0x3F));
      buf[(*j)++] = (char) (0x80 | (cp & 0x3F));
    }
  else
    {
      buf[(*j)++] = (char) (0xF0 | (cp >> 18));
      buf[(*j)++] = (char) (0x80 | ((cp >> 12) & 0x3F));
      buf[(*j)++] = (char) (0x80 | ((cp >> 6) & 0x3F));
      buf[(*j)++] = (char) (0x80 | (cp & 0x3F));
    }
}

/* Unescape XML entities (named and numeric) and return a new string.  */

static char *
xml_unescape (const char *src, size_t len)
{
  /* Worst case: numeric refs of the form "&#1114111;" (10 bytes)
     decode to up to 4 UTF-8 bytes — output is bounded by the input
     length.  */
  char *buf = xmalloc (len + 1);
  size_t j = 0;
  size_t i;

  for (i = 0; i < len; i++)
    {
      if (src[i] == '&')
        {
          /* Named entities.  */
          if (i + 5 <= len && strncmp (src + i, "&amp;", 5) == 0)
            { buf[j++] = '&'; i += 4; }
          else if (i + 4 <= len && strncmp (src + i, "&lt;", 4) == 0)
            { buf[j++] = '<'; i += 3; }
          else if (i + 4 <= len && strncmp (src + i, "&gt;", 4) == 0)
            { buf[j++] = '>'; i += 3; }
          else if (i + 6 <= len && strncmp (src + i, "&quot;", 6) == 0)
            { buf[j++] = '"'; i += 5; }
          else if (i + 6 <= len && strncmp (src + i, "&apos;", 6) == 0)
            { buf[j++] = '\''; i += 5; }
          else if (i + 3 <= len && src[i + 1] == '#')
            {
              /* Numeric character reference: &#NNN; or &#xHH;  */
              size_t k = i + 2;
              unsigned int cp = 0;
              bool hex = false;

              if (k < len && (src[k] == 'x' || src[k] == 'X'))
                {
                  hex = true;
                  k++;
                }

              size_t digit_start = k;
              while (k < len && src[k] != ';')
                {
                  unsigned int d;
                  if (src[k] >= '0' && src[k] <= '9')
                    d = src[k] - '0';
                  else if (hex && src[k] >= 'a' && src[k] <= 'f')
                    d = src[k] - 'a' + 10;
                  else if (hex && src[k] >= 'A' && src[k] <= 'F')
                    d = src[k] - 'A' + 10;
                  else
                    {
                      cp = 0;
                      break;
                    }
                  cp = cp * (hex ? 16 : 10) + d;
                  if (cp > 0x10FFFF)
                    {
                      cp = 0;
                      break;
                    }
                  k++;
                }

              if (k < len && src[k] == ';' && k > digit_start && cp != 0)
                {
                  xml_emit_utf8 (buf, &j, cp);
                  i = k;
                }
              else
                buf[j++] = src[i];
            }
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
    {
      size_t line, col;
      xml_get_position (&line, &col);
      recutl_fatal (_("expected '\"' in XML attribute at line %zu, column %zu\n"),
                    line, col);
    }
  xp++;

  start = xp;
  end = strchr (xp, '"');
  if (!end)
    {
      size_t line, col;
      xml_get_position (&line, &col);
      recutl_fatal (_("unterminated attribute value in XML at line %zu, column %zu\n"),
                    line, col);
    }

  xp = end + 1;
  return xml_unescape (start, end - start);
}

/* Parse text content up to (but not including) the next '</' end-tag,
   honoring CDATA sections.  Returns the unescaped content.  */

static char *
xml_parse_text (void)
{
  size_t cap = 64;
  size_t len = 0;
  char *buf = xmalloc (cap);

  while (*xp)
    {
      if (strncmp (xp, "<![CDATA[", 9) == 0)
        {
          const char *cd_start = xp + 9;
          const char *cd_end = strstr (cd_start, "]]>");
          size_t cd_len;

          if (!cd_end)
            {
              size_t line, col;
              xml_get_position (&line, &col);
              free (buf);
              recutl_fatal (_("unterminated CDATA section in XML at line %zu, column %zu\n"),
                            line, col);
            }

          cd_len = cd_end - cd_start;
          while (len + cd_len + 1 > cap)
            cap *= 2;
          buf = xrealloc (buf, cap);
          memcpy (buf + len, cd_start, cd_len);
          len += cd_len;
          xp = cd_end + 3;
          continue;
        }

      if (*xp == '<')
        break;

      /* Append a chunk of plain text up to the next special character.  */
      const char *chunk_start = xp;
      while (*xp && *xp != '<' && *xp != '&')
        xp++;
      size_t chunk_len = xp - chunk_start;

      if (chunk_len > 0)
        {
          while (len + chunk_len + 1 > cap)
            cap *= 2;
          buf = xrealloc (buf, cap);
          memcpy (buf + len, chunk_start, chunk_len);
          len += chunk_len;
        }

      if (*xp == '&')
        {
          /* Decode a single entity via xml_unescape over the entity span.  */
          const char *e_start = xp;
          xp++;
          while (*xp && *xp != ';' && *xp != '<' && *xp != '&')
            xp++;
          size_t e_len = (*xp == ';' ? (xp - e_start + 1) : (xp - e_start));
          if (*xp == ';')
            xp++;
          char *decoded = xml_unescape (e_start, e_len);
          size_t d_len = strlen (decoded);
          while (len + d_len + 1 > cap)
            cap *= 2;
          buf = xrealloc (buf, cap);
          memcpy (buf + len, decoded, d_len);
          len += d_len;
          free (decoded);
        }
    }

  buf[len] = '\0';
  return buf;
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
        {
          size_t line, col;
          xml_get_position (&line, &col);
          recutl_fatal (_("unterminated XML declaration at line %zu, column %zu\n"),
                        line, col);
        }
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
        {
          size_t line, col;
          xml_get_position (&line, &col);
          recutl_fatal (_("unterminated DOCTYPE declaration at line %zu, column %zu\n"),
                        line, col);
        }
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
        {
          size_t line, col;
          xml_get_position (&line, &col);
          recutl_fatal (_("unterminated XML comment at line %zu, column %zu\n"),
                        line, col);
        }
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

  if (xml_match ("/>"))
    {
      /* Self-closing <field name="..."/> with empty value.  */
      value = xstrdup ("");
    }
  else
    {
      xml_expect (">");
      value = xml_parse_text ();
      xml_expect ("</field>");
    }

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
        {
          size_t line, col;
          xml_get_position (&line, &col);
          recutl_fatal (_("expected <field> or </record> in XML input at line %zu, column %zu\n"),
                        line, col);
        }

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
        {
          size_t line, col;
          xml_get_position (&line, &col);
          recutl_fatal (_("expected <record> or </recordset> in XML input at line %zu, column %zu\n"),
                        line, col);
        }

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
  xp_start = input;

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
        {
          size_t line, col;
          xml_get_position (&line, &col);
          recutl_fatal (_("expected <recordset> or </recdata> in XML input at line %zu, column %zu\n"),
                        line, col);
        }

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
