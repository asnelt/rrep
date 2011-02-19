/* rrep.c - source file of rrep, a search and replace utility.
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
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <regex.h>
#include "rrep.h"
#include "messages.h"
#include "bufferio.h"

/* Program invocation name.  */
char *invocation_name;

/* Writes string to fp or file_buffer and reallocates memory of
   file_buffer if necessary. *pos points to the end of the written
   string.  */
inline int
write_string (FILE *fp, const char *string, const size_t string_len,
	      const char *file_name, char **pos, const int options)
{
  char *tmp;

  if (fp == NULL)
    {
      /* Check if remaining file_buffer space is sufficient.  */
      while (file_buffer_size-(*pos-file_buffer) < string_len)
	{
	  /* Reallocate memory.  */
	  tmp = realloc (file_buffer,
			 file_buffer_size+INIT_BUFFER_SIZE);
	  if (tmp == NULL)
	    {
	      rrep_error (ERR_REALLOC_FILEBUFFER, file_name, options);
	      return FAILURE;
	    }
	  *pos = tmp + (*pos - file_buffer);
	  file_buffer = tmp;
	  file_buffer_size += INIT_BUFFER_SIZE;
	}
      /* Copy string to file_buffer and increase pos.  */
      memcpy (*pos, string, string_len * sizeof (char));
      *pos += string_len;
    }
  else
    {
      fwrite (string, sizeof (char), string_len, fp);
    }

  return SUCCESS;
}

/* Copies in to out and replaces compiled by string.  */
int
replace_string (FILE *in, FILE *out, regex_t *compiled,
		const char *string, const size_t string_len,
		const char *file_name, size_t *file_len,
		const int options)
{
  size_t line_len;
  char *line, *start, *pos;
  int rr; /* Return value of read_line.  */
  int regerror; /* Return value of regexec.  */
  regmatch_t match[1]; /* Matched regular expression.  */
  int last_empty_flag; /* Last regular expression had zero length.  */
  int break_flag; /* Signals break of while loop.  */

  if (out == NULL)
    {
      /* Try to use file_buffer instead of tmpfile.  */
      if (file_buffer == NULL)
        {
	  file_buffer = (char *) malloc (INIT_BUFFER_SIZE
					 * sizeof (char));
	  if (file_buffer == NULL)
            {
	      rrep_error (ERR_ALLOC_FILEBUFFER, file_name, options);
	      return FAILURE;
            }
	  file_buffer_size = INIT_BUFFER_SIZE;
        }
      /* Current position in file_buffer.  */
      pos = file_buffer;
    }

  line = NULL;
  /* Copy in to out with replaced string.  */
  while ((rr = read_line (in, &line, &line_len, file_name, options))
	 == SUCCESS)
    {
      start = line;
      last_empty_flag = TRUE;
      /* Search for regular expression.  */
      while ((regerror = regexec (compiled, start, 1, match, 0)) == 0)
        {
	  break_flag = (*start == '\0');
	  if (break_flag && start > line && *(start-1) == '\n')
	    break;

	  if (match[0].rm_eo > 0)
	    *(start+match[0].rm_so) = '\0';

	  if (match[0].rm_eo > 0
	      && write_string (out, start, match[0].rm_so, file_name,
			       &pos, options) != SUCCESS)
	    return FAILURE;
	  if (last_empty_flag || match[0].rm_eo > 0)
	    if (write_string (out, string, string_len, file_name, &pos,
			      options) != SUCCESS)
	      return FAILURE;

	  if (break_flag)
	    break;

	  if (match[0].rm_eo == 0)
            {
	      /* Found string has zero length.  */
	      if (write_string (out, start, 1, file_name,
				&pos, options) != SUCCESS)
		return FAILURE;

	      start++;
	      last_empty_flag = TRUE;
            }
	  else
            {
	      start = start + match[0].rm_eo;
	      last_empty_flag = FALSE;
            }
        }
      if (regerror == REG_ESPACE)
        {
	  rrep_error (ERR_MEMORY, file_name, options);
	  return FAILURE;
        }
      /* Flush rest of line into out or file_buffer.  */
      if (write_string (out, start, line_len-(start-line), file_name,
			&pos, options) != SUCCESS)
	return FAILURE;
    }
  /* Set file_len if we are using file_buffer.  */
  if (out == NULL && file_len != NULL)
    *file_len = pos - file_buffer;

  /* End of file reached?  */
  if (rr != END_REACHED)
    return FAILURE;

  return SUCCESS;
}

