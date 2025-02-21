#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_MESSAGE_LENGTH 100

int main() {
    int k;
    pid_t pid;

    // Capture number of processes
    printf("Enter number of processes: \n");
    fflush(stdout);
    if (scanf("%d", &k) != 1 || k < 1) {
        fprintf(stderr, "Invalid input! Please enter a positive integer.\n");
        return 1;
    }
    printf("# of processes to spawn: %d\n", k);

    // Create k pipes for the ring
    printf("Creating pipes...\n");
    int pipes[k][2];
    for (int i = 0; i < k; ++i) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe failed");
            exit(1);
        }
        printf("Created pipe: Node %d -> Node %d\n", i, (i + 1) % k);
    }

    // Spawn k-1 children (parent is Node 0)
    for (int i = 1; i < k; ++i) { // Start at 1 since parent is 0
        pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(1);
        } else if (pid == 0) { // Child process
            int nodeId = i;
            int hasApple = 0;
            int destination = 0;
            char message[MAX_MESSAGE_LENGTH];

            // Close unused pipe ends
            for (int j = 0; j < k; j++) {
                if (j != (nodeId - 1)) close(pipes[j][0]); // Keep read from previous
                if (j != nodeId) close(pipes[j][1]);       // Keep write to next
            }
            printf("Node %d (PID: %d): Created\n", nodeId, getpid());

            // Contribution 1: Persistent ring with apple sync
            while (1) {
                if (hasApple) {
                    read(pipes[nodeId - 1][0], &destination, sizeof(int));
                    read(pipes[nodeId - 1][0], message, MAX_MESSAGE_LENGTH);
                    printf("Node %d (PID: %d): Received apple with message '%s' for Node %d\n",
                           nodeId, getpid(), message, destination);

                    if (destination == nodeId) {
                        printf("Node %d: Message '%s' is for me! Clearing it.\n", nodeId, message);
                        strcpy(message, "empty");
                        destination = 0;
                    } else {
                        printf("Node %d: Forwarding '%s' to Node %d\n", nodeId, message, destination);
                    }

                    // Pass apple and message to next node
                    int nextNode = (nodeId + 1) % k;
                    write(pipes[nodeId][1], &hasApple, sizeof(int)); // Pass apple
                    write(pipes[nodeId][1], &destination, sizeof(int));
                    write(pipes[nodeId][1], message, MAX_MESSAGE_LENGTH);
                    printf("Node %d: Passed apple to Node %d\n", nodeId, nextNode);
                    hasApple = 0;
                } else {
                    int appleReceived;
                    read(pipes[nodeId - 1][0], &appleReceived, sizeof(int));
                    if (appleReceived) {
                        read(pipes[nodeId - 1][0], &destination, sizeof(int));
                        read(pipes[nodeId - 1][0], message, MAX_MESSAGE_LENGTH);
                        printf("Node %d (PID: %d): Got apple with '%s' for Node %d\n",
                               nodeId, getpid(), message, destination);
                        hasApple = 1;
                    }
                }
            }
            exit(0); // Unreachable due to loop
        } else { // Parent
            printf("Parent (Node 0, PID: %d): Created Node %d (PID: %d)\n", getpid(), i, pid);
        }
    }

    // Parent (Node 0) logic
    int hasApple = 1; // Node 0 starts with apple
    int destination = 0;
    char message[MAX_MESSAGE_LENGTH];

    // Close unused pipe ends in parent
    for (int j = 0; j < k; j++) {
        if (j != (k - 1)) close(pipes[j][0]); // Keep read from k-1
        if (j != 0) close(pipes[j][1]);       // Keep write to 1
    }
    printf("Node 0 (PID: %d): Ready\n", getpid());

    // Contribution 2: Parent message input and forwarding
    while (1) {
        if (hasApple) {
            char inputMessage[MAX_MESSAGE_LENGTH];
            if (destination == 0) { // Only prompt if no message is in transit
                printf("Node 0: Enter message to send: ");
                fgets(inputMessage, MAX_MESSAGE_LENGTH, stdin); // Multi-word support
                inputMessage[strcspn(inputMessage, "\n")] = 0;  // Remove newline
                printf("\nNode 0: Enter destination node (0 to %d): ", k - 1);
                scanf("%d", &destination);
                getchar(); // Clear newline
                strncpy(message, inputMessage, MAX_MESSAGE_LENGTH);
                printf("Node 0: Sending '%s' to Node %d\n", message, destination);
            } else {
                printf("Node 0: Apple returned with '%s' for Node %d\n", message, destination);
            }

            // Pass apple and message to Node 1
            write(pipes[0][1], &hasApple, sizeof(int));
            write(pipes[0][1], &destination, sizeof(int));
            write(pipes[0][1], message, MAX_MESSAGE_LENGTH);
            printf("Node 0: Passed apple to Node 1\n");
            hasApple = 0;
        } else {
            int appleReceived;
            read(pipes[k - 1][0], &appleReceived, sizeof(int));

            if (appleReceived) {
                read(pipes[k - 1][0], &destination, sizeof(int));
                read(pipes[k - 1][0], message, MAX_MESSAGE_LENGTH);
                printf("Node 0 (PID: %d): Got apple with '%s' for Node %d\n",
                       getpid(), message, destination);
                if (destination == 0) {
                    strcpy(message, "empty"); // Clear if delivered to Node 0
                }
                hasApple = 1;
            }
        }
    }

    // Parent waits (not needed due to infinite loop)
    return 0;
}