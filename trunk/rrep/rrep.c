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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>

#define PROGRAM_NAME "rrep"
#define VERSION "1.0.3"

#ifndef FALSE
#define FALSE (0)
#endif
#ifndef TRUE
#define TRUE (1)
#endif

/* initial size of the buffer for reading lines */
#define INIT_BUFFER_SIZE (4096)


/* pointer to buffer */
char *buffer = NULL;
/* size of  buffer */
size_t buffer_size = 0;

/* pointer to buffer for tmpfile replacement */
char *file_buffer = NULL;
/* size of file_buffer */
size_t file_buffer_size = 0;


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
    Read in a buffered line from fp. The line starts at *line and has length *line_len. Line delimiters
    are '\n' and '\0'. If a line could be placed at the line pointer, 0 is returned. Otherwise, if the
    end of file was reached -1 is returned or if an error occurred -2 is returned.
*/
int read_line(FILE *fp, char **line, size_t *line_len, const char *file_name)
{
    static size_t start = 0; /* start of line */
    static size_t search_pos = 1; /* search position for end of line */
    static size_t buffer_fill = 0; /* number of read characters in buffer */
    static char null_replace = '\0'; /* character buffer for string termination */
    char *tmp;
    size_t nr; /* number of characters read by fread */
    int i, search_flag;

    *line_len = 0;
    if (*line == NULL)
    {
        /* new file */
        start = 0;
        search_pos = 1;
        buffer_fill = 0;
        /* fill complete buffer */
        nr = fread(buffer, sizeof(char), buffer_size-1, fp);
        if (nr != buffer_size-1 && ferror(fp))
        {
            fprintf(stderr, "%s: Could not read file %s.\n", PROGRAM_NAME, file_name);
            fclose(fp);
            return -2;
        }
        buffer_fill = nr;
    }
    else if (feof(fp) && search_pos >= buffer_fill)
    {
        /* reset static variables and signal eof */
        *line = NULL;
        start = 0;
        search_pos = 1;
        buffer_fill = 0;
        null_replace = '\0';
        return -1;
    }
    else
    {
        /* restore character after newline and set start to it */
        *(buffer+search_pos) = null_replace;
        start = search_pos;
        search_pos++;
    }

    /* search for end of line */
    search_flag = TRUE;
    while (search_flag)
    {
        while (search_pos < buffer_fill && *(buffer+search_pos-1) != '\n' && *(buffer+search_pos-1) != '\0')
            search_pos++;

        if (search_pos >= buffer_fill && !feof(fp))
        {
            /* end of buffer reached */
            if (start > 0)
            {
                /* let line start at the beginning of buffer */
                for (i = 0; i < buffer_fill-start; i++)
                    *(buffer+i) = *(buffer+start+i);
                search_pos -= start;

                /* fill rest of buffer */
                nr = fread(buffer+search_pos, sizeof(char), buffer_size-search_pos-1, fp);
                if (nr != buffer_size-search_pos-1 && ferror(fp))
                {
                    fprintf(stderr, "%s: Could not read file %s.\n", PROGRAM_NAME, file_name);
                    fclose(fp);
                    return -2;
                }
                buffer_fill += nr - start;
                start = 0;
            }
            else
            {
                /* reallocate memory */
                tmp = realloc(buffer, buffer_size+INIT_BUFFER_SIZE);
                if (tmp == NULL)
                {
                    fprintf(stderr, "%s: Could not reallocate memory for buffer.\n", PROGRAM_NAME);
                    fclose(fp);
                    return -2;
                }
                buffer = tmp;
                buffer_size += INIT_BUFFER_SIZE;

                /* fill allocated memory */
                nr = fread(buffer+search_pos, sizeof(char), INIT_BUFFER_SIZE, fp);
                if (nr != INIT_BUFFER_SIZE && ferror(fp))
                {
                    fprintf(stderr, "%s: Could not read file %s.\n", PROGRAM_NAME, file_name);
                    fclose(fp);
                    return -2;
                }
                buffer_fill += nr;
            }
        }
        else
        {
            /* end of line found of file complete */
            search_flag = FALSE;
        }
    }

    /* set pointer to line */
    *line = buffer+start;
    /* temporarily replace character after line to generate a terminated string */
    null_replace = *(buffer+search_pos);
    *(buffer+search_pos) = '\0';
    /* set line length */
    *line_len = search_pos - start;

    return 0;
}

