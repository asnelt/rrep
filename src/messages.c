/* messages.c - message routines for rrep.
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
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>
#include <errno.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "gettext.h"
#include "progname.h"
#include "propername.h"
#include "yesno.h"
#include "rrep.h"
#include "messages.h"

#define _(string) gettext (string)
#define AUTHORS proper_name ("Arno Onken")

#define ANSI_RESET "\033[0m"
#define ANSI_BOLD "\033[1m"
#define ANSI_RED "\033[31m"
#define ANSI_GREEN "\033[32m"
#define ANSI_CYAN "\033[36m"

/* Prints version information.  */
void
print_version (void)
{
  printf ("%s\n\n", PACKAGE_STRING);
  printf (_("\
Copyright (C) 2011, 2013, 2019, 2022, 2026 %s\n\
License GPLv3+: GNU GPL version 3 or later \
<https://gnu.org/licenses/gpl.html>\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n"), AUTHORS);
  printf ("\n");
}

/* Prints usage information to fp.  */
void
print_usage (FILE *fp)
{
  fprintf (fp, _("Usage: %s [OPTION]... PATTERN REPLACEMENT [FILE]...\n"),
           program_name);
}

/* Prints the help.  */
void
print_help (void)
{
  print_usage (stdout);
  printf (_("\
Replace PATTERN by REPLACEMENT in each FILE or standard input.\n"));
  printf (_("\
PATTERN is, by default, a basic regular expression (BRE).  REPLACEMENT may\n\
contain the special character & to refer to that portion of the line which\n\
matched, and the special escapes \\1 through \\9 to refer to the\n\
corresponding matching sub-expressions in PATTERN.\n"));
  printf (_("Example: %s 'hello world' 'Hello, World!' menu.h main.c\n"),
          program_name);
  printf ("\n");
  printf (_("\
Options:\n\
  -E, --extended-regexp          PATTERN is an extended regular expression\
 (ERE)\n\
  -F, --fixed-strings            PATTERN and REPLACEMENT are fixed strings\n\
  -r, --recursive                process directories recursively\n\
  -R, --dereference-recursive    likewise, but follow all symlinks\n\
      --include=FILE_PATTERN     process only files that match FILE_PATTERN\n\
      --exclude=FILE_PATTERN     files that match FILE_PATTERN will be\
 skipped\n\
      --exclude-dir=PATTERN      directories that match PATTERN will be\
 skipped\n\
  -S, --suffix=SUFFIX            backup with suffix SUFFIX\n\
  -V, --version                  print version information and exit\n\
  -a, --all                      do not ignore files starting with .\n\
  -b                             backup before overwriting files\n\
      --backup[=CONTROL]         like -b but accepts a version control\
 argument\n\
      --binary                   do not ignore binary files\n\
      --dry-run                  simulation mode\n\
  -e, --regexp=PATTERN           use PATTERN for matching\n\
  -h, --help                     display this help and exit\n\
  -i, --ignore-case              ignore case distinctions\n\
      --keep-times               keep access and modification times\n\
  -p, --replace-with=REPLACEMENT use REPLACEMENT for substitution\n\
      --interactive              prompt before modifying a file\n\
  -q, --quiet, --silent          suppress all normal messages\n\
  -s, --no-messages              suppress error messages\n\
  -v, --verbose                  print changed lines\n\
  -w, --word-regexp              force PATTERN to match only whole words\n\
  -x, --line-regexp              force PATTERN to match only whole lines\n"));
  printf ("\n");
  printf (_("\
With no FILE, or when the only FILE is -, read standard input and write to\n\
standard output.  Exit status is %d if any error occurs, %d otherwise.\n"),
          EXIT_FAILURE, EXIT_SUCCESS);
  printf ("\n");
  printf (_("\
The backup suffix is ~, unless set with --suffix or SIMPLE_BACKUP_SUFFIX.\n\
The version control method may be selected via the --backup option or\n\
through the VERSION_CONTROL environment variable.  Here are the values:\n\
\n\
  none, off       never make backups (even if --backup is given)\n\
  numbered, t     make numbered backups\n\
  existing, nil   numbered if numbered backups exist, simple otherwise\n\
  simple, never   always make simple backups\n"));
  printf ("\n");
  printf (_("Report bugs to: %s\n"), PACKAGE_BUGREPORT);
}

/* Prints the program invocation to stderr.  */
void
print_invocation (void)
{
  print_usage (stderr);
  fprintf (stderr, _("Try `%s --help' for more information.\n"), program_name);
}

