#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_ARGS 20
#define BUFFER_SIZE 100

void parser(char *input, char **args);

int main()
{
    pid_t child_pid;
    char input[BUFFER_SIZE];
    char *args[MAX_ARGS];
    
    while(1)
    {
        printf("MyShell-->Enter command: ");
        if(fgets(input, sizeof(input), stdin) == NULL)
            break;

        input[strcspn(input, "\n")] = 0;  //удаляем символ новой строки из инпут
        
        if (strcmp(input, "exit") == 0)
        {
            printf("MyShell-->Exiting the terminal...\n");
            break;
        }

        parser(input, args);

        child_pid = fork();

        if (child_pid == 0)
        {   
            if (execvp(args[0], args) == -1)
            {
                fprintf(stderr, "MyShell-->Command not found: %s\n", args[0]);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            wait(NULL);
        }

    }
    return 0;
}

void parser(char *input, char **args)
{
    int i = 0;
    char *token = strtok(input, " ");
    while ((token != NULL) && (i < MAX_ARGS - 1))
    {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;
}
