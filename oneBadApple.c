#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int main() {

        int k;
        pid_t pid;


        printf("Enter number of processes: \n");
        fflush(stdout);
        if (scanf("%d", &k) != 1 || k < 1) {
                fprintf(stderr, "Invalid input! Please enter a positive integer.\n");
                return 1;
        }
        printf("# of processes to spawn: %d\n", k);

        int pipes[k][2]; // create k pipes (each with read/write ends)

        for (int i = 0; i < k; ++i) {
                if (pipe(pipes[i]) == -1) {
                        perror("pipe failed");
                        exit(1);
                }
        }

        for (int i = 0; i < k; ++i) {
                pid = fork();


                if (pid < 0) {
                        perror("fork");
                        exit(1);
                } else if (pid == 0) {
                        // child process
                        int apple;
                        char message[100];

                        // close unused pipe ends
                        close(pipes[i][1]); // close write end of the previous pipe
                        close(pipes[(i + 1) % k][0]);

                        read(pipes[i][0], &apple, sizeof(int)); // read apple
                        read(pipes[i][0], message, sizeof(message)); // read message

                        if (apple == 1) {
  				printf("Process %d (PID: %d) has the apple and received message: %s\n", i, getpid(), message);
                        }
                        ///////////////////////////////

                } else {
                        // parent process
                        printf("Parent process (PID: %d) created child %d (PID: %d)\n", getpid(), i + 1, pid);

                }

        }
        // parent waits for all child processes to finish
        for (int i = 0; i < k; ++i) {
                wait(NULL);
        }

        printf("Process (PID: %d) finished\n", getpid());


        return 0;


}