int write_file_buffer(const char *string, const size_t string_len, char **pos)
{
    char *tmp;

    /* check if remaining file_buffer space is sufficient */
    while (file_buffer_size-(*pos-file_buffer) < string_len)
    {
        /* reallocate memory */
        tmp = realloc(file_buffer, file_buffer_size+INIT_BUFFER_SIZE);
        if (tmp == NULL)
        {
            fprintf(stderr, "%s: Could not reallocate memory for file_buffer.\n", PROGRAM_NAME);
            return 1;
        }
        *pos = tmp + (*pos - file_buffer);
        file_buffer = tmp;
        file_buffer_size += INIT_BUFFER_SIZE;
    }
    /* copy string to file_buffer and increase pos */
    memcpy(*pos, string, string_len*sizeof(char));
    *pos += string_len;

    return 0;
}

/*
    Copies in to out and replaces the string string1 by string2.
*/
int replace_string(FILE *in, FILE *out, const char *string1, const char *string2, const char *file_name, size_t *file_len)
{
    size_t string1_len, line_len;
    char *line, *start, *next, *pos;
    int rr; /* return value of read_line */

    string1_len = strlen(string1);

    if (out == NULL)
    {
        /* try to use file_buffer instead of tmpfile */
        if (file_buffer == NULL)
        {
            file_buffer = (char*)malloc(INIT_BUFFER_SIZE * sizeof(char));
            if (file_buffer == NULL)
            {
                fprintf(stderr, "%s: Could not allocate memory for file_buffer.\n", PROGRAM_NAME);
                return 1;
            }
            buffer_size = INIT_BUFFER_SIZE;
        }
        /* current position in file_buffer */
        pos = file_buffer;
    }

    line = NULL;
    /* copy in to out with replaced string */
    while ((rr = read_line(in, &line, &line_len, file_name)) == 0)
    {
        start = line;
        /* search for next string1 */
        while ((next = strstr(start, string1)))
        {
            *next = '\0';
            if (out == NULL)
            {
                if (write_file_buffer(start, strlen(start), &pos) != 0)
                    return 1;
                if (write_file_buffer(string2, strlen(string2), &pos) != 0)
                    return 1;
            }
            else
            {
                fputs(start, out);
                fputs(string2, out);
            }
            start = next + string1_len;
        }
        /* flush rest of line into out or file_buffer */
        if (out == NULL)
        {
            if (write_file_buffer(start, line_len-(start-line), &pos) != 0)
                return 1;
        }
        else
            fwrite(start, sizeof(char), line_len-(start-line), out);
    }
    /* set file_len if we are using file_buffer */
    if (out == NULL && file_len != NULL)
        *file_len = pos - file_buffer;

    /* end of file reached? */
    if (rr == -1)
        return 0;
    else
        return 1;
}

