/* bufferio.h - declarations for buffer functions for rrep.
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

/* Pointer to buffer.  */
extern char *buffer;
/* Size of  buffer.  */
extern size_t buffer_size;

/* Pointer to buffer for tmpfile replacement.  */
extern char *file_buffer;
/* Size of file_buffer.  */
extern size_t file_buffer_size;


/* Read in a buffered line from fp. The line starts at *line and has
   length *line_len. Line delimiters are '\n' and, if binary files are not
   ignored, '\0'. If a line could be placed at the line pointer, SUCCESS is
   returned. Otherwise, if the end of file was reached END_REACHED is returned
   or if an error occurred FAILURE is returned.  */
extern int read_line (FILE *, char **, size_t *, const char *);
