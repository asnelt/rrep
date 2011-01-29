/* rrep.c - source file for rrep.
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
#define VERSION "1.0.0"

#ifndef FALSE
#define FALSE (0)
#endif
#ifndef TRUE
#define TRUE (1)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>


static char *buffer = NULL;
static size_t buffer_len = 0;

void print_version()
{
    printf("%s %s\n\n", PROGRAM_NAME, VERSION);
    printf("Copyright (C) 2011 Arno Onken <asnelt@asnelt.org>\n");
    printf("License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n");
    printf("This is free software: you are free to change and redistribute it.\n");
    printf("There is NO WARRANTY, to the extent permitted by law.\n\n");
}

void print_usage()
{
    printf("Usage: %s STRING1 STRING2 [FILE]...\n", PROGRAM_NAME);
}

void print_help()
{
    print_usage();
    printf("Replace STRING1 by STRING2 in each FILE or standard input.\n");
    printf("Example: %s 'hello world' 'Hello, World!' menu.h main.c\n\n", PROGRAM_NAME);
    printf("Options:\n");
    printf("  -v, --version  print version information and exit\n");
    printf("  -h, --help     display this help and exit\n\n");
    printf("If FILE is a directory, then the complete directory tree of FILE will be\n");
    printf("processed. With no FILE, or when FILE is -, read standard input.\n");
    printf("Exit status is %d if any error occurs, %d otherwise.\n", EXIT_FAILURE, EXIT_SUCCESS);
}

/*
    Checks if the file 'file_name' contains the string 'string' and returns TRUE if the string is
    found or if an error occurs.
*/
int contains_string(const char *file_name, const char *string)
{
    FILE *fp;

    fp = fopen(file_name, "r");
    if (fp == NULL)
    {
        return TRUE;
    }

    while (getline(&buffer, &buffer_len, fp) != -1)
    {
        if (strstr(buffer, string))
        {
            fclose(fp);
            return TRUE;
        }
    }
    fclose(fp);
    return FALSE;
}

/*
    Copies 'in' to 'out' and replaces the string 'string1' by 'string2'.
*/
int replace_string(FILE *in, FILE *out, const char *string1, const char *string2)
{
    size_t string1_len;
    char *start, *next;

    string1_len = strlen(string1);

    /* copy 'in' to 'out' with replaced string */
    while (getline(&buffer, &buffer_len, in) != -1)
    {
        start = buffer;
        while ((next = strstr(start, string1)))
        {
            *next = '\0';
            fputs(start, out);
            start = next + string1_len;
            fputs(string2, out);
        }
        fputs(start, out);
    }
    if (!feof(in))
        return TRUE;

    return FALSE;
}

/*
    Replace the string 'string1' by 'string2' in the file 'file_name'.
*/
int replace_file(const char *file_name, const char *string1, const char *string2)
{
    FILE *fp, *tmp;
    char *start, *next;

    fp = fopen(file_name, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "%s: could not open file '%s' for reading.\n", PROGRAM_NAME, file_name);
        return TRUE;
    }
    tmp = tmpfile();
    if (tmp == NULL)
    {
        fprintf(stderr, "%s: could not create a temporary file.\n", PROGRAM_NAME);
        fclose(fp);
        return TRUE;
    }

    /* copy 'f' to 'tmp' with replaced string */
    if (replace_string(fp, tmp, string1, string2))
    {
        fprintf(stderr, "%s: could not read file '%s'.\n", PROGRAM_NAME, file_name);
        fclose(fp);
        fclose(tmp);
        return TRUE;
    }

    /* copy 'tmp' back to 'f' */
    fclose(fp);
    rewind(tmp);
    fp = fopen(file_name, "w");
    if (fp == NULL)
    {
        fprintf(stderr, "%s: could not open file '%s' for writing.\n", PROGRAM_NAME, file_name);
        fclose(tmp);
        return TRUE;
    }
    while (getline(&buffer, &buffer_len, tmp) != -1)
        fputs(buffer, fp);

    if (!feof(tmp))
    {
        fprintf(stderr, "%s: could not read temporary file.\n", PROGRAM_NAME);
        fclose(fp);
        fclose(tmp);
        return TRUE;
    }

    fclose(fp);
    fclose(tmp);

    return FALSE;
}