/*
    Replace the string string1 by string2 in the file file_name.
*/
int process_file(const char *file_name, const char *string1, const char *string2)
{
    FILE *fp, *tmp;
    char *line;
    size_t line_len, file_len;
    int rr; /* return value of read_line */
    int found_flag;

    fp = fopen(file_name, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "%s: Could not open file '%s' for reading.\n", PROGRAM_NAME, file_name);
        return 1;
    }

    /* first check whether file file_name contains string1 at all */
    found_flag = FALSE;
    line = NULL;
    while (!found_flag && (rr = read_line(fp, &line, &line_len, file_name)) == 0)
    {
        if (strstr(line, string1))
            found_flag = TRUE;
    }
    if (rr < -1)
    {
        fclose(fp);
        return 1;
    }

    if (found_flag)
    {
        rewind(fp);
        tmp = tmpfile();
        /* copy f to tmp or file_buffer with replaced string */
        if (replace_string(fp, tmp, string1, string2, file_name, &file_len))
        {
            fclose(fp);
            if (tmp != NULL)
                fclose(tmp);
            return 1;
        }

        /* copy from tmp or file_buffer back to f */
        fp = freopen(file_name, "w", fp);
        if (fp == NULL)
        {
            fprintf(stderr, "%s: Could not open file '%s' for writing.\n", PROGRAM_NAME, file_name);
            if (tmp != NULL)
                fclose(tmp);
            return 1;
        }
        if (tmp == NULL)
        {
            /* use file_buffer */
            if (fwrite(file_buffer, sizeof(char), file_len, fp) != file_len)
            {
                fprintf(stderr, "%s: Could not overwrite file '%s'.\n", PROGRAM_NAME, file_name);
                fclose(fp);
                return 1;
            }
        }
        else
        {
            /* use tmp */
            rewind(tmp);
            while (!feof(tmp))
            {
                line_len = fread(buffer, sizeof(char), buffer_size, tmp);
                if (line_len != buffer_size && ferror(tmp))
                {
                    fprintf(stderr, "%s: Could not read temporary file.\n", PROGRAM_NAME);
                    fclose(fp);
                    fclose(tmp);
                    return 1;
                }
                if (fwrite(buffer, sizeof(char), line_len, fp) != line_len)
                {
                    fprintf(stderr, "%s: Could not overwrite file '%s'.\n", PROGRAM_NAME, file_name);
                    fclose(fp);
                    fclose(tmp);
                    return 1;
                }
            }
            fclose(tmp);
        }
        printf("Replaced in '%s'.\n", file_name);
    }
    fclose(fp);

    return 0;
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
        fprintf(stderr, "%s: Could not open directory.\n", PROGRAM_NAME);
        return 1;
    }

    while ((entry = readdir(d)))
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
                    fprintf(stderr, "%s: Could not process directory '%s'.\n", PROGRAM_NAME, entry->d_name);
                    failure_flag = TRUE;
                }
            }
        }
    }

    return failure_flag;
}

/*
    Parses command-line-arguments and processes file list.
*/
int main(int argc, char** argv)
{
    char *string1, *string2;
    struct stat st; /* stat for obtaining file type */
    int wd; /* file descriptor for current working directory */
    int i;
    int failure_flag, omit_dir_flag;

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

    /* Allocate initial memory for buffer */
    buffer = (char*)malloc(INIT_BUFFER_SIZE * sizeof(char));
    if (buffer == NULL)
    {
        fprintf(stderr, "%s: Could not allocate memory for buffer.\n", PROGRAM_NAME);
        return EXIT_FAILURE;
    }
    buffer_size = INIT_BUFFER_SIZE;

    failure_flag = FALSE;
    /* replace string in file */
    if (argc < 4 || (argc == 4 && !strcmp(argv[3], "-")))
    {
        /* default input from stdin and output stdout */
        if (replace_string(stdin, stdout, string1, string2, "stdin", NULL))
        {
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
            fprintf(stderr, "%s: Could not save current working directory.\n", PROGRAM_NAME);
            failure_flag = TRUE;
        }

        /* process file list */
        for (i = 3; i < argc; i++)
        {
            if (lstat(argv[i], &st) == -1)
            {
                fprintf(stderr, "%s: Could not process argument '%s'.\n", PROGRAM_NAME, argv[i]);
                failure_flag = TRUE;
            }

            if (S_ISDIR(st.st_mode)) /* directory */
            {
                if (omit_dir_flag)
                {
                    printf("%s: Omitting directory '%s'.\n", PROGRAM_NAME, argv[i]);
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
                    fprintf(stderr, "%s: Could not process directory '%s'.\n", PROGRAM_NAME, argv[i]);
                    failure_flag = TRUE;
                }
            }
            else if (S_ISREG(st.st_mode)) /* regular file */
                failure_flag |= process_file(argv[i], string1, string2);
        }
        close(wd);
    }

    free(buffer);
    buffer = NULL;
    free(file_buffer);
    file_buffer = NULL;

    if (failure_flag)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