/* Replace compiled by string in the file file_name.  */
int
process_file (const char *relative_path, const char *file_name,
	      regex_t *compiled, const char *string,
	      const size_t string_len, const int options)
{
  FILE *fp, *tmp;
  char *line;
  size_t line_len, file_len;
  int rr; /* Return value of read_line.  */
  int regerror; /* Return value of regexec.  */
  size_t path_len;
  int found_flag;

  fp = fopen (file_name, "r");
  if (fp == NULL)
    {
      rrep_error (ERR_OPEN_READ, file_name, options);
      return FAILURE;
    }

  /* First check whether file file_name contains compiled at all.  */
  found_flag = FALSE;
  line = NULL;
  while (!found_flag && (rr = read_line (fp, &line, &line_len,
					 file_name, options))
	 == SUCCESS)
    {
      regerror = regexec (compiled, line, 0, NULL, 0);
      if (regerror == 0)
	found_flag = TRUE;
      else if (regerror == REG_ESPACE)
        {
	  rrep_error (ERR_MEMORY, file_name, options);
	  fclose (fp);
	  return FAILURE;
        }
    }
  if (rr == FAILURE)
    {
      fclose (fp);
      return FAILURE;
    }

  if (found_flag)
    {
      rewind (fp);
      tmp = tmpfile ();
      /* Copy f to tmp or file_buffer with replaced string.  */
      if (replace_string (fp, tmp, compiled, string, string_len,
			  file_name, &file_len, options))
        {
	  fclose (fp);
	  if (tmp != NULL)
	    fclose (tmp);
	  return FAILURE;
        }

      /* Copy from tmp or file_buffer back to f.  */
      fp = freopen (file_name, "w", fp);
      if (fp == NULL)
        {
	  rrep_error (ERR_OPEN_WRITE, file_name, options);
	  if (tmp != NULL)
	    fclose (tmp);
	  return FAILURE;
        }
      if (tmp == NULL)
        {
	  /* Use file_buffer.  */
	  if (fwrite (file_buffer, sizeof (char), file_len, fp)
	      != file_len)
            {
	      rrep_error (ERR_OVERWRITE, file_name, options);
	      fclose (fp);
	      return FAILURE;
            }
        }
      else
        {
	  /* Use tmp.  */
	  rewind (tmp);
	  while (!feof (tmp))
            {
	      line_len = fread (buffer, sizeof (char), buffer_size,
				tmp);
	      if (line_len != buffer_size && ferror (tmp))
                {
		  rrep_error (ERR_READ_TEMP, file_name, options);
		  fclose (fp);
		  fclose (tmp);
		  return FAILURE;
                }
	      if (fwrite (buffer, sizeof (char), line_len, fp)
		  != line_len)
                {
		  rrep_error (ERR_OVERWRITE, file_name, options);
		  fclose (fp);
		  fclose (tmp);
		  return FAILURE;
                }
            }
	  fclose (tmp);
        }
      if (!(options & OPT_QUIET))
	{
	  if (relative_path != NULL)
	    {
	      printf ("%s", relative_path);
	      path_len = strlen (relative_path);
	      if (path_len == 0 || relative_path[path_len-1] != '/')
		printf ("/");
	    }
	  printf ("%s: Pattern replaced\n", file_name);
	}
    }
  fclose (fp);

  return SUCCESS;
}

/* Processes the current directory and all subdirectories
   recursively.  */
