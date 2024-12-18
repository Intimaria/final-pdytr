#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <signal.h>

#include "checksum.h" // Include your checksum header

#define BACKLOG 5 // Number of pending connections queue will hold

int sockfd, newsockfd;
pthread_t cpu_load_thread;
int cpu_load_flag = 0;


void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void handle_signal(int signal) {
    close(sockfd);
    exit(EXIT_SUCCESS);
}

// Function to perform matrix multiplication for CPU load
void *matrix_multiply(void *arg) {
    int size = *((int *)arg);
    while (cpu_load_flag) {
        double **a = malloc(size * sizeof(double *));
        double **b = malloc(size * sizeof(double *));
        double **c = malloc(size * sizeof(double *));
        if (!a || !b || !c) {
            perror("ERROR allocating matrices");
            exit(EXIT_FAILURE);
        }

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

        for (int i = 0; i < size; i++)
            for (int j = 0; j < size; j++)
                for (int k = 0; k < size; k++)
                    c[i][j] += a[i][k] * b[k][j];

        for (int i = 0; i < size; i++) {
            free(a[i]);
            free(b[i]);
            free(c[i]);
        }
        free(a);
        free(b);
        free(c);
    }
    return NULL;
}

// Function to wait for the full buffer and calculate checksum
void receive_and_process_data(int client_fd, char *buffer, int buffer_size) {
    ssize_t total_read = 0;
    while (total_read < buffer_size) {
        ssize_t n = read(client_fd, buffer + total_read, buffer_size - total_read);
        if (n < 0) {
            error("ERROR reading from socket");
        }
        total_read += n;
        printf("Server received %zd bytes (total %zd).\n", n, total_read);
        fflush(stdout);
    }

    // Calculate checksum
    unsigned char checksum[SHA512_DIGEST_LENGTH];
    calculate_checksum(buffer, buffer_size, checksum);

    // Print checksum
    printf("Received checksum: ");
    for (int i = 0; i < SHA512_DIGEST_LENGTH; i++) {
        printf("%02x", checksum[i]);
    }
    printf("\n");
    fflush(stdout);
}
int main(int argc, char *argv[]) {
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    struct timespec recv_time;
    int matrix_size = 500;

    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);

    // Check command-line arguments
    if (argc < 3) {
        fprintf(stderr, "Usage: %s port buffer_size\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int portno = atoi(argv[1]);
    int buffer_size = atoi(argv[2]);
    cpu_load_flag = atoi(argv[3]);  // Enable/disable CPU load based on argument

    // Allocate buffer
    char *buffer = malloc(buffer_size);
    if (!buffer) {
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

    // Set up server address
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

    // Optional CPU load
    if (cpu_load_flag) {
        pthread_create(&cpu_load_thread, NULL, matrix_multiply, &matrix_size);
    }

    // Receive and process data
    receive_and_process_data(newsockfd, buffer, buffer_size);

    // Record the receive time
    if (clock_gettime(CLOCK_REALTIME, &recv_time) == -1) {
        free(buffer);
        error("ERROR getting receive time");
    }

    // Send the receive time back to the client
    ssize_t total_sent = 0;
    size_t bytes_to_send = sizeof(recv_time);

    while (total_sent < bytes_to_send) {
        ssize_t n = write(newsockfd, ((char *)&recv_time) + total_sent, bytes_to_send - total_sent);
        if (n < 0) {
            perror("ERROR writing receive time to socket");
            close(newsockfd);
            free(buffer);
            exit(EXIT_FAILURE);
        }
        total_sent += n;
    }
    printf("Sent %zu bytes of server receive time to client.\n", total_sent);

    // Optional: Stop CPU load thread
    if (cpu_load_flag) {
        cpu_load_flag = 0;
        pthread_join(cpu_load_thread, NULL);
    }

    // Close sockets and free resources
    close(newsockfd);
    close(sockfd);
    free(buffer);

    return 0;
}
