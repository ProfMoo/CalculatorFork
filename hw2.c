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
	printf("My expression is \"%c\"\n", singleEx);
	int convert = singleEx - '0';
	return convert;
}

int parenProcess(char* fullEx, char* singleEx, int location) {
	printf("inside process: %s\n", singleEx);
	printf("location: %d\n", location);

	//cut parentheses
	char cutParen[location];
	int i,j = 0;
	while (i < location-1) {
		cutParen[i] = singleEx[i+1];
		i++;
	}
	cutParen[i+1] = '\0';

	printf("cut paren: %s\n", cutParen);
	fflush(NULL);

	int lengthEx = i;
	char newEx[lengthEx];
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
			parenProcess(cutParen, newEx, j);
		}
		if (isdigit(cutParen[i])) {
			singleProcess(cutParen, cutParen[i], i);
		}
		i++;
	}

	return 1;
}

int calculator(char* expression) {
	printf("expression: %s\n", expression);
	int i,j = 0;

	i = 0;
	while (expression[i] != '\0') {
		i++;
	}
	i++;

	int length = i;
	printf("length: %d\n", length);

	//cut parentheses
	i = 0;
	char cutParen[length];
	while (i < length - 3) {
		printf("char: %c\n", expression[i+1]);
		cutParen[i] = expression[i+1];
		i++;
	}
	cutParen[i+1] = '\0';

	//get operator

	char operator = cutParen[0];
	printf("Starting \"%c\" operation\n", operator);
	fflush(NULL);

	printf("cut paren: %s\n", cutParen);

	i = 0;
	char singleEx[20];
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
			parenProcess(cutParen, singleEx, j);
			i += j;
		}
		if (isdigit(cutParen[i])) {
			singleProcess(cutParen, cutParen[i], i);
		}
		i++;
	}

	return 1;
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