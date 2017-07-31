// Matthew Kerr
// April 21, 2014

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "utility.h"
#include "linkedList.h"

void executeCommand(Command * com)
{
    // check number of pipes
    if (com->third_pipe != NULL)
    {
        // two pipe characters
        executeDoublePipe(com);
    }
    else if (com->second_pipe != NULL)
    {
        // one pipe character
        executeSinglePipe(com);
    }
    else
    {
        // no pipe character
        executeNoPipe(com);
    }
}
void executeNoPipe(Command * com)
{
    int status;
    pid_t pid = fork();
    char ** first_command;
    int argc = makeargs(com->first_pipe, &first_command);

    if (pid != 0)
    {
        // parent
        waitpid(pid, &status, 0);
        cleanArgs(argc, first_command);
    }
    else
    {
        // child
        if (com->redirect_in != NULL)
        {
            // there is a redirect in, need to change stdin
            int in;
            in = open(com->redirect_in, O_RDONLY, S_IRWXU);
            if (in >= 0)
            {
                dup2(in, 0);
                close(in);
            }
            else
            {
                fprintf(stderr, "Error: could not open %s for redirection in\n", com->redirect_in);
                exit(-1);
            }
        }
        if (com->redirect_out != NULL)
        {
            // there is a redirect out, need to change stdout
            int out;
            out = open(com->redirect_out, O_WRONLY|O_CREAT, S_IRWXU);
            if (out >= 0)
            {
                dup2(out, 1);
                close(out);
            }
            else
            {
                fprintf(stderr, "Error: could not open %s for redirection out\n", com->redirect_out);
                exit(-1);
            }
        }

        execvp(first_command[0], first_command);
        exit(-1);
    }
}
void executeSinglePipe(Command * com)
{
    int status;

    char ** first_command;
    char ** second_command;
    int argc_first = makeargs(com->first_pipe, &first_command);
    int argc_second = makeargs(com->second_pipe, &second_command);

    pid_t pid = fork();
    pid_t pid2;

    if (pid != 0) // parent
    {
        waitpid(pid, &status, 0);
        cleanArgs(argc_first, first_command);
        cleanArgs(argc_second, second_command);
    }
    else // child
    {
        int fd[2];
        if (pipe(fd) != 0)
        {
            exit(-1);
        }

        pid2 = fork();

        if (pid2 != 0) // child-parent
        {
            // make pipe
            close(fd[1]);
            //close(0);
            dup2(fd[0], 0);
            close(fd[0]);
            execvp(second_command[0], second_command);

            // exec did not work
            exit(-1);
        }
        else // child-child
        {
            // make pipe
            close(fd[0]);
            //close(1);
            dup2(fd[1], 1);
            close(fd[1]);
            execvp(first_command[0], first_command);

            // exec did not work
            exit(-1);
        }
    }
}

void executeDoublePipe(Command * com)
{
    int status;

    char ** first_command;
    char ** second_command;
    char ** third_command;
    int argc_first = makeargs(com->first_pipe, &first_command);
    int argc_second = makeargs(com->second_pipe, &second_command);
    int argc_third = makeargs(com->third_pipe, &third_command);
    int fd_left[2];
    int fd_right[2];
    pid_t pid;
    pid_t pid2;
    pid_t pid3;

    pid = fork();

    if (pid != 0)
    {
        // main grandparent
        waitpid(pid, &status, 0);
        cleanArgs(argc_first, first_command);
        cleanArgs(argc_second, second_command);
        cleanArgs(argc_third, third_command);
    }
    else
    {
        // main grandchild starts here
        if (pipe(fd_right) != 0)
        {
            exit(-1);
        }
        if (pipe(fd_left) != 0)
        {
            exit(-1);
        }

        pid2 = fork();

        if (pid2 != 0)
        {
            // main parent starts here
            // right-most part of the pipe
            close(fd_left[0]);
            close(fd_left[1]);
            close(fd_right[1]);
            dup2(fd_right[0], 0);
            execvp(third_command[0], third_command);

            // exec did not work
            exit(-1);
        }
        else
        {
            // pid2 == 0
            // main child starts here

            // final fork
            pid3 = fork();

            if (pid3 != 0)
            {
                // main child-parent starts here
                // middle part of the pipe
                close(fd_left[1]);
                close(fd_right[0]);
                dup2(fd_left[0], 0);
                dup2(fd_right[1], 1);
                close(fd_left[0]);
                close(fd_right[1]);
                execvp(second_command[0], second_command);

                // exec did not work
                exit(-1);
            }
            else
            {
                // pid3 == 0
                // main child-child starts here
                // left-most part of the pipe
                close(fd_left[0]);
                close(fd_right[0]);
                close(fd_right[1]);
                dup2(fd_left[1], 1);
                close(fd_left[1]);
                execvp(first_command[0], first_command);

                // exec did not work
                exit(-1);
            }
        }
    }
}

