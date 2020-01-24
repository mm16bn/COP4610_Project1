//COP4610
//Project 1 Starter Code
//example code for initial parsing

//*** if any problems are found with this code,
//*** please report them to the TA

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>


int numCommands = 0;

typedef struct
{
	char** tokens;
	int numTokens;
} instruction;

void addToken(instruction* instr_ptr, char* tok);
void printTokens(instruction* instr_ptr);
void clearInstruction(instruction* instr_ptr);
void addNull(instruction* instr_ptr);
void expandEnv(instruction* instr_ptr);
void pathResolution(instruction* instr_ptr);
void printPrompt();
void piping(instruction* instr_ptr);
void ioRedirection(instruction* instr_ptr);
void builtIns(instruction* instr_ptr);

int main() {
	char* token = NULL;
	char* temp = NULL;
	instruction instr;
	instr.tokens = NULL;
	instr.numTokens = 0;

	while (1) {
        printPrompt();

        // loop reads character sequences separated by whitespace
		do {
			//scans for next token and allocates token var to size of scanned token
			scanf("%ms", &token);
			temp = (char*)malloc((strlen(token) + 1) * sizeof(char));

			int i;
			int start = 0;
			for (i = 0; i < strlen(token); i++) {
				//pull out special characters and make them into a separate token in the instruction
				if (token[i] == '|' || token[i] == '>' || token[i] == '<' || token[i] == '&') {
					if (i-start > 0) {
						memcpy(temp, token + start, i - start);
						temp[i-start] = '\0';
						addToken(&instr, temp);

					}

					char specialChar[2];
					specialChar[0] = token[i];
					specialChar[1] = '\0';

					addToken(&instr,specialChar);

					start = i + 1;

				}
			}

			if (start < strlen(token)) {
				memcpy(temp, token + start, strlen(token) - start);
				temp[i-start] = '\0';
				addToken(&instr, temp);
			}

			//free and reset variables
			free(token);
			free(temp);

			token = NULL;
			temp = NULL;
		} while ('\n' != getchar());    //until end of line is reached

		numCommands++;
		addNull(&instr);
		//printPrompt();
		expandEnv(&instr);
		ioRedirection(&instr);        
		printTokens(&instr);	
		piping(&instr);
		builtIns(&instr);
		clearInstruction(&instr);
	}

	return 0;
}

//reallocates instruction array to hold another token
//allocates for new token within instruction array
void addToken(instruction* instr_ptr, char* tok)
{
	//extend token array to accomodate an additional token
	if (instr_ptr->numTokens == 0)
		instr_ptr->tokens = (char**) malloc(sizeof(char*));
	else
		instr_ptr->tokens = (char**) realloc(instr_ptr->tokens, (instr_ptr->numTokens+1) * sizeof(char*));

	//allocate char array for new token in new slot
	instr_ptr->tokens[instr_ptr->numTokens] = (char *)malloc((strlen(tok)+1) * sizeof(char));
	strcpy(instr_ptr->tokens[instr_ptr->numTokens], tok);

	instr_ptr->numTokens++;
}

void addNull(instruction* instr_ptr)
{
	//extend token array to accomodate an additional token
	if (instr_ptr->numTokens == 0)
		instr_ptr->tokens = (char**)malloc(sizeof(char*));
	else
		instr_ptr->tokens = (char**)realloc(instr_ptr->tokens, (instr_ptr->numTokens+1) * sizeof(char*));

	instr_ptr->tokens[instr_ptr->numTokens] = (char*) NULL;
	instr_ptr->numTokens++;
}

void printTokens(instruction* instr_ptr)
{
	int i;
	printf("Tokens:\n");
	for (i = 0; i < instr_ptr->numTokens; i++) {
		if ((instr_ptr->tokens)[i] != NULL)
			printf("%s\n", (instr_ptr->tokens)[i]);
	}
}

void clearInstruction(instruction* instr_ptr)
{
	int i;
	for (i = 0; i < instr_ptr->numTokens; i++)
		free(instr_ptr->tokens[i]);

	free(instr_ptr->tokens);

	instr_ptr->tokens = NULL;
	instr_ptr->numTokens = 0;
}

void expandEnv(instruction* instr_ptr)
{
    int numTok = instr_ptr->numTokens-1;
    int i;
    for (i = 0; i < numTok; i++)
    {
        if (instr_ptr->tokens[i][0] == '$')
        {
            char* env = (char*) malloc(1000);
            instr_ptr->tokens[i]++;
            memcpy(env, getenv(instr_ptr->tokens[i]), 100);
            instr_ptr->tokens[i] = env;
        }
    }
}

