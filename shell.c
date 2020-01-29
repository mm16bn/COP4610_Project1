//COP4610
//Project 1
//Team: Ashley Ellis, Justin McKenzie, Melissa Ma

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

int numCommands = 0;
typedef struct
{
	char** tokens;
	int numTokens;
} instruction;

typedef struct
{
    char* cmd;
    int pid;
} process;

typedef struct
{
    process *array;
    size_t used;
    size_t size;
} Array;

void addToken(instruction* instr_ptr, char* tok);

void clearInstruction(instruction* instr_ptr);
void addNull(instruction* instr_ptr);
void expandEnv(instruction* instr_ptr);
char* pathResolution(instruction* instr_ptr);
void execute(char* path, instruction* instr_ptr);
void printPrompt();
void piping(instruction* instr_ptr);
void ioRedirection(instruction* instr_ptr);
void builtIns(instruction* instr_ptr);
char* shortcutRes(char* instr_ptr);
int fileExists(const char *path);
int insert(process process1);
void checkProcesses();
void initializeProcess();
void executeBg(instruction* instr_ptr, int i, char* path);

process processes[100];


int main() {
    initializeProcess();
	char* token = NULL;
	char* temp = NULL;
	instruction instr;
	instr.tokens = NULL;
	instr.numTokens = 0;
 //Variables to keep track of piping and ioRedirection
	bool pipeFlag = false, redFlag=false;
	while (1) {
        printPrompt();
		// loop reads character sequences separated by whitespace
        //scans for next token and allocates token var to size of scanned token

		do {
			//scans for next token and allocates token var to size of scanned token
			scanf("%ms", &token);
			temp = (char *) malloc((strlen(token) + 1) * sizeof(char));

			int i;
			int start = 0;
			for (i = 0; i < strlen(token); i++) {
				//pull out special characters and make them into a separate token in the instruction
				if (token[i] == '|' || token[i] == '>' || token[i] == '<' || token[i] == '&') {
					if (i - start > 0) {
						memcpy(temp, token + start, i - start);
						temp[i - start] = '\0';
						addToken(&instr, temp);

					}

					if ( token[i] == '|' )
						pipeFlag = true;

					else if ( token[i] == '>' || token[i] == '>' )
						redFlag = true;

					char specialChar[2];
					specialChar[0] = token[i];
					specialChar[1] = '\0';

					addToken(&instr, specialChar);

					start = i + 1;

				}
			}

			if (start < strlen(token)) {
				memcpy(temp, token + start, strlen(token) - start);
				temp[i - start] = '\0';
				addToken(&instr, temp);
			}

			//free and reset variables
			free(token);
			free(temp);

			token = NULL;
			temp = NULL;
		} while ('\n' != getchar());    //until end of line is reached

		char* execPath = (char*) malloc(1000);

    //Number of commands executed increments
		numCommands++;
		addNull(&instr);
		expandEnv(&instr);

		if( pipeFlag == true )
		{
		        piping(&instr);
			// Reset
			pipeFlag = false;
		}

		else
		{
			if( redFlag == true )
			{
				ioRedirection(&instr);
				// Reset
				redFlag = false;
			}


			execPath = pathResolution(&instr);
			execute(execPath, &instr);
		}
	
		clearInstruction(&instr);
		checkProcesses(processes);
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

void clearInstruction(instruction* instr_ptr)
{
	int i;
	for (i = 0; i < instr_ptr->numTokens; i++)
		free(instr_ptr->tokens[i]);

	free(instr_ptr->tokens);

	instr_ptr->tokens = NULL;
	instr_ptr->numTokens = 0;
}

// If file or directory exists, return 1; otherwise 0.
int fileExists(const char *path)
{
    if(access(path, F_OK) == -1)
        return 0;

    return 1;
}

char* shortcutRes(char* instr_ptr)
{
    char* envvar = (char*) malloc(1000);
	char* path_name = (char*) malloc(1000);
    char* path = (char*) malloc(1000);
    char *parent = (char*)malloc(1000);
    char *cwd = (char*) malloc(1000);

    // Copies input into path_name
    strcpy(path_name, instr_ptr);
	// Copies input into path_name

    if (instr_ptr[0] != '/') {
        // If relative path:
        path = strtok(path_name, " /");
//        printf("PATH: %s\n", path);
        while(path!=NULL) {
            if (strcmp(path, "~") == 0) {
                strcpy(envvar, getenv("HOME"));
                // concatenates path after first ~/
                while (path != NULL) {
                    if (strcmp(path, "~") != 0) {
                        strcat(envvar, "/");
                        strcat(envvar, path);
                    }
                    path = strtok(NULL, " /");
                }
                strcpy(cwd, envvar);
            }
                // Expands to parent
            else if (strcmp(path, "..") == 0) {
                chdir("..");
                cwd = getcwd(parent, 100);

                // concatenates parents with input
                if (strcmp(path, "..") != 0) {
                    strcat(cwd, "/");
                    strcat(cwd, path);
                }
                path = strtok(NULL, " /");
                strcpy(envvar, getenv("PWD"));
            }

                // Expands to PWD
            else if (strcmp(path, ".") == 0) {
                strcpy(envvar, getenv("PWD"));

                while (path != NULL) {
                    if (strcmp(path, ".") != 0) {
                        strcat(envvar, "/");
                        strcat(envvar, path);
                    }
                    path = strtok(NULL, " /");
                }

                strcpy(cwd, envvar);
            }
        }
        strcpy(envvar, getenv("PWD"));

        // checks if file or directory (at envvar) exists
        if (chdir(cwd) != 0)
        {
            perror(cwd);
        }
        else
        {
            printf("-Bash: %s is a directory\n", cwd);
        }
        chdir(envvar);
    }
    else
    {
        path = path_name;

        // Expands to root directory
        if (instr_ptr[0] == '/') {
            char *root = (char*)malloc(1000);
            chdir("/");
            cwd = getcwd(root, 100);
            path = strtok(path_name, " /");

            while (path != NULL)
            {
                strcat(cwd, path);
                strcat(cwd, "/");
                path = strtok(NULL, " /");
            }

            // If file/dir doesn't exist, throw error
            if(chdir(cwd) != 0)
            {
                perror(cwd);
            }

            else
            {
                printf("-bash: %s\n", cwd);
            }

        }
    }

    return cwd;
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
    int fd = 0;
    char* path = (char*)malloc(1000);
    char *path_name = (char*)malloc(1000);
    char *path_res = (char*)malloc(1000);
    char *io = (char*)malloc(1000);
    char **new_arr = (char**)malloc(1000);

//    strcpy(path_name, instr_ptr->tokens);

//    instruction instr;
//    instr.tokens = NULL;
//    instr.numTokens = 0;

    for( i = 0; i < numTok; i++)
    {
        if ((strcmp(instr_ptr->tokens[i],  ">") == 0)
        || (strcmp(instr_ptr->tokens[i], "<") == 0)) {

            // Check if first or last character
            if( i == 0 || i == numTok-1 ) {
                printf("Missing name for redirect.\n");
            }

            else {
                if (strcmp(instr_ptr->tokens[i],"<") == 0) {
                    int index = i;

                    strcpy(path, instr_ptr->tokens[i + 1]);

                    io = strtok(path_name, " <");
                    int j;
                    for (j = 0; j < index; j++) {
                        strcpy(path_name, instr_ptr->tokens[j]);

                        io = strtok(path_name, " <");

                        while (io != NULL) {
                            new_arr[j] = io;
                            io = strtok(NULL, " <");
                        }
                    }

                    fd = open(path, O_RDONLY);

                    if (fd == -1) {
                        perror(path);
                        exit(EXIT_FAILURE);
                    }

                    pid_t child_pid;
                    int stat_loc;
                    child_pid = fork();

                    if (child_pid == 0) {
                        close(STDIN_FILENO);
                        dup(fd);
                        close(fd);

                        path_res = pathResolution(instr_ptr);

                        execv(path_res, new_arr);

                    }
                    else {
                        waitpid(child_pid, &stat_loc, WUNTRACED);
                        close(fd);
                    }
                }

                else if (strcmp(instr_ptr->tokens[i],">") == 0) {
                    int index = i;

                    strcpy(path, instr_ptr->tokens[i+1]);

                    io = strtok(path_name, " >");
                    int j;
                    for (j = 0; j < index; j++)
                    {
                        strcpy(path_name, instr_ptr->tokens[j]);

                        io = strtok(path_name, " >");

                        while(io != NULL)
                        {
                            new_arr[j] = io;
                            io = strtok(NULL, " >");
                        }
                    }

                    fd = open(path, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR |
                    S_IRGRP | S_IWGRP | S_IWUSR);

                    if(fd == -1)
                    {
                        perror(path);
                        exit(EXIT_FAILURE);
                    }

                    pid_t child_pid;
                    int stat_loc;
                    child_pid = fork();

                    if (child_pid == 0) {
                        path = pathResolution(instr_ptr);

                        close(STDOUT_FILENO);
                        dup(fd);
                        close(fd);

                        path_res = pathResolution(instr_ptr);

                        execv(path_res, new_arr);

                    }

                    else {
                        waitpid(child_pid, &stat_loc, WUNTRACED);
                        close(fd);
                    }

                }
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

      // To store value of environment variable
			char envVar[300];
      
			for( i=1; i < numTok; i++ )
			{
				// Environment vars
				if( instr_ptr->tokens[i][0] == '$' )
				{
					strcpy(envVar, getenv((instr_ptr->tokens)[i]));
					
          // Print argument to the user
					if( envVar != NULL )
						printf("%s\n", envVar);
			
          // Invalid environment variable sent in		
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
		int i, status;
   //Wait for all processes to complete
		while( processes[i].pid != 0 )
		{
			waitpid(processes[i].pid, &status, 0);
			i++;
		}

    // Exiting statements
		printf("%s \n\t%s %d\n", "Exiting now!", "Commands executed:", numCommands);
		exit(0);
	}


	else if ( strcmp((instr_ptr->tokens)[0], "jobs") == 0 )
    {
        printf("%s \n", (instr_ptr->tokens)[0]);
        int i;
        for (i = 0; i < 100; i++){
            if(strcmp(processes[i].cmd, "*") != 0){
                printf("[%d]+ [%d] [%s]", i, processes[i].pid, processes[i].cmd);
            }
        }
    }

	else if ( strcmp((instr_ptr->tokens)[0], "cd") == 0 ){
	    //convert path to absolute
	    //call chdir on the path and update pwd
	    //copy path to pwd using setenv
	    //also check that entered directory is valid
        char *path = (char*)malloc(1000);
        path = shortcutRes(instr_ptr->tokens[1]);
        printf("PATH %s\n", path);
        chdir(path);
        setenv("PWD", path, 1);
        return;
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

char* pathResolution(instruction* instr_ptr)
{
	int i;
	int statReturn;
	int numTok = (instr_ptr->numTokens) -  1;
	char *ptr, *pRes, *path, *isAbsolute, *temp = (char*) malloc(strlen(getenv("PATH")));
	struct stat stats;

// Check for built in commands
for(i=0; i < numTok; i++)
{

    if( strcmp((instr_ptr->tokens)[i],"echo") == 0 || strcmp((instr_ptr->tokens)[i],"cd") == 0 || strcmp((instr_ptr->tokens)[i],"jobs") == 0 || strcmp((instr_ptr->tokens)[i],"exit") == 0)
    {
        // Leave the function to execute built ins
        builtIns(instr_ptr);
        return;
    }
}

	path = (char*) malloc(strlen(getenv("PATH")));
	memcpy(path, getenv("PATH"), strlen(getenv("PATH")));
	
 // Check for existence of absolute path
	isAbsolute = strchr((instr_ptr->tokens)[0], '/');
	
	if ( isAbsolute == NULL )
	{
		// Prefix with location in the path and search for file existence
		// The first file in the concatenated path list to exist is the path of the command
		// If none of the files exist, signal an error
		pRes = strtok(path,":");
		
   // Parse through the path to find where the file exists
		while( pRes != NULL )
		{
			strcpy(temp, pRes);
			strcat(temp, "/");
			
			char* temp2 = (char*) malloc(strlen(getenv("PATH")));
			strcat(temp2, temp);
			
			strcat(temp2, instr_ptr->tokens[0]);
		
       // Check for valid location of file
			statReturn = stat(temp2, &stats);
   
            if ( statReturn  == 0 )
            {
                // Appropriate location found return path
                strcat(temp, instr_ptr->tokens[0]);
                return temp;
            }

            pRes = strtok(NULL, ":");
            
        }
    } else {
        // Handle as Shortcut Resolution and return path
        char* path_res= shortcutRes(instr_ptr->tokens[0]);
        return path_res;

    }

    // Invalid command or file
    if ( statReturn == -1 ) {
        printf("%s\n", "Failure command not found");
        return 0;
    }

}

void execute(char* path, instruction* instr_ptr) {
    pid_t child_pid;
    int stat_loc;
    int i;

//Check for built in instructions
int j;
int numTok = instr_ptr->numTokens-1;
for(j=0; j < numTok; j++)
{
    if( strcmp((instr_ptr->tokens)[i],"echo") == 0 || strcmp((instr_ptr->tokens)[i],"cd") == 0 || strcmp((instr_ptr->tokens)[i],"jobs") == 0 || strcmp((instr_ptr->tokens)[i],"exit") == 0)
    {
        // Leave function to execute built in commands
        builtIns(instr_ptr);
        return;
    }
}

    if(instr_ptr->numTokens > 2){
        //printf("Numtokens = %d", instr_ptr->numTokens);
        for(i = 1; i < instr_ptr->numTokens; i++){

            if(strcmp(instr_ptr->tokens[i], "&") == 0){
                //printf("going to bg");
                executeBg(instr_ptr, i, path);
                return;
            }
        }
    } else {
        //printf("No background");
        child_pid = fork();
        if (child_pid == 0) {
            /* Never returns if the call is successful */
            execv(path, instr_ptr->tokens);
            printf("This won't be printed if execv is successul\n");
        } else {
            waitpid(child_pid, &stat_loc, WUNTRACED);
        }
    }
}

int insert(process process1)
{
    int i;
    for(i = 0; i < 100; i++){
        if (processes[i].cmd == "*"){
            processes[i] = process1;
            return i;
        }
    }
    printf("No room in queue");
}

void checkProcesses()
{
    int i;
    int stat_loc;
    for(i = 0; i < 100; i++){
        if (processes[i].pid == waitpid(-1, &stat_loc, WUNTRACED)){
            printf("[%d]+ [%s]\n", i, processes[i].cmd);
            int c;
            for (c = i - 1; c < 100 - 1; c++)
                processes[c] = processes[c+1];
        }
    }
}

void initializeProcess(){
    int i;
    for (i = 0; i < 100; i++){
        processes[i].cmd = "*";
    }
}

void executeBg(instruction* instr_ptr, int i, char* path) {
    pid_t child_pid;
    int stat_loc;
    child_pid = fork();
    //printf("%d", child_pid);
    if (child_pid == 0) {
        /* Never returns if the call is successful */
        int j;
        char **args = malloc(i + 1 * sizeof(char *));
        for (j = 0; j < i; j++) {
            args[j] = instr_ptr->tokens[i];
        }
        execv(path, args);
        printf("This won't be printed if execv is successul\n");
    } else {
        //printf("Forked");
        process process1;
        process1.cmd = malloc(sizeof(instr_ptr->tokens[0]));
        memcpy(process1.cmd, instr_ptr->tokens[0], sizeof(instr_ptr->tokens[0]));
        //process1.cmd = instr_ptr->tokens[0];
        process1.pid = child_pid;
        int index = insert(process1);
        printf("[%d] [%d] \n", index, process1.pid);
        waitpid(child_pid, &stat_loc, WNOHANG);
        return;
    }
}

void piping(instruction* instr_ptr)
{
	// Allocate memory for instructions
	// Add null to the end of the tokens
	
	int numTok = instr_ptr->numTokens - 1;
	int i, firstPipe=0, secondPipe=0;
	instruction *instr1, *instr2, *instr3;

	for( i=0; i < numTok; i++)
	{	
 
		if ( instr_ptr->tokens[i][0] == '|' )
		{
   //Check for proper piping syntax
			if ( i == 0 || i == numTok-1 )
			{
				printf("Invalid null command.\n");
				return;
			}

			else
			{
        //Find location of the pipe
				if ( firstPipe == 0 )
				{
					firstPipe = i;
				}	
			
				// Multiple pipes
				else
				{
					secondPipe = i;
				}
			
			}
		}	
	}		
       // Allocate memory for temporary instruction objects and initialize values
				instr1 = (instruction*) malloc(sizeof(char *) * (firstPipe + 1));
				instr1->numTokens = 0;
				instr2 = (instruction*) malloc(sizeof(char*)* (numTok-firstPipe + 1));
				instr2->numTokens = 0;
					 

        // First instruction up until the pipe
				int j; 
				for(j = 0; j < firstPipe; j++)
				{
					instr1->tokens[j] = instr_ptr->tokens[j];
					
					instr1->numTokens++;		
				}
				
        // Add null to send to execv
				instr1->tokens[firstPipe] = NULL;	
				
        //Second instruction until the end
        int k=0;	
				for(j=firstPipe+1; j < numTok; j++)
				{
					instr2->tokens[k] = instr_ptr->tokens[j];
					instr2->numTokens++;
					k++;
				}

        // Add null to send to execv
				instr2->tokens[numTok] = NULL;
			

        // Allocate memory to store path for pipe execution
				char* envVar = (char*) malloc(strlen(getenv("PATH")));
				memcpy( envVar, getenv("PATH"), strlen(getenv("PATH")));
			
				char* path = strtok(envVar,":");
				char* cmd1 = (char*) malloc(strlen(getenv("PATH"))), *cmd2 = (char*) malloc(strlen(getenv("PATH")));
				struct stat stats;
				
				char *tempPath = (char*) malloc(strlen(getenv("PATH")));
			
        // Parse through path to find where file exists for first instruction
				while( path != NULL )
				{
					strcpy(cmd1, path);
					strcat(cmd1, "/");

					char *tempPath = (char*) malloc(strlen(getenv("PATH")));
					strcpy(tempPath, cmd1);
					strcat(tempPath, instr1->tokens[0]);

					if( stat(tempPath, &stats) == 0 )
					{
             // Valid path found leave the loop
						strcat(cmd1, instr1->tokens[0]);
						break;
					}
					
					path = strtok(NULL,":");

				}
				free(tempPath);
			
				strcpy(tempPath, "");
				 
       //Reset variable value
				memcpy(envVar, getenv("PATH"), strlen(getenv("PATH")));
				
        path = strtok(envVar,":");
			
				struct stat stats2;
				
        // Parse through path to find where file exists
				while( path != NULL )
				{
					strcpy(cmd2, path);	
					strcat(cmd2, "/");

					strcpy(tempPath, cmd2);
					strcat(tempPath, instr2->tokens[0]);
					if( stat(tempPath, &stats2) == 0)
					{
						// Valid path found leave loop
						strcat(cmd2, instr2->tokens[0]);
						break;
					}

					path = strtok(NULL, ":");
				}		

				int fd[2];

        // Check for valid pipe
				if ( pipe(fd) < 0 )
				{
					printf("%s\n", "Pipe could not be initialized.");
					return;
				}
	
				pid_t pid = fork();
				if (pid == 0) {
					// Child (cmd1|cmd2)
					pipe(fd);

					if ( fork() == 0 ) {
						
						close(STDOUT_FILENO);
						dup(fd[1]);
						close(fd[0]);
						close(fd[1]);	 
						
           // Execute process
						execv(cmd1, instr1->tokens);
					
					 	exit(1); 	
					 }

					else {
					
						//Cmd 2 (Reader)
						//Handle fds
						
						close(STDIN_FILENO);
						dup(fd[0]);
						close(fd[0]);
						close(fd[1]);
					
             //Execute process
						execv(cmd2, instr2->tokens);	
					
						exit(1);

					}
				}
				
				else{

					//Parent (Shell)
          // Wait for process to complete
					waitpid(pid, NULL, 0);
					
					close(fd[0]);
					close(fd[1]);


				}

				
				return;
}
