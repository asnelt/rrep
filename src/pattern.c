/* pattern.c - pattern and replacement routines for rrep.
   Copyright (C) 2011, 2013, 2022, 2026 Arno Onken <asnelt@asnelt.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <regex.h>
#include "rrep.h"
#include "messages.h"
#include "pattern.h"

/* Checks the whole word and whole line options.  */
bool
check_whole (const char *line, const char *start, size_t len)
{
  char c;

  /* Check whether any of these options was selected.  */
  if (!(options & OPT_WHOLE_WORD) && !(options & OPT_WHOLE_LINE))
    return true;

  /* Check beginning.  */
  if (start > line)
    {
      if (options & OPT_WHOLE_LINE)
        return false;

      c = *(start-1);
      if (isalnum ((unsigned char) c) || c == '_')
        return false;
    }

  /* Check end.  */
  c = *(start + len);
  if (c != '\n' && c != '\0')
    {
      if (options & OPT_WHOLE_LINE)
        return false;

      if (isalnum ((unsigned char) c) || c == '_')
        return false;
    }

  return true;
}

/* Matches regular expression or string in start.  Returns 0 if a match was
   found, REG_NOMATCH if no match was found or regerror error value if a
   regerror occurred.  Match offsets are stored in match.  */
int
match_pattern (pattern_t *pattern, const char *line, const char *start,
               regmatch_t *match)
{
  int i;
  const char *first; /* Start of first occurrence of pattern->string.  */
  int errcode; /* Return value of regexec.  */
  int eflags; /* Flags for regexec.  */
  regoff_t so, eo; /* Search range offsets for regexec.  */
  regoff_t eolo; /* End of line offset.  */

  if (options & OPT_FIXED)
    {
      /* Prepare match.  */
      for (i = 1; i < 10; i++)
        {
          match[i].rm_so = -1;
          match[i].rm_eo = -1;
        }
      /* Match string.  */
      first = start;
      while (true)
        {
          if (options & OPT_ICASE)
            first = strcasestr (first, pattern->string);
          else
            first = strstr (first, pattern->string);
          if (first == NULL || check_whole (line, first, pattern->string_len))
            break;
          first++;
        }

      if (first == NULL)
        {
          /* String not found.  */
          match[0].rm_so = -1;
          match[0].rm_eo = -1;
          return REG_NOMATCH;
        }
      else
        {
          /* Set offsets in match[0].  */
          match[0].rm_so = first - start;
          match[0].rm_eo = match[0].rm_so + pattern->string_len;
          return 0;
        }
    }
  else
    {
      /* Match regular expression.  */
      eolo = strlen (start);
      so = 0;
      eo = eolo;
      while (true)
        {
          /* Set flags for regexec.  */
          eflags = REG_STARTEND;
          if (start + so != line)
            eflags |= REG_NOTBOL;
          if (eo < eolo)
            eflags |= REG_NOTEOL;

          match[0].rm_so = so;
          match[0].rm_eo = eo;
          errcode = regexec (pattern->compiled, start, 10, match, eflags);
          if (errcode != 0 && errcode != REG_NOMATCH)
            return errcode;

          if (errcode == 0 && (match[0].rm_so == so || eo == eolo))
            {
              so = match[0].rm_so;
              if (check_whole (line, start+so, match[0].rm_eo-so))
                return 0;
              if (so < match[0].rm_eo)
                {
                  /* Check for shorter match.  */
                  eo = match[0].rm_eo - 1;
                  continue;
                }
            }
          else if (eo == eolo)
            return REG_NOMATCH;

          if (so == eolo)
            return REG_NOMATCH;

          so++;
          eo = eolo;
        }

      return errcode;
    }
}

/* Frees the memory that was allocated for the fields of pattern.  */
void
free_pattern (pattern_t *pattern)
{
  if (pattern->string != NULL)
    {
      free (pattern->string);
      pattern->string = NULL;
    }
  /* Free compiled regular expression.  */
  if (pattern->compiled != NULL)
    {
      regfree (pattern->compiled);
      free (pattern->compiled);
      pattern->compiled = NULL;
    }
}

/* Allocates memory for the fields of pattern and compiles the regular
   expression in string with cflags.  */
int
parse_pattern (const char *string, pattern_t *pattern, int cflags)
{
  int errcode; /* Error code for regcomp.  */

  pattern->string_len = strlen (string);
  if (pattern->string_len < 1)
    {
      rrep_error (ERR_PATTERN, NULL);
      return FAILURE;
    }

  /* Copy original string into replacement.  */
  pattern->string = (char *) malloc ((pattern->string_len + 1) * sizeof (char));
  if (pattern->string == NULL)
    {
      rrep_error (ERR_ALLOC_PATTERN, NULL);
      return FAILURE;
    }
  strcpy (pattern->string, string);

  if (options & OPT_FIXED)
    return SUCCESS;

  pattern->compiled = (regex_t *) malloc (sizeof (regex_t));
  if (pattern->compiled == NULL)
    {
      rrep_error (ERR_ALLOC_PATTERN, NULL);
      free_pattern (pattern);
      return FAILURE;
    }
  /* Compile regular expression.  */
  errcode = regcomp (pattern->compiled, string, cflags);
  if (errcode != 0)
    {
      print_regerror (errcode, pattern->compiled);
      free_pattern (pattern);
      return FAILURE;
    }

  return SUCCESS;
}

