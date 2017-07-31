// Matthew Kerr
// April 21, 2014

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "linkedList.h"
#include "utility.h"

void sigtstp_handler(int sig);

int main()
{
    signal(SIGTSTP, sigtstp_handler);

    int i = 0;
    int history_index = 0;
    int can_execute = 1;
    int cd_success = 0;
    FILE * fp = NULL;
    Node * history_head;
    createList(&history_head);
    char temp[101];
    char shell_name[100];
    char current_directory[1000];
    char history_filename[1024];
    char user_input[101];
    char last_executed_command[101];
    char * temp_cd;
    Command * com;

    strcpy(shell_name, "KERR SHELL");
    strcpy(last_executed_command, "");

    // find location ofhistory file
    getcwd(current_directory, sizeof(current_directory));
    strcpy(history_filename, current_directory);
    strcat(history_filename, "/");
    strcat(history_filename, ".ssh_history");

    // read in history if it exists
    if ((fp = fopen(history_filename, "r")) != NULL)
    {
        // history file found
        processHistory(fp, &history_head);
        fclose(fp);
        fp = NULL;
    }

    // start main loop
    while (1)
    {

        // get current directory
        getcwd(current_directory, sizeof(current_directory));

        // make sure input is 100 characters or less
        printf("%s: %s$ ", shell_name, current_directory);
        fgets(temp, 1000, stdin);
        while (strlen(temp) > 100)
        {
            fprintf(stderr, "%s: Max input length is 100\n", shell_name);
            printf("%s: %s$ ", shell_name, current_directory);
            fgets(temp, 1000, stdin);
        }
        strcpy(user_input, temp);
        strip(user_input);
        strcpy(temp, "");

        // add to history
        // will not write "exit" or "history"
        if (checkHistoryAdd(user_input, &history_head))
        {
            addLast(&history_head, user_input);

            // write history file
            if ((fp = fopen(history_filename, "w")) != NULL)
            {
                writeHistory(fp, &history_head);
                fclose(fp);
                fp = NULL;

            }
            else
            {
                fprintf(stderr, "%s: Error writing to .ssh_history\n", shell_name);
            }
        }

        // check history cases

        // check if user enters "!!"
        if (strcmp(user_input, "!!") == 0)
        {
            // check if there has been no !! entered this session
            if (strcmp(last_executed_command, "") != 0)
            {
                strcpy(user_input, last_executed_command);
            }
            // check if there is history in the history linked list
            else if (history_head->length >= 1)
            {
                strcpy(user_input, findHistory(&history_head, history_head->length));
            }
            // no history at all
            else
            {
                fprintf(stderr, "%s: No history to go back to\n", shell_name);
                strcpy(user_input, "");
            }

        }
        else if (isAnyHistoryCommand(user_input))
        {
            if ((history_index = isValidHistoryCommand(user_input, history_head->length)) != 0)
            {
                // check if user enters a "!num" command
                strcpy(user_input, findHistory(&history_head, history_index));
            }
            else
            {
                fprintf(stderr, "%s: You don't have that many history commands\n", shell_name);
                strcpy(user_input, "");
            }

        }

        if (user_input[0] == 'c' && user_input[1] == 'd' && user_input[2] == ' ')
        {
            if (isValidCdCommand(user_input))
            {
                // user wants to change directory
                i = 3;
                while (user_input[i] != '\0')
                {
                    user_input[i-3] = user_input[i];
                    i++;
                }
                user_input[i-3] = '\0';
                chdir(user_input);
                strcpy(user_input, "");
            }
            else
            {
                // user entered a poorly structured cd command
                fprintf(stderr, "%s: cd command format should be: cd <destination>\n", shell_name);
                strcpy(user_input, "");
            }
        }



        // check all execution cases
        if (strcmp(user_input, "exit") == 0)
        {
            // check if user enters "exit"
            break;
        }
        else if (strcmp(user_input, "history") == 0)
        {
            // check if user enters "history"
            printList(&history_head);
        }

        else if (isValidCommand(user_input))
        {
            //have valid input
            // create struct command
            com = NULL;
            createCommandStruct(&com, user_input);

            //printCommandStruct(com);
            executeCommand(com);
            strcpy(last_executed_command, user_input);
            clearCommandStruct(com);
        }

    }

    // clean up history allocs
    clearList(&history_head);
}

void sigtstp_handler(int sig)
{
    printf("\nCtrl-Z detected.  Exiting shell.\n");
    exit(-1);
}
