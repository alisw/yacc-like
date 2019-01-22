/* Output Graphviz specification of a state machine generated by Bison.

   Copyright (C) 2006-2007, 2009-2015, 2018 Free Software Foundation,
   Inc.

   This file is part of Bison, the GNU Compiler Compiler.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Written by Paul Eggert and Satya Kiran Popuri.  */

#include <config.h>
#include "system.h"

#include <quotearg.h>

#include "files.h"
#include "gram.h"
#include "graphviz.h"
#include "tables.h"

/* Return an unambiguous printable representation for NAME, suitable
   for C strings.  Use slot 2 since the user may use slots 0 and 1.  */

static char *
quote (char const *name)
{
  return quotearg_n_style (2, c_quoting_style, name);
}

void
start_graph (FILE *fout)
{
  fprintf (fout,
           _("// Generated by %s.\n"
             "// Report bugs to <%s>.\n"
             "// Home page: <%s>.\n"
             "\n"),
           PACKAGE_STRING,
           PACKAGE_BUGREPORT,
           PACKAGE_URL);
  fprintf (fout,
           "digraph %s\n"
           "{\n",
           quote (grammar_file));
  fprintf (fout,
           "  node [fontname = courier, shape = box, colorscheme = paired6]\n"
           "  edge [fontname = courier]\n"
           "\n");
}

void
output_node (int id, char const *label, FILE *fout)
{
  fprintf (fout, "  %d [label=\"%s\"]\n", id, label);
}

void
output_edge (int source, int destination, char const *label,
             char const *style, FILE *fout)
{
  fprintf (fout, "  %d -> %d [style=%s", source, destination, style);
  if (label)
    fprintf (fout, " label=%s", quote (label));
  fputs ("]\n", fout);
}

char const *
escape (char const *name)
{
  char *q = quote (name);
  q[strlen (q) - 1] = '\0';
  return q + 1;
}

static void
no_reduce_bitset_init (state const *s, bitset *no_reduce_set)
{
  *no_reduce_set = bitset_create (ntokens, BITSET_FIXED);
  bitset_zero (*no_reduce_set);
  {
    int n;
    FOR_EACH_SHIFT (s->transitions, n)
      bitset_set (*no_reduce_set, TRANSITION_SYMBOL (s->transitions, n));
  }
  for (int n = 0; n < s->errs->num; ++n)
    if (s->errs->symbols[n])
      bitset_set (*no_reduce_set, s->errs->symbols[n]->content->number);
}

static void
conclude_red (struct obstack *out, int source, rule_number ruleno,
              bool enabled, bool first, FILE *fout)
{
  /* If no lookahead tokens were valid transitions, this reduction is
     actually hidden, so cancel everything. */
  if (first)
    (void) obstack_finish0 (out);
  else
    {
      char const *ed = enabled ? "" : "d";
      char const *color = enabled ? ruleno ? "3" : "1" : "5";

      /* First, build the edge's head. The name of reduction nodes is "nRm",
         with n the source state and m the rule number. This is because we
         don't want all the reductions bearing a same rule number to point to
         the same state, since that is not the desired format. */
      fprintf (fout, "  %d -> \"%dR%d%s\" [",
               source, source, ruleno, ed);

      /* (The lookahead tokens have been added to the beginning of the
         obstack, in the caller function.) */
      if (! obstack_empty_p (out))
        {
          char *label = obstack_finish0 (out);
          fprintf (fout, "label=\"[%s]\", ", label);
          obstack_free (out, label);
        }

      /* Then, the edge's tail. */
      fprintf (fout, "style=solid]\n");

      /* Build the associated diamond representation of the target rule. */
      fprintf (fout, " \"%dR%d%s\" [label=\"",
               source, ruleno, ed);
      if (ruleno)
        fprintf (fout, "R%d", ruleno);
      else
        fprintf (fout, "Acc");

      fprintf (fout, "\", fillcolor=%s, shape=diamond, style=filled]\n",
               color);
    }
}

static bool
print_token (struct obstack *out, bool first, char const *tok)
{
  if (! first)
    obstack_sgrow (out, ", ");
  obstack_sgrow (out, escape (tok));
  return false;
}

void
output_red (state const *s, reductions const *reds, FILE *fout)
{
  bitset no_reduce_set;
  no_reduce_bitset_init (s, &no_reduce_set);

  /* Two obstacks are needed: one for the enabled reductions, and one
     for the disabled reductions, because in the end we want two
     separate edges, even though in most cases only one will actually
     be printed. */
  struct obstack dout;
  struct obstack eout;
  obstack_init (&dout);
  obstack_init (&eout);

  const int source = s->number;
  for (int j = 0; j < reds->num; ++j)
    {
      rule *default_reduction =
        yydefact[s->number]
        ? &rules[yydefact[s->number] - 1]
        : NULL;

      bool defaulted = default_reduction && default_reduction == reds->rules[j];

      /* Build the lookahead tokens lists, one for enabled transitions
         and one for disabled transitions. */
      bool firstd = true;
      bool firste = true;
      rule_number ruleno = reds->rules[j]->number;

      if (reds->lookahead_tokens)
        for (int i = 0; i < ntokens; i++)
          if (bitset_test (reds->lookahead_tokens[j], i))
            {
              if (bitset_test (no_reduce_set, i))
                firstd = print_token (&dout, firstd, symbols[i]->tag);
              else
                {
                  if (! defaulted)
                    firste = print_token (&eout, firste, symbols[i]->tag);
                  bitset_set (no_reduce_set, i);
                }
            }

      /* Do the actual output. */
      conclude_red (&dout, source, ruleno, false, firstd, fout);
      conclude_red (&eout, source, ruleno, true, firste && !defaulted, fout);
    }
  obstack_free (&dout, 0);
  obstack_free (&eout, 0);
  bitset_free (no_reduce_set);
}

void
finish_graph (FILE *fout)
{
  fputs ("}\n", fout);
}
