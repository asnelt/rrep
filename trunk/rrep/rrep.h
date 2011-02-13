/* rrep.h - header file of rrep, a search and replace utility.
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

#define PROGRAM_NAME "rrep"
#define VERSION "1.1.4"

/* Initial size of the buffer for reading lines.  */
#define INIT_BUFFER_SIZE (4096)

/* Option flag definitions.  */
#define OPT_QUIET       0x01 /* Do not print regular messages.  */
#define OPT_RECURSIVE   0x02 /* Recurse into directories.  */
#define OPT_NO_MESSAGES 0x04 /* Do not print error messages.  */

/* Boolean values.  */
enum
  {
    FALSE = 0,
    TRUE = 1
  };


/* Processing constants.  */
enum
  {
    SUCCESS = 0,
    FAILURE = 1,
    END_REACHED = 2
  };
