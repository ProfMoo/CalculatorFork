#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/wait.h>

void parenProcess(char* singleEx, int location, int pipeBack, int first) {
	int i = 0;
	int j = 0;
	int pid = getpid();

	//print expression
	if (location <= 4) { //could make this depend on a value w eput into the function if needed
		if (location == 1) {
			printf("PID %d: My expression is \"%c\"\n", pid, *singleEx);
			printf("PID %d: Sending \"%c\" on pipe to parent\n", pid, *singleEx);
		}
		else {
			printf("PID %d: My expression is \"%s\"\n", pid, singleEx);
			printf("PID %d: Sending \"%s\" on pipe to parent\n", pid, singleEx);
		}
		
		fflush(NULL);

		char* buff;
		buff = singleEx;
		int bytes_written = write(pipeBack, buff, 128);

		if (bytes_written == -1) {
			perror("write() failed");
		}
		_exit(0);
	}

	//making list for pids and pipes
	int pidList[20];
	i = 0;
	while (i < 20) {
		pidList[i] = 0;
		i += 1;
	}
	int pipeList[20][2];
	i = 0;
	while (i < 20) {
		j = 0;
		while (j < 2) {
			pipeList[i][j] = 0;
			j += 1;
		}
		i += 1;
	}

	//print expression
	printf("PID %d: My expression is \"%s\"\n", pid, singleEx);
	fflush(NULL);
	#if DEBUG_MODE
		printf("location: %d\n", location);
		fflush(NULL);
	#endif

	//cut parentheses
	i = 0;
	char* cutParen;
	cutParen = (char*)calloc(location, sizeof(char));
	while (i < location-2) {
		cutParen[i] = singleEx[i+1];
		//printf("cut paren: \"%s\"\n", cutParen);
		i++;
	}
	int lengthEx = i;
	cutParen[i] = '\0';

	#if DEBUG_MODE
		printf("cut paren: %s\n", cutParen);
		fflush(NULL);
	#endif

	//dealing with operator
	i = 0;
	while (i < strlen(cutParen)) {
		if (isspace(cutParen[i])) {
			break;
		}
		i++;
	}
	int convOp = 0;
	if (i == 1) {
		convOp = (int)cutParen[0];
		if(convOp != 42 && convOp != 43 && convOp != 45 && convOp != 47) {
			pid = getpid();
			printf("PID %d: ERROR: unknown \"%c\" operator; exiting\n", pid, convOp);
			fflush(NULL);
			_exit(EXIT_FAILURE);
		}
		else {
			printf("PID %d: Starting \"%c\" operation\n", pid, convOp);
			fflush(NULL);
		}
	}
	else {
		char wrongOp[128];
		j = 0;
		while (j < i) {
			wrongOp[j] = cutParen[j];
			j++;
		}
		pid = getpid();
		printf("PID %d: ERROR: unknown \"%s\" operator; exiting\n", pid, wrongOp);
		fflush(NULL);
		_exit(EXIT_FAILURE);
	}

	//========================================================================================//
	//split up the operation and send the operands into different children
	//========================================================================================//

	//parsing through expression and sending back into function
	int numForks = 0;
	char* newEx;
	newEx = (char*)calloc(128, sizeof(char));
	char* newExDig;
	newExDig = (char*)calloc(128, sizeof(char));
	//int tempResult = 0;
	i = 0;
	j = 0;
	while (i < lengthEx) {
		int con = cutParen[i];
		#if DEBUG_MODE_PAREN
			//printf("cutParen[i]: %c\n", cutParen[i]);
			//printf("con: %d\n", con);
			fflush(NULL);
		#endif

		int numParen = 0;
		if (con == 40) { //need to send whole parentheses
			numParen++;
			// printf("INSIDE PARENTHESES\n");
			// fflush(NULL);
			j = 0;
			while(1) {
				int con2 = cutParen[i+j];
				newEx[j] = cutParen[i+j];

				#if DEBUG_MODE
					//printf("digit: %d\n", cutParen[i+j]);
					//fflush(NULL);
				#endif

				if (con2 == 41 && numParen == 1) { //end parentheses
					newEx[++j] = '\0';
					break;
				}
				else if (con2 == 41 && numParen > 1) {
					numParen--;
				}
				else if (con2 == 40 && j != 0) { //another parentheses
					numParen++;
				}
				j++;
			}
			i+=j;

			int rc = pipe(pipeList[numForks]);
			if (rc == -1) {
				perror("pipe() failed");
			}

			//pipeList[numForks] = p[0];

			numForks++;
			pid = fork();
			if (pid == -1) {
				perror("fork() failed");
			}

			if (pid > 0) { /*parent*/
				pidList[numForks-1] = pid;
			}

			if (pid == 0) {/*child*/

				close(pipeList[numForks-1][0]);
				pipeList[numForks-1][0] = -1;
				//handle forks in here

				parenProcess(newEx, j, pipeList[numForks-1][1], 0);
			}
		}
		else if (isdigit(cutParen[i]) || (con == 45 && isdigit(cutParen[i+1]))) {
			// printf("INSIDE DIGIT\n");
			// fflush(NULL);
			j = 0;
			while(1) {
				int con2 = cutParen[i+j];
				#if DEBUG_MODE_PAREN
					printf("cutParen[i+j]: %c\n", cutParen[i+j]);
					printf("con2: %d\n", con2);
					fflush(NULL);
				#endif
				newExDig[j] = cutParen[i+j];
				if (con2 == 41 || con2 == 32 || cutParen[i+j] == '\0') { //end parentheses or space
					break;
				}
				else{
					j++;
				}
			}
			newExDig[j++] = '\0';
			i+=j;

			//checking for zero here
			if (strlen(newExDig) == 1) {
				int con3 = newExDig[0];
				#if DEBUG_MODE
					printf("con3: %d\n", con3);
					printf("numForks: %d\n", numForks);
					printf("convOp: %c\n", convOp);
					fflush(NULL);
				#endif
				if (con3 == 48 && numForks > 0 && convOp == '/') {
					pid = getpid();
					printf("PID %d: ERROR: division by zero is not allowed; exiting\n", pid);
					fflush(NULL);
					_exit(1);
				}
			}

			int rc = pipe(pipeList[numForks]);
			if (rc == -1) {
				perror("pipe() failed");
			}

			numForks++;
			pid = fork();
			if (pid == -1) {
				perror("fork() failed");
			}

			if (pid > 0) { /*parent*/
				pidList[numForks-1] = pid;
			}

			if (pid == 0) {/*child*/
				close(pipeList[numForks-1][0]);
				pipeList[numForks-1][0] = -1;

				parenProcess(newExDig, j, pipeList[numForks-1][1], 0);
			}
		}
		else {
			i++;
		}
	}

	//========================================================================================//
	//wait for a return from all the children
	//========================================================================================//

	//check here for the correct amount of operands
	#if DEBUG_MODE
		printf("numForks: %d\n", numForks);
	#endif
	if (numForks < 2) {
		pid = getpid();
		printf("PID %d: ERROR: not enough operands; exiting\n", pid);
		fflush(NULL);
		_exit(1);
	}

	fflush(NULL);
	i = 0;
	char** results;
	results = (char**)calloc(20, sizeof(char*));
	while (i < 20) {
		results[i] = (char*)calloc(128, sizeof(char));
		i += 1;
	}
	i = 0;
	int status[numForks];
	while (i < numForks) {
		pid_t child_pid = waitpid(pidList[i], &(status[i]), 0);
		if (WEXITSTATUS(status[i]) != 0) {
			pid = getpid();
			printf("PID %d: child terminated with nonzero exit status 1 [child pid %d]\n", pid, pidList[i]);
			fflush(NULL);
			_exit(1);
		}

		char* buffer;
		buffer = (char*)calloc(128, sizeof(char));

		int bytes_read = read(pipeList[i][0], buffer, 128);

		if (bytes_read == 0) {
			perror("read() failed. the child process terminated without writing data");
		}

		#if DEBUG_MODE
			printf("buffer: %s\n", buffer);
		#endif

		results[i] = buffer;
		#if DEBUG_MODE
			printf("results[i]: %s\n", results[i]);
			printf("PARENT: child %d terminated.. \n", child_pid);
			fflush(NULL);
		#endif
		
		child_pid += 1;

		i += 1;
	}

	//========================================================================================//
	//get the results from the children and calculate a single combined answer
	//========================================================================================//

	//calculating right answer
	i = 0;
	int tempAnswer = 0;
	while (i < numForks) {
		int get = 0;
		get = (int)strtol(results[i], (char**)NULL, 10);
		if (i == 0) {
			tempAnswer = get;
		}
		else {
			if (convOp == 43) {
				tempAnswer += get;
			}
			else if (convOp == 42) {
				tempAnswer *= get;
			}
			else if (convOp == 45) {
				tempAnswer -= get;
			}
			else if (convOp == 47) {
				if (get == 0) {
					pid = getpid();
					printf("PID %d: ERROR: division by zero is not allowed; exiting\n", pid);
					fflush(NULL);
					_exit(1);
				}
				else {
					tempAnswer /= get;
				}
			}
		}
		i += 1;
		#if DEBUG_MODE
			printf("tempAnswer: %d\n", tempAnswer); 
			fflush(NULL);
		#endif
	}

	//========================================================================================//
	//send the correct answer back through the pipe
	//========================================================================================//

	//sending right answer back
	#if DEBUG_MODE
		printf("tempAnswer: %d\n", tempAnswer);
		fflush(NULL);
	#endif

	if (first == 0) {
		pid = getpid();
		printf("PID %d: Processed \"(%s)\"; sending \"%d\" on pipe to parent\n", pid, cutParen, tempAnswer);
		fflush(NULL);
	}
	char buff[128];
	sprintf(buff, "%d", tempAnswer);

	#if DEBUG_MODE
		printf("pipeback: %d\n", pipeBack);
		fflush(NULL);
	#endif
	int bytes_written = write(pipeBack, buff, 128);

	if (bytes_written == -1) {
		perror("write() failed");
	}

	_exit(0);
}

