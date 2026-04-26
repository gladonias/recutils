/* rec-sex-ast.c - SEX Abstract Syntax Trees.  */

/* Copyright (C) 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018,
 * 2019, 2020, 2022 Jose E. Marchesi */

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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <rec-sex-ast.h>

#define REC_SEX_AST_MAX_CHILDREN 3

struct rec_sex_ast_node_s
{
  enum rec_sex_ast_node_type_e type;
  union {
    int integer;
    double real;
    char *string;
    char *name[2];
  } val;

  int index;
  bool fixed;
  char *fixed_val;
  rec_sex_ast_node_t children[REC_SEX_AST_MAX_CHILDREN];
  size_t num_children;
};

struct rec_sex_ast_s
{
  rec_sex_ast_node_t top;
};

rec_sex_ast_t
rec_sex_ast_new ()
{
  rec_sex_ast_t new;

  new = malloc (sizeof (struct rec_sex_ast_s));
  if (new)
    new->top = NULL;

  return new;
}

void
rec_sex_ast_destroy (rec_sex_ast_t ast)
{
  if (ast->top)
    rec_sex_ast_node_destroy (ast->top);

  free (ast);
}

rec_sex_ast_node_t
rec_sex_ast_node_new (void)
{
  rec_sex_ast_node_t new;

  new = malloc (sizeof(struct rec_sex_ast_node_s));
  if (new)
    {
      new->type = REC_SEX_NOVAL;
      new->num_children = 0;
      new->index = -1;
      new->fixed = false;
      new->fixed_val = NULL;
    }

  return new;
}

void
rec_sex_ast_node_destroy (rec_sex_ast_node_t node)
{
  size_t i;

  /* Destroy children.  */
  for (i = 0; i < node->num_children; i++)
    rec_sex_ast_node_destroy (node->children[i]);

  /* Destroy values.  */
  if (node->type == REC_SEX_STR)
    {
      free (node->val.string);
    }
  else if (node->type == REC_SEX_NAME)
    {
      free (node->val.name[0]);
      free (node->val.name[1]);
    }

  free (node->fixed_val);
  free (node);
}

enum rec_sex_ast_node_type_e
rec_sex_ast_node_type (rec_sex_ast_node_t node)
{
  return node->type;
}

void
rec_sex_ast_node_set_type (rec_sex_ast_node_t node,
                           enum rec_sex_ast_node_type_e type)
{
  node->type = type;
}

int
rec_sex_ast_node_int (rec_sex_ast_node_t node)
{
  return node->val.integer;
}

void
rec_sex_ast_node_set_int (rec_sex_ast_node_t node,
                          int num)
{
  node->type = REC_SEX_INT;
  node->val.integer = num;
}

double
rec_sex_ast_node_real (rec_sex_ast_node_t node)
{
  return node->val.real;
}

void
rec_sex_ast_node_set_real (rec_sex_ast_node_t node,
                           double num)
{
  node->type = REC_SEX_REAL;
  node->val.real = num;
}

char *
rec_sex_ast_node_str (rec_sex_ast_node_t node)
{
  return node->val.string;
}

bool
rec_sex_ast_node_set_str (rec_sex_ast_node_t node,
                          char *str)
{
  char *copy = strdup (str);
  if (!copy)
    return false;

  if (node->type == REC_SEX_STR)
    free (node->val.string);

  node->type = REC_SEX_STR;
  node->val.string = copy;
  return true;
}

const char *
rec_sex_ast_node_name (rec_sex_ast_node_t node)
{
  return node->val.name[0];
}

const char *
rec_sex_ast_node_subname (rec_sex_ast_node_t node)
{
  return node->val.name[1];
}

bool
rec_sex_ast_node_set_name (rec_sex_ast_node_t node,
                           const char *name,
                           const char *subname)
{
  char *name_copy = strdup (name);
  if (!name_copy)
    return false;

  char *subname_copy = NULL;
  if (subname)
    {
      subname_copy = strdup (subname);
      if (!subname_copy)
        {
          free (name_copy);
          return false;
        }
    }

  if (node->type == REC_SEX_NAME)
    {
      free (node->val.name[0]);
      free (node->val.name[1]);
    }

  node->type = REC_SEX_NAME;
  node->val.name[0] = name_copy;
  node->val.name[1] = subname_copy;
  return true;
}

void
rec_sex_ast_node_link (rec_sex_ast_node_t parent,
                       rec_sex_ast_node_t child)
{
  if (parent->num_children < REC_SEX_AST_MAX_CHILDREN)
    parent->children[parent->num_children++] = child;
}

rec_sex_ast_node_t
rec_sex_ast_top (rec_sex_ast_t ast)
{
  return ast->top;
}

void
rec_sex_ast_set_top (rec_sex_ast_t ast,
                     rec_sex_ast_node_t node)
{
  ast->top = node;
}

