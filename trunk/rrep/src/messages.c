/* messages.c - message routines for rrep.
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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <regex.h>
#include "config.h"
#include "rrep.h"
#include "messages.h"
#include "gettext.h"

#define _(string) gettext (string)

/* Prints version information.  */
void
print_version ()
{
  printf ("%s\n\n", PACKAGE_STRING);
  printf (_("\
Copyright (C) 2011 %s\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n"),
	  "Arno Onken <asnelt@asnelt.org>");
  printf ("\n");
}

/* Prints usage information.  */
void
print_usage ()
{
  printf (_("Usage: %s [OPTION]... PATTERN REPLACEMENT [FILE]...\n"),
	  invocation_name);
}

/* Prints the help.  */
void
print_help ()
{
  print_usage ();
  printf (_("\
Replace PATTERN by REPLACEMENT in each FILE or standard input.\n"));
  printf (_("\
PATTERN is, by default, a basic regular expression (BRE). REPLACEMENT may\n\
contain the special character & to refer to that portion of the pattern space\n\
which matched, and the special escapes \\1 through \\9 to refer to the\n\
corresponding matching sub-expressions in PATTERN.\n"));
  printf (_("Example: %s 'hello world' 'Hello, World!' menu.h main.c\n"),
	  invocation_name);
  printf ("\n");
  printf (_("\
Options:\n\
  -E, --extended-regexp          PATTERN is an extended regular expression\
 (ERE)\n\
  -F, --fixed-strings            PATTERN and REPLACEMENT are fixed strings\n\
  -R, -r, --recursive            process directories recursively\n\
      --include=FILE_PATTERN     process only files that match FILE_PATTERN\n\
      --exclude=FILE_PATTERN     files that match FILE_PATTERN will be\
 skipped\n\
      --exclude-dir=PATTERN      directories that match PATTERN will be\
 skipped\n\
  -V, --version                  print version information and exit\n\
  -a, --all                      do not ignore files starting with .\n\
      --backup                   backup before overwriting files\n\
      --binary                   do not ignore binary files\n\
      --dry-run                  simulation mode\n\
  -e, --regex=PATTERN            use PATTERN for matching\n\
  -h, --help                     display this help and exit\n\
  -i, --ignore-case              ignore case distinctions\n\
      --keep-times               keep access and modification times\n\
  -p, --replace-with=REPLACEMENT use REPLACEMENT for substitution\n\
      --interactive              prompt before modifying a file\n\
  -q, --quiet, --silent          suppress all normal messages\n\
  -s, --no-messages              suppress error messages\n\
  -w, --word-regexp              force PATTERN to match only whole words\n\
  -x, --line-regexp              force PATTERN to match only whole lines\n"));
  printf ("\n");
  printf (_("\
With no FILE, or when FILE is -, read standard input and write to standard\n\
output. Exit status is %d if any error occurs, %d otherwise.\n"), EXIT_FAILURE,
	  EXIT_SUCCESS);
  printf ("\n");
  printf (_("Report bugs to: %s\n"), PACKAGE_BUGREPORT);
}

/* Prints the program invocation.  */
void
print_invocation ()
{
  print_usage ();
  printf (_("Try `%s --help' for more information.\n"), invocation_name);
}