char* calculator(char* expression, char* pidback) {
	int i = 0;

	i = 0;
	while (expression[i] != '\0') {
		i++;
	}
	i+=2;

	int length = i;
	int p[2];
	int p2[2];

	int rc = pipe(p);
	if (rc == -1) {
		perror("pipe() failed");
	}

	int rc2 = pipe(p2);
	if (rc2 == -1) {
		perror("pipe() failed");
	}

	int pid = fork();
	if (pid == -1) {
		perror("fork() failed");
	}

	if (pid == 0) {/*child*/
		close(p[0]);
		p[0] = -1;
		close(p2[0]);
		p2[0] = -1;
		
		int sendpid = getpid();
		char pidy[5];
		sprintf(pidy, "%d", sendpid);

		int bytes_written = write(p2[1], pidy, 128);

		if (bytes_written == -1) {
			perror("ERROR: write() failed");
		}

		parenProcess(expression, length-2, p[1], 1);

		_exit(0);
	}

	else { //parent
		close(p[1]);
		p[1] = -1;
		close(p2[1]);
		p2[1] = -1;

		char* buffer;
		buffer = (char*)calloc(128, sizeof(char));
		char pidd[5];

		#if DEBUG_MODE
			printf("pipe: %d\n", p[0]);
			fflush(NULL);
		#endif

		int status;
		wait(&status);
		if(WIFEXITED(status)) { //when i exit with 1
			if (WEXITSTATUS(status) != 0) {
				_exit(1);
			}
		}
		else { 
			printf("Child did not terminate with exit\n");
			_exit(1);
		}

		int bytes_read = read(p[0], buffer, 128);

		#if DEBUG_MODE
			printf("buffer: %s\n", buffer);
			fflush(NULL);
		#endif

		if (bytes_read == 0) {
			perror("read() failed. the child process terminated without writing data");
		}

		int bytes_read2 = read(p2[0], pidd, 128);
		if (bytes_read2 == 0) {
			perror("read() failed. the child process terminated without writing data");
		}
		strcpy(pidback,pidd);

		return buffer;
	}

	//return buffer;
}