int
process_dir (const char *relative_path, regex_t *compiled,
	     const char *string, const size_t string_len,
	     const int options)
{
  DIR *d; /* Current directory.  */
  struct dirent *entry; /* Directory entry.  */
  int failure_flag;
  size_t path_len = strlen (relative_path);
  size_t next_path_len;
  char *next_path = NULL; /* The relative_path with directory names
			     attached.  */

  failure_flag = FALSE;
  d = opendir (".");
  if (d == NULL)
    {
      rrep_error (ERR_OPEN_DIR, NULL, options);
      return FAILURE;
    }

  while ((entry = readdir (d)))
    {
      if (entry->d_type == DT_REG) /* The entry is a regular file.  */
	failure_flag |= process_file (relative_path, entry->d_name,
				      compiled, string, string_len,
				      options);
      else if (entry->d_type == DT_DIR) /* The entry is a
					   directory.  */
        {
	  if (options & OPT_RECURSIVE && strcmp (entry->d_name, ".")
	      && strcmp (entry->d_name, ".."))
            {
	      /* Recurse into directory.  */
	      if (!chdir (entry->d_name))
                {
		  next_path_len = path_len + strlen (entry->d_name);
		  next_path = (char *) malloc ((next_path_len + 2)
					       * sizeof (char));
		  if (next_path == NULL)
		    {
		      rrep_error (ERR_ALLOC_PATHBUFFER, entry->d_name,
				  options);
		      return FAILURE;
		    }
		  strcpy (next_path, relative_path);
		  if (path_len == 0
		      || relative_path[path_len-1] != '/')
		    strcat (next_path, "/");
		  strcat (next_path, entry->d_name);
		  failure_flag |= process_dir (next_path, compiled,
					       string, string_len,
					       options);
		  free (next_path);
		  next_path = NULL;
		  if (chdir (".."))
		    {
		      rrep_error (ERR_PROCESS_DIR, entry->d_name, options);
		      return FAILURE;
		    }
                }
	      else
                {
		  rrep_error (ERR_PROCESS_DIR, entry->d_name, options);
		  failure_flag = TRUE;
                }
            }
        }
    }

  return failure_flag;
}

/* Processes the file_counter files in file_list.  */
int
process_file_list (char **file_list, const size_t file_counter,
		   regex_t *compiled, const char *string,
		   const size_t string_len, const int options)
{
  struct stat st; /* The stat for obtaining file type.  */
  int wd; /* File descriptor for current working directory.  */
  int i;
  int omit_dir_flag = FALSE;
  int failure_flag = FALSE;

  /* Save current working directory.  */
  wd = open (".", O_RDONLY);
  omit_dir_flag = (wd < 0);
  if (omit_dir_flag)
    {
      rrep_error (ERR_SAVE_DIR, NULL, options);
      failure_flag = TRUE;
    }
  
  /* Process file list.  */
  for (i = 0; i < file_counter; i++)
    {
      if (lstat (file_list[i], &st) == -1)
	{
	  rrep_error (ERR_PROCESS_ARG, file_list[i], options);
	  failure_flag = TRUE;
	}

      if (S_ISDIR (st.st_mode)) /* The st is a directory.  */
	{
	  if (omit_dir_flag)
	    {
	      if (!(options & OPT_QUIET))
		printf ("%s: %s: Omitting directory\n",
			invocation_name, file_list[i]);
	      continue;
	    }
	  if (!chdir (file_list[i]))
	    {
	      failure_flag |= process_dir (file_list[i], compiled,
					   string, string_len,
					   options);
	      /* Return to working directory.  */
	      if (fchdir (wd))
		{
		  rrep_error (ERR_PROCESS_DIR, file_list[i], options);
		  close (wd);
		  return FAILURE;
		}
	    }
	  else
	    {
	      rrep_error (ERR_PROCESS_DIR, file_list[i], options);
	      failure_flag = TRUE;
	    }
	}
      else if (S_ISREG (st.st_mode)) /* The st is a regular
					file.  */
	failure_flag |= process_file (NULL, file_list[i], compiled,
				      string, string_len, options);
    }
  close (wd);

  return failure_flag;
}

