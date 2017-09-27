#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>

int singleProcess(char* expression, char singleEx, int location) {
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

int parenProcess(char* fullEx, char* singleEx, int location) {
	printf("inside process: %s\n", singleEx);
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
	cutParen[location] = '\0';

	//printf("cut paren: %s\n", cutParen);
	//fflush(NULL);

	int lengthEx = i;
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
			//send expression here
			tempResult += parenProcess(cutParen, newEx, j);
		}
		if (isdigit(cutParen[i])) {
			tempResult += singleProcess(cutParen, cutParen[i], i);
		}
		i++;
	}

	return tempResult;
}

int calculator(char* expression) {
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

	char operator = cutParen[0];
	printf("PID %d: Starting \"%c\" operation\n", pid, operator);
	fflush(NULL);

	#if DEBUG_MODE
		printf("cut paren: %s\n", cutParen);
		fflush(NULL);
	#endif

	i = 0;
	char singleEx[20];
	int result;
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
			//send cutParen here
			result += parenProcess(cutParen, singleEx, j);
			i += j;
		}
		if (isdigit(cutParen[i])) {
			result += singleProcess(cutParen, cutParen[i], i);
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

	int answer = calculator(argv[1]);
	printf("answer: %d\n", answer);

	return EXIT_SUCCESS;
}