void
rec_sex_ast_print_node (rec_sex_ast_node_t node)
{
  int i;

  for (i = 0; i < node->num_children; i++)
    rec_sex_ast_print_node (node->children[i]);

  printf ("------- node\n");
  printf ("type: %d\n", node->type);
  if (node->type == REC_SEX_INT)
    printf("value: %d\n", node->val.integer);
  if (node->type == REC_SEX_NAME)
    printf("value: %s\n", node->val.name[0]);
  if (node->type == REC_SEX_STR)
    printf("value: %s\n", node->val.string);

  printf("\n");
}

int
rec_sex_ast_node_num_children (rec_sex_ast_node_t node)
{
  return node->num_children;
}

rec_sex_ast_node_t
rec_sex_ast_node_child (rec_sex_ast_node_t node,
                        int n)
{
  rec_sex_ast_node_t res;

  res = NULL;
  if (n < node->num_children)
    res = node->children[n];

  return res;
}

void
rec_sex_ast_node_reset (rec_sex_ast_node_t node)
{
  int i;

  for (i = 0; i < node->num_children; i++)
    rec_sex_ast_node_reset (node->children[i]);

  node->index = 0;
}

bool
rec_sex_ast_node_fix (rec_sex_ast_node_t node,
                      char *val)
{
  char *copy = strdup (val);
  if (!copy)
    return false;

  free (node->fixed_val);
  node->fixed = true;
  node->fixed_val = copy;
  return true;
}

void
rec_sex_ast_node_unfix (rec_sex_ast_node_t node)
{
  int i;

  for (i = 0; i < node->num_children; i++)
    rec_sex_ast_node_unfix (node->children[i]);

  node->fixed = false;
}

bool
rec_sex_ast_node_fixed (rec_sex_ast_node_t node)
{
  return node->fixed;
}

char *
rec_sex_ast_node_fixed_val (rec_sex_ast_node_t node)
{
  return node->fixed_val;
}

int
rec_sex_ast_node_index (rec_sex_ast_node_t node)
{
  return node->index;
}

void
rec_sex_ast_node_set_index (rec_sex_ast_node_t node,
                            int index)
{
  node->index = index;
}

void
rec_sex_ast_print (rec_sex_ast_t ast)
{
  rec_sex_ast_print_node (ast->top);
}

bool
rec_sex_ast_name_p_1 (rec_sex_ast_node_t node,
                      const char *name,
                      size_t idx)
{
  size_t i = 0;

  if (node)
    {
      if ((node->type == REC_SEX_NAME)
          && ((node->index == -1) || (node->index < idx))
          && (strcmp (name, node->val.name[0]) == 0))
        return true;

      for (i = 0; i < node->num_children; i++)
        {
          if (rec_sex_ast_name_p_1 (node->children[i], name, idx))
            return true;
        }
    }

  return false;
}

bool
rec_sex_ast_name_p (rec_sex_ast_t ast,
                    const char *name,
                    size_t idx)
{
  /* Traverse the AST looking for any name node NAME[I] where I <
     idx.  */
  return rec_sex_ast_name_p_1 (ast->top, name, idx);
}

static bool
rec_sex_ast_hash_name_p_1 (rec_sex_ast_node_t node,
                           const char *name)
{
  if (node)
    {
      size_t i = 0;

      if ((node->type == REC_SEX_OP_SHA)
          && (node->num_children == 1)
          && (node->children[0]->type == REC_SEX_NAME)
          && (strcmp (name, node->children[0]->val.name[0]) == 0))
        return true;

      for (i = 0; i < node->num_children; i++)
        if (rec_sex_ast_hash_name_p_1 (node->children[i], name))
          return true;
    }

  return false;
}

bool
rec_sex_ast_hash_name_p (rec_sex_ast_t ast,
                         const char *name)
{
  /* Traverse the AST looking for any name node NAME whose father is a
     REC_SEX_OP_SHA.  */
  return rec_sex_ast_hash_name_p_1 (ast->top, name);
}

static bool
rec_sex_ast_fix_name_1 (rec_sex_ast_node_t node,
                         const char *name,
                         const char *val)
{
  if (!node)
    return true;

  if (node->type == REC_SEX_OP_SHA)
    return true;

  if ((node->type == REC_SEX_NAME)
      && (node->index == -1)
      && (strcmp (name, node->val.name[0]) == 0))
    return rec_sex_ast_node_fix (node, (char *) val);
  else
    {
      size_t i;
      for (i = 0; i < node->num_children; i++)
        if (!rec_sex_ast_fix_name_1 (node->children[i], name, val))
          return false;
    }

  return true;
}

bool
rec_sex_ast_fix_name (rec_sex_ast_t ast,
                      const char *name,
                      const char *val)
{
  return rec_sex_ast_fix_name_1 (ast->top, name, val);
}
