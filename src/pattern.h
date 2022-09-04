/* pattern.h - declarations for pattern and replacement functions for
   rrep.
   Copyright (C) 2011 Arno Onken <asnelt@asnelt.org>

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

/* Data structure for PATTERN.  */
typedef struct
{
  char *string; /* Original pattern string.  */
  size_t string_len; /* Length of string.  */
  regex_t *compiled; /* Data structure for regular expression.  */
} pattern_t;

/* Data structure for REPLACEMENT.  */
typedef struct
{
  char *string; /* Original replacement string.  */
  size_t string_len; /* Length of string.  */
  char **part; /* Parts of the replacement string.  */
  size_t *part_len; /* Lengths of the parts.  */
  int *sub; /* Indices of regular expression subpatterns.  */
  size_t nsub; /* Number of subpatterns in replacement.  */
} replace_t;

/* Matches regular expression or string in start.  Returns 0 if a match was
   found, REG_NOMATCH if no match was found or regerror error value if a
   regerror occurred.  Match offsets are stored in match.  */
extern int match_pattern (pattern_t *, const char *, const char *,
                          regmatch_t *);

/* Frees the memory that was allocated for the fields of pattern.  */
extern void free_pattern (pattern_t *);

/* Allocates memory for the fields of pattern and compiles the regular
   expression in string with cflags.  */
extern int parse_pattern (const char *, pattern_t *, int);

/* Frees the memory that was allocated for the fields of replacement.  */
extern void free_replace (replace_t *);

/* Prepares replacement string for quick processing.  The string can contain
   escape sequences which are replaced in this function.  Moreover, the string
   is split into substrings.  The result is stored in replacement.  */
extern int parse_replace (const char *, replace_t *);