int countPipes(char * st)
{
    if (st == NULL)
    {
        return 0;
    }

    int count = 0;
    char temp[100];
    strcpy(temp, st);
    char * token = strtok(temp, " ");
    while (token != NULL)
    {
        if (strcmp(token, "|") == 0)
        {
            count++;
        }
        token = strtok(NULL, " ");
    }
    return count;
}

void createCommandStruct(Command ** com, char * st)
{
    // Command ** com should be sent in as NULL
    // this should only be run when you know are working with a valid command

    // next_destination locations
    // 0 = left part of pipe
    // 1 = middle part of pipe
    // 2 = right part of pipe
    // 3 = redirect in
    // 4 = redirect out
    int next_destination = 0;
    int num_pipes = countPipes(st);
    char * token;
    char temp[100];
    char buf[100];
    strcpy(temp, st);

    *com = (Command *)malloc(sizeof(Command));
    (*com)->first_pipe = NULL;
    (*com)->second_pipe = NULL;
    (*com)->third_pipe = NULL;
    (*com)->redirect_in = NULL;
    (*com)->redirect_out = NULL;

    token = strtok(temp, " ");
    strcpy(buf, "");

    while (1)
    {
        //printf("debuf: token is %s\n", token);
        if (token == NULL)
        {
            addOneCommand(com, buf, next_destination);
            break;
        }
        else if (strcmp(token, "|") != 0 && strcmp(token, "<") != 0 && strcmp(token, ">") != 0)
        {
            // normal word
            if (strcmp(buf, "") != 0)
            {
                strcat(buf, " ");
            }
            strcat(buf, token);
        }
        else if (strcmp(token, "|") == 0)
        {
            addOneCommand(com, buf, next_destination);
            next_destination++;

            // clear buffer
            strcpy(buf, "");
        }
        else if (strcmp(token, "<") == 0)
        {
            addOneCommand(com, buf, next_destination);
            next_destination = 3;

            // clear buffer
            strcpy(buf, "");
        }
        else if (strcmp(token, ">") == 0)
        {
            addOneCommand(com, buf, next_destination);
            next_destination = 4;

            // clear buffer
            strcpy(buf, "");
        }
        else
        {
            fprintf(stderr, "Error: Got something bad when parsing in createCommandStruct\n");
            exit(-1);
        }

        token = strtok(NULL, " ");
    }
    return;

}

void addOneCommand(Command ** com, char * st, int loc)
{
    if (loc == 0)
    {
        // left pipe area
        (*com)->first_pipe = (char *)calloc((strlen(st) + 1), sizeof(char));
        strcpy((*com)->first_pipe, st);
    }
    else if (loc == 1)
    {
        // middle pipe area
        (*com)->second_pipe = (char *)calloc((strlen(st) + 1), sizeof(char));
        strcpy((*com)->second_pipe, st);
    }
    else if (loc == 2)
    {
        // right pipe area
        (*com)->third_pipe = (char *)calloc((strlen(st) + 1), sizeof(char));
        strcpy((*com)->third_pipe, st);
    }
    else if (loc == 3)
    {
        // redirect in
        (*com)->redirect_in = (char *)calloc((strlen(st) + 1), sizeof(char));
        strcpy((*com)->redirect_in, st);
    }
    else if (loc == 4)
    {
        // redirect out
        (*com)->redirect_out = (char *)calloc((strlen(st) + 1), sizeof(char));
        strcpy((*com)->redirect_out, st);
    }
    else
    {
        fprintf(stderr, "Error: Invalid Command location\n");
    }
    return;
}

void clearCommandStruct(Command * com)
{
    if (com == NULL)
    {
        return;
    }
    free(com->first_pipe);
    free(com->second_pipe);
    free(com->third_pipe);
    free(com->redirect_in);
    free(com->redirect_out);
    com->first_pipe = NULL;
    com->second_pipe = NULL;
    com->third_pipe = NULL;
    com->redirect_in = NULL;
    com->redirect_out = NULL;
    free(com);
    com = NULL;
    return;
}

int isValidCharacter(char ch)
{
    // checks if the character is legal in a word for a command
    return ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '-' || ch == '!' || ch == '.' || ch == '/' || ch == '_');
}

