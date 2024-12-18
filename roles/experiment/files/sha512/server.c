#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>        // For close()
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>          // For clock_gettime()
#include <signal.h>        // For signal handling (optional)

#include "checksum.h"      // Include checksum header if needed

#define BACKLOG 5          // Number of pending connections queue will hold

void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

// Function to perform matrix multiplication to introduce CPU load
void matrix_multiply(int size) {
    double **a = malloc(size * sizeof(double *));
    double **b = malloc(size * sizeof(double *));
    double **c = malloc(size * sizeof(double *));
    if (!a || !b || !c) {
        perror("ERROR allocating matrices");
        exit(EXIT_FAILURE);
    }

    // Initialize matrices
    for (int i = 0; i < size; i++) {
        a[i] = malloc(size * sizeof(double));
        b[i] = malloc(size * sizeof(double));
        c[i] = malloc(size * sizeof(double));
        if (!a[i] || !b[i] || !c[i]) {
            perror("ERROR allocating matrix rows");
            exit(EXIT_FAILURE);
        }
        for (int j = 0; j < size; j++) {
            a[i][j] = rand() / (double)RAND_MAX;
            b[i][j] = rand() / (double)RAND_MAX;
            c[i][j] = 0.0;
        }
    }

    // Perform multiplication
    for (int i = 0; i < size; i++)
        for (int j = 0; j < size; j++)
            for (int k = 0; k < size; k++)
                c[i][j] += a[i][k] * b[k][j];

    // Free memory
    for (int i = 0; i < size; i++) {
        free(a[i]);
        free(b[i]);
        free(c[i]);
    }
    free(a);
    free(b);
    free(c);
}

// Optional: Handle SIGTERM to allow graceful shutdown
void handle_sigterm(int signum) {
    // Implement any cleanup if necessary
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    struct timespec receive_time;
    int cpu_load = 0;
    int matrix_size = 500;  // Adjust as needed

    // Optional: Handle SIGTERM for graceful shutdown
    signal(SIGTERM, handle_sigterm);

    // Check command-line arguments
    if (argc < 3) {
        fprintf(stderr, "Usage: %s port buffer_size [cpu_load]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Parse command-line arguments
    portno = atoi(argv[1]);
    int buffer_size = atoi(argv[2]);
    if (argc >= 4) {
        cpu_load = atoi(argv[3]);
    }

    // Allocate buffer
    char *buffer = malloc(buffer_size);
    if (buffer == NULL) {
        error("ERROR allocating memory for buffer");
    }

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        free(buffer);
        error("ERROR opening socket");
    }

    // Allow address reuse
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        free(buffer);
        error("ERROR setting socket options");
    }

    // Set up server address structure
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    // Bind socket
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        free(buffer);
        error("ERROR on binding");
    }

    // Listen for connections
    if (listen(sockfd, BACKLOG) < 0) {
        free(buffer);
        error("ERROR on listen");
    }

    // Accept a connection
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd < 0) {
        free(buffer);
        error("ERROR on accept");
    }

    // Read data from client
    ssize_t total_read = 0;
    while (total_read < buffer_size) {
        ssize_t n = read(newsockfd, buffer + total_read, buffer_size - total_read);
        if (n < 0) {
            free(buffer);
            error("ERROR reading from socket");
        }
        total_read += n;
    }

    // Record receive time
    if (clock_gettime(CLOCK_REALTIME, &receive_time) == -1) {
        free(buffer);
        error("ERROR getting receive time");
    }

    // Optional CPU load
    if (cpu_load) {
        matrix_multiply(matrix_size);
    }

    // Send receive time back to client
    ssize_t n = write(newsockfd, &receive_time, sizeof(receive_time));
    if (n < 0) {
        free(buffer);
        error("ERROR writing receive time to socket");
    } else if (n != sizeof(receive_time)) {
        free(buffer);
        fprintf(stderr, "ERROR incomplete send of receive time\n");
        exit(EXIT_FAILURE);
    }

    // Clean up
    close(newsockfd);
    close(sockfd);
    free(buffer);
    return 0;
}