/* Parses command-line-arguments and processes file list.  */
int
main (int argc, char** argv)
{
  char *pattern = NULL; /* Regular expression to search for.  */
  char *string = NULL; /* Replacement string.  */
  char **file_list; /* List of files to process.  */
  size_t file_counter = 0; /* Counter for number of files.  */
  size_t string_len; /* Length of string.  */
  regex_t compiled; /* Data structure for regular expression.  */
  int errcode; /* Error code for regcomp.  */
  int i;
  int cflags = 0; /* Flags for regcomp.  */
  int options = 0; /* Option flags set by arguments.  */
  int failure_flag = FALSE;

  /* Set program invocation name.  */
  invocation_name = argv[0];

  /* Allocate memory for file list.  */
  file_list = (char **) malloc (argc * sizeof (char *));
  if (file_list == NULL)
    {
      rrep_error (ERR_ALLOC_FILELIST, NULL, options);
      return EXIT_FAILURE;
    }
  /* Parse command line arguments.  */
  for (i = 1; i < argc; i++)
    {
      if (!(strcmp (argv[i], "-E")
		 && strcmp (argv[i], "--extended-regexp")))
	{
	  cflags |= REG_EXTENDED;
	}
      else if (!(strcmp (argv[i], "-h") && strcmp (argv[i], "--help")))
	{
	  print_help ();
	  free (file_list);
	  return EXIT_SUCCESS;
	}
      else if (!(strcmp (argv[i], "-i")
		 && strcmp (argv[i], "--ignore-case")))
	{
	  cflags |= REG_ICASE;
	}
      else if (!(strcmp (argv[i], "-q") && strcmp (argv[i], "--quiet")
		 && strcmp (argv[i], "--silent")))
	{
	  options |= OPT_QUIET;
	}
      else if (!(strcmp (argv[i], "-R") && strcmp (argv[i], "-r")
		 && strcmp (argv[i], "--recursive")))
	{
	  options |= OPT_RECURSIVE;
	}
      else if (!(strcmp (argv[i], "-s")
		 && strcmp (argv[i], "--no-messages")))
	{
	  options |= OPT_NO_MESSAGES;
	}
      else if (!(strcmp (argv[i], "-V")
	    && strcmp (argv[i], "--version")))
	{
	  print_version ();
	  free (file_list);
	  return EXIT_SUCCESS;
	}
      else
	{
	  if (pattern == NULL)
	    pattern = argv[i];
	  else if (string == NULL)
	    string = argv[i];
	  else
	    {
	      /* Add argument to file_list.  */
	      file_list[file_counter] = argv[i];
	      file_counter++;
	    }
	}
    }

  if (pattern == NULL || string == NULL)
    {
      print_usage ();
      printf ("Try `%s --help' for more information.\n",
	      invocation_name);
      free (file_list);
      return EXIT_FAILURE;
    }
  string_len = strlen (string);

  if (strlen (pattern) < 1)
    {
      rrep_error (ERR_PATTERN, NULL, options);
      free (file_list);
      return EXIT_FAILURE;
    }
  /* Compile regular expression.  */
  errcode = regcomp (&compiled, pattern, cflags);
  if (errcode != 0)
    {
      print_regerror (errcode, &compiled);
      free (file_list);
      return EXIT_FAILURE;
    }

  /* Allocate initial memory for buffer.  */
  buffer = (char *) malloc (INIT_BUFFER_SIZE * sizeof (char));
  if (buffer == NULL)
    {
      rrep_error (ERR_ALLOC_BUFFER, NULL, options);
      free (file_list);
      regfree (&compiled);
      return EXIT_FAILURE;
    }
  buffer_size = INIT_BUFFER_SIZE;

  /* Replace pattern in file.  */
  if (file_counter == 0
      || (file_counter == 1 && !strcmp (file_list[0], "-")))
    {
      /* Default input from stdin and output stdout.  */
      failure_flag |= replace_string (stdin, stdout, &compiled, string,
				      string_len, "stdin", NULL,
				      options);
    }
  else
    {
      failure_flag |= process_file_list (file_list, file_counter,
					 &compiled, string,
					 string_len, options);
    }

  if (file_buffer != NULL)
    free (file_buffer);

  free (buffer);
  free (file_list);
  /* Free compiled regular expression.  */
  regfree (&compiled);

  if (failure_flag)
    return EXIT_FAILURE;

  return EXIT_SUCCESS;
}