/* Frees the memory that was allocated for the fields of replacement.  */
void
free_replace (replace_t *replacement)
{
  size_t i;

  if (replacement->string != NULL)
    {
      free (replacement->string);
      replacement->string = NULL;
    }
  if (replacement->sub != NULL)
    {
      free (replacement->sub);
      replacement->sub = NULL;
    }
  if (replacement->part != NULL)
    {
      for (i=0; i < replacement->nsub+1; i++)
        {
          if (replacement->part[i] != NULL)
            {
              free (replacement->part[i]);
            }
        }
      free (replacement->part);
      replacement->part = NULL;
    }
  if (replacement->part_len != NULL)
    {
      free (replacement->part_len);
      replacement->part_len = NULL;
    }
}

/* Prepares replacement string for quick processing.  The string can contain
   escape sequences which are replaced in this function.  Moreover, the string
   is split into substrings.  The result is stored in replacement.  */
int
parse_replace (const char *string, replace_t *replacement)
{
  const char *next;
  char escape_substring[3]; /* Substring for escape error message.  */
  size_t nsub = 0;
  size_t len; /* Counter for lengths of substrings.  */
  size_t i, j;

  /* Copy original string into replacement.  */
  replacement->string_len = strlen (string);
  replacement->string = (char *) malloc ((replacement->string_len + 1)
                                         * sizeof (char));
  if (replacement->string == NULL)
    {
      rrep_error (ERR_ALLOC_REPLACEMENT, NULL);
      return FAILURE;
    }
  strcpy (replacement->string, string);

  if (options & OPT_FIXED)
    return SUCCESS;

  /* Do three swipes over string.  */
  /* First swipe: count number of subpatterns.  */
  next = string;
  while (*next != '\0')
    {
      if (*next == '&')
        nsub++;
      else if (*next == '\\')
        {
          if (*(next+1) >= '1' && *(next+1) <= '9')
            nsub++;
          else if (*(next+1) != '&' && *(next+1) != 'n' && *(next+1) != '\\')
            {
              escape_substring[0] = '\\';
              escape_substring[1] = *(next+1);
              escape_substring[2] = '\0';
              rrep_error (ERR_UNKNOWN_ESCAPE, escape_substring);
              free_replace (replacement);
              return FAILURE;
            }
          next++;
        }
      next++;
    }

  /* Allocate memory for string and index arrays.  */
  replacement->nsub = nsub;
  replacement->part = (char **) malloc ((nsub+1) * sizeof (char *));
  if (replacement->part == NULL)
    {
      rrep_error (ERR_ALLOC_REPLACEMENT, NULL);
      free_replace (replacement);
      return FAILURE;
    }
  for (i = 0; i < nsub+1; i++)
    replacement->part[i] = NULL;
  replacement->part_len = (size_t *) malloc ((nsub+1) * sizeof (size_t));
  if (replacement->part_len == NULL)
    {
      rrep_error (ERR_ALLOC_REPLACEMENT, NULL);
      free_replace (replacement);
      return FAILURE;
    }
  replacement->sub = (int *) malloc (nsub * sizeof (int));
  if (replacement->sub == NULL)
    {
      rrep_error (ERR_ALLOC_REPLACEMENT, NULL);
      free_replace (replacement);
      return FAILURE;
    }

  /* Second swipe: allocate memory for substrings.  */
  next = string;
  len = 0;
  i = 0;
  while (true)
    {
      len++;
      if (*next == '\0' || *next == '&'
          || (*next == '\\' && *(next+1) >= '1' && *(next+1) <= '9'))
        {
          replacement->part[i] = (char *) malloc (len * sizeof (char));
          if (replacement->part[i] == NULL)
            {
              rrep_error (ERR_ALLOC_REPLACEMENT, NULL);
              free_replace (replacement);
              return FAILURE;
            }
          replacement->part_len[i] = len-1;
          len = 0;
          i++;
          if (*next == '\\')
            next++;
        }
      else if (*next == '\\')
        next++;
      if (*next == '\0')
        break;
      next++;
    }

  /* Third swipe: extract substrings.  */
  next = string;
  i = 0;
  j = 0;
  while (true)
    {
      if (*next == '\0')
        {
          replacement->part[i][j] = '\0';
          break;
        }
      else if (*next == '&')
        {
          replacement->part[i][j] = '\0';
          replacement->sub[i] = 0;
          i++;
          j = 0;
        }
      else if (*next == '\\')
        {
          if (*(next+1) >= '1' && *(next+1) <= '9')
            {
              replacement->part[i][j] = '\0';
              replacement->sub[i] = *(next+1)-'0';
              i++;
              j = 0;
            }
          else
            {
              if (*(next+1) == 'n')
                replacement->part[i][j] = '\n';
              else
                replacement->part[i][j] = *(next+1);
              j++;
            }
          next++;
        }
      else
        {
          replacement->part[i][j] = *next;
          j++;
        }

      next++;
    }

  return SUCCESS;
}
