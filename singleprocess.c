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