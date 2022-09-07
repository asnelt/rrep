/* bufferio.c - buffer routines for rrep.
   Copyright (C) 2011, 2022 Arno Onken <asnelt@asnelt.org>

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
#include <regex.h>
#include "rrep.h"
#include "messages.h"

/* Pointer to buffer.  */
char *buffer = NULL;
/* Size of buffer.  */
size_t buffer_size = 0;

/* Pointer to buffer for tmpfile replacement.  */
char *file_buffer = NULL;
/* Size of file_buffer.  */
size_t file_buffer_size = 0;


/* Read in a buffered line from fp.  The line starts at *line and has
   length *line_len.  Line delimiters are '\n' and, if binary files are not
   ignored, '\0'.  If a line could be placed at the line pointer, SUCCESS is
   returned.  Otherwise, if the end of file was reached END_REACHED is returned
   or if an error occurred FAILURE is returned.  */
int
read_line (FILE *fp, char **line, size_t *line_len, const char *file_name)
{
  static size_t start = 0; /* Start of line.  */
  static size_t search_pos = 1; /* Search position for end of line.  */
  static size_t buffer_fill = 0; /* Number of read characters in buffer.  */
  static char null_replace = '\0'; /* Character buffer for string
                                      termination.  */
  char *tmp;
  size_t nr; /* Number of characters read by fread.  */
  int i;
  bool search_flag;

  *line_len = 0;
  if (*line == NULL)
    {
      /* New file.  */
      start = 0;
      search_pos = 1;
      buffer_fill = 0;
      /* Fill complete buffer.  */
      nr = fread (buffer, sizeof (char), buffer_size-1, fp);
      if (nr != buffer_size-1 && ferror (fp))
        {
          rrep_error (ERR_READ_FILE, file_name);
          fclose (fp);
          return FAILURE;
        }
      buffer_fill = nr;
    }
  else if (feof (fp) && search_pos >= buffer_fill)
    {
      /* Reset static variables and signal eof.  */
      *line = NULL;
      start = 0;
      search_pos = 1;
      buffer_fill = 0;
      null_replace = '\0';
      return END_REACHED;
    }
  else
    {
      /* Restore character after newline and set start to it.  */
      *(buffer+search_pos) = null_replace;
      start = search_pos;
      search_pos++;
    }

  /* Search for end of line.  */
  search_flag = true;
  while (search_flag)
    {
      while (search_pos < buffer_fill && *(buffer+search_pos-1) != '\n'
             && (!(options & OPT_BINARY) || *(buffer+search_pos-1) != '\0'))
        search_pos++;

      if (search_pos >= buffer_fill && !feof (fp))
        {
          /* End of buffer reached.  */
          if (start > 0)
            {
              /* Let line start at the beginning of buffer.  */
              for (i = 0; i < buffer_fill-start; i++)
                *(buffer+i) = *(buffer+start+i);
              search_pos -= start;

              /* Fill rest of buffer.  */
              nr = fread (buffer+search_pos, sizeof (char),
                          buffer_size-search_pos-1, fp);
              if (nr != buffer_size-search_pos-1 && ferror (fp))
                {
                  rrep_error (ERR_READ_FILE, file_name);
                  fclose (fp);
                  return FAILURE;
                }
              buffer_fill += nr - start;
              start = 0;
            }
          else
            {
              /* Reallocate memory.  */
              tmp = realloc (buffer, buffer_size+INIT_BUFFER_SIZE);
              if (tmp == NULL)
                {
                  rrep_error (ERR_REALLOC_BUFFER, file_name);
                  fclose (fp);
                  return FAILURE;
                }
              buffer = tmp;
              buffer_size += INIT_BUFFER_SIZE;

              /* Fill allocated memory.  */
              nr = fread (buffer+search_pos, sizeof (char),
                          INIT_BUFFER_SIZE, fp);
              if (nr != INIT_BUFFER_SIZE && ferror (fp))
                {
                  rrep_error (ERR_READ_FILE, file_name);
                  fclose (fp);
                  return FAILURE;
                }
              buffer_fill += nr;
            }
        }
      else
        {
          /* End of line found or file complete.  */
          search_flag = false;
        }
    }

  /* Set pointer to line.  */
  *line = buffer+start;
  /* Temporarily replace character after line to generate a terminated
     string.  */
  null_replace = *(buffer+search_pos);
  *(buffer+search_pos) = '\0';
  /* Set line length.  */
  *line_len = search_pos - start;

  return SUCCESS;
}
