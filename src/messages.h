/* messages.h - declarations for message functions for rrep.
   Copyright (C) 2011, 2022, 2026 Arno Onken <asnelt@asnelt.org>

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

#ifndef MESSAGES_H
#define MESSAGES_H

#include <regex.h>
#include <stdbool.h>
#include <stdio.h>

/* Error codes for rrep_error.  */
enum
{
  ERR_PROCESS_ARG, /* Could not process an argument.  */
  ERR_PROCESS_DIR, /* Could not process a directory.  */
  ERR_DIR_NO_RECURSION, /* Directory as argument without recursion.  */
  ERR_STDIN_WITH_FILES, /* Standard in as argument with other files.  */
  ERR_PATTERN, /* Error in PATTERN.  */
  ERR_UNKNOWN_ESCAPE, /* Unknown escape sequence encountered.  */
  ERR_INVALID_REFERENCE, /* Invalid reference in REPLACEMENT.  */
  ERR_DUPLICATE_OPTION, /* Duplicate PATTERN or REPLACEMENT option.  */
  ERR_ALLOC_BUFFER, /* Error for allocating buffer.  */
  ERR_ALLOC_FILEBUFFER, /* Error for allocating file_buffer.  */
  ERR_ALLOC_FILELIST, /* Error for allocating file_list.  */
  ERR_ALLOC_PATTERN, /* Error for allocating pattern.  */
  ERR_ALLOC_REPLACEMENT, /* Error for allocating replacement.  */
  ERR_ALLOC_BACKUP, /* Error for allocating backup string.  */
  ERR_ALLOC_VERBOSE, /* Error for allocating verbose buffer.  */
  ERR_REALLOC_BUFFER, /* Error for reallocating buffer.  */
  ERR_REALLOC_FILEBUFFER, /* Error for reallocating file_buffer.  */
  ERR_OPEN_READ, /* Could not open a file for reading.  */
  ERR_OPEN_WRITE, /* Could not open a file for writing.  */
  ERR_READ_FILE, /* Could not read from a file.  */
  ERR_READ_TEMP, /* Could not read from a temporary file.  */
  ERR_WRITE_FILE, /* Could not write to a file.  */
  ERR_WRITE_TEMP, /* Could not write to a temporary file.  */
  ERR_WRITE_BACKUP, /* Could not write to backup file.  */
  ERR_OVERWRITE, /* Could not overwrite a file.  */
  ERR_FILE_REPLACED, /* File replaced during processing.  */
  ERR_KEEP_TIMES /* Could not keep file times.  */
};

/* Prints version information.  */
extern void print_version (void);

/* Prints usage information to fp.  */
extern void print_usage (FILE *);

/* Prints the help.  */
extern void print_help (void);

/* Prints the program invocation to stderr.  */
extern void print_invocation (void);

/* Prints an error message.  */
extern void rrep_error (const int, const char *);

/* Prints a regerror error message.  */
extern void print_regerror (const int, regex_t *);

/* Prints replacement confirmation.  */
extern void print_confirmation (const char *);

/* Prints a given file name.  */
extern void print_file_name (const char *);

/* Prints a header for a changed file.  */
extern void print_file_header (const char *);

/* Prints replacement details for a line.  */
extern void print_line_replacement (const size_t, const char *, size_t,
                                    const char *, size_t);

/* Prints the simulation message.  */
extern void print_dry (void);

/* Prompts user before modification.  */
extern bool prompt_user (const char *);

#endif /* MESSAGES_H */
