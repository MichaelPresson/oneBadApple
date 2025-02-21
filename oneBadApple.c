#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>

#define MAX_MESSAGE_LENGTH 100

// Global variables for shutdown
volatile sig_atomic_t keepRunning = 1;

void signalHandler(int sig) {
    keepRunning = 0; // Gracefully stop on Ctrl+C
}

int main() {
    int numNodes;
    pid_t pid;

    // Set up signal handler for Ctrl+C
    signal(SIGINT, signalHandler);

    // Get number of processes from user
    printf("Enter number of processes: \n");
    fflush(stdout);
    if (scanf("%d", &numNodes) != 1 || numNodes < 1) {
        fprintf(stderr, "Invalid input! Please enter a positive integer.\n");
        return 1;
    }
    while (getchar() != '\n' && getchar() != EOF); // Clear input buffer
    printf("Number of processes to spawn: %d\n", numNodes);

    // Create pipes for the ring
    printf("Creating pipes for %d nodes...\n", numNodes);
    int pipes[numNodes][2];
    for (int i = 0; i < numNodes; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("Pipe creation failed");
            exit(1);
        }
        printf("Created pipe: Node %d -> Node %d\n", i, (i + 1) % numNodes);
    }

    // Spawn child processes (parent is Node 0)
    pid_t childPids[numNodes - 1]; // Store child PIDs for shutdown
    for (int i = 1; i < numNodes; i++) {
        pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            exit(1);
        } else if (pid == 0) { // Child process
            int nodeId = i;
            int hasApple = 0;
            int destinationNode = 0;
            char message[MAX_MESSAGE_LENGTH];

            // Close unused pipe ends
            for (int j = 0; j < numNodes; j++) {
                if (j != (nodeId - 1)) close(pipes[j][0]); // Keep read from previous
                if (j != nodeId) close(pipes[j][1]);       // Keep write to next
            }

            printf("\nNode %d (PID: %d): Created\n", nodeId, getpid());

            // Child node logic
            while (keepRunning) {
                if (hasApple) {
                    printf("Node %d: Has apple, checking message\n", nodeId);
                    if (destinationNode == nodeId) {
                        printf("Node %d: Message '%s' delivered to me, clearing\n", nodeId, message);
                        strcpy(message, "empty");
                        destinationNode = 0;
                    } else {
                        printf("Node %d: Forwarding '%s' to Node %d\n", nodeId, message, destinationNode);
                    }

                    // Pass apple to next node
                    int nextNode = (nodeId + 1) % numNodes;
                    int appleFlag = 1;
                    if (write(pipes[nodeId][1], &appleFlag, sizeof(int)) == -1 ||
                        write(pipes[nodeId][1], &destinationNode, sizeof(int)) == -1 ||
                        write(pipes[nodeId][1], message, MAX_MESSAGE_LENGTH) == -1) {
                        perror("Node write failed");
                        exit(1);
                    }
                    printf("Node %d: Passed apple to Node %d\n", nodeId, nextNode);
                    hasApple = 0;
                } else {
                    int appleReceived;
                    ssize_t bytesRead = read(pipes[nodeId - 1][0], &appleReceived, sizeof(int));
                    if (bytesRead == -1) {
                        perror("Node read failed");
                        exit(1);
                    }
                    if (bytesRead == 0) {
                        continue; // No data yet, keep looping
                    }
                    if (appleReceived) {
                        if (read(pipes[nodeId - 1][0], &destinationNode, sizeof(int)) == -1 ||
                            read(pipes[nodeId - 1][0], message, MAX_MESSAGE_LENGTH) == -1) {
                            perror("Node read failed");
                            exit(1);
                        }
                        printf("Node %d (PID: %d): Received apple with '%s' for Node %d\n",
                               nodeId, getpid(), message, destinationNode);
                        hasApple = 1;
                    }
                }
            }
            printf("Node %d (PID: %d): Shutting down\n", nodeId, getpid());
            close(pipes[nodeId - 1][0]);
            close(pipes[nodeId][1]);
            exit(0);
        } else { // Parent
            childPids[i - 1] = pid;
            printf("Node 0 (PID: %d): Spawned Node %d (PID: %d)\n", getpid(), i, pid);
        }
    }

    // Parent (Node 0) logic
    int hasApple = 1; // Node 0 starts with apple
    int destinationNode = 0;
    char message[MAX_MESSAGE_LENGTH] = "empty";

    // Close unused pipe ends in parent
    for (int j = 0; j < numNodes; j++) {
        if (j != (numNodes - 1)) close(pipes[j][0]); // Keep read from last node
        if (j != 0) close(pipes[j][1]);              // Keep write to Node 1
    }
    printf("Node 0 (PID: %d): Ready\n", getpid());

    // Parent loop
    while (keepRunning) {
        if (hasApple) {
            char inputMessage[MAX_MESSAGE_LENGTH];
            if (destinationNode == 0) { // Idle, prompt user
                printf("Node 0: Enter message to send: ");
                fflush(stdout);
                if (fgets(inputMessage, MAX_MESSAGE_LENGTH, stdin) == NULL) {
                    if (!keepRunning) break; // Ctrl+C detected
                    perror("Input error");
                    continue;
                }
                inputMessage[strcspn(inputMessage, "\n")] = '\0';

                printf("Node 0: Enter destination node (0 to %d): ", numNodes - 1);
                fflush(stdout);
                if (scanf("%d", &destinationNode) != 1 || destinationNode < 0 || destinationNode >= numNodes) {
                    printf("Node 0: Invalid destination, must be 0 to %d\n", numNodes - 1);
                    while (getchar() != '\n' && getchar() != EOF);
                    destinationNode = 0;
                    continue;
                }
                while (getchar() != '\n' && getchar() != EOF);

                strncpy(message, inputMessage, MAX_MESSAGE_LENGTH);
                message[MAX_MESSAGE_LENGTH - 1] = '\0';
                printf("Node 0: Sending '%s' to Node %d\n", message, destinationNode);
            } else {
                printf("Node 0: Apple returned with '%s' for Node %d\n", message, destinationNode);
            }

            // Pass apple to Node 1
            int appleFlag = 1;
            if (write(pipes[0][1], &appleFlag, sizeof(int)) == -1 ||
                write(pipes[0][1], &destinationNode, sizeof(int)) == -1 ||
                write(pipes[0][1], message, MAX_MESSAGE_LENGTH) == -1) {
                perror("Node 0 write failed");
                break;
            }
            printf("Node 0: Passed apple to Node 1\n");
            hasApple = 0;
        } else {
            int appleReceived;
            ssize_t bytesRead = read(pipes[numNodes - 1][0], &appleReceived, sizeof(int));
            if (bytesRead == -1) {
                perror("Node 0 read failed");
                break;
            }
            if (bytesRead == 0) {
                continue; // No data yet
            }
            if (appleReceived) {
                if (read(pipes[numNodes - 1][0], &destinationNode, sizeof(int)) == -1 ||
                    read(pipes[numNodes - 1][0], message, MAX_MESSAGE_LENGTH) == -1) {
                    perror("Node 0 read failed");
                    break;
                }
                printf("Node 0 (PID: %d): Received apple with '%s' for Node %d\n",
                       getpid(), message, destinationNode);
                if (destinationNode == 0) {
                    printf("Node 0: Message delivered, clearing\n");
                    strcpy(message, "empty");
                    destinationNode = 0;
                }
                hasApple = 1;
            }
        }
    }

    // Shutdown: Signal children and clean up
    printf("Node 0: Received Ctrl+C, shutting down...\n");
    for (int i = 0; i < numNodes - 1; i++) {
        kill(childPids[i], SIGINT);
    }
    for (int i = 0; i < numNodes - 1; i++) {
        wait(NULL); // Wait for children to exit
    }
    printf("Node 0: All nodes terminated\n");
    close(pipes[numNodes - 1][0]);
    close(pipes[0][1]);
    return 0;
}