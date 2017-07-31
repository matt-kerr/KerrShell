// Matthew Kerr
// April 21, 2014

#ifndef utility_h
#define utlility_h

#include <stdio.h>
#include "linkedList.h"

typedef struct command
{
    char * first_pipe;
    char * second_pipe;
    char * third_pipe;
    char * redirect_in;
    char * redirect_out;
}Command;

void executeCommand(Command * com);
void executeNoPipe(Command * com);
void executeSinglePipe(Command * com);
void executeDoublePipe(Command * com);
int countPipes(char * st);
void createCommandStruct(Command ** com, char * st);
void addOneCommand(Command ** com, char * st, int loc);
void clearCommandStruct(Command * com);
int isValidCharacter(char ch);
int isValidWord(char * st);
int isValidCdCommand(char * st);
int isAnyHistoryCommand(char * st);
int isValidHistoryCommand(char * st, int length);
int isValidCommand(char * st);
void processHistory(FILE * fp, Node ** head);
void writeHistory(FILE * fp, Node ** head);
int checkHistoryAdd(char * user_input, Node ** head);
void strip(char * );
int makeargs(char * s, char *** argv);
void cleanArgs(int argc, char ** argv);

#endif
