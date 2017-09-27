#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>

int singleProcess(char* expression, char singleEx, int location, int* err) {
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

		int mypid = getpid();
		printf("PID %d: My expression is \"%c\"\n", mypid, singleEx);
		printf("PID %d: Sending \"%c\" on pipe to parent\n", mypid, singleEx);
		fflush(NULL);

		char buff[1];
		buff[0] = singleEx;
		int bytes_written = write(p[1], buff, 1);

		if (bytes_written != 1) {
			perror("write() failed");
		}
		_exit(0);
	}

	else { //parent
		close(p[1]);
		p[1] = -1;

		char buffer[80];
		int bytes_read = read(p[0], buffer, 1);

		if (bytes_read == 0) {
			perror("read() failed. the child process terminated without writing data");
		}

		int convert = buffer[0] - '0';
		return convert;
	}
}

int parenProcess(char* fullEx, char* singleEx, int location, int* err) {
	int pid = getpid();
	printf("PID %d: My expression is \"%s\"\n", pid, singleEx);
	fflush(NULL);
	// printf("location: %d\n", location);
	// fflush(NULL);

	//cut parentheses
	char cutParen[location];
	int i = 0;
	int j = 0;
	while (i < location-1) {
		cutParen[i] = singleEx[i+1];
		i++;
	}
	int lengthEx = i;
	cutParen[location] = '\0';

	//printf("cut paren: %s\n", cutParen);
	//fflush(NULL);

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
			*err = 1;
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
		*err = 1;
	}

	if (*err == 0) {
		printf("PID %d: Starting \"%c\" operation\n", pid, convOp);
		fflush(NULL);
	}

	//now, to fork:
	int p[2];

	int rc = pipe(p);
	if (rc == -1) {
		perror("pipe() failed");
	}

	pid = fork();
	if (pid == -1) {
		perror("fork() failed");
	}

	if (pid == 0) {/*child*/
		close(p[0]);
		p[0] = -1;

		int mypid = getpid();

		//write expression here

		char newEx[lengthEx];
		int tempResult = 0;
		i = 0;
		while (i < lengthEx) {
			int con = cutParen[i];
			if (con == 40) { //need to send whole parentheses
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
				if (convOp == 43) {
					tempResult += parenProcess(cutParen, newEx, j, err);
				}
				if (convOp == 47) {
					tempResult /= parenProcess(cutParen, newEx, j, err);
				}
				if (convOp == 42) {
					tempResult *= parenProcess(cutParen, newEx, j, err);
				}
				if (convOp == 45) {
					tempResult -= parenProcess(cutParen, newEx, j, err);
				}
				
			}
			if (isdigit(cutParen[i])) {
				if (convOp == 43) {
					tempResult += singleProcess(cutParen, cutParen[i], i, err);
				}
				if (convOp == 47) {
					if (i == 2) {
						tempResult = singleProcess(cutParen, cutParen[i], i, err);
					}
					else {
						if (cutParen[i] == '0') {
							printf("PID %d: ERROR: division by zero is not allowed; exiting\n", mypid);
							fflush(NULL);
							*err = 1;
						}
						else {
							tempResult /= singleProcess(cutParen, cutParen[i], i, err);
						}
					}
				}
				if (convOp == 42) {
					if (i == 2) {
						tempResult = singleProcess(cutParen, cutParen[i], i, err);
					}
					else {
						tempResult *= singleProcess(cutParen, cutParen[i], i, err);
					}
				}
				if (convOp == 45) {
					if (i == 2) {
						tempResult = singleProcess(cutParen, cutParen[i], i, err);
					}
					else {
						tempResult -= singleProcess(cutParen, cutParen[i], i, err);
					}
				}
			}
			i++;
		}

		if (*err == 0) {
			printf("PID %d: Processed \"(%s)\"; sending \"%d\" on pipe to parent\n", mypid, cutParen, tempResult);
			char buff[128];
			sprintf(buff, "%d", tempResult);

			int bytes_written = write(p[1], buff, 128);

			if (bytes_written == -1) {
				perror("write() failed");
			}
		}
		//say returning expression here, with result
		_exit(0);
	}

	else { //parent
		close(p[1]);
		p[1] = -1;

		char buffer[128];
		int bytes_read = read(p[0], buffer, 128);

		if (bytes_read == 0) {
			*err = 1;
		}

		//convert to int and return
		int toReturn = atoi(buffer);

		return toReturn;
	}
}

