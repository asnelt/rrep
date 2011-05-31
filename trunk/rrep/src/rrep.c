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
#include <fnmatch.h>
#include <utime.h>
#include <locale.h>
#include "config.h"
#include "rrep.h"
#include "messages.h"
#include "bufferio.h"
#include "pattern.h"
#include "gettext.h"

/* Program invocation name.  */
char *invocation_name = NULL;
/* Option flags set by arguments.  */
int options = 0;

/* Writes string to fp or file_buffer and reallocates memory of file_buffer if
   necessary. *pos points to the end of the written string.  */
inline int
write_string (FILE *fp, const char *string, const size_t string_len,
	      const char *file_name, char **pos)
{
  char *tmp;

  if (fp == NULL)
    {
      /* Check if remaining file_buffer space is sufficient.  */
      while (file_buffer_size-(*pos-file_buffer) < string_len)
	{
	  /* Reallocate memory.  */
	  tmp = realloc (file_buffer, file_buffer_size+INIT_BUFFER_SIZE);
	  if (tmp == NULL)
	    {
	      rrep_error (ERR_REALLOC_FILEBUFFER, file_name);
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

/* Writes the replacement to fp or file_buffer. *pos points to the end
   of the written string.  */
inline int
write_replacement (FILE *fp, const char *start, const regmatch_t *match,
		   const replace_t *replacement, const char *file_name,
		   char **pos)
{
  int failure_flag = FALSE;
  size_t i;
  
  if (options & OPT_FIXED)
    {
      /* REPLACEMENT is a fixed string.  */
      failure_flag |= write_string (fp, replacement->string,
				    replacement->string_len, file_name, pos);
    }
  else
    {
      failure_flag |= write_string (fp, replacement->part[0],
				    replacement->part_len[0], file_name, pos);
      for (i = 0; i < replacement->nsub; i++)
	{
	  /* Match for next index available?  */
	  if (match[replacement->sub[i]].rm_so > -1)
	    failure_flag |=
	      write_string (fp, start + match[replacement->sub[i]].rm_so,
			    match[replacement->sub[i]].rm_eo
			    - match[replacement->sub[i]].rm_so, file_name, pos);
	  failure_flag |= write_string (fp, replacement->part[i+1],
					replacement->part_len[i+1], file_name,
					pos);
	}
    }

  return failure_flag;
}

/* Copies in to out and replaces pattern by replacement.  */
int
replace_string (FILE *in, FILE *out, pattern_t *pattern,
		const replace_t *replacement, const char *file_name,
		size_t *file_len)
{
  size_t line_len;
  char *line, *start, *pos;
  int rr; /* Return value of read_line.  */
  int errcode; /* Return value of regexec.  */
  regmatch_t match[10]; /* Matched regular expression.  */
  int last_empty_flag; /* Last regular expression had zero length.  */
  int break_flag; /* Signals break of while loop.  */
  char tmp_c; /* Buffer for a single character.  */

  if (out == NULL)
    {
      /* Try to use file_buffer instead of tmpfile.  */
      if (file_buffer == NULL)
        {
	  file_buffer = (char *) malloc (INIT_BUFFER_SIZE * sizeof (char));
	  if (file_buffer == NULL)
            {
	      rrep_error (ERR_ALLOC_FILEBUFFER, file_name);
	      return FAILURE;
            }
	  file_buffer_size = INIT_BUFFER_SIZE;
        }
      /* Current position in file_buffer.  */
      pos = file_buffer;
    }

  line = NULL;
  /* Copy in to out with replaced string.  */
  while ((rr = read_line (in, &line, &line_len, file_name)) == SUCCESS)
    {
      start = line;
      last_empty_flag = TRUE;
      /* Search for regular expression or pattern string.  */
      while ((errcode = match_pattern (pattern, line, start, match)) == 0)
        {
	  break_flag = (*start == '\0');
	  if (break_flag && start > line && *(start-1) == '\n')
	    break;

	  if (match[0].rm_eo > 0)
	    {
	      /* Save character in tmp_c.  */
	      tmp_c = *(start+match[0].rm_so);
	      /* Write beginning of line before matched pattern.  */
	      *(start+match[0].rm_so) = '\0';
	      if (write_string (out, start, match[0].rm_so, file_name, &pos)
		  != SUCCESS)
		return FAILURE;
	      /* Restore character from tmp_c.  */
	      *(start+match[0].rm_so) = tmp_c;
	    }
	  if (last_empty_flag || match[0].rm_eo > 0)
	    if (write_replacement (out, start, match, replacement, file_name,
				   &pos) != SUCCESS)
	      return FAILURE;

	  if (break_flag)
	    break;

	  if (match[0].rm_eo == 0)
            {
	      /* Found string has zero length.  */
	      if (write_string (out, start, 1, file_name, &pos) != SUCCESS)
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
      if (errcode != 0 && errcode != REG_NOMATCH)
        {
	  print_regerror (errcode, pattern->compiled);
	  return FAILURE;
        }
      /* Flush rest of line into out or file_buffer.  */
      if (write_string (out, start, line_len-(start-line), file_name, &pos)
	  != SUCCESS)
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

/* Make a backup of the opened file in to file_name~.  */
int
backup_file (FILE *in, const char *file_name)
{
  size_t len, line_len;
  char *backup_name;
  FILE *out;

  /* Generate string for back file name.  */
  len = strlen (file_name);
  backup_name = (char *) malloc ((len + 2) * sizeof (char));
  if (backup_name == NULL)
    {
      rrep_error (ERR_ALLOC_BACKUP, file_name);
      return FAILURE;
    }
  strcpy (backup_name, file_name);
  strcat (backup_name, "~");
  /* Create backup file.  */
  out = fopen (backup_name, "w");
  if (out == NULL)
    {
      rrep_error (ERR_OPEN_WRITE, file_name);
      if (backup_name != NULL)
	free (backup_name);
      return FAILURE;
    }
  if (backup_name != NULL)
    {
      free (backup_name);
      backup_name = NULL;
    }
  /* Copy file.  */
  while (!feof (in))
    {
      line_len = fread (buffer, sizeof (char), buffer_size, in);
      if (line_len != buffer_size && ferror (in))
	{
	  rrep_error (ERR_READ_FILE, file_name);
	  fclose (out);
	  return FAILURE;
	}
      if (fwrite (buffer, sizeof (char), line_len, out) != line_len)
	{
	  rrep_error (ERR_WRITE_BACKUP, file_name);
	  fclose (out);
	  return FAILURE;
	}
    }
  fclose (out);

  return SUCCESS;
}

/* Replace pattern by replacement in the file file_name.  */
int
process_file (const char *relative_path, const char *file_name,
	      pattern_t *pattern, const replace_t *replacement)
{
  FILE *fp, *tmp;
  char *line;
  size_t line_len, file_len;
  regmatch_t match[10]; /* Matched regular expression.  */
  int rr; /* Return value of read_line.  */
  int errcode; /* Return value of regexec.  */
  size_t path_len;
  int found_flag; /* Flag for pattern found.  */

  fp = fopen (file_name, "r");
  if (fp == NULL)
    {
      rrep_error (ERR_OPEN_READ, file_name);
      return FAILURE;
    }

  /* Check whether file file_name contains pattern at all.  */
  found_flag = FALSE;
  line = NULL;
  while ((!found_flag || !(options & OPT_BINARY))
	 && (rr = read_line (fp, &line, &line_len, file_name))
	 == SUCCESS)
    {
      if (!(options & OPT_BINARY))
	{
	  /* Check whether file is binary.  */
	  if (memchr (line, '\0', line_len) != NULL)
	    {
	      /* Null character found, cancel search.  */
	      fclose (fp);
	      return SUCCESS;
	    }
	}
      errcode = match_pattern (pattern, line, line, match);
      if (errcode == 0)
	found_flag = TRUE;
      else if (errcode != REG_NOMATCH)
        {
	  print_regerror (errcode, pattern->compiled);
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
      if (options & OPT_DRY && !(options & OPT_QUIET))
	{
	  printf ("%s\n", file_name);
	  fclose (fp);
	  return SUCCESS;
	}

      if (options & OPT_PROMPT)
	{
	  if (prompt_user (file_name) != SUCCESS)
	    {
	      fclose (fp);
	      return SUCCESS;
	    }
	}

      if (options & OPT_BACKUP)
	{
	  rewind (fp);
	  if (backup_file (fp, file_name) != SUCCESS)
	    {
	      fclose (fp);
	      return FAILURE;
	    }
	}

      rewind (fp);
      tmp = tmpfile ();
      /* Copy f to tmp or file_buffer with replaced string.  */
      if (replace_string (fp, tmp, pattern, replacement, file_name, &file_len))
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
	  rrep_error (ERR_OPEN_WRITE, file_name);
	  if (tmp != NULL)
	    fclose (tmp);
	  return FAILURE;
        }
      if (tmp == NULL)
        {
	  /* Use file_buffer.  */
	  if (fwrite (file_buffer, sizeof (char), file_len, fp) != file_len)
            {
	      rrep_error (ERR_OVERWRITE, file_name);
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
	      line_len = fread (buffer, sizeof (char), buffer_size, tmp);
	      if (line_len != buffer_size && ferror (tmp))
                {
		  rrep_error (ERR_READ_TEMP, file_name);
		  fclose (fp);
		  fclose (tmp);
		  return FAILURE;
                }
	      if (fwrite (buffer, sizeof (char), line_len, fp) != line_len)
                {
		  rrep_error (ERR_OVERWRITE, file_name);
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
	  print_confirmation (file_name);
	}
    }
  fclose (fp);

  return SUCCESS;
}

/* Checks the include and exclude options and returns TRUE if file_name
   qualifies.  */
int
check_include_name (const char *file_name, const char *included_name,
		    const char *excluded_name)
{
  /* Check include_string.  */
  if (included_name != NULL && fnmatch (included_name, file_name, 0) != 0)
    return FALSE;
  /* Check exclude_string.  */
  if (excluded_name != NULL && fnmatch (excluded_name, file_name, 0)
      != FNM_NOMATCH)
    return FALSE;

  return TRUE;
}

/* Returns TRUE if file_name qualifies for processing.  */
int
check_name (const char *file_name, const char *included_name,
	    const char *excluded_name)
{
  if (file_name == NULL || file_name[0] == '\0')
    return FALSE;
  /* Check --all option.  */
  if (file_name[0] == '.' && file_name[1] != '\0'
      && !(options & OPT_ALL))
    return FALSE;

  return check_include_name (file_name, included_name, excluded_name);
}

/* Processes the current directory and all subdirectories recursively.  */
int
process_dir (const char *relative_path, pattern_t *pattern,
	     const replace_t *replacement, const char *include_string,
	     const char *exclude_string, const char *exclude_dir_string)
{
  DIR *d; /* Current directory.  */
  struct dirent *entry; /* Directory entry.  */
  struct stat st; /* The stat for obtaining file times.  */
  struct utimbuf times; /* File times.  */
  int times_saved; /* Flag for time keeping.  */
  int failure_flag;
  size_t path_len = strlen (relative_path);
  size_t next_path_len;
  char *next_path = NULL; /* The relative_path with directory names
			     attached.  */

  failure_flag = FALSE;
  d = opendir (".");
  if (d == NULL)
    {
      rrep_error (ERR_OPEN_DIR, NULL);
      return FAILURE;
    }

  while ((entry = readdir (d)))
    {
      if (entry->d_type == DT_REG
	  && check_name (entry->d_name, include_string, exclude_string))
	{
	  /* The entry is a regular file.  */
	  if (options & OPT_KEEP_TIMES)
	    {
	      /* Obtain file times.  */
	      if (lstat (entry->d_name, &st) < 0)
		{
		  rrep_error (ERR_KEEP_TIMES, entry->d_name);
		  failure_flag = TRUE;
		  times_saved = FALSE;
		}
	      else
		{
		  times.actime = st.st_atime;
		  times.modtime = st.st_mtime;
		  times_saved = TRUE;
		}
	    }
	  failure_flag |= process_file (relative_path, entry->d_name, pattern,
					replacement);
	  if (options & OPT_KEEP_TIMES && times_saved)
	    {
	      /* Restore file times.  */
	      if (utime (entry->d_name, &times) != 0)
		{
		  rrep_error (ERR_KEEP_TIMES, entry->d_name);
		  failure_flag = TRUE;
		}
	    }
	}
      else if (entry->d_type == DT_DIR) /* The entry is a directory.  */
        {
	  if (options & OPT_RECURSIVE && strcmp (entry->d_name, ".")
	      && strcmp (entry->d_name, "..")
	      && check_name (entry->d_name, NULL, exclude_dir_string))
            {
	      if (options & OPT_KEEP_TIMES)
		{
		  /* Obtain file times.  */
		  if (lstat (entry->d_name, &st) < 0)
		    {
		      rrep_error (ERR_KEEP_TIMES, entry->d_name);
		      failure_flag = TRUE;
		      times_saved = FALSE;
		    }
		  else
		    {
		      times.actime = st.st_atime;
		      times.modtime = st.st_mtime;
		      times_saved = TRUE;
		    }
		}
	      /* Recurse into directory.  */
	      if (!chdir (entry->d_name))
                {
		  next_path_len = path_len + strlen (entry->d_name);
		  next_path = (char *) malloc ((next_path_len + 2)
					       * sizeof (char));
		  if (next_path == NULL)
		    {
		      rrep_error (ERR_ALLOC_PATHBUFFER, entry->d_name);
		      return FAILURE;
		    }
		  strcpy (next_path, relative_path);
		  if (path_len == 0 || relative_path[path_len-1] != '/')
		    strcat (next_path, "/");
		  strcat (next_path, entry->d_name);
		  failure_flag |= process_dir (next_path, pattern, replacement,
					       include_string, exclude_string,
					       exclude_dir_string);
		  if (next_path != NULL)
		    {
		      free (next_path);
		      next_path = NULL;
		    }
		  if (chdir (".."))
		    {
		      rrep_error (ERR_PROCESS_DIR, entry->d_name);
		      return FAILURE;
		    }
		  if (options & OPT_KEEP_TIMES && times_saved)
		    {
		      /* Restore file times.  */
		      if (utime (entry->d_name, &times) != 0)
			{
			  rrep_error (ERR_KEEP_TIMES, entry->d_name);
			  failure_flag = TRUE;
			}
		    }
                }
	      else
                {
		  rrep_error (ERR_PROCESS_DIR, entry->d_name);
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
		   pattern_t *pattern, const replace_t *replacement,
		   const char *include_string, const char *exclude_string,
		   const char *exclude_dir_string)
{
  struct stat st; /* The stat for obtaining file type.  */
  struct utimbuf times; /* File times.  */
  int wd; /* File descriptor for current working directory.  */
  int i;
  int omit_dir_flag = FALSE;
  int failure_flag = FALSE;

  /* Save current working directory.  */
  wd = open (".", O_RDONLY);
  omit_dir_flag = (wd < 0);
  if (omit_dir_flag)
    {
      rrep_error (ERR_SAVE_DIR, NULL);
      failure_flag = TRUE;
    }
  
  /* Process file list.  */
  for (i = 0; i < file_counter; i++)
    {
      if (lstat (file_list[i], &st) < 0)
	{
	  rrep_error (ERR_PROCESS_ARG, file_list[i]);
	  failure_flag = TRUE;
	  continue;
	}

      if (S_ISDIR (st.st_mode)) /* The st is a directory.  */
	{
	  if (omit_dir_flag)
	    {
	      print_dir_skip (file_list[i]);
	      continue;
	    }
	  if (!check_include_name (file_list[i], NULL, exclude_dir_string))
	    continue;
	  if (options & OPT_KEEP_TIMES)
	    {
	      /* Save file times.  */
	      times.actime = st.st_atime;
	      times.modtime = st.st_mtime;
	    }
	  if (!chdir (file_list[i]))
	    {
	      failure_flag |= process_dir (file_list[i], pattern, replacement,
					   include_string, exclude_string,
					   exclude_dir_string);
	      /* Return to working directory.  */
	      if (fchdir (wd))
		{
		  rrep_error (ERR_PROCESS_DIR, file_list[i]);
		  close (wd);
		  return FAILURE;
		}
	      if (options & OPT_KEEP_TIMES)
		{
		  /* Restore file times.  */
		  if (utime (file_list[i], &times) != 0)
		    {
		      rrep_error (ERR_KEEP_TIMES, file_list[i]);
		      failure_flag = TRUE;
		    }
		}
	    }
	  else
	    {
	      rrep_error (ERR_PROCESS_DIR, file_list[i]);
	      failure_flag = TRUE;
	    }
	}
      else if (S_ISREG (st.st_mode) && check_include_name (file_list[i],
							   include_string,
							   exclude_string))
	{
	  if (options & OPT_KEEP_TIMES)
	    {
	      /* Save file times.  */
	      times.actime = st.st_atime;
	      times.modtime = st.st_mtime;
	    }
	  /* The st is a regular file.  */
	  failure_flag |= process_file (NULL, file_list[i], pattern,
					replacement);
	  if (options & OPT_KEEP_TIMES)
	    {
	      /* Restore file times.  */
	      if (utime (file_list[i], &times) != 0)
		{
		  rrep_error (ERR_KEEP_TIMES, file_list[i]);
		  failure_flag = TRUE;
		}
	    }
	}
    }
  close (wd);

  return failure_flag;
}

/* Parses command line arguments and processes file list.  */
int
main (int argc, char** argv)
{
  const char *pattern_string = NULL; /* Regular expression to search for.  */
  const char *replacement_string = NULL; /* Replacement string.  */
  const char *include_string = NULL; /* File names to be included.  */
  const char *exclude_string = NULL; /* File names to be excluded.  */
  const char *exclude_dir_string = NULL; /* Directory names to be excluded.  */
  pattern_t pattern; /* Pattern struct.  */
  replace_t replacement; /* Replacement struct.  */
  char **file_list; /* List of files to process.  */
  size_t file_counter = 0; /* Counter for number of files.  */
  int i;
  int cflags = 0; /* Flags for regcomp.  */
  int failure_flag = FALSE;

#if ENABLE_NLS
  /* Initialization of gettext.  */
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);
#endif

  /* Initialize pattern.  */
  pattern.string = NULL;
  pattern.compiled = NULL;
  /* Initialize replacement.  */
  replacement.string = NULL;
  replacement.sub = NULL;
  replacement.part = NULL;
  replacement.part_len = NULL;
  /* Set program invocation name.  */
  invocation_name = argv[0];

  /* Allocate memory for file list.  */
  file_list = (char **) malloc (argc * sizeof (char *));
  if (file_list == NULL)
    {
      rrep_error (ERR_ALLOC_FILELIST, NULL);
      return EXIT_FAILURE;
    }
  /* Parse command line arguments.  */
  for (i = 1; i < argc; i++)
    {
      if (!(strcmp (argv[i], "-a") && strcmp (argv[i], "--all")))
	{
	  options |= OPT_ALL;
	}
      else if (!strcmp (argv[i], "--backup"))
	{
	  options |= OPT_BACKUP;
	}
      else if (!strcmp (argv[i], "--binary"))
	{
	  options |= OPT_BINARY;
	}
      else if (!strcmp (argv[i], "--dry-run"))
	{
	  options |= OPT_DRY;
	}
      else if (!(strcmp (argv[i], "-E")
		 && strcmp (argv[i], "--extended-regexp")))
	{
	  cflags |= REG_EXTENDED;
	}
      else if (!strcmp (argv[i], "-e"))
	{
	  i++;
	  if (i >= argc || pattern_string != NULL)
	    {
	      print_invocation ();
	      if (file_list != NULL)
		free (file_list);
	      return EXIT_FAILURE;
	    }
	  pattern_string = argv[i];
	}
      else if (!strncmp (argv[i], "--regexp=", 9))
	{
	  if (pattern_string != NULL)
	    {
	      print_invocation ();
	      if (file_list != NULL)
		free (file_list);
	      return EXIT_FAILURE;
	    }
	  pattern_string = argv[i]+9;
	}
      else if (!strncmp (argv[i], "--exclude=", 10))
	{
	  exclude_string = argv[i]+10;
	}
      else if (!strncmp (argv[i], "--exclude-dir=", 14))
	{
	  exclude_dir_string = argv[i]+14;
	}
      else if (!(strcmp (argv[i], "-F") && strcmp (argv[i], "--fixed-strings")))
	{
	  options |= OPT_FIXED;
	}
      else if (!(strcmp (argv[i], "-h") && strcmp (argv[i], "--help")))
	{
	  print_help ();
	  if (file_list != NULL)
	    free (file_list);
	  return EXIT_SUCCESS;
	}
      else if (!(strcmp (argv[i], "-i") && strcmp (argv[i], "--ignore-case")))
	{
	  cflags |= REG_ICASE;
	}
      else if (!strncmp (argv[i], "--include=", 10))
	{
	  include_string = argv[i]+10;
	}
      else if (!(strcmp (argv[i], "--keep-times")))
	{
	  options |= OPT_KEEP_TIMES;
	}
      else if (!strcmp (argv[i], "-p"))
	{
	  i++;
	  if (i >= argc || replacement_string != NULL)
	    {
	      print_invocation ();
	      if (file_list != NULL)
		free (file_list);
	      return EXIT_FAILURE;
	    }
	  replacement_string = argv[i];
	}
      else if (!strncmp (argv[i], "--replace-with=", 15))
	{
	  if (replacement_string != NULL)
	    {
	      print_invocation ();
	      if (file_list != NULL)
		free (file_list);
	      return EXIT_FAILURE;
	    }
	  replacement_string = argv[i]+15;
	}
      else if (!(strcmp (argv[i], "-q") && strcmp (argv[i], "--quiet")
		 && strcmp (argv[i], "--silent")))
	{
	  options |= OPT_QUIET;
	}
      else if (!(strcmp (argv[i], "--interactive")))
	{
	  options |= OPT_PROMPT;
	}
      else if (!(strcmp (argv[i], "-R") && strcmp (argv[i], "-r")
		 && strcmp (argv[i], "--recursive")))
	{
	  options |= OPT_RECURSIVE;
	}
      else if (!(strcmp (argv[i], "-s") && strcmp (argv[i], "--no-messages")))
	{
	  options |= OPT_NO_MESSAGES;
	}
      else if (!(strcmp (argv[i], "-V") && strcmp (argv[i], "--version")))
	{
	  print_version ();
	  if (file_list != NULL)
	    free (file_list);
	  return EXIT_SUCCESS;
	}
      else if (!(strcmp (argv[i], "-w") && strcmp (argv[i], "--word-regexp")))
	{
	  options |= OPT_WHOLE_WORD;
	}
      else if (!(strcmp (argv[i], "-x") && strcmp (argv[i], "--line-regexp")))
	{
	  options |= OPT_WHOLE_LINE;
	}
      else
	{
	  if (pattern_string == NULL)
	    pattern_string = argv[i];
	  else if (replacement_string == NULL)
	    replacement_string = argv[i];
	  else
	    {
	      /* Add argument to file_list.  */
	      file_list[file_counter] = argv[i];
	      file_counter++;
	    }
	}
    }

  if (pattern_string == NULL || replacement_string == NULL)
    {
      print_invocation ();
      if (file_list != NULL)
	free (file_list);
      return EXIT_FAILURE;
    }

  /* Allocate initial memory for buffer.  */
  buffer = (char *) malloc (INIT_BUFFER_SIZE * sizeof (char));
  if (buffer == NULL)
    {
      rrep_error (ERR_ALLOC_BUFFER, NULL);
      if (file_list != NULL)
	free (file_list);
      return EXIT_FAILURE;
    }
  buffer_size = INIT_BUFFER_SIZE;

  /* Parse pattern string.  */
  if (pattern_parse (pattern_string, &pattern, cflags) == FAILURE)
    {
      if (file_list != NULL)
	free (file_list);
      if (buffer != NULL)
	free (buffer);
      return EXIT_FAILURE;
    }

  /* Parse replacement string.  */
  if (replace_parse (replacement_string, &replacement) == FAILURE)
    {
      if (file_list != NULL)
	free (file_list);
      if (buffer != NULL)
	free (buffer);
      pattern_free (&pattern);
      return FAILURE;
    }

  /* Replace pattern in file.  */
  if (file_counter == 0 || (file_counter == 1 && !strcmp (file_list[0], "-")))
    {
      /* Default input from stdin and output stdout.  */
      failure_flag |= replace_string (stdin, stdout, &pattern, &replacement,
				      "stdin", NULL);
    }
  else
    {
      print_dry ();
      failure_flag |= process_file_list (file_list, file_counter, &pattern,
					 &replacement, include_string,
					 exclude_string, exclude_dir_string);
    }

  replace_free (&replacement);
  pattern_free (&pattern);
  if (file_buffer != NULL)
    free (file_buffer);
  if (buffer != NULL)
    free (buffer);
  if (file_list != NULL)
    free (file_list);

  if (failure_flag)
    return EXIT_FAILURE;

  return EXIT_SUCCESS;
}