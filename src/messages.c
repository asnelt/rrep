/* messages.c - message routines for rrep.
   Copyright (C) 2011, 2013 Arno Onken <asnelt@asnelt.org>

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
#include <errno.h>
#include <regex.h>
#include "gettext.h"
#include "progname.h"
#include "propername.h"
#include "yesno.h"
#include "rrep.h"
#include "messages.h"

#define _(string) gettext (string)
#define AUTHORS proper_name ("Arno Onken")

/* Prints version information.  */
void
print_version ()
{
  printf ("%s\n\n", PACKAGE_STRING);
  printf (_("\
Copyright (C) 2011, 2013 %s\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n"), AUTHORS);
  printf ("\n");
}

/* Prints usage information.  */
void
print_usage ()
{
  printf (_("Usage: %s [OPTION]... PATTERN REPLACEMENT [FILE]...\n"),
	  program_name);
}

/* Prints the help.  */
void
print_help ()
{
  print_usage ();
  printf (_("\
Replace PATTERN by REPLACEMENT in each FILE or standard input.\n"));
  printf (_("\
PATTERN is, by default, a basic regular expression (BRE).  REPLACEMENT may\n\
contain the special character & to refer to that portion of the pattern space\n\
which matched, and the special escapes \\1 through \\9 to refer to the\n\
corresponding matching sub-expressions in PATTERN.\n"));
  printf (_("Example: %s 'hello world' 'Hello, World!' menu.h main.c\n"),
	  program_name);
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
  -S, --suffix=SUFFIX            override default backup suffix\n\
  -V, --version                  print version information and exit\n\
  -a, --all                      do not ignore files starting with .\n\
  -b                             backup before overwriting files\n\
      --backup[=CONTROL]         like -b but accepts a version control\
 argument\n\
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
output.  Exit status is %d if any error occurs, %d otherwise.\n"), EXIT_FAILURE,
	  EXIT_SUCCESS);
  printf ("\n");
  printf (_("\
The backup suffix is ~, unless set with --suffix or SIMPLE_BACKUP_SUFFIX.\n\
The version control method may be selected via the --backup option or through\n\
the VERSION_CONTROL environment variable.  Here are the values:\n\
\n\
  none, off       never make backups (even if --backup is given)\n\
  numbered, t     make numbered backups\n\
  existing, nil   numbered if numbered backups exist, simple otherwise\n\
  simple, never   always make simple backups\n"));
  printf ("\n");
  printf (_("Report bugs to: %s\n"), PACKAGE_BUGREPORT);
}

/* Prints the program invocation.  */
void
print_invocation ()
{
  print_usage ();
  printf (_("Try `%s --help' for more information.\n"), program_name);
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
	       program_name, file_name);
      perror (NULL);
      break;
    case ERR_PROCESS_DIR:
      fprintf (stderr, _("%s: %s: Could not process directory: "),
	       program_name, file_name);
      perror (NULL);
      break;
    case ERR_PATTERN:
      fprintf (stderr, _("%s: PATTERN must have at least one character\n"),
	       program_name);
      break;
    case ERR_UNKNOWN_ESCAPE:
      fprintf (stderr, _("%s: %s: Unknown escape sequence in REPLACEMENT\n"),
	       program_name, file_name);
      break;
    case ERR_SAVE_DIR:
      fprintf (stderr, _("%s: Could not save current working directory: "),
	       program_name);
      perror (NULL);
      break;
    case ERR_ALLOC_SUFFIX:
      fprintf (stderr, _("%s: Could not allocate memory for suffix: "),
	       program_name);
      perror (NULL);
      break;
    case ERR_ALLOC_BUFFER:
      fprintf (stderr, _("%s: Could not allocate memory for buffer: "),
	       program_name);
      perror (NULL);
      break;
    case ERR_ALLOC_FILEBUFFER: 
      fprintf (stderr, _("%s: %s: Could not allocate memory for file_buffer: "),
	       program_name, file_name);
      perror (NULL);
      break;
    case ERR_ALLOC_FILELIST:
      fprintf (stderr, _("%s: Could not allocate memory for file_list: "),
	       program_name);
      perror (NULL);
      break;
    case ERR_ALLOC_PATHBUFFER:
      fprintf (stderr, _("%s: %s: Could not allocate memory for next_path: "),
	       program_name, file_name);
      perror (NULL);
      break;
    case ERR_ALLOC_PATTERN:
      fprintf (stderr, _("%s: Could not allocate memory for pattern: "),
	       program_name);
      perror (NULL);
      break;
    case ERR_ALLOC_REPLACEMENT:
      fprintf (stderr, _("%s: Could not allocate memory for replacement: "),
	       program_name);
      perror (NULL);
      break;
    case ERR_ALLOC_BACKUP:
      fprintf (stderr, _("\
%s: %s: Could not allocate memory for backup string: "), program_name,
	       file_name);
      perror (NULL);
      break;
    case ERR_REALLOC_BUFFER:
      fprintf (stderr, _("%s: %s: Could not reallocate memory for buffer: "),
	       program_name, file_name);
      perror (NULL);
      break;
    case ERR_REALLOC_FILEBUFFER:
      fprintf (stderr, _("\
%s: %s: Could not reallocate memory for file_buffer: "), program_name,
	       file_name);
      perror (NULL);
      break;
    case ERR_MEMORY:
      fprintf (stderr, _("%s: %s: Not enough memory to process file: "),
	       program_name, file_name);
      perror (NULL);
      break;
    case ERR_OPEN_READ:
      fprintf (stderr, _("%s: %s: Could not open file for reading: "),
	       program_name, file_name);
      perror (NULL);
      break;
    case ERR_OPEN_WRITE:
      fprintf (stderr, _("%s: %s: Could not open file for writing: "),
	       program_name, file_name);
      perror (NULL);
      break;
    case ERR_OPEN_DIR:
      fprintf (stderr, _("%s: Could not open directory: "),
	       program_name);
      perror (NULL);
      break;
    case ERR_READ_FILE:
      fprintf (stderr, _("%s: %s: Could not read file: "),
	       program_name, file_name);
      perror (NULL);
      break;
    case ERR_READ_TEMP:
      fprintf (stderr, _("%s: %s: Could not read temporary file: "),
	       program_name, file_name);
      perror (NULL);
      break;
    case ERR_WRITE_BACKUP:
      fprintf (stderr, _("%s: %s: Could not write backup file: "),
	       program_name, file_name);
      perror (NULL);
      break;
    case ERR_OVERWRITE:
      fprintf (stderr, _("%s: %s: Could not overwrite file: "),
	       program_name, file_name);
      perror (NULL);
      break;
    case ERR_KEEP_TIMES:
      fprintf (stderr, _("%s: %s: Could keep file times: "),
	       program_name, file_name);
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
      fprintf (stderr, "%s: %s\n", program_name, message);
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
    printf (_("%s: %s: Omitting directory\n"), program_name,
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
bool
prompt_user (const char *file_name)
{
  /* TRANSLATORS: This is a user prompt. In English the user can answer with y
     for 'yes' or n for 'no'. [y/n] should be translated as well.  */
  printf (_("Pattern found in %s. Replace pattern [y/n]? "), file_name);

  return yesno ();
}