// Check for redirection
void ioRedirection(instruction* instr_ptr)
{
	int numTok = instr_ptr->numTokens -1;
	int i;

	for( i = 0; i < numTok; i++)
	{
		if ( instr_ptr->tokens[i][0] == '>' || instr_ptr->tokens[i][0] == '<' )
		{
			// Check if first or last character
			if( i == 0 || i == numTok-1 )
			{
				printf("Missing name for redirect.\n");
			}
			
			else
			{

			}
		}
	}
}

void builtIns(instruction* instr_ptr)
{
	int numTok = instr_ptr->numTokens -1;
	int i;
	
	if( strcmp((instr_ptr->tokens)[0],"echo") == 0 )
	{

			char envVar[300];
			for( i=1; i < numTok; i++ )
			{
				// Environment vars
				if( instr_ptr->tokens[i][0] == '$' )
				{
					strcpy(envVar, getenv((instr_ptr->tokens)[i]));
					
					if( envVar != NULL )
						printf("%s\n", envVar);
					
					else
					{
						printf("%s\n", "Invalid or NULL environment variable"); 	
						return;
					}
				}
			
				// Output without modification
				else
				{
					printf("%s ", (instr_ptr->tokens)[i]);
				}
			} 
			printf("%s", "\n");
	}

	else if ( strcmp((instr_ptr->tokens)[0], "exit" ) == 0 )
	{
		printf("%s \n\t%s %d\n", "Exiting now!", "Commands executed:", numCommands);
		exit(0);
	}

	else if ( strcmp((instr_ptr->tokens)[0], "jobs") == 0 )
	{	
		printf("%s \n", (instr_ptr->tokens)[0]);
	}

	else if ( strcmp((instr_ptr->tokens)[0], "cd") == 0 )
	{
	    //convert path to absolute
	    //call chdir on the path and update pwd
	    //copy path to pwd using setenv
	    //also check that entered directory is valid
        struct stat buf;

	    if (instr_ptr->tokens[1][0] != '/') { //path is relative
	        //printf("first char is %s", instr_ptr->tokens[1][0]);
            char *temp = (char*)malloc(strlen(getenv("PWD")));
            memcpy(temp, getenv("PWD"), strlen(getenv("PWD")));
            strcat(temp, "/");
            strcat(temp, instr_ptr->tokens[1]);
            //printf("%s", temp);
            if (stat(temp, &buf) == 0 && S_ISDIR(buf.st_mode)) {
                printf("relative directory is %s \n", temp);
                chdir(temp);
                setenv("PWD", temp, 1);
            } else {
                printf("Directory doesn't exist\n");
                return;
            }

        } else {
            if (stat(instr_ptr->tokens[1], &buf) == 0 && S_ISDIR(buf.st_mode)) {
                printf("directory is %s\n", instr_ptr->tokens[1]);
                chdir(instr_ptr->tokens[1]);
                setenv("PWD", instr_ptr->tokens[1], 1);
            }
            else {
                printf("Directory doesn't exist \n");
                return;
            }
	    }


		//printf("%s \n", (instr_ptr->tokens)[0]);

	}

}

void piping(instruction* instr_ptr)
{
	int numTok = instr_ptr->numTokens - 1;
	int i;

	for( i=0; i < numTok; i++)
	{	
		if ( instr_ptr->tokens[i][0] == '|' )
		{
			if ( i == 0 || i == numTok-1 )
			{
				printf("Invalid null command.\n");
				return;
			}

			else
			{
				printf("Pipe!\n");
						
				int fd[2];
			
				if (fork() == 0) {
					// Child (cmd1|cmd2)
					pipe(fd);
					if ( fork() == 0 ) {
					 // Cmd1 (Writer)
						close(1);
						dup(fd[1]);
						close(fd[0]);
						close(fd[1]);	 
					 //TODO:  Execute command
					 exit(0);
					 	
					 }

					else {
					
						//Cmd 2 (Reader)
						//Handle fds
						close(0);
						dup(fd[0]);
						close(fd[0]);
						close(fd[1]);
						//TODO: Execute command
						exit(0);
					
					
					}
				}
				
				else{
					//Parent (Shell)
					close(fd[0]);
					close(fd[1]);
				}
				return;
			}

		}		
	}

}

void printPrompt()
{
    char user[30];
    char machine[80];
    char pwd[100];

    strcpy(user, getenv("USER"));
    strcpy(machine, getenv("MACHINE"));
    strcpy(pwd, getenv("PWD"));

    printf("%s@%s:%s> ", user, machine, pwd);

}