/*
    Processes a single file 'file_name'.
*/
int process_file(const char *file_name, const char *string1, const char *string2)
{
    int failure_flag;

    failure_flag = FALSE;
    if (contains_string(file_name, string1))
    {
        failure_flag |= replace_file(file_name, string1, string2);
    }

    return failure_flag;
}

/*
    Processes the current directory and all subdirectories recursively.
*/
int process_dir(const char *string1, const char *string2)
{
    DIR *d; /* current directory */
    struct dirent *entry; /* directory entry */
    int failure_flag;

    failure_flag = FALSE;
    d = opendir(".");
    if (d == NULL)
    {
        fprintf(stderr, "%s: could not open directory.\n", PROGRAM_NAME);
        return TRUE;
    }

    while (entry = readdir(d))
    {
        if (entry->d_type == DT_REG) /* regular file */
            failure_flag |= process_file(entry->d_name, string1, string2);
        else if (entry->d_type == DT_DIR) /* directory */
        {
            if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
            {
                if (!chdir(entry->d_name))
                {
                    failure_flag |= process_dir(string1, string2);
                    chdir("..");
                }
                else
                {
                    fprintf(stderr, "%s: could not process directory '%s'.\n", PROGRAM_NAME, entry->d_name);
                    failure_flag = TRUE;
                }
            }
        }
    }

    return failure_flag;
}

/*
    Parses command-line-arguments, creates a control-object and runs it.
*/
int main(int argc, char** argv)
{
    char *string1, *string2;
    int wd; /* file descriptor for current working directory */
    int i;
    int failure_flag, omit_dir_flag;
    struct stat st;

    /* parse command line arguments */
    if (argc == 2 && !(strcmp(argv[1], "-v") && strcmp(argv[1], "--version")))
    {
        print_version();
        return EXIT_SUCCESS;
    }
    else if (argc == 2 && !(strcmp(argv[1], "-h") && strcmp(argv[1], "--help")))
    {
        print_help();
        return EXIT_SUCCESS;
    }
    else if (argc < 3)
    {
        print_usage();
        printf("Try `%s --help' for more information.\n", PROGRAM_NAME);
        return EXIT_FAILURE;
    }
    string1 = argv[1];
    string2 = argv[2];

    if (strlen(string1) < 1)
    {
        fprintf(stderr, "%s: STRING1 must have at least one character.\n", PROGRAM_NAME);
        return EXIT_FAILURE;
    }

    failure_flag = FALSE;
    /* replace string in file */
    if (argc < 4 || (argc == 4 && !strcmp(argv[3], "-")))
    {
        /* default input from 'stdin' and output 'stdout' */
        if (replace_string(stdin, stdout, string1, string2))
        {
            fprintf(stderr, "%s: could not read stdin.\n", PROGRAM_NAME);
            failure_flag = TRUE;
        }
    }
    else
    {
        /* save current working directory */
        wd = open(".", O_RDONLY);
        omit_dir_flag = (wd < 0);
        if (omit_dir_flag)
        {
            fprintf(stderr, "%s: could not save current working directory.\n", PROGRAM_NAME);
            failure_flag = TRUE;
        }

        /* process file list */
        for (i = 3; i < argc; i++)
        {
            if (lstat(argv[i], &st) == -1)
            {
                fprintf(stderr, "%s: could not process argument '%s'.\n", PROGRAM_NAME, argv[i]);
                failure_flag = TRUE;
            }

            if (S_ISDIR(st.st_mode))
            {
                if (omit_dir_flag)
                {
                    printf("%s: omitting directory '%s'.\n", PROGRAM_NAME, argv[i]);
                    continue;
                }
                if (!chdir(argv[i]))
                {
                    failure_flag |= process_dir(string1, string2);
                    /* return to working directory */
                    fchdir(wd);
                }
                else
                {
                    fprintf(stderr, "%s: could not process directory '%s'.\n", PROGRAM_NAME, argv[i]);
                    failure_flag = TRUE;
                }
            }
            else if (S_ISREG(st.st_mode))
                failure_flag |= process_file(argv[i], string1, string2);
        }
        close(wd);
    }

    free(buffer);

    if (failure_flag)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

