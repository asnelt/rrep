/* bufferio.h - declarations for buffer functions for rrep.
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

/* Pointer to buffer.  */
extern char *buffer;
/* Size of buffer.  */
extern size_t buffer_size;

/* Pointer to buffer for tmpfile replacement.  */
extern char *file_buffer;
/* Size of file_buffer.  */
extern size_t file_buffer_size;

/* Pointer to buffer for verbose line.  */
extern char *verbose_buffer;
/* Size of verbose_buffer.  */
extern size_t verbose_buffer_size;
/* Number of characters in verbose_buffer.  */
extern size_t verbose_buffer_fill;

/* Reads in a buffered line from fp.  The line starts at *line and has
   length *line_len.  Line delimiters are '\n' and, if binary files are not
   ignored, '\0'.  If a line could be placed at the line pointer, SUCCESS is
   returned.  Otherwise, if the end of file was reached END_REACHED is returned
   or if an error occurred FAILURE is returned.  */
extern int read_line (FILE *, char **, size_t *, const char *);

/* Appends a string to the current string in the verbose_buffer.  If needed,
   the size of the verbose_buffer is reallocated.  */
extern int append_verbose (const char *, const size_t, const char *);

