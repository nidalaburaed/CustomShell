/*Shell made by Mika Savolainen and Nidal Abu-Raed*/
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>


void SignalHandler(int sig) {
	if(sig == SIGINT){
		printf("\nExiting shell.\n");
		exit(0);
	}
	return;
	
}

/*CD command*/
int command_CD(char **args){
	//If no argument, return to home
	if (args[1] == NULL) {	
		chdir(getenv("HOME"));
	} 
	else {
		if (chdir(args[1]) != 0) {
			perror("cd failed.");
		}
	}
	return 1;
}

/*Exit command*/
int command_EXIT(char **args){
	return 0;
}

/*Echo command*/
int command_ECHO(char **args){
	if (args[1] == NULL) {
		printf("\n");	
	}
	else{
		printf("%s \n",args[1]);
	}

	return 1;
}

void ExecutePipes(char **args) {
	pid_t pid;
	
	int output;
	int status;

	int fd[2];
	int pfd;

	/*Parse parts of pipe*/
	for(int i = 0; args[i] != NULL; i++){	
	
		if(strncmp(args[i],"|", 1) == 0){
			args[i] = NULL;
			output = i+1;
		}      
	}
	
	pipe(fd);
	
	// Forking new process
	if((pid = fork()) < 0){
		perror("Fork error.");
		exit(1);
	}
	 
	if(pid == 0){		
		if((pid = fork()) < 0){
			perror("Fork error.");
			exit(1);
		}
		
		if(pid == 0) {
			
			close(fd[0]); 
			
			if((pfd = dup2(fd[1], STDOUT_FILENO)) == -1){
				perror("dup2 error.");
			}

			execvp((args)[0], args);
			_exit(1);
			
		}
		else{			
			close(fd[1]); 

			if((pfd = dup2(fd[0], STDIN_FILENO)) == -1){ 
				perror("dup2 error.");
			}
					
			execvp(args[output], args+output);
			exit(0);
		}
		
	} 
	else {		
		close(fd[0]);
		close(fd[1]);	
		waitpid(pid, 0, 0);	
	}
}

void ExecuteRedirect(char **args) {

	pid_t pid; 
	
	if ((pid = fork()) < 0){
		perror("Fork error.");
		exit(0);
	}
	else if (pid == 0){
		int in = 0, out = 0;
		char input[64], output[64];
		int i;
		for(i = 0 ; args[i] != '\0' ; i++){

			if(strncmp(args[i], "<", 1) == 0){
				args[i] = '\0';
				strcpy(input, args[i+1]);

				in = 1;           
			}
			else if(strncmp(args[i], ">", 1) == 0){  
				args[i] = '\0';
				strcpy(output, args[i+1]);

				out = 1;
			}         
		}
		
		if(in){   
			int file;
			if ((file = open(input, O_RDONLY, 0)) < 0) {
				perror("Error opening file.");
				exit(0);
			}           
			dup2(file, 0); 

			close(file); 
		}

		if (out){
			int file;
			if ((file = open(output, O_WRONLY|O_CREAT, 0644)) < 0) {
				perror("Error opening file.");
				exit(0);
			}           

			dup2(file, 1);
			close(file);
			
		}
	
		execvp(*args, args);
		perror("execvp");
		_exit(1);
		
	}
	else {
        	waitpid(pid, 0, 0);
	}
	
}

int Execute(char **args){
	pid_t pid;
	int status;

	pid = fork();

	/* CHILD*/
	if(pid == 0){
		if(execvp(args[0],args) == -1){
			perror("Execute failure.");
		}
		exit(EXIT_FAILURE);
	}
	else if(pid < 0){
		perror("Fork failed.");
	}
	else{
		do{
			waitpid(pid,&status, WUNTRACED);
		}
		while(!WIFEXITED(status) && !WIFSIGNALED(status));
	}
	return 1;
}

int CountPipes(char **args) {
	int pipes = 0;
	int i = 0;

	while (args[i] != NULL) {		
		if(strcmp(args[i], "<") == 0){
			pipes--;
		}
		else if (strcmp(args[i], ">") == 0 ){
			pipes--;
		}
		else if (strcmp(args[i], "|") == 0){
			pipes++;
		}
		i++;		
	}		
	return pipes;
	
}

int ExecuteLine(char **args){
	int pipes = CountPipes(args);
	if(args[0] == NULL){
		return 1;	
	}	
	else if(strcmp(args[0], "cd") == 0){
		return command_CD(args);
	}
	else if(strcmp(args[0], "exit") == 0){
		return command_EXIT(args);
	}
	else if(strcmp(args[0], "echo") == 0){
		return command_ECHO(args);
	}
	else if(pipes > 0 ){
		ExecutePipes(args);
		return 1;
	}
	else if(pipes < 0 ){
		ExecuteRedirect(args);
		return 1;
	}
	return Execute(args);
}



char **SplitLine(char *line){
	int position = 0;
	char ** args = malloc(64 * sizeof(char*));
	char *arg;


	line[strlen(line) - 1] = '\0';
	arg = strtok(line, " ");

	while(arg != NULL){
		args[position] = arg;
		position++;

		arg = strtok(NULL, " ");
	}

	//Add null to end of array
	args[position] = NULL;
	return args;
}




void MainLoop(){
	char userInput[256];
	char **args;
	int status;
	char cwd[FILENAME_MAX];
	do {
		getcwd(cwd, sizeof(cwd));
		printf("%s> ",cwd);
		fgets(userInput, sizeof(userInput), stdin);
		args = SplitLine(userInput);
		status = ExecuteLine(args);

		free(args);
	}
	while(status);
}

int main(int argc, char ** argv){
	//Signal handling
	signal(SIGINT, SignalHandler);
	printf("Shell made by Mika Savolainen and Nidal Aburaed\n");
	MainLoop();
	return 0;

}