/* Prints the error message of saved_errno.  */
static void
print_errno (int saved_errno)
{
  fprintf (stderr, "%s\n", strerror (saved_errno));
}

/* Prints an error message.  */
void
rrep_error (const int errcode, const char *file_name)
{
  int saved_errno = errno;

  if (options & OPT_NO_MESSAGES)
    return;

  switch (errcode)
    {
    case ERR_PROCESS_ARG:
      fprintf (stderr, _("%s: %s: could not process argument: "),
               program_name, file_name);
      print_errno (saved_errno);
      break;
    case ERR_PROCESS_DIR:
      fprintf (stderr, _("%s: %s: could not process directory: "),
               program_name, file_name);
      print_errno (saved_errno);
      break;
    case ERR_DIR_NO_RECURSION:
      fprintf (stderr, _("%s: %s: directory given without recursive option\n"),
               program_name, file_name);
      break;
    case ERR_STDIN_WITH_FILES:
      fprintf (stderr, _("%s: standard input given with other files\n"),
               program_name);
      break;
    case ERR_PATTERN:
      fprintf (stderr, _("%s: PATTERN must have at least one character\n"),
               program_name);
      break;
    case ERR_UNKNOWN_ESCAPE:
      fprintf (stderr, _("%s: %s: unknown escape sequence in REPLACEMENT\n"),
               program_name, file_name);
      break;
    case ERR_INVALID_REFERENCE:
      fprintf (stderr, _("%s: %s: invalid reference in REPLACEMENT\n"),
               program_name, file_name);
      break;
    case ERR_DUPLICATE_OPTION:
      fprintf (stderr, _("%s: duplicate PATTERN or REPLACEMENT option\n"),
               program_name);
      break;
    case ERR_ALLOC_SUFFIX:
      fprintf (stderr, _("%s: could not allocate memory for suffix: "),
               program_name);
      print_errno (saved_errno);
      break;
    case ERR_ALLOC_BUFFER:
      fprintf (stderr, _("%s: could not allocate memory for buffer: "),
               program_name);
      print_errno (saved_errno);
      break;
    case ERR_ALLOC_FILEBUFFER:
      fprintf (stderr,
               _("%s: %s: could not allocate memory for file_buffer: "),
               program_name, file_name);
      print_errno (saved_errno);
      break;
    case ERR_ALLOC_FILELIST:
      fprintf (stderr, _("%s: could not allocate memory for file_list: "),
               program_name);
      print_errno (saved_errno);
      break;
    case ERR_ALLOC_PATTERN:
      fprintf (stderr, _("%s: could not allocate memory for pattern: "),
               program_name);
      print_errno (saved_errno);
      break;
    case ERR_ALLOC_REPLACEMENT:
      fprintf (stderr, _("%s: could not allocate memory for replacement: "),
               program_name);
      print_errno (saved_errno);
      break;
    case ERR_ALLOC_BACKUP:
      fprintf (stderr, _("\
%s: %s: could not allocate memory for backup string: "), program_name,
               file_name);
      print_errno (saved_errno);
      break;
    case ERR_ALLOC_VERBOSE:
      fprintf (stderr, _("\
%s: %s: could not allocate memory for verbose buffer: "), program_name,
               file_name);
      print_errno (saved_errno);
      break;
    case ERR_REALLOC_BUFFER:
      fprintf (stderr, _("%s: %s: could not reallocate memory for buffer: "),
               program_name, file_name);
      print_errno (saved_errno);
      break;
    case ERR_REALLOC_FILEBUFFER:
      fprintf (stderr, _("\
%s: %s: could not reallocate memory for file_buffer: "), program_name,
               file_name);
      print_errno (saved_errno);
      break;
    case ERR_OPEN_READ:
      fprintf (stderr, _("%s: %s: could not open file for reading: "),
               program_name, file_name);
      print_errno (saved_errno);
      break;
    case ERR_OPEN_WRITE:
      fprintf (stderr, _("%s: %s: could not open file for writing: "),
               program_name, file_name);
      print_errno (saved_errno);
      break;
    case ERR_READ_FILE:
      fprintf (stderr, _("%s: %s: could not read file: "),
               program_name, file_name);
      print_errno (saved_errno);
      break;
    case ERR_READ_TEMP:
      fprintf (stderr, _("%s: %s: could not read temporary file: "),
               program_name, file_name);
      print_errno (saved_errno);
      break;
    case ERR_WRITE_FILE:
      fprintf (stderr, _("%s: %s: could not write file: "),
               program_name, file_name);
      print_errno (saved_errno);
      break;
    case ERR_WRITE_TEMP:
      fprintf (stderr, _("%s: %s: could not write temporary file: "),
               program_name, file_name);
      print_errno (saved_errno);
      break;
    case ERR_WRITE_BACKUP:
      fprintf (stderr, _("%s: %s: could not write backup file: "),
               program_name, file_name);
      print_errno (saved_errno);
      break;
    case ERR_OVERWRITE:
      fprintf (stderr, _("%s: %s: could not overwrite file: "),
               program_name, file_name);
      print_errno (saved_errno);
      break;
    case ERR_FILE_REPLACED:
      fprintf (stderr, _("%s: %s: file replaced during processing\n"),
               program_name, file_name);
      break;
    case ERR_KEEP_TIMES:
      fprintf (stderr, _("%s: %s: could not keep file times: "),
               program_name, file_name);
      print_errno (saved_errno);
      break;
    }
}

