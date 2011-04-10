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
#include "rrep.h"
#include "messages.h"

/* Prints version information.  */
void
print_version ()
{
  printf ("%s %s\n\n", PROGRAM_NAME, VERSION);
  printf ("Copyright (C) 2011 Arno Onken <asnelt@asnelt.org>\n");
  printf ("License GPLv3+: GNU GPL version 3 or later");
  printf (" <http://gnu.org/licenses/gpl.html>\n");
  printf ("This is free software: you are free to change and");
  printf (" redistribute it.\n");
  printf ("There is NO WARRANTY, to the extent permitted by law.\n\n");
}

/* Prints usage information.  */
void
print_usage ()
{
  printf ("Usage: %s [OPTION]... PATTERN REPLACEMENT [FILE]...\n",
	  invocation_name);
}

/* Prints the help.  */
void
print_help ()
{
  print_usage ();
  printf ("Replace PATTERN by REPLACEMENT in each FILE or standard");
  printf (" input.\n");
  printf ("PATTERN is, by default, a basic regular expression (BRE).");
  printf (" REPLACEMENT may\n");
  printf ("contain the special character & to refer to that portion");
  printf (" of the pattern space\n");
  printf ("which matched, and the special escapes \\1 through \\9 to");
  printf (" refer to the\n");
  printf ("corresponding matching sub-expressions in PATTERN.\n");
  printf ("Example: %s 'hello world' 'Hello, World!' menu.h",
	  invocation_name);
  printf (" main.c\n\n");
  printf ("Options:\n");
  printf ("  -a, --all                      do not ignore files and");
  printf (" directories starting\n");
  printf ("                                 with .\n");
  printf ("  -E, --extended-regexp          PATTERN is an extended");
  printf (" regular expression (ERE)\n");
  printf ("  -e, --regex=PATTERN            use PATTERN for");
  printf (" matching\n");
  printf ("  -F, --fixed-strings            PATTERN and REPLACEMENT");
  printf (" are fixed strings\n");
  printf ("  -h, --help                     display this help and");
  printf (" exit\n");
  printf ("  -i, --ignore-case              ignore case");
  printf (" distinctions\n");
  printf ("  -p, --replace-with=REPLACEMENT use REPLACEMENT for");
  printf (" substitution\n");
  printf ("  -q, --quiet, --silent          suppress all normal");
  printf (" messages\n");
  printf ("  -R, -r, --recursive            process directories");
  printf (" recursively\n");
  printf ("  -s, --no-messages              suppress error");
  printf (" messages\n");
  printf ("  -V, --version                  print version");
  printf (" information and exit\n");
  printf ("  -w, --word-regexp              force PATTERN to match");
  printf (" only whole words\n");
  printf ("  -x, --line-regexp              force PATTERN to match");
  printf (" only whole lines\n");
  printf ("\n");
  printf ("With no FILE, or when FILE is -, read standard input.");
  printf (" Exit status is %d if any\n", EXIT_FAILURE);
  printf ("error occurs, %d otherwise.\n", EXIT_SUCCESS);
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
      fprintf (stderr, "%s: %s: Could not process argument: ",
	       invocation_name, file_name);
      perror (NULL);
      break;
    case ERR_PROCESS_DIR:
      fprintf (stderr, "%s: %s: Could not process directory: ",
	       invocation_name, file_name);
      perror (NULL);
      break;
    case ERR_PATTERN:
      fprintf (stderr, "%s: PATTERN must have at least one",
	       invocation_name);
      fprintf (stderr, " character\n");
      break;
    case ERR_UNKNOWN_ESCAPE:
      fprintf (stderr, "%s: %s: Unknown escape sequence in",
	       invocation_name, file_name);
      fprintf (stderr, " REPLACEMENT\n");
      break;
    case ERR_SAVE_DIR:
      fprintf (stderr, "%s: Could not save current working",
	       invocation_name);
      fprintf (stderr, " directory: ");
      perror (NULL);
      break;
    case ERR_ALLOC_BUFFER:
      fprintf (stderr, "%s: Could not allocate memory for buffer: ",
	       invocation_name);
      perror (NULL);
      break;
    case ERR_ALLOC_FILEBUFFER: 
      fprintf (stderr, "%s: %s: Could not allocate memory for",
	       invocation_name, file_name);
      fprintf (stderr, " file_buffer: ");
      perror (NULL);
      break;
    case ERR_ALLOC_FILELIST:
      fprintf (stderr, "%s: Could not allocate memory for file_list: ",
	       invocation_name);
      perror (NULL);
      break;
    case ERR_ALLOC_PATHBUFFER:
      fprintf (stderr, "%s: %s: Could not allocate memory for",
	       invocation_name, file_name);
      fprintf (stderr, " next_path: ");
      perror (NULL);
      break;
    case ERR_ALLOC_PATTERN:
      fprintf (stderr, "%s: %s: Could not allocate memory for",
	       invocation_name, file_name);
      fprintf (stderr, " pattern: ");
      perror (NULL);
      break;
    case ERR_ALLOC_REPLACEMENT:
      fprintf (stderr, "%s: %s: Could not allocate memory for",
	       invocation_name, file_name);
      fprintf (stderr, " replacement: ");
      perror (NULL);
      break;
    case ERR_REALLOC_BUFFER:
      fprintf (stderr, "%s: %s: Could not reallocate memory for",
	       invocation_name, file_name);
      fprintf (stderr, " buffer: ");
      perror (NULL);
      break;
    case ERR_REALLOC_FILEBUFFER:
      fprintf (stderr, "%s: %s: Could not reallocate memory for",
	       invocation_name, file_name);
      fprintf (stderr, " file_buffer: ");
      perror (NULL);
      break;
    case ERR_MEMORY:
      fprintf (stderr, "%s: %s: Not enough memory to process file: ",
	       invocation_name, file_name);
      perror (NULL);
      break;
    case ERR_OPEN_READ:
      fprintf (stderr, "%s: %s: Could not open file for reading: ",
	       invocation_name, file_name);
      perror (NULL);
      break;
    case ERR_OPEN_WRITE:
      fprintf (stderr, "%s: %s: Could not open file for writing: ",
	       invocation_name, file_name);
      perror (NULL);
      break;
    case ERR_OPEN_DIR:
      fprintf (stderr, "%s: Could not open directory: ",
	       invocation_name);
      perror (NULL);
      break;
    case ERR_READ_FILE:
      fprintf (stderr, "%s: %s: Could not read file: ",
	       invocation_name, file_name);
      perror (NULL);
      break;
    case ERR_READ_TEMP:
      fprintf (stderr, "%s: %s: Could not read temporary file: ",
	       invocation_name, file_name);
      perror (NULL);
      break;
    case ERR_OVERWRITE:
      fprintf (stderr, "%s: %s: Could not overwrite file: ",
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
  regerror (errcode, compiled, message, len);
  fprintf (stderr, "%s: %s\n", invocation_name, message);
  free (message);
}