int isValidWord(char * st)
{
    // checks if a word in a command is legal by looping through each character
    int i = 0;
    while (st[i] != '\0')
    {
        if (!isValidCharacter(st[i]))
        {
            return 0;
        }
        i++;
    }
    return 1;
}

int isValidCdCommand(char * st)
{
    // user entered a cd command
    // checks to make sure the arguments following cd are legal
    if (st == NULL)
    {
        return 0;
    }
    if (strlen(st) < 4)
    {
        return 0;
    }
    char temp[100];
    strcpy(temp, st);
    char * token = strtok(temp, " ");
    if (strcmp(token, "cd") != 0)
    {
        return 0;
    }
    token = strtok(NULL, " ");
    if (token == NULL)
    {
        return 0;
    }
    token = strtok(NULL, " ");
    if (token != NULL)
    {
        return 0;
    }
    return 1;
}

int isAnyHistoryCommand(char * st)
{
    // used to check if st is a !number command
    if (st == NULL)
    {
        return 0;
    }
    char temp[100];
    char num_temp[100];
    strcpy(temp, st);
    if (strlen(temp) < 2)
    {
        return 0;
    }
    if (temp[0] != '!')
    {
        return 0;
    }
    int i = 1;

    while (temp[i] != '\0')
    {
        if (!(temp[i] >= '0' && temp[i] <= '9'))
        {
            return 0;
        }
        i++;
    }
    return 1;
}

int isValidHistoryCommand(char * st, int length)
{
    // if the command is valid, returns the number given by user
    // otherwise returns 0;
    if (st == NULL)
    {
        return 0;
    }
    char temp[100];
    char num_temp[100];
    strcpy(temp, st);
    if (strlen(temp) < 2)
    {
        return 0;
    }
    if (temp[0] != '!')
    {
        return 0;
    }
    int i = 1;

    while (temp[i] != '\0')
    {
        if (!(temp[i] >= '0' && temp[i] <= '9'))
        {
            return 0;
        }
        i++;
    }

    int j = 0;
    int num = 0;
    i = 1;
    // remove the beginning '!'
    while (temp[i] != '\0')
    {
        temp[i-1] = temp[i];
        i++;
    }
    temp[i-1] = '\0';
    num = atoi(temp);
    if (num < 1 || num > length)
    {
        return 0;
    }
    return num;

}

int isValidCommand(char * st)
{
    // checks if the command is valid for execution
    if (st == NULL)
    {
        return 0;
    }
    if (strcmp(st, "") == 0)
    {
        return 0;
    }
    if (strlen(st) > 100)
    {
        return 0;
    }
    int i = 0;
    int num_pipes = 0;
    int num_redirect_in = 0;
    int num_redirect_out = 0;
    int need_word_next = 0;
    int last_operator = 0; // 0 = no operator, 1 = pipe, 2 = redirect in, 3 = redirect out
    char temp[101];
    char buf[100];
    strcpy(temp, st);
    char * token = strtok(temp, " ");
    strcpy(buf, token);
    if (token == NULL)
    {
        printf("Error: No input\n");
        return 0;
    }
    if (!isValidWord(token))
    {
        printf("Error: First token must be a valid command\n");
        return 0;
    }
    while (token != NULL)
    {
        if (strcmp(token, "|") == 0)
        {
            // test pipe
            if (need_word_next == 1)
            {
                printf("Error: Cannot have pipes and redirects without input between them\n");
                return 0;
            }
            if (num_pipes == 2)
            {
                printf("Error: Can only have a maximum of 2 pipe characters\n");
                return 0;
            }
            else if (num_redirect_in == 1 || num_redirect_out == 1)
            {
                printf("Error: Cannot pipe and redirect in the same line\n");
                return 0;
            }
            else
            {
                strcpy(buf, "");
                need_word_next = 1;
                num_pipes++;
                last_operator = 1;
            }
        }
        else if (strcmp(token, "<") == 0)
        {
            // test <
            if (need_word_next == 1)
            {
                printf("Error: Cannot have pipes and redirects without input between them\n");
                return 0;
            }
            if (num_pipes > 0)
            {
                printf("Error: Cannot pipe and redirect in the same line\n");
                return 0;
            }
            if (num_redirect_in == 1)
            {
                printf("Error: Too many redirect in attempts (<)\n");
                return 0;
            }
            else
            {
                strcpy(buf, "");
                need_word_next = 1;
                num_redirect_in++;
                last_operator = 2;
            }
        }
        else if (strcmp(token, ">") == 0)
        {
            // test >
            if (need_word_next == 1)
            {
                printf("Error: Cannot have pipes and redirects without input between them\n");
                return 0;
            }
            if (num_pipes > 0)
            {
                printf("Error: Cannot pipe and redirect in the same line\n");
                return 0;
            }
            if (1 == num_redirect_out)
            {
                printf("Error: too many redirect out attempts (>)\n");
                return 0;
            }
            else
            {
                strcpy(buf, "");
                need_word_next = 1;
                num_redirect_out++;
                last_operator = 3;
            }
        }
        else if (!isValidWord(token))
        {
            // test any normal command
            printf("Error: %s is not a valid input\n", token);
            return 0;
        }
        else
        {
            // got a valid word
            if (strcmp(buf, "") != 0)
            {
                strcat(buf, " ");
            }
            strcat(buf, token);
            if (num_redirect_in == 1 || num_redirect_out == 1)
            {
                // make sure there is only one word after a redirect in or out
                int i = 0;
                while (buf[i] != '\0')
                {
                    if (buf[i] == ' ')
                    {
                        printf("Error: target file for redirect in and redirect out must be 1 word\n");
                        return 0;
                    }
                    i++;
                }
            }
            need_word_next = 0;
        }
        token = strtok(NULL, " ");
    }

    if (need_word_next == 1)
    {
        printf("Invalid input: Need word after ");
        if (last_operator == 1)
        {
            printf("|\n");
        }
        else if (last_operator == 2)
        {
            printf("<\n");
        }
        else
        {
            printf(">\n");
        }
        return 0;
    }

    return 1;
}

