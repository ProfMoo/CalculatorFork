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
	if (location <= 5) { //could make this depend on a value w eput into the function if needed
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
	cutParen = (char*)calloc(128, sizeof(char));
	while (i < location-1) {
		cutParen[i] = singleEx[i+1];
		i++;
	}
	int lengthEx = i;
	cutParen[location] = '\0';

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
			printf("PID %d: ERROR: unknown \"%c\" operator; exiting\n", pid, convOp);
			fflush(NULL);
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
		printf("PID %d: ERROR: unknown \"%s\" operator; exiting\n", pid, wrongOp);
		fflush(NULL);
	}


	//parsing through expression and sending back into function
	int numForks = 0;
	char* newEx;
	newEx = (char*)calloc(128, sizeof(char));
	char* newExDig;
	newExDig = (char*)calloc(128, sizeof(char));
	int tempResult = 0;
	i = 0;
	j = 0;
	while (i < lengthEx) {
		int con = cutParen[i];
		#if DEBUG_MODE_PAREN
			//printf("cutParen[i]: %c\n", cutParen[i]);
			//printf("con: %d\n", con);
			fflush(NULL);
		#endif

		if (con == 40) { //need to send whole parentheses
			// printf("INSIDE PARENTHESES\n");
			// fflush(NULL);
			j = 0;
			while(1) {
				int con2 = cutParen[i+j];
				newEx[j] = cutParen[i+j];
				if (con2 == 41) { //end parentheses
					break;
				}
				else{
					j++;
				}
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
		else if (isdigit(cutParen[i]) || con == 45) {
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

	//check for num of operands here using numForks

	//reading in the results after waiting
	i = 0;
	char** results;
	results = (char**)calloc(20, sizeof(char*));
	while (i < 20) {
		results[i] = (char*)calloc(128, sizeof(char));
		i += 1;
	}
	i = 0;
	while (i < numForks) {
		int status;
		pid_t child_pid = waitpid(pidList[i], &status, 0);

		char* buffer;
		buffer = (char*)calloc(128, sizeof(char));

		#if DEBUG_MODE
			//printf("pipeList[i][0]: %d\n", pipeList[i][0]);
			//fflush(NULL);
		#endif

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
					printf("PID %d: ERROR: division by zero is not allowed; exiting\n", pid);
					fflush(NULL);
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


	//sending right answer back
	#if DEBUG_MODE
		printf("tempAnswer: %d\n", tempAnswer);
		fflush(NULL);
	#endif

	if (first == 0) {
		printf("PID %d: Processed \"(%s)\"; sending \"%d\" on pipe to parent\n", pid, cutParen, tempAnswer);
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
	//say returning expression here, with result
	_exit(0);
}

int calculator(char* expression) {
	int i = 0;

	i = 0;
	while (expression[i] != '\0') {
		i++;
	}
	i++;

	int length = i;
	int answer = 0;

	int p[2];

	int rc = pipe(p);
	if (rc == -1) {
		perror("pipe() failed");
	}

	int pid = fork();
	if (pid == -1) {
		perror("fork() failed");
	}

	if (pid == 0) {/*child*/
		close(p[0]);
		p[0] = -1;

		parenProcess(expression, length-2, p[1], 1);

		_exit(0);
	}

	else { //parent
		close(p[1]);
		p[1] = -1;

		char buffer[128];

		#if DEBUG_MODE
			printf("pipe: %d\n", p[0]);
			fflush(NULL);
		#endif

		int bytes_read = read(p[0], buffer, 128);

		#if DEBUG_MODE
			printf("buffer: %s\n", buffer);
			fflush(NULL);
		#endif

		if (bytes_read == 0) {
			perror("read() failed. the child process terminated without writing data");
		}

		answer = (int)strtol(buffer, (char**)NULL, 10);
	}

	return answer;
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

	int answer = 0;
	answer = calculator(argv[1]);
	int pid = getpid();

	if (argc != 2) {
		perror("ERROR: Invalid arguments\nUSAGE: ./a.out <input-file>");
		return EXIT_FAILURE;
	}

	printf("PID %d: Processed \"%s\"; final answer is \"%d\"\n", pid, argv[1], answer);
	fflush(NULL);

	return EXIT_SUCCESS;
}