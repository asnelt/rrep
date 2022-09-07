/* rrep.c - source file of rrep, a search and replace utility.
   Copyright (C) 2011, 2013, 2022 Arno Onken <asnelt@asnelt.org>

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
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <regex.h>
#include <utime.h>
#include <locale.h>
#include <getopt.h>
#include "gettext.h"
#include "progname.h"
#include "backupfile.h"
#include "copy-file.h"
#include "exclude.h"
#include "rrep.h"
#include "messages.h"
#include "bufferio.h"
#include "pattern.h"

static char const short_options[] = "EFRrS:Vabe:hip:qswx";

/* Long options that have no equivalent short option.  */
enum
{
  INCLUDE_OPTION = CHAR_MAX + 1,
  EXCLUDE_OPTION,
  EXCLUDE_DIR_OPTION,
  BINARY_OPTION,
  DRY_RUN_OPTION,
  KEEP_TIMES_OPTION,
  INTERACTIVE_OPTION
};

/* Long options equivalences.  */
static struct option const long_options[] =
{
  {"extended-regexp", no_argument, NULL, 'E'},
  {"fixed-strings", no_argument, NULL, 'F'},
  {"recursive", no_argument, NULL, 'R'},
  {"recursive", no_argument, NULL, 'r'},
  {"include", required_argument, NULL, INCLUDE_OPTION},
  {"exclude", required_argument, NULL, EXCLUDE_OPTION},
  {"exclude-dir", required_argument, NULL, EXCLUDE_DIR_OPTION},
  {"suffix", required_argument, NULL, 'S'},
  {"version", no_argument, NULL, 'V'},
  {"all", no_argument, NULL, 'a'},
  {"backup", optional_argument, NULL, 'b'},
  {"binary", no_argument, NULL, BINARY_OPTION},
  {"dry-run", no_argument, NULL, DRY_RUN_OPTION},
  {"regex", required_argument, NULL, 'e'},
  {"help", no_argument, NULL, 'h'},
  {"ignore-case", no_argument, NULL, 'i'},
  {"keep-times", no_argument, NULL, KEEP_TIMES_OPTION},
  {"replace-with", no_argument, NULL, 'p'},
  {"interactive", no_argument, NULL, INTERACTIVE_OPTION},
  {"quiet", no_argument, NULL, 'q'},
  {"silent", no_argument, NULL, 'q'},
  {"no-messages", no_argument, NULL, 's'},
  {"word-regexp", no_argument, NULL, 'w'},
  {"line-regexp", no_argument, NULL, 'x'},
  {NULL, 0, NULL, 0}
};

static struct exclude *included_patterns = NULL;
static struct exclude *excluded_patterns = NULL;
static struct exclude *excluded_directory_patterns = NULL;

enum backup_type backup_method = no_backups;

/* Option flags set by arguments.  */
int options = 0;

/* Writes string to fp or file_buffer and reallocates memory of file_buffer if
   necessary.  *pos points to the end of the written string.  */
static inline int
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

/* Writes the replacement to fp or file_buffer.  *pos points to the end
   of the written string.  */
