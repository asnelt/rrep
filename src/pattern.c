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
#include <ctype.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>
#include "rrep.h"
#include "messages.h"
#include "pattern.h"

/* Determines the byte length of the multibyte character at position pos and
   advances the state at ps.  Falls back to length 1 if an error occurs.  */
size_t
count_bytes (const char *pos, mbstate_t *ps)
{
  size_t len;

  if (MB_CUR_MAX == 1)
    return 1;

  len = mbrlen (pos, MB_CUR_MAX, ps);
  if (len == 0 || len == (size_t) -1 || len == (size_t) -2)
    {
      /* Fallback in case of null, invalid or incomplete character.  */
      if (len != 0)
        memset (ps, 0, sizeof (*ps));
      return 1;
    }

  return len;
}

/* Checks if the character at position pos is part of a word.  */
static bool
check_word_part (const char *pos, mbstate_t *ps)
{
  wchar_t wc;
  size_t len;

  if (MB_CUR_MAX == 1)
    return isalnum ((unsigned char) *pos) || *pos == '_';

  len = mbrtowc (&wc, pos, MB_CUR_MAX, ps);

  if (len == 0 || len == (size_t) -1 || len == (size_t) -2)
    {
      /* Fallback in case of null, invalid or incomplete character.  */
      if (len != 0)
        memset (ps, 0, sizeof (*ps));
      return isalnum ((unsigned char) *pos) || *pos == '_';
    }

  return iswalnum ((wint_t) wc) || wc == L'_';
}

/* Check if the character at position start is the beginning of a word.  */
static bool
check_word_start (const char *line, const char *start, mbstate_t before_state,
                  size_t before_len)
{
  if (start == line)
    return true;

  return !check_word_part (start - before_len, &before_state);
}

/* Check if the character at the position len bytes after start is the end of a
   word.  */
static bool
check_word_end (const char *start, size_t len, mbstate_t start_state)
{
  char c;
  size_t adv_len;

  c = *(start + len);
  if (c != '\n' && c != '\0')
    {
      /* Advance state.  */
      adv_len = 0;
      while (adv_len < len)
        adv_len += count_bytes (start + adv_len, &start_state);
      if (check_word_part (start + len, &start_state))
        return false;
    }

  return true;
}

/* Checks the whole word option.  */
static bool
check_whole_word (const char *line, const char *start, size_t len,
                  mbstate_t start_state, mbstate_t before_state,
                  size_t before_len)
{
  if (!(options & OPT_WHOLE_WORD))
    return true;

  return check_word_start (line, start, before_state, before_len)
    && check_word_end (start, len, start_state);
}

/* Finds the offset after start between so and eo of the last multibyte
   character that is not part of a word, or returns -1 if there is no word
   gap.  */
static regoff_t
find_last_word_gap (const char *start, regoff_t so, regoff_t eo,
                    mbstate_t so_state)
{
  mbstate_t dummy_state;
  regoff_t i;
  regoff_t lgo = -1; /* Last gap character offset.  */

  for (i = so; i < eo; i += count_bytes (start + i, &so_state))
    {
      dummy_state = so_state;
      if (!check_word_part (start + i, &dummy_state))
        lgo = i;
    }

  return lgo;
}

/* Matches a whole line against regular expression or string.  Returns 0 if a
   match was found, REG_NOMATCH if no match was found or regerror error value if
   a regerror occurred.  Match offsets are stored in match.  */
static int
match_line (pattern_t *pattern, const char *line, size_t line_len,
            regmatch_t *match)
{
  int i;
  int errcode; /* Return value of regexec.  */
  regoff_t eolo; /* End of line offset.  */

  eolo = line_len;
  if (pattern->compiled == NULL)
    {
      for (i = 1; i < 10; i++)
        {
          match[i].rm_so = -1;
          match[i].rm_eo = -1;
        }
      if (strcmp (line, pattern->string) != 0)
        return REG_NOMATCH;
    }
  else
    {
      /* Check if match offsets are beginning and end of line.  */
      match[0].rm_so = 0;
      match[0].rm_eo = eolo;
      errcode = regexec (pattern->compiled, line, 10, match, REG_STARTEND);
      if (errcode != 0)
        return errcode;
      if (match[0].rm_so != 0 || match[0].rm_eo != eolo)
        return REG_NOMATCH;
    }
  match[0].rm_so = 0;
  match[0].rm_eo = eolo;

  return 0;
}

/* Matches regular expression or string in start.  Returns 0 if a match was
   found, REG_NOMATCH if no match was found or regerror error value if a
   regerror occurred.  Match offsets are stored in match.  */
