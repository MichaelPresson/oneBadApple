#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>

#define MAX_MSG 100
#define MAX_NODES 10

pid_t kids[MAX_NODES];

volatile sig_atomic_t running = 1;

void handle_sigint(int sig) {
    running = 0;

    printf("\nSIGINT received. Terminating all child processes...\n");

    for (int i = 0; i < MAX_NODES; i++) {
		if (kids[i] > 0) {
			kill(kids[i], SIGINT);
		}
    }

    exit(0);
}

void clear_input(void) {
    while (getchar() != '\n' && getchar() != EOF);
}

int setup_pipes(int pipes[][2], int count) {
    for (int i = 0; i < count; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe failed");
            return -1;
        }
        printf("pipe %d -> %d\n", i, (i + 1) % count);
    }
    return 0;
}

void child_node(int id, int total, int pipes[][2]) {
    int has_apple = 0, dest = 0;
    char msg[MAX_MSG] = "empty";

    for (int j = 0; j < total; j++) {
        if (j != (id - 1)) close(pipes[j][0]);
        if (j != id) close(pipes[j][1]);
    }

    printf("Node %d (pid %d) started\n", id, getpid());

    while (running) {
        if (has_apple) {
            if (dest == id) {
                printf("Node %d: got '%s' for me\n", id, msg);
                strcpy(msg, "empty");
                dest = 0;
            } else {
                printf("Node %d: passing '%s' to %d\n", id, msg, dest);
            }

            int next = (id + 1) % total;
            int apple = 1;
            write(pipes[id][1], &apple, sizeof apple);
            write(pipes[id][1], &dest, sizeof dest);
            write(pipes[id][1], msg, MAX_MSG);
            printf("Node %d: sent apple to %d\n", id, next);
            has_apple = 0;
        } else {
            int apple;
            if (read(pipes[id - 1][0], &apple, sizeof apple) > 0 && apple) {
                read(pipes[id - 1][0], &dest, sizeof dest);
                read(pipes[id - 1][0], msg, MAX_MSG);
                printf("Node %d: received apple with '%s' for %d\n", id, msg, dest);
                has_apple = 1;
            }

	    // exit immediately if SIGINT received
	    if (!running) break;
        }
    }

    printf("Node %d shutting down\n", id);
    close(pipes[id - 1][0]);
    close(pipes[id][1]);
    exit(0);
}

int main(void) {
    int node_count;
    pid_t kids[10];

    signal(SIGINT, handle_sigint);

    printf("How many nodes? ");
    if (scanf("%d", &node_count) != 1 || node_count < 1 || node_count > 10) {
        fprintf(stderr, "Need a number between 1 and 10\n");
        return 1;
    }
    clear_input();
    printf("Spawning %d nodes\n", node_count);

    int pipes[node_count][2];
    if (setup_pipes(pipes, node_count) < 0) {
        return 1;
    }

    for (int i = 1; i < node_count; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
            return 1;
        }
        if (pid == 0) {
            child_node(i, node_count, pipes);
        }
        kids[i - 1] = pid;
        printf("Node 0: spawned %d (pid %d)\n", i, pid);
    }

    int has_apple = 1, dest = 0;
    char msg[MAX_MSG] = "empty";

    for (int j = 0; j < node_count; j++) {
        if (j != node_count - 1) close(pipes[j][0]);
        if (j != 0) close(pipes[j][1]);
    }

    printf("Node 0 (pid %d) ready\n", getpid());

    while (running) {
        if (has_apple) {
            if (dest == 0) {
                printf("Node 0: message to send: ");
		fflush(stdout);

                if (fgets(msg, MAX_MSG, stdin) == NULL || !running) {
                    break;
                }

                msg[strcspn(msg, "\n")] = 0;

                printf("Node 0: to which node (0-%d)? ", node_count - 1);
                if (scanf("%d", &dest) != 1 || dest >= node_count) {
                    printf("Node 0: bad destination\n");
                    clear_input();
                    dest = 0;
                    continue;
                }
                clear_input();
                printf("Node 0: sending '%s' to %d\n", msg, dest);
            } else {
                printf("Node 0: apple back with '%s' for %d\n", msg, dest);
            }

            int apple = 1;
            write(pipes[0][1], &apple, sizeof apple);
            write(pipes[0][1], &dest, sizeof dest);
            write(pipes[0][1], msg, MAX_MSG);
            printf("Node 0: apple to Node 1\n");
            has_apple = 0;
        } else {
            int apple;
            if (read(pipes[node_count - 1][0], &apple, sizeof apple) > 0 && apple) {
                read(pipes[node_count - 1][0], &dest, sizeof dest);
                read(pipes[node_count - 1][0], msg, MAX_MSG);
                printf("Node 0: got apple with '%s' for %d\n", msg, dest);
                if (dest == 0) {
                    printf("Node 0: delivered, clearing\n");
                    strcpy(msg, "empty");
                    dest = 0;
                }
                has_apple = 1;
            }
        }
    }

    printf("Node 0: shutting down\n");
    for (int i = 0; i < node_count - 1; i++) {
        if (kids[i] > 0) {
		kill(kids[i], SIGINT);
	}
   }

    for (int i = 0; i < node_count - 1; i++) {
		wait(NULL);
    }

    close(pipes[node_count - 1][0]);
    close(pipes[0][1]);
    printf("Node 0: done\n");
    return 0;
}
