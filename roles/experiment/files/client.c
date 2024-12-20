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
#include <pthread.h>
#include "checksum.h"      // Include checksum header if needed

#define BUFFER_FILL_CHAR 'A'
#define PROGRESS_INTERVAL 1024

static int cpu_load_flag = 0;

// matrix_multiply function (copied from server)
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
        sleep(1000);
    }
    return NULL;
}

void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    int sockfd, portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    struct timespec send_time, recv_time, server_send_time, server_recv_time;
    double one_way_delay, adjusted_delay, clock_offset = 0.0;
    struct timeval timeout;
    pthread_t cpu_load_thread;
    int matrix_size = 52443348; 

    timeout.tv_sec = 15;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));

    // Check command-line arguments
    if (argc < 4) {
        fprintf(stderr, "Usage: %s hostname port buffer_size cpu_load_flag [clock_offset]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Parse command-line arguments
    const char *hostname = argv[1];
    portno = atoi(argv[2]);
    int buffer_size = atoi(argv[3]);
    cpu_load_flag = atoi(argv[5]);
    if (argc >= 6) {
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

    // CPU Load
/*    if (cpu_load_flag) {
      if (pthread_create(&cpu_load_thread, NULL, matrix_multiply, &matrix_size) != 0) {
        error("ERROR creating CPU load thread");
      }
    }
*/
    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    pthread_t *cpu_load_threads = malloc(num_cores * sizeof(pthread_t));
    for (int i = 0; i < num_cores; i++) {
        pthread_create(&cpu_load_threads[i], NULL, matrix_multiply, &matrix_size);
    }


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
    size_t bytes_to_read = sizeof(server_recv_time) + sizeof(server_send_time);
    char time_buffer[bytes_to_read];
    size_t total_read = 0;
/*    while (total_read < bytes_to_read) {
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
    long sec_diff = server_recv_time.tv_sec - send_time.tv_sec;
    long nsec_diff = server_recv_time.tv_nsec - send_time.tv_nsec;

    if (nsec_diff < 0) {
        sec_diff -= 1;
        nsec_diff += 1000000000; // Add one second in nanoseconds
    }



    one_way_delay = sec_diff + nsec_diff / 1e9;

    // Adjust for clock offset - adjusted_delay = one_way_delay - clock_offset;
    adjusted_delay = one_way_delay;

    // Output the adjusted one-way delay
    printf("One-way delay (%f seconds\n", adjusted_delay);
*/


    while (total_read < bytes_to_read) {
        ssize_t n = read(sockfd, time_buffer + total_read, bytes_to_read - total_read);
        if (n < 0) {
            free(buffer);
            error("ERROR reading times from server");
        } else if (n == 0) {
            free(buffer);
            fprintf(stderr, "ERROR: server closed connection unexpectedly\n");
            exit(EXIT_FAILURE);
        }
        total_read += n;
    }

    // Extract the two times
    memcpy(&server_recv_time, time_buffer, sizeof(server_recv_time));
    memcpy(&server_send_time, time_buffer + sizeof(server_recv_time), sizeof(server_send_time));

    // Record client receive time (already done after read)
    if (clock_gettime(CLOCK_REALTIME, &recv_time) == -1) {
        free(buffer);
        error("ERROR getting client receive time");
    }

    // Compute both directions (assuming you've done any offset correction if desired)
    /*
    double c2s_delay = (server_recv_time.tv_sec - send_time.tv_sec) +
                      (server_recv_time.tv_nsec - send_time.tv_nsec) / 1e9;

    double s2c_delay = (recv_time.tv_sec - server_send_time.tv_sec) +
                      (recv_time.tv_nsec - server_send_time.tv_nsec) / 1e9;
*/
    // Compute Client → Server delay
    long sec_diff_c2s = server_recv_time.tv_sec - send_time.tv_sec;
    long nsec_diff_c2s = server_recv_time.tv_nsec - send_time.tv_nsec;

    if (nsec_diff_c2s < 0) {
        sec_diff_c2s -= 1;
        nsec_diff_c2s += 1000000000;  // Add 1 second's worth of nanoseconds
    }

    double c2s_delay = sec_diff_c2s + nsec_diff_c2s / 1e9;

    // Compute Server → Client delay
    long sec_diff_s2c = recv_time.tv_sec - server_send_time.tv_sec;
    long nsec_diff_s2c = recv_time.tv_nsec - server_send_time.tv_nsec;

    if (nsec_diff_s2c < 0) {
        sec_diff_s2c -= 1;
        nsec_diff_s2c += 1000000000;  // Add 1 second's worth of nanoseconds
    } else if (sec_diff_s2c < 0 && nsec_diff_s2c > 0) {
        // Adjust for negative seconds with positive nanoseconds
        sec_diff_s2c += 1;
        nsec_diff_s2c -= 1000000000;
    }

    double s2c_delay = sec_diff_s2c + nsec_diff_s2c / 1e9;

// Print results
    // If you have a clock_offset from your local NTP server, apply it as needed
    // This offset would adjust the measured times, e.g.:
    //c2s_delay -= clock_offset;
    // s2c_delay -= clock_offset;

    
    printf("Seconds Difference (C2S): %ld\n", sec_diff_c2s);
    printf("Nanoseconds Difference (C2S): %ld\n", nsec_diff_c2s);
    printf("Seconds Difference (S2C): %ld\n", sec_diff_s2c);
    printf("Nanoseconds Difference (S2C): %ld\n", nsec_diff_s2c);

    printf("Client→Server One-Way Delay: %.6f seconds\n", c2s_delay);
    printf("Server→Client One-Way Delay: %.6f seconds\n", s2c_delay);
//    printf("Client→Server One-Way Delay: %f seconds\n", c2s_delay);
//    printf("Server→Client One-Way Delay: %f seconds\n", s2c_delay);
    // Clean up
 /*   if (cpu_load_flag) {
      cpu_load_flag = 0;
      for (int i = 0; i < num_cores; i++) {
        pthread_join(cpu_load_threads[i], NULL);
      }
      free(cpu_load_threads);
    } */
    close(sockfd);
    free(buffer);
    return 0;
}