/* Prints an error message.  */
void
rrep_error (const int errcode, const char *file_name)
{
  if (options & OPT_NO_MESSAGES)
    return;

  switch (errcode)
    {
    case ERR_PROCESS_ARG:
      fprintf (stderr, _("%s: %s: Could not process argument: "),
	       invocation_name, file_name);
      perror (NULL);
      break;
    case ERR_PROCESS_DIR:
      fprintf (stderr, _("%s: %s: Could not process directory: "),
	       invocation_name, file_name);
      perror (NULL);
      break;
    case ERR_PATTERN:
      fprintf (stderr, _("%s: PATTERN must have at least one character\n"),
	       invocation_name);
      break;
    case ERR_UNKNOWN_ESCAPE:
      fprintf (stderr, _("%s: %s: Unknown escape sequence in REPLACEMENT\n"),
	       invocation_name, file_name);
      break;
    case ERR_SAVE_DIR:
      fprintf (stderr, _("%s: Could not save current working directory: "),
	       invocation_name);
      perror (NULL);
      break;
    case ERR_ALLOC_BUFFER:
      fprintf (stderr, _("%s: Could not allocate memory for buffer: "),
	       invocation_name);
      perror (NULL);
      break;
    case ERR_ALLOC_FILEBUFFER: 
      fprintf (stderr, _("%s: %s: Could not allocate memory for file_buffer: "),
	       invocation_name, file_name);
      perror (NULL);
      break;
    case ERR_ALLOC_FILELIST:
      fprintf (stderr, _("%s: Could not allocate memory for file_list: "),
	       invocation_name);
      perror (NULL);
      break;
    case ERR_ALLOC_PATHBUFFER:
      fprintf (stderr, _("%s: %s: Could not allocate memory for next_path: "),
	       invocation_name, file_name);
      perror (NULL);
      break;
    case ERR_ALLOC_PATTERN:
      fprintf (stderr, _("%s: Could not allocate memory for pattern: "),
	       invocation_name);
      perror (NULL);
      break;
    case ERR_ALLOC_REPLACEMENT:
      fprintf (stderr, _("%s: Could not allocate memory for replacement: "),
	       invocation_name);
      perror (NULL);
      break;
    case ERR_ALLOC_BACKUP:
      fprintf (stderr, _("\
%s: %s: Could not allocate memory for backup string: "), invocation_name,
	       file_name);
      perror (NULL);
      break;
    case ERR_REALLOC_BUFFER:
      fprintf (stderr, _("%s: %s: Could not reallocate memory for buffer: "),
	       invocation_name, file_name);
      perror (NULL);
      break;
    case ERR_REALLOC_FILEBUFFER:
      fprintf (stderr, _("\
%s: %s: Could not reallocate memory for file_buffer: "), invocation_name,
	       file_name);
      perror (NULL);
      break;
    case ERR_MEMORY:
      fprintf (stderr, _("%s: %s: Not enough memory to process file: "),
	       invocation_name, file_name);
      perror (NULL);
      break;
    case ERR_OPEN_READ:
      fprintf (stderr, _("%s: %s: Could not open file for reading: "),
	       invocation_name, file_name);
      perror (NULL);
      break;
    case ERR_OPEN_WRITE:
      fprintf (stderr, _("%s: %s: Could not open file for writing: "),
	       invocation_name, file_name);
      perror (NULL);
      break;
    case ERR_OPEN_DIR:
      fprintf (stderr, _("%s: Could not open directory: "),
	       invocation_name);
      perror (NULL);
      break;
    case ERR_READ_FILE:
      fprintf (stderr, _("%s: %s: Could not read file: "),
	       invocation_name, file_name);
      perror (NULL);
      break;
    case ERR_READ_TEMP:
      fprintf (stderr, _("%s: %s: Could not read temporary file: "),
	       invocation_name, file_name);
      perror (NULL);
      break;
    case ERR_WRITE_BACKUP:
      fprintf (stderr, _("%s: %s: Could not write backup file: "),
	       invocation_name, file_name);
      perror (NULL);
      break;
    case ERR_OVERWRITE:
      fprintf (stderr, _("%s: %s: Could not overwrite file: "),
	       invocation_name, file_name);
      perror (NULL);
      break;
    case ERR_KEEP_TIMES:
      fprintf (stderr, _("%s: %s: Could keep file times: "),
	       invocation_name, file_name);
      perror (NULL);
      break;
    }
}

/* Prints a regerror error message.  */
void
print_regerror (const int errcode, regex_t *compiled)
{
  size_t len = regerror (errcode, compiled, NULL, 0);
  char *message = malloc (len);
  if (message != NULL)
    {
      regerror (errcode, compiled, message, len);
      fprintf (stderr, "%s: %s\n", invocation_name, message);
      free (message);
    }
}

/* Prints replacement confirmation.  */
void
print_confirmation (const char *file_name)
{
  printf (_("%s: Pattern replaced\n"), file_name);
}

/* Prints directory omission.  */
void
print_dir_skip (const char *file_name)
{
  if (!(options & OPT_QUIET))
    printf (_("%s: %s: Omitting directory\n"), invocation_name,
	    file_name);
}

/* Prints the simulation message.  */
void
print_dry ()
{
  if (options & OPT_DRY && !(options & OPT_QUIET))
    {
      printf (_("Simulation mode: No files are modified.\n"));
      printf (_("PATTERN found in the following files:\n"));
    }
}

/* Prompt user before modification.  */
int
prompt_user (const char *file_name)
{
  int i;
  char c;
  /* Characters that are considered as a positive response.  */
  const char *yes_char;

  /* TRANSLATORS: This is a user prompt. In English the user can answer with y
     for 'yes' or n for 'no'. [y/n] should be translated as well and the "yY"
     string adapted accordingly.  */
  printf (_("Pattern found in %s. Replace pattern [y/n]? "), file_name);
  /* TRANSLATORS: This string contains a list of single characters that
     represent an affirmative answer. In English these are the first letters of
     'yes' and 'Yes'.  */
  yes_char = _("yY");
  c = getchar ();

  for (i = 0; i < strlen (yes_char); i++)
    {
      if (c == yes_char[i])
	return SUCCESS;
    }
  return FAILURE;
}