static inline int
write_replacement (FILE *fp, const char *start, const regmatch_t *match,
                   const replace_t *replacement, const char *file_name,
                   char **pos)
{
  bool failure_flag = false;
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

  if (failure_flag)
    return FAILURE;

  return SUCCESS;
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
  bool last_empty_flag; /* Last regular expression had zero length.  */
  bool break_flag; /* Signals break of while loop.  */
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
      last_empty_flag = true;
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
              last_empty_flag = true;
            }
          else
            {
              start = start + match[0].rm_eo;
              last_empty_flag = false;
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

/* Make a backup of the file.  */
int
backup_file (const char *file_name)
{
  char *backup_name;

  if (backup_method == no_backups)
    return SUCCESS;

  /* Generate string for backup file name.  */
  backup_name = find_backup_file_name (AT_FDCWD, file_name, backup_method);
  if (backup_name == NULL)
    {
      rrep_error (ERR_ALLOC_BACKUP, file_name);
      return FAILURE;
    }
  /* Create backup file.  */
  copy_file_preserving (file_name, backup_name);
  free (backup_name);

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
  bool found_flag; /* Flag for pattern found.  */

  fp = fopen (file_name, "r");
  if (fp == NULL)
    {
      rrep_error (ERR_OPEN_READ, file_name);
      return FAILURE;
    }

  /* Check whether file file_name contains pattern at all.  */
  found_flag = false;
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
        found_flag = true;
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
          if (prompt_user (file_name) == false)
            {
              fclose (fp);
              return SUCCESS;
            }
        }

      if (options & OPT_BACKUP)
        {
          if (backup_file (file_name) != SUCCESS)
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

/* Checks the include and exclude options and returns true if file_name
   qualifies.  */
bool
check_include_name (const char *file_name, const struct exclude *included_name,
                    const struct exclude *excluded_name)
{
  /* Check include option.  */
  if (included_name && excluded_file_name (included_name, file_name))
    return false;
  /* Check excluded option.  */
  if (excluded_name && excluded_file_name (excluded_name, file_name))
    return false;

  return true;
}

/* Returns true if file_name qualifies for processing.  */
bool
check_name (const char *file_name, const struct exclude *included_name,
            const struct exclude *excluded_name)
{
  if (file_name == NULL || file_name[0] == '\0')
    return false;
  /* Check --all option.  */
  if (file_name[0] == '.' && file_name[1] != '\0'
      && !(options & OPT_ALL))
    return false;

  return check_include_name (file_name, included_name, excluded_name);
}

/* Processes the current directory and all subdirectories recursively.  */
int
process_dir (const char *relative_path, pattern_t *pattern,
             const replace_t *replacement)
{
  DIR *d; /* Current directory.  */
  struct dirent *entry; /* Directory entry.  */
  struct stat st; /* The stat for obtaining file times.  */
  struct utimbuf times; /* File times.  */
  bool times_saved; /* Flag for time keeping.  */
  bool failure_flag;
  size_t path_len = strlen (relative_path);
  size_t next_path_len;
  char *next_path = NULL; /* The relative_path with directory names
                             attached.  */

  failure_flag = false;
  d = opendir (".");
  if (d == NULL)
    {
      rrep_error (ERR_OPEN_DIR, NULL);
      return FAILURE;
    }

  while ((entry = readdir (d)))
    {
      if (entry->d_type == DT_REG
          && check_name (entry->d_name, included_patterns, excluded_patterns))
        {
          /* The entry is a regular file.  */
          if (options & OPT_KEEP_TIMES)
            {
              /* Obtain file times.  */
              if (lstat (entry->d_name, &st) < 0)
                {
                  rrep_error (ERR_KEEP_TIMES, entry->d_name);
                  failure_flag = true;
                  times_saved = false;
                }
              else
                {
                  times.actime = st.st_atime;
                  times.modtime = st.st_mtime;
                  times_saved = true;
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
                  failure_flag = true;
                }
            }
        }
      else if (entry->d_type == DT_DIR) /* The entry is a directory.  */
        {
          if (options & OPT_RECURSIVE && strcmp (entry->d_name, ".")
              && strcmp (entry->d_name, "..")
              && check_name (entry->d_name, NULL, excluded_directory_patterns))
            {
              if (options & OPT_KEEP_TIMES)
                {
                  /* Obtain file times.  */
                  if (lstat (entry->d_name, &st) < 0)
                    {
                      rrep_error (ERR_KEEP_TIMES, entry->d_name);
                      failure_flag = true;
                      times_saved = false;
                    }
                  else
                    {
                      times.actime = st.st_atime;
                      times.modtime = st.st_mtime;
                      times_saved = true;
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
                  failure_flag |= process_dir (next_path, pattern, replacement);
                  free (next_path);
                  next_path = NULL;
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
                          failure_flag = true;
                        }
                    }
                }
              else
                {
                  rrep_error (ERR_PROCESS_DIR, entry->d_name);
                  failure_flag = true;
                }
            }
        }
    }

  if (failure_flag)
    return FAILURE;

  return SUCCESS;
}

/* Processes the file_counter files in file_list.  */
int
process_file_list (char **file_list, const size_t file_counter,
                   pattern_t *pattern, const replace_t *replacement)
{
  struct stat st; /* The stat for obtaining file type.  */
  struct utimbuf times; /* File times.  */
  int wd; /* File descriptor for current working directory.  */
  int i;
  bool omit_dir_flag = false;
  bool failure_flag = false;

  /* Save current working directory.  */
  wd = open (".", O_RDONLY);
  omit_dir_flag = (wd < 0);
  if (omit_dir_flag)
    {
      rrep_error (ERR_SAVE_DIR, NULL);
      failure_flag = true;
    }

  /* Process file list.  */
  for (i = 0; i < file_counter; i++)
    {
      if (lstat (file_list[i], &st) < 0)
        {
          rrep_error (ERR_PROCESS_ARG, file_list[i]);
          failure_flag = true;
          continue;
        }

      if (S_ISDIR (st.st_mode)) /* The st is a directory.  */
        {
          if (omit_dir_flag)
            {
              print_dir_skip (file_list[i]);
              continue;
            }
          if (!check_include_name (file_list[i], NULL,
                                   excluded_directory_patterns))
            continue;
          if (options & OPT_KEEP_TIMES)
            {
              /* Save file times.  */
              times.actime = st.st_atime;
              times.modtime = st.st_mtime;
            }
          if (!chdir (file_list[i]))
            {
              failure_flag |= process_dir (file_list[i], pattern, replacement);
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
                      failure_flag = true;
                    }
                }
            }
          else
            {
              rrep_error (ERR_PROCESS_DIR, file_list[i]);
              failure_flag = true;
            }
        }
      else if (S_ISREG (st.st_mode) && check_include_name (file_list[i],
                                                           included_patterns,
                                                           excluded_patterns))
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
                  failure_flag = true;
                }
            }
        }
    }
  close (wd);

  if (failure_flag)
    return FAILURE;

  return SUCCESS;
}