int
match_pattern (pattern_t *pattern, const char *line, size_t line_len,
               const char *start, regmatch_t *match, mbstate_t first_state,
               mbstate_t before_state, size_t before_len)
{
  int i;
  const char *first; /* Start of first occurrence of pattern->string.  */
  const char *pos; /* Current search position for pattern->string.  */
  size_t string_len;
  int errcode; /* Return value of regexec.  */
  int eflags; /* Flags for regexec.  */
  regoff_t so, eo; /* Search range offsets for regexec.  */
  regoff_t eolo; /* End of line offset.  */
  regoff_t sso; /* Search start offset.  */
  regoff_t co; /* Character offset.  */
  regoff_t bco; /* Before character offset.  */
  mbstate_t co_state; /* State at character offset.  */

  /* Check if a whole line match was requested.  */
  if (options & OPT_WHOLE_LINE)
    {
      if (start != line)
        return REG_NOMATCH;
      return match_line (pattern, line, line_len, match);
    }

  first = start;
  if (pattern->compiled == NULL)
    {
      /* Prepare match.  */
      for (i = 1; i < 10; i++)
        {
          match[i].rm_so = -1;
          match[i].rm_eo = -1;
        }
      /* Match string.  */
      pos = first;
      while (true)
        {
          first = mbsstr (pos, pattern->string);
          if (first == NULL)
            {
              /* String not found.  */
              match[0].rm_so = -1;
              match[0].rm_eo = -1;
              return REG_NOMATCH;
            }
          /* Advance pos to first.  */
          while (pos < first)
            {
              before_state = first_state;
              before_len = count_bytes (pos, &first_state);
              pos += before_len;
            }
          string_len = pattern->string_len;
          if (check_whole_word (line, first, string_len, first_state,
                                before_state, before_len))
            {
              /* Set offsets in match[0].  */
              match[0].rm_so = first - start;
              match[0].rm_eo = match[0].rm_so + string_len;
              return 0;
            }
          /* Continue after next multibyte character.  */
          before_state = first_state;
          before_len = count_bytes (pos, &first_state);
          pos += before_len;
        }
    }
  else
    {
      /* Match regular expression.  */
      eolo = line_len - (start - line);
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

          sso = so - (regoff_t) before_len;
          match[0].rm_so = so - sso;
          match[0].rm_eo = eo - sso;
          errcode = regexec (pattern->compiled, start + sso, 10, match, eflags);
          if (errcode != 0 && errcode != REG_NOMATCH)
            return errcode;
          if (errcode == 0)
            {
              for (i = 0; i < 10; i++)
                if (match[i].rm_so > -1)
                  {
                    match[i].rm_so += sso;
                    match[i].rm_eo += sso;
                  }
            }

          if (errcode == 0 && (match[0].rm_so == so || eo == eolo))
            {
              while (so < match[0].rm_so)
                {
                  before_state = first_state;
                  before_len = count_bytes (start + so, &first_state);
                  so += before_len;
                }

              first = start + so;
              if (MB_CUR_MAX > 1)
                {
                  if (so != match[0].rm_so)
                    {
                      /* Start of match not aligned with first byte of multibyte
                         character.  */
                      eo = eolo;
                      continue;
                    }
                  co = so;
                  bco = co;
                  co_state = first_state;
                  while (co < match[0].rm_eo)
                    {
                      bco = co;
                      co += count_bytes (start + co, &co_state);
                    }
                  if (co != match[0].rm_eo)
                    {
                      /* End of match not aligned with last byte of multibyte
                         character.  */
                      eo = bco;
                      continue;
                    }
                }
              if (!(options & OPT_WHOLE_WORD))
                return 0;
              /* Check whole word in two steps.  */
              if (check_word_start (line, first, before_state, before_len))
                {
                  if (check_word_end (first, match[0].rm_eo - so,
                                      first_state))
                    return 0;
                  /* Check if a shorter match is possible.  */
                  eo = find_last_word_gap (start, so, match[0].rm_eo,
                                           first_state);
                  if (eo >= 0)
                    continue;
                }
              else
                {
                  /* Continue after word.  */
                  co_state = first_state;
                  while (so < eolo && check_word_part (start + so, &co_state))
                    {
                      before_state = first_state;
                      before_len = count_bytes (start + so, &first_state);
                      so += before_len;
                      co_state = first_state;
                    }
                  first = start + so;
                }
            }
          else if (eo == eolo)
            return REG_NOMATCH;

          if (so == eolo)
            return REG_NOMATCH;

          before_state = first_state;
          before_len = count_bytes (first, &first_state);;
          so += before_len;
          first = start + so;
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
  mbstate_t state;
  size_t len;
  char *pos;
  char *fixed_string = NULL; /* Fixed string with escaped characters.  */

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
    {
      if (!(options & OPT_ICASE))
        return SUCCESS;
      /* Allocate memory for fixed_string and add escapes.  */
      fixed_string = (char *) malloc ((2 * strlen (string) + 1)
                                      * sizeof (char));
      if (fixed_string == NULL)
        {
          rrep_error (ERR_ALLOC_PATTERN, NULL);
          free_pattern (pattern);
          return FAILURE;
        }
      memset (&state, 0, sizeof (state));
      pos = fixed_string;
      while (*string != '\0')
        {
          len = count_bytes (string, &state);
          if (len == 1 && strchr (".[\\*^$", *string) != NULL)
            {
              *pos = '\\';
              pos++;
            }
          memcpy (pos, string, len);
          pos += len;
          string += len;
        }
      *pos = '\0';
      string = fixed_string;
      cflags = REG_ICASE;
    }

  pattern->compiled = (regex_t *) malloc (sizeof (regex_t));
  if (pattern->compiled == NULL)
    {
      rrep_error (ERR_ALLOC_PATTERN, NULL);
      free (fixed_string);
      free_pattern (pattern);
      return FAILURE;
    }
  /* Compile regular expression.  */
  errcode = regcomp (pattern->compiled, string, cflags);
  free (fixed_string);
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
      for (i = 0; i < replacement->nsub + 1; i++)
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
          if (*(next + 1) >= '1' && *(next + 1) <= '9')
            nsub++;
          else if (*(next + 1) != '&' && *(next + 1) != 'n'
                   && *(next + 1) != '\\')
            {
              escape_substring[0] = '\\';
              escape_substring[1] = *(next + 1);
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
  replacement->part = (char **) malloc ((nsub + 1) * sizeof (char *));
  if (replacement->part == NULL)
    {
      rrep_error (ERR_ALLOC_REPLACEMENT, NULL);
      free_replace (replacement);
      return FAILURE;
    }
  for (i = 0; i < nsub + 1; i++)
    replacement->part[i] = NULL;
  replacement->part_len = (size_t *) malloc ((nsub + 1) * sizeof (size_t));
  if (replacement->part_len == NULL)
    {
      rrep_error (ERR_ALLOC_REPLACEMENT, NULL);
      free_replace (replacement);
      return FAILURE;
    }
  if (nsub > 0)
    {
      replacement->sub = (int *) malloc (nsub * sizeof (int));
      if (replacement->sub == NULL)
        {
          rrep_error (ERR_ALLOC_REPLACEMENT, NULL);
          free_replace (replacement);
          return FAILURE;
        }
    }

  /* Second swipe: allocate memory for substrings.  */
  next = string;
  len = 0;
  i = 0;
  while (true)
    {
      len++;
      if (*next == '\0' || *next == '&'
          || (*next == '\\' && *(next + 1) >= '1' && *(next + 1) <= '9'))
        {
          replacement->part[i] = (char *) malloc (len * sizeof (char));
          if (replacement->part[i] == NULL)
            {
              rrep_error (ERR_ALLOC_REPLACEMENT, NULL);
              free_replace (replacement);
              return FAILURE;
            }
          replacement->part_len[i] = len - 1;
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
          if (*(next + 1) >= '1' && *(next + 1) <= '9')
            {
              replacement->part[i][j] = '\0';
              replacement->sub[i] = *(next + 1) - '0';
              i++;
              j = 0;
            }
          else
            {
              if (*(next + 1) == 'n')
                replacement->part[i][j] = '\n';
              else
                replacement->part[i][j] = *(next + 1);
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

/* Checks the validity of references to pattern in replacement.  */
int
check_replace (const pattern_t *pattern, const replace_t *replacement)
{
  char escape_substring[3]; /* Substring for reference error message.  */
  size_t i;

  if (options & OPT_FIXED)
    return SUCCESS;

  for (i = 0; i < replacement->nsub; i++)
    if ((size_t) replacement->sub[i] > pattern->compiled->re_nsub)
      {
        escape_substring[0] = '\\';
        escape_substring[1] = '0' + replacement->sub[i];
        escape_substring[2] = '\0';
        rrep_error (ERR_INVALID_REFERENCE, escape_substring);
        return FAILURE;
      }

  return SUCCESS;
}