/* Prints a regerror error message.  */
void
print_regerror (const int errcode, regex_t *compiled)
{
  size_t len;
  char *message;

  if (options & OPT_NO_MESSAGES)
    return;

  len = regerror (errcode, compiled, NULL, 0);
  message = malloc (len);
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
  printf (_("%s: pattern replaced\n"), file_name);
}

/* Prints a given file name.  */
void
print_file_name (const char *file_name)
{
  printf ("%s\n", file_name);
}

/* Prints a header for a changed file.  */
void
print_file_header (const char *file_name)
{
  bool color_flag;

  color_flag = isatty (fileno (stdout));
  printf ("%s--- %s\n+++ %s%s\n", color_flag ? ANSI_BOLD : "", file_name,
          file_name, color_flag ? ANSI_RESET : "");
}

/* Prints replacement details for a line.  */
void
print_line_replacement (const size_t line_num, const char *old_line,
                        size_t old_len, const char *new_line, size_t new_len)
{
  bool color_flag;
  size_t i, line_counter;
  size_t next_start; /* Start offset of next line to print.  */

  color_flag = isatty (fileno (stdout));

  /* Do not print line delimiters.  */
  if (old_len > 0
      && (old_line[old_len - 1] == '\n' || old_line[old_len - 1] == '\0'))
    old_len--;
  if (new_len > 0
      && (new_line[new_len - 1] == '\n' || new_line[new_len - 1] == '\0'))
    new_len--;

  /* Count and print number of new lines.  */
  line_counter = 1;
  for (i = 0; i < new_len; i++)
    if (new_line[i] == '\n')
      line_counter++;
  if (color_flag)
    fputs (ANSI_CYAN, stdout);
  if (line_counter > 1)
    printf ("@@ -%lu +%lu,%lu @@", (unsigned long) line_num,
            (unsigned long) line_num, (unsigned long) line_counter);
  else
    printf ("@@ -%lu +%lu @@", (unsigned long) line_num,
            (unsigned long) line_num);
  if (color_flag)
    fputs (ANSI_RESET, stdout);
  putchar ('\n');

  /* Print removed line.  */
  if (color_flag)
    fputs (ANSI_RED, stdout);
  putchar ('-');
  fwrite (old_line, sizeof (char), old_len, stdout);
  if (color_flag)
    fputs (ANSI_RESET, stdout);
  putchar ('\n');

  /* Print added lines.  */
  next_start = 0;
  for (i = 0; i <= new_len; i++)
    if (i == new_len || new_line[i] == '\n')
      {
        if (color_flag)
          fputs (ANSI_GREEN, stdout);
        putchar ('+');
        fwrite (new_line + next_start, sizeof (char), i - next_start, stdout);
        if (color_flag)
          fputs (ANSI_RESET, stdout);
        putchar ('\n');
        next_start = i + 1;
      }
}

/* Prints the simulation message.  */
void
print_dry (void)
{
  if (options & OPT_DRY && !(options & OPT_QUIET))
    {
      printf (_("Simulation mode: no files are modified.\n"));
      if (!(options & OPT_VERBOSE))
        printf (_("PATTERN found in the following files:\n"));
    }
}

/* Prompts user before modification.  */
bool
prompt_user (const char *file_name)
{
  /* TRANSLATORS: this is a user prompt.  In English the user can answer with y
     for 'yes' or n for 'no'.  [y/n] should be translated as well.  */
  printf (_("Pattern found in %s.  Replace pattern [y/n]? "), file_name);
  fflush (stdout);

  return yesno ();
}
