/* messages.h - declarations for message functions for rrep.
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

/* Error codes for rrep_error.  */
enum
  {
    ERR_PROCESS_ARG, /* Could not process an argument.  */
    ERR_PROCESS_DIR, /* Could not process a directory.  */
    ERR_PATTERN, /* Error in PATTERN.  */
    ERR_UNKNOWN_ESCAPE, /* Unknown escape sequence encountered. */
    ERR_SAVE_DIR, /* Could not save a directory.  */
    ERR_ALLOC_BUFFER, /* Error for allocating buffer.  */
    ERR_ALLOC_FILEBUFFER, /* Error for allocating file_buffer.  */
    ERR_ALLOC_FILELIST, /* Error for allocating file_list.  */
    ERR_ALLOC_PATHBUFFER, /* Error for allocating next_path.  */
    ERR_ALLOC_PATTERN, /* Error for allocating pattern.  */
    ERR_ALLOC_REPLACEMENT, /* Error for allocating replacement.  */
    ERR_REALLOC_BUFFER, /* Error for reallocating buffer.  */
    ERR_REALLOC_FILEBUFFER, /* Error for reallocating file_buffer.  */
    ERR_MEMORY, /* Error for insufficient memory.  */
    ERR_OPEN_READ, /* Could not open a file for reading.  */
    ERR_OPEN_WRITE, /* Could not open a file for writing.  */
    ERR_OPEN_DIR, /* Could not open a directory.  */
    ERR_READ_FILE, /* Could not read from a file.  */
    ERR_READ_TEMP, /* Could not read from a temporary file.  */
    ERR_OVERWRITE /* Could not overwrite a file.  */
  };

/* Prints version information.  */
extern void print_version ();

/* Prints usage information.  */
extern void print_usage ();

/* Prints the help.  */
extern void print_help ();

/* Prints an error message.  */
extern void rrep_error (const int, const char *);

/* Prints a regerror error message.  */
extern void print_regerror (const int, regex_t *);
