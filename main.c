#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <time.h>

#define BUFFER_SIZE 48
#define READ_END 0
#define WRITE_END 1
#define SLEEP_DURATION 3
#define PROGRAM_DURATION 30
#define CHILDREN 5

//global variables
double startTime;

char write_msg[BUFFER_SIZE];
char read_msg[BUFFER_SIZE] = "";

//create sleep time with 0, 1, or 2
int sleepTime() {
	return rand()%SLEEP_DURATION;
}

//getting elapsed time
double getElapsedTime() {
	struct timeval now;
    gettimeofday(&now, NULL);
	double currentTime = (now.tv_sec) * 1000 + (now.tv_usec) / 1000;
	return (currentTime - startTime)/1000;
}

//main function
int main() {
    struct timeval start;
	FILE *fp;
	gettimeofday(&start, NULL);         //timer begins
	startTime = (start.tv_sec) * 1000 + (start.tv_usec) / 1000;
    pid_t pid, wpid;                    // child process id
    int i, j, k;
    int seedtime;
    int x = -1;
    int fd[CHILDREN][2];                // file descriptors for the pipe

    // Create the pipe.
    for (i=0; i< CHILDREN; i++)
    {
        if (pipe(fd[i]) == -1) {
        fprintf(stderr,"pipe() failed");
        return 1;
        }
    }

    fd_set inputs, inputfds;            //set file descriptors array
    struct timeval timeout;
    FD_ZERO(&inputs);                   //empty set
	fp = fopen("output.txt","w");       //open file

    // Fork a child process.
    for (i = 0; i < CHILDREN; i++) {
		pid = fork();
		if (pid > 0) {
            // PARENT PROCESS.
            FD_SET(fd[i][READ_END], &inputs);

            // Close the WRITE end of the pipe.
            close (fd[i][WRITE_END]);
			continue;
		}
		else if (pid == 0) {
            // CHILD PROCESS.
			x = i+1;
			break;
		}
		else {
			printf("fork() failed\n");
			return 1;
		}
	}


    // Parent
    if (pid > 0)
	{
        while(PROGRAM_DURATION > getElapsedTime())
		{
            inputfds = inputs;
            int result;
            timeout.tv_sec = 3;
            timeout.tv_usec =000000;

            result = select(FD_SETSIZE, &inputfds, NULL, NULL, &timeout);

            switch(result)
			{
				case 0: {
					fflush(stdout);
					break;
				}

				case -1: {
					perror("select error");
					return 1;
				}
				default: {
					for (k=0; k<CHILDREN; k++)
					{
						if (FD_ISSET(fd[k][READ_END], &inputfds))
						{
							if(read(fd[k][READ_END], read_msg, BUFFER_SIZE)) {
								double p_sec = getElapsedTime();
								double p_min = 0;

								while (p_sec >=60){
									p_min++;
									p_sec -=60;
								}

								char message[BUFFER_SIZE*2];
								sprintf(message,"%02.0f:%06.3lf : %s \n", p_min, p_sec, read_msg);
								fputs(message, fp);
							}
						}
					}
					break;
				}
			}
		}
		for (i=0; i<CHILDREN; i++)
		{
			close(fd[i][READ_END]);
		}
		fclose(fp);
    }
    // Child
    else {
        for (i = 0; i<CHILDREN; i++)
        {
             close(fd[i][READ_END]);
        }
        srand(x);
        for (j=1; PROGRAM_DURATION > getElapsedTime(); j++) {
            double sec = getElapsedTime();
            double min = 0;

            while (sec >=60){
                min++;
                sec -=60;
            }

			if(x == 5)
			{
				printf("Write message to the pipe:\n");
				char input[BUFFER_SIZE];
				gets(input);

				sprintf(write_msg, "Child %d message %s ", x, input);
				write(fd[x-1][WRITE_END], write_msg, BUFFER_SIZE);
				printf("\n");
			}
			else
			{
				sprintf(write_msg, "Child %d message %d", x, j);
				write(fd[x-1][WRITE_END], write_msg, BUFFER_SIZE);
				sleep(sleepTime());             //sleeps 0, 1, or 2 sec
			}
        }

        close(fd[x-1][WRITE_END]);

    }
}