char* getString(char* fileName) {
	char* buff = NULL;
	buff = calloc(128, sizeof(char));
	FILE* fp = fopen(fileName, "r");

	if (fp == NULL) {
		perror("ERROR: fopen() failed");
	}
	if (fp != NULL) {
		#if DEBUG_MODE
			printf("reading %s\n", fileName);
		#endif
		//go to the end of the file	
		if (fseek(fp, 0L, SEEK_END) == 0) {
			long bufsize = ftell(fp); //get size of file
			if ( bufsize == -1 ) {
				perror( "ERROR: ftell() failed" );
			}

			buff = calloc(bufsize+1, sizeof(char));

			if ( fseek(fp, 0L, SEEK_SET) != 0 ) {
				perror( "ERROR: fseek() failed" );
			}

			size_t newLen = fread(buff, sizeof(char), bufsize, fp); //read into memory
			if ( ferror(fp) != 0 ) {
				perror(" ERROR: fread() failed" );
			}
			else {
				buff[--newLen] = '\0';
			}
		}
		fclose(fp);
	}
	return(buff);
}

int main(int argc, char* argv[]) {
	#if DEBUG_MODE
		printf("argc: %d\n", argc);
		int i = 0;
		while (i < argc) {
			printf("argv[%d]: %s\n", i, argv[i]);
			i++;
		}
	#endif

	if (argc != 2) {
		fprintf(stderr, "ERROR: Invalid arguments\nUSAGE: ./a.out <input-file>");
		return EXIT_FAILURE;
	}

	char* expression = getString(argv[1]);

	char* answer;
	answer = (char*)calloc(64, sizeof(char));
	char pidy[5];
	answer = calculator(expression, pidy);
	//int pid = getpid();

	printf("PID %s: Processed \"%s\"; final answer is \"%s\"", pidy, expression, answer);
	fflush(NULL);

	return EXIT_SUCCESS;
}