/* Parses command line arguments and processes file list.  */
int
main (int argc, char** argv)
{
  const char *pattern_string = NULL; /* Regular expression to search for.  */
  const char *replacement_string = NULL; /* Replacement string.  */
  char *suffix_string = NULL; /* Suffix for backups.  */
  char *version_control = NULL; /* Version control for backups.  */
  pattern_t pattern; /* Pattern struct.  */
  replace_t replacement; /* Replacement struct.  */
  char **file_list; /* List of files to process.  */
  size_t file_counter = 0; /* Counter for number of files.  */
  int i, opt;
  int cflags = 0; /* Flags for regcomp.  */
  bool failure_flag = false;
  bool exit_flag = false;

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
  set_program_name (argv[0]);

  /* Initialize backup suffix.  */
  suffix_string = getenv ("SIMPLE_BACKUP_SUFFIX");

  /* Parse command line arguments.  */
  while ((opt = getopt_long (argc, argv, short_options, long_options, NULL))
         != -1)
    {
      switch (opt)
        {
        case 'E':
          cflags |= REG_EXTENDED;
          break;

        case 'F':
          options |= OPT_FIXED;
          break;

        case 'R':
        case 'r':
          options |= OPT_RECURSIVE;
          break;

        case INCLUDE_OPTION:
          if (!included_patterns)
            included_patterns = new_exclude ();
          add_exclude (included_patterns, optarg,
                       EXCLUDE_WILDCARDS | EXCLUDE_INCLUDE);
          break;

        case EXCLUDE_OPTION:
          if (!excluded_patterns)
            excluded_patterns = new_exclude ();
          add_exclude (excluded_patterns, optarg, EXCLUDE_WILDCARDS);
          break;

        case EXCLUDE_DIR_OPTION:
          if (!excluded_directory_patterns)
            excluded_directory_patterns = new_exclude ();
          add_exclude (excluded_directory_patterns, optarg, EXCLUDE_WILDCARDS);
          break;

        case 'V':
          print_version ();
          exit_flag = true;
          break;

        case 'S':
          options |= OPT_BACKUP;
          suffix_string = optarg;
          break;

        case 'a':
          options |= OPT_ALL;
          break;

        case 'b':
          options |= OPT_BACKUP;
          if (optarg)
            version_control = optarg;
          break;

        case BINARY_OPTION:
          options |= OPT_BINARY;
          break;

        case DRY_RUN_OPTION:
          options |= OPT_DRY;
          break;

        case 'e':
          if (pattern_string != NULL)
            {
              failure_flag = true;
            }
          else
            pattern_string = optarg;
          break;

        case 'h':
          print_help ();
          exit_flag = true;
          break;

        case 'i':
          cflags |= REG_ICASE;
          break;

        case KEEP_TIMES_OPTION:
          options |= OPT_KEEP_TIMES;
          break;

        case 'p':
          if (replacement_string != NULL)
            {
              failure_flag = true;
            }
          else
            replacement_string = optarg;
          break;

        case INTERACTIVE_OPTION:
          options |= OPT_PROMPT;
          break;

        case 'q':
          options |= OPT_QUIET;
          break;

        case 's':
          options |= OPT_NO_MESSAGES;
          break;

        case 'w':
          options |= OPT_WHOLE_WORD;
          break;

        case 'x':
          options |= OPT_WHOLE_LINE;
          break;

        default:
          failure_flag = true;
          break;

        }
    }

  if (failure_flag)
    {
      print_invocation ();
      return EXIT_FAILURE;
    }

  if (exit_flag)
    return EXIT_SUCCESS;

  if (suffix_string)
    {
      /* Make a copy of suffix_string, because getenv might overwrite the
         memory.  */
      suffix_string = strdup (suffix_string);
      if (suffix_string == NULL)
        {
          rrep_error (ERR_ALLOC_SUFFIX, NULL);
          return EXIT_FAILURE;
        }
      simple_backup_suffix = suffix_string;
    }

  if (options & OPT_BACKUP)
    backup_method = xget_version ("backup type", version_control);
  else
    backup_method = no_backups;

  /* Allocate memory for file list.  */
  file_list = (char **) malloc ((argc-optind) * sizeof (char *));
  if (file_list == NULL)
    {
      rrep_error (ERR_ALLOC_FILELIST, NULL);
      return EXIT_FAILURE;
    }
  /* Parse remaining arguments.  */
  for (i = optind; i < argc; i++)
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

  if (pattern_string == NULL || replacement_string == NULL)
    {
      print_invocation ();
      if (suffix_string != NULL)
        free (suffix_string);
      if (file_list != NULL)
        free (file_list);
      return EXIT_FAILURE;
    }

  /* Allocate initial memory for buffer.  */
  buffer = (char *) malloc (INIT_BUFFER_SIZE * sizeof (char));
  if (buffer == NULL)
    {
      rrep_error (ERR_ALLOC_BUFFER, NULL);
      if (suffix_string != NULL)
        free (suffix_string);
      if (file_list != NULL)
        free (file_list);
      return EXIT_FAILURE;
    }
  buffer_size = INIT_BUFFER_SIZE;

  /* Parse pattern string.  */
  if (parse_pattern (pattern_string, &pattern, cflags) == FAILURE)
    {
      if (file_list != NULL)
        free (file_list);
      if (buffer != NULL)
        free (buffer);
      return EXIT_FAILURE;
    }

  /* Parse replacement string.  */
  if (parse_replace (replacement_string, &replacement) == FAILURE)
    {
      if (suffix_string != NULL)
        free (suffix_string);
      if (file_list != NULL)
        free (file_list);
      if (buffer != NULL)
        free (buffer);
      free_pattern (&pattern);
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
                                         &replacement);
    }

  if (included_patterns)
    free_exclude (included_patterns);
  if (excluded_patterns)
    free_exclude (excluded_patterns);
  if (excluded_directory_patterns)
    free_exclude (excluded_directory_patterns);
  free_replace (&replacement);
  free_pattern (&pattern);
  if (file_buffer)
    free (file_buffer);
  if (buffer)
    free (buffer);
  if (file_list)
    free (file_list);
  if (suffix_string != NULL)
    free (suffix_string);

  if (failure_flag)
    return EXIT_FAILURE;

  return EXIT_SUCCESS;
}