int calculator(char* expression, int* err) {
	int i = 0;
	int j = 0;

	int pid = getpid();
	printf("PID %d: My expression is \"%s\"\n", pid, expression);
	fflush(NULL);

	i = 0;
	while (expression[i] != '\0') {
		i++;
	}
	i++;

	int length = i;

	//cut parentheses
	i = 0;
	char cutParen[length];
	while (i < length - 3) {
		#if DEBUG_MODE
			printf("char: %c\n", expression[i+1]);
		#endif
		cutParen[i] = expression[i+1];
		i++;
	}
	cutParen[i+1] = '\0';

	//get operator
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
			*err = 1;
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
		*err = 1;
	}

	if (err == 0) {
		printf("PID %d: Starting \"%c\" operation\n", pid, cutParen[0]);
		fflush(NULL);
	}

	#if DEBUG_MODE
		printf("cut paren: %s\n", cutParen);
		fflush(NULL);
	#endif

	i = 0;
	char singleEx[128];
	int result = 0;
	while(i < strlen(cutParen)) {
		int con = cutParen[i];
		if (con == 40) { //need to send whole parentheses
			j = 0;
			while(1) {
				int con2 = cutParen[i+j];
				singleEx[j] = cutParen[i+j];
				if (con2 == 41) { //end parentheses
					break;
				}
				else{
					j++;
				}
			}
			if (convOp == 43) {
				result += parenProcess(cutParen, singleEx, j, err);
			}
			if (convOp == 47) {
				result /= parenProcess(cutParen, singleEx, j, err);
			}
			if (convOp == 42) {
				result *= parenProcess(cutParen, singleEx, j, err);
			}
			if (convOp == 45) {
				result -= parenProcess(cutParen, singleEx, j, err);
			}
			i += j;
		}
		if (isdigit(cutParen[i])) {
			if (convOp == 43) {
				result += singleProcess(cutParen, cutParen[i], i, err);
			}
			if (convOp == 47) {
				if (i == 2) {
					result = singleProcess(cutParen, cutParen[i], i, err);
				}
				else {
					if (cutParen[i] == 0) {
						printf("PID %d: ERROR: division by zero is not allowed; exiting\n", pid);
						fflush(NULL);
						*err = 1;
					}
					else {
						result /= singleProcess(cutParen, cutParen[i], i, err);
					}
				}
			}
			if (convOp == 42) {
				if (i == 2) {
					result = singleProcess(cutParen, cutParen[i], i, err);
				}
				else {
					result *= singleProcess(cutParen, cutParen[i], i, err);
				}
			}
			if (convOp == 45) {
				if (i == 2) {
					result = singleProcess(cutParen, cutParen[i], i, err);
				}
				else {
					result -= singleProcess(cutParen, cutParen[i], i, err);
				}
			}
		}
		i++;
	}

	return result;
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

	int err = 0;
	int answer = 0;
	answer = calculator(argv[1], &err);
	int pid = getpid();

	if (argc != 2) {
		perror("ERROR: Invalid arguments\nUSAGE: ./a.out <input-file>");
		return EXIT_FAILURE;
	}

	if (err == 0) {
		printf("PID %d: Processed \"%s\"; final answer is \"%d\"\n", pid, argv[1], answer);
		fflush(NULL);
	}

	return EXIT_SUCCESS;
}