#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>        // For close()
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>          // For clock_gettime()
#include <inttypes.h>      // For precise integer types

#include "checksum.h"      // Include checksum header if needed

#define BUFFER_FILL_CHAR 'A'
#define PROGRESS_INTERVAL 1024

void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    int sockfd, portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    struct timespec send_time, recv_time, server_recv_time;
    double one_way_delay, adjusted_delay, clock_offset = 0.0;
    struct timeval timeout;
    timeout.tv_sec = 15;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));

    // Check command-line arguments
    if (argc < 4) {
        fprintf(stderr, "Usage: %s hostname port buffer_size [clock_offset]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Parse command-line arguments
    const char *hostname = argv[1];
    portno = atoi(argv[2]);
    int buffer_size = atoi(argv[3]);
    if (argc >= 5) {
        clock_offset = atof(argv[4]);
    }

    // Allocate buffer
    char *buffer = malloc(buffer_size);
    if (buffer == NULL) {
        error("ERROR allocating memory for buffer");
    }
    memset(buffer, BUFFER_FILL_CHAR, buffer_size);

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        free(buffer);
        error("ERROR opening socket");
    }

    // Get server by hostname
    server = gethostbyname(hostname);
    if (server == NULL) {
        free(buffer);
        fprintf(stderr, "ERROR, no such host\n");
        exit(EXIT_FAILURE);
    }

    // Set up server address structure
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr,
           server->h_addr,
           server->h_length);
    serv_addr.sin_port = htons(portno);

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        free(buffer);
        error("ERROR connecting");
    }

    // Record send time
    if (clock_gettime(CLOCK_REALTIME, &send_time) == -1) {
        free(buffer);
        error("ERROR getting send time");
    }

    printf("Connecting to server %s on port %d...\n", hostname, portno);
    // Send data to server
    ssize_t total_sent = 0;
    while (total_sent < buffer_size) {
        ssize_t n = write(sockfd, buffer + total_sent, buffer_size - total_sent);
        if (n < 0) {
            free(buffer);
            error("ERROR writing to socket");
        }
        total_sent += n;
        printf("Sent %zd bytes to server (%zd/%d).\n", n, total_sent, buffer_size);
        fflush(stdout);
    }

    // Receive server's receive time
    size_t bytes_to_read = sizeof(server_recv_time);
    size_t total_read = 0;
    while (total_read < bytes_to_read) {
        ssize_t n = read(sockfd, ((char *)&server_recv_time) + total_read, bytes_to_read - total_read);
        if (n < 0) {
            free(buffer);
            error("ERROR reading server receive time");
        } else if (n == 0) {
            free(buffer);
            fprintf(stderr, "ERROR server closed the connection unexpectedly\n");
            exit(EXIT_FAILURE);
        }
        total_read += n;
    }

    if (total_read != bytes_to_read) {
        free(buffer);
        fprintf(stderr, "ERROR incomplete server receive time received\n");
        exit(EXIT_FAILURE);
    }


//    if (total_sent % PROGRESS_INTERVAL == 0) {
//        printf("Sent %zd/%d bytes\n", total_sent, buffer_size);
//    }
//
//    // Receive server's receive time
//    ssize_t n = read(sockfd, &server_recv_time, sizeof(server_recv_time));
//    if (n < 0) {
//        free(buffer);
//        error("ERROR reading server receive time");
//    } else if (n != sizeof(server_recv_time)) {
//        free(buffer);
//        fprintf(stderr, "ERROR incomplete server receive time received\n");
//        exit(EXIT_FAILURE);
//    }

    // Record receive time
    if (clock_gettime(CLOCK_REALTIME, &recv_time) == -1) {
        free(buffer);
        error("ERROR getting receive time");
    }


    // Calculate one-way delay
    long sec_diff = server_recv_time.tv_sec - send_time.tv_sec + clock_offset;
    long nsec_diff = server_recv_time.tv_nsec - send_time.tv_nsec;

    if (nsec_diff < 0) {
        sec_diff -= 1;
        nsec_diff += 1000000000; // Add one second in nanoseconds
    }



    one_way_delay = sec_diff + nsec_diff / 1e9;

    // Adjust for clock offset - adjusted_delay = one_way_delay - clock_offset;
    adjusted_delay = one_way_delay;

    // Output the adjusted one-way delay
    printf("One-way delay (adjusted): %f seconds\n", adjusted_delay);

    // Clean up
    close(sockfd);
    free(buffer);
    return 0;
}