void processHistory(FILE * fp, Node ** head)
{
    if (fp == NULL)
    {
        return;
    }

    char temp[100];
    while (fgets(temp, 100, fp) != NULL)
    {
        strip(temp);
        addLast(head, temp);
    }
    return;
}

void writeHistory(FILE * fp, Node ** head)
{
    // write all history to a file
    if (fp == NULL)
    {
        return;
    }
    if ((*head)->next == NULL)
    {
        return;
    }
    Node * curr = (Node *) (*head)->next;
    while (curr != NULL)
    {
        fprintf(fp, "%s\n", curr->data);
        curr = curr->next;
    }
    return;
}

int checkHistoryAdd(char * user_input, Node ** head)
{
    // check cases which should NEVER be aded
    if ((strcmp(user_input, "exit") == 0) || (strcmp(user_input, "!!") == 0) || (strcmp(user_input, "") == 0) || isAnyHistoryCommand(user_input))
    {
        return 0;
    }

    // if history is empty return 1
    if ((*head)->length == 0)
    {
        return 1;
    }

    // check if user_input matches the last entry in history
    // bash does not add repeat consecutive items in history
    if (strcmp(user_input, findHistory(head, (*head)->length)) == 0)
    {
        return 0;
    }
    return 1;
}

void strip(char * s)
{
  int len;
  len = strlen(s);
  if(s[len - 2] == '\r')
    s[len -2] = '\0';

  else if(s[len - 1] == '\n')
    s[len -1] = '\0';
}

int makeargs(char * s, char *** argv)
{
    // determine argc
    int argc = 0;
	char num_args_temp[100];
    strcpy(num_args_temp, s);

    if (strlen(num_args_temp) > 1)
    {
        strip(num_args_temp);

        if (strtok(num_args_temp, " ") != NULL)
        {
            argc = 1;
            while (strtok(NULL, " ") != NULL)
            {
                argc++;
            }
        }
    }

    // create 2d array
    if ((*argv = (char **)calloc((argc + 1), sizeof(char *))) == NULL)
    {
        printf("Error allocating argv\n");
        return -1;
    }

    if (argc == 0)
    {
        // there were 0 arguments
        return argc;
    }

    int i;
    char temp[100];
    strcpy(temp, s);
    strip(temp);
    char * token = strtok(temp, " ");
    for (i = 0; i < argc; i++)
    {
        if (((*argv)[i] = (char *)calloc((strlen(token) + 1), sizeof(char))) == NULL)
        {
            printf("Error allocating char * for argv[%d]\n", i);
            return -1;
        }
        strcpy((*argv)[i], token);
        token = strtok(NULL, " ");
    }

    // make sure the last index is NULL
    (*argv)[argc] = NULL;
    return argc;

}

void cleanArgs(int argc, char ** argv)
{
    int i;
    for (i = 0; i < argc; i++)
    {
        free(argv[i]);
        argv[i] = NULL;
    }
    free(argv);
    argv = NULL;
}
