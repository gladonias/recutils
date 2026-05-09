/* json2rec.c - JSON to rec converter.  */

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

char *json2rec_record_type = NULL;
char *json2rec_json_file = NULL;

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
 * Simple recursive-descent JSON parser.
 */

enum json_type
  {
    JSON_STRING,
    JSON_NUMBER,
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_BOOL,
    JSON_NULL
  };

struct json_pair;

struct json_value
{
  enum json_type type;
  union
  {
    char *string;
    /* Numbers are kept as their original textual representation so we
       round-trip exactly (preserves precision for integers above 2^53,
       trailing zeros, exponent form, etc.).  */
    char *number;
    bool boolean;
    struct
    {
      struct json_pair *pairs;
      size_t count;
    } object;
    struct
    {
      struct json_value *elements;
      size_t count;
    } array;
  } u;
};

struct json_pair
{
  char *key;
  struct json_value value;
};

static const char *jp;
static const char *jp_start;

static void json_value_free (struct json_value *v);

static void
json_get_position (size_t *line, size_t *col)
{
  const char *p;
  *line = 1;
  *col = 1;

  for (p = jp_start; p < jp; p++)
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
json_skip_ws (void)
{
  while (*jp == ' ' || *jp == '\t' || *jp == '\n' || *jp == '\r')
    jp++;
}

static char *
json_parse_string_value (void)
{
  size_t cap = 64;
  size_t len = 0;
  char *buf = xmalloc (cap);

  if (*jp != '"')
    {
      free (buf);
      return NULL;
    }
  jp++;

  while (*jp && *jp != '"')
    {
      if (len + 6 >= cap)
        {
          cap *= 2;
          buf = xrealloc (buf, cap);
        }

      if (*jp == '\\')
        {
          jp++;
          switch (*jp)
            {
            case '"':  buf[len++] = '"';  break;
            case '\\': buf[len++] = '\\'; break;
            case '/':  buf[len++] = '/';  break;
            case 'b':  buf[len++] = '\b'; break;
            case 'f':  buf[len++] = '\f'; break;
            case 'n':  buf[len++] = '\n'; break;
            case 'r':  buf[len++] = '\r'; break;
            case 't':  buf[len++] = '\t'; break;
            case 'u':
              {
                unsigned int cp = 0;
                int i;

                jp++;
                for (i = 0; i < 4 && *jp; i++, jp++)
                  {
                    cp <<= 4;
                    if (*jp >= '0' && *jp <= '9')
                      cp |= *jp - '0';
                    else if (*jp >= 'a' && *jp <= 'f')
                      cp |= *jp - 'a' + 10;
                    else if (*jp >= 'A' && *jp <= 'F')
                      cp |= *jp - 'A' + 10;
                    else
                      {
                        free (buf);
                        return NULL;
                      }
                  }

                /* Handle UTF-16 surrogate pairs.  */
                if (cp >= 0xD800 && cp <= 0xDBFF)
                  {
                    unsigned int lo = 0;

                    if (jp[0] != '\\' || jp[1] != 'u')
                      {
                        free (buf);
                        return NULL;
                      }
                    jp += 2;

                    for (i = 0; i < 4 && *jp; i++, jp++)
                      {
                        lo <<= 4;
                        if (*jp >= '0' && *jp <= '9')
                          lo |= *jp - '0';
                        else if (*jp >= 'a' && *jp <= 'f')
                          lo |= *jp - 'a' + 10;
                        else if (*jp >= 'A' && *jp <= 'F')
                          lo |= *jp - 'A' + 10;
                        else
                          {
                            free (buf);
                            return NULL;
                          }
                      }

                    if (lo < 0xDC00 || lo > 0xDFFF)
                      {
                        free (buf);
                        return NULL;
                      }

                    cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                  }
                else if (cp >= 0xDC00 && cp <= 0xDFFF)
                  {
                    free (buf);
                    return NULL;
                  }

                /* Encode as UTF-8.  */
                if (cp < 0x80)
                  buf[len++] = (char) cp;
                else if (cp < 0x800)
                  {
                    buf[len++] = (char) (0xC0 | (cp >> 6));
                    buf[len++] = (char) (0x80 | (cp & 0x3F));
                  }
                else if (cp < 0x10000)
                  {
                    buf[len++] = (char) (0xE0 | (cp >> 12));
                    buf[len++] = (char) (0x80 | ((cp >> 6) & 0x3F));
                    buf[len++] = (char) (0x80 | (cp & 0x3F));
                  }
                else
                  {
                    buf[len++] = (char) (0xF0 | (cp >> 18));
                    buf[len++] = (char) (0x80 | ((cp >> 12) & 0x3F));
                    buf[len++] = (char) (0x80 | ((cp >> 6) & 0x3F));
                    buf[len++] = (char) (0x80 | (cp & 0x3F));
                  }

                continue;
              }
            default:
              free (buf);
              return NULL;
            }
          jp++;
        }
      else
        {
          buf[len++] = *jp++;
        }
    }

  if (*jp != '"')
    {
      free (buf);
      return NULL;
    }
  jp++;

  buf[len] = '\0';
  return buf;
}

static bool json_parse_value (struct json_value *out);

static bool
json_parse_object (struct json_value *out)
{
  size_t cap = 8;
  size_t count = 0;
  struct json_pair *pairs;

  if (*jp != '{')
    return false;
  jp++;

  pairs = xmalloc (cap * sizeof *pairs);
  json_skip_ws ();

  if (*jp == '}')
    {
      jp++;
      out->type = JSON_OBJECT;
      out->u.object.pairs = pairs;
      out->u.object.count = 0;
      return true;
    }

  for (;;)
    {
      json_skip_ws ();
      char *key = json_parse_string_value ();
      if (!key)
        goto fail;

      json_skip_ws ();
      if (*jp != ':')
        {
          free (key);
          goto fail;
        }
      jp++;

      json_skip_ws ();
      struct json_value val;
      if (!json_parse_value (&val))
        {
          free (key);
          goto fail;
        }

      if (count >= cap)
        {
          cap *= 2;
          pairs = xrealloc (pairs, cap * sizeof *pairs);
        }

      pairs[count].key = key;
      pairs[count].value = val;
      count++;

      json_skip_ws ();
      if (*jp == ',')
        {
          jp++;
          continue;
        }
      if (*jp == '}')
        {
          jp++;
          break;
        }
      goto fail;
    }

  out->type = JSON_OBJECT;
  out->u.object.pairs = pairs;
  out->u.object.count = count;
  return true;

 fail:
  for (size_t i = 0; i < count; i++)
    {
      free (pairs[i].key);
      json_value_free (&pairs[i].value);
    }
  free (pairs);
  return false;
}

static bool
json_parse_array (struct json_value *out)
{
  size_t cap = 8;
  size_t count = 0;
  struct json_value *elems;

  if (*jp != '[')
    return false;
  jp++;

  elems = xmalloc (cap * sizeof *elems);
  json_skip_ws ();

  if (*jp == ']')
    {
      jp++;
      out->type = JSON_ARRAY;
      out->u.array.elements = elems;
      out->u.array.count = 0;
      return true;
    }

  for (;;)
    {
      json_skip_ws ();
      struct json_value val;
      if (!json_parse_value (&val))
        goto fail;

      if (count >= cap)
        {
          cap *= 2;
          elems = xrealloc (elems, cap * sizeof *elems);
        }

      elems[count++] = val;

      json_skip_ws ();
      if (*jp == ',')
        {
          jp++;
          continue;
        }
      if (*jp == ']')
        {
          jp++;
          break;
        }
      goto fail;
    }

  out->type = JSON_ARRAY;
  out->u.array.elements = elems;
  out->u.array.count = count;
  return true;

 fail:
  for (size_t i = 0; i < count; i++)
    json_value_free (&elems[i]);
  free (elems);
  return false;
}

static bool
json_parse_value (struct json_value *out)
{
  json_skip_ws ();

  if (*jp == '"')
    {
      char *s = json_parse_string_value ();
      if (!s)
        return false;
      out->type = JSON_STRING;
      out->u.string = s;
      return true;
    }

  if (*jp == '{')
    return json_parse_object (out);

  if (*jp == '[')
    return json_parse_array (out);

  if (*jp == 't' && strncmp (jp, "true", 4) == 0)
    {
      jp += 4;
      out->type = JSON_BOOL;
      out->u.boolean = true;
      return true;
    }

  if (*jp == 'f' && strncmp (jp, "false", 5) == 0)
    {
      jp += 5;
      out->type = JSON_BOOL;
      out->u.boolean = false;
      return true;
    }

  if (*jp == 'n' && strncmp (jp, "null", 4) == 0)
    {
      jp += 4;
      out->type = JSON_NULL;
      return true;
    }

  /* Number — capture the textual span verbatim so we preserve the
     exact representation the input used.  Grammar (RFC 8259):

       number = [ '-' ] int [ frac ] [ exp ]
       int    = '0' | [1-9] [0-9]*
       frac   = '.' [0-9]+
       exp    = ('e'|'E') [+-]? [0-9]+  */
  if (*jp == '-' || (*jp >= '0' && *jp <= '9'))
    {
      const char *start = jp;

      if (*jp == '-')
        jp++;

      if (*jp == '0')
        jp++;
      else if (*jp >= '1' && *jp <= '9')
        while (*jp >= '0' && *jp <= '9')
          jp++;
      else
        return false;

      if (*jp == '.')
        {
          jp++;
          if (!(*jp >= '0' && *jp <= '9'))
            return false;
          while (*jp >= '0' && *jp <= '9')
            jp++;
        }

      if (*jp == 'e' || *jp == 'E')
        {
          jp++;
          if (*jp == '+' || *jp == '-')
            jp++;
          if (!(*jp >= '0' && *jp <= '9'))
            return false;
          while (*jp >= '0' && *jp <= '9')
            jp++;
        }

      size_t len = jp - start;
      char *buf = xmalloc (len + 1);
      memcpy (buf, start, len);
      buf[len] = '\0';

      out->type = JSON_NUMBER;
      out->u.number = buf;
      return true;
    }

  return false;
}

static void
json_value_free (struct json_value *v)
{
  size_t i;

  switch (v->type)
    {
    case JSON_STRING:
      free (v->u.string);
      break;
    case JSON_NUMBER:
      free (v->u.number);
      break;
    case JSON_OBJECT:
      for (i = 0; i < v->u.object.count; i++)
        {
          free (v->u.object.pairs[i].key);
          json_value_free (&v->u.object.pairs[i].value);
        }
      free (v->u.object.pairs);
      break;
    case JSON_ARRAY:
      for (i = 0; i < v->u.array.count; i++)
        json_value_free (&v->u.array.elements[i]);
      free (v->u.array.elements);
      break;
    default:
      break;
    }
}

/*
 * Conversion: JSON value to string representation for rec fields.
 */

static char *
json_value_to_string (struct json_value *v)
{
  switch (v->type)
    {
    case JSON_STRING:
      return xstrdup (v->u.string);
    case JSON_NUMBER:
      return xstrdup (v->u.number);
    case JSON_BOOL:
      return xstrdup (v->u.boolean ? "true" : "false");
    case JSON_NULL:
      return xstrdup ("");
    default:
      return xstrdup ("");
    }
}

/*
 * Build a rec record from a JSON object.
 */

static rec_record_t
json2rec_build_record (struct json_value *obj)
{
  rec_record_t record;
  size_t i;

  if (obj->type != JSON_OBJECT)
    return NULL;

  record = rec_record_new ();
  if (!record)
    recutl_out_of_memory ();

  for (i = 0; i < obj->u.object.count; i++)
    {
      struct json_pair *pair = &obj->u.object.pairs[i];

      if (!rec_field_name_p (pair->key))
        {
          recutl_warning (_("skipping invalid field name '%s'\n"),
                         pair->key);
          continue;
        }

      if (pair->value.type == JSON_ARRAY)
        {
          /* Multi-valued field.  */
          size_t j;
          for (j = 0; j < pair->value.u.array.count; j++)
            {
              char *val = json_value_to_string
                (&pair->value.u.array.elements[j]);
              rec_field_t field = rec_field_new (pair->key, val);
              rec_mset_append (rec_record_mset (record), MSET_FIELD,
                               (void *) field, MSET_ANY);
              free (val);
            }
        }
      else
        {
          char *val = json_value_to_string (&pair->value);
          rec_field_t field = rec_field_new (pair->key, val);
          rec_mset_append (rec_record_mset (record), MSET_FIELD,
                           (void *) field, MSET_ANY);
          free (val);
        }
    }

  return record;
}

/*
 * Build a rec record set from a JSON array of objects.
 */

static rec_rset_t
json2rec_build_rset (struct json_value *arr, const char *type)
{
  rec_rset_t rset;
  size_t i;

  if (arr->type != JSON_ARRAY)
    return NULL;

  rset = rec_rset_new ();
  if (!rset)
    recutl_out_of_memory ();

  if (type)
    rec_rset_set_type (rset, type);

  for (i = 0; i < arr->u.array.count; i++)
    {
      struct json_value *elem = &arr->u.array.elements[i];

      if (elem->type != JSON_OBJECT)
        {
          recutl_warning (_("skipping non-object element in array\n"));
          continue;
        }

      rec_record_t record = json2rec_build_record (elem);
      if (record)
        rec_mset_append (rec_rset_mset (rset), MSET_RECORD,
                         (void *) record, MSET_ANY);
    }

  return rset;
}

/*
 * Functions.
 */

void
recutl_print_help (void)
{
  printf (_("\
Usage: json2rec [OPTIONS]... [JSON_FILE]\n"));

  fputs (_("\
Convert JSON data into rec data.\n"), stdout);

  puts ("");
  fputs (_("\
  -t, --type=TYPE                     type name for the converted records; if this\n\
                                        parameter is omitted then the type is inferred\n\
                                        from the JSON structure.\n"),
         stdout);

  recutl_print_help_common ();
  puts ("");
  recutl_print_help_footer ();
}

static void
json2rec_parse_args (int argc, char **argv)
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
          json2rec_record_type = xstrdup (optarg);
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

      json2rec_json_file = argv[optind++];
    }
}

static char *
json2rec_read_input (void)
{
  FILE *in;
  size_t cap = 4096;
  size_t len = 0;
  char *buf;
  size_t n;

  if (json2rec_json_file)
    {
      if (!(in = fopen (json2rec_json_file, "r")))
        recutl_fatal (_("cannot read file %s\n"), json2rec_json_file);
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

  if (json2rec_json_file)
    fclose (in);

  return buf;
}

static rec_db_t
json2rec_process_input (void)
{
  char *input;
  struct json_value root;
  rec_db_t db;

  input = json2rec_read_input ();
  jp = input;
  jp_start = input;

  json_skip_ws ();
  if (*jp == '\0')
    {
      free (input);
      return NULL;
    }

  if (!json_parse_value (&root))
    {
      size_t line, col;
      json_get_position (&line, &col);
      free (input);
      recutl_fatal (_("invalid JSON input at line %zu, column %zu\n"),
                    line, col);
    }

  db = rec_db_new ();
  if (!db)
    recutl_out_of_memory ();

  if (root.type == JSON_ARRAY)
    {
      /* Array of objects → single record set.  */
      const char *type = json2rec_record_type;
      rec_rset_t rset = json2rec_build_rset (&root, type);
      if (rset)
        rec_db_insert_rset (db, rset, rec_db_size (db));
    }
  else if (root.type == JSON_OBJECT)
    {
      /* Object with named arrays → typed record sets.  */
      size_t i;
      for (i = 0; i < root.u.object.count; i++)
        {
          struct json_pair *pair = &root.u.object.pairs[i];
          const char *type = json2rec_record_type
            ? json2rec_record_type : pair->key;

          if (pair->value.type == JSON_ARRAY)
            {
              rec_rset_t rset = json2rec_build_rset (&pair->value, type);
              if (rset)
                rec_db_insert_rset (db, rset, rec_db_size (db));
            }
          else if (pair->value.type == JSON_OBJECT)
            {
              /* Single object treated as a one-record set.  */
              struct json_value arr;
              arr.type = JSON_ARRAY;
              arr.u.array.elements = &pair->value;
              arr.u.array.count = 1;

              rec_rset_t rset = json2rec_build_rset (&arr, type);
              if (rset)
                rec_db_insert_rset (db, rset, rec_db_size (db));
            }
          else
            {
              recutl_warning (
                _("skipping non-array value for key '%s'\n"), pair->key);
            }
        }
    }
  else
    {
      json_value_free (&root);
      free (input);
      recutl_fatal (_("JSON input must be an array or an object\n"));
    }

  json_value_free (&root);
  free (input);

  return db;
}

int
main (int argc, char *argv[])
{
  int ret;
  rec_db_t db;
  rec_writer_t writer;

  recutl_init ("json2rec");

  json2rec_parse_args (argc, argv);
  db = json2rec_process_input ();
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
