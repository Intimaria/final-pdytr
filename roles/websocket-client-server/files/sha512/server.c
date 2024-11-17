/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include "checksum.h"

void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno, clilen;
     unsigned char checksum[SHA512_DIGEST_LENGTH];
     struct sockaddr_in serv_addr, cli_addr;
     int n;
     memset(checksum, 0, SHA512_DIGEST_LENGTH);

     // Agregado: SE AGREGA BUFFER SIZE COMO ARGUMENTO
     if (argc < 3) {
         fprintf(stderr,"usage: ./server port buffer_size\n");
         exit(1);
     }
     // Agregado: ALOCAR MEMORIA PARA BUFFER 
     int buffer_size = atoi(argv[2]);  // Take the buffer size from the arguments
     char *buffer = malloc(buffer_size * sizeof(char));

     if (buffer == NULL) {
        error("ERROR allocating memory for buffer");
     } 

	// CREA EL FILE DESCRIPTOR DEL SOCKET PARA LA CONEXION
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
	// AF_INET - FAMILIA DEL PROTOCOLO - IPV4 PROTOCOLS INTERNET
	//SOCK_STREAM - TIPO DE SOCKET 
	
     if (sockfd < 0) 
        error("ERROR opening socket");

     // Agregado: PERMITE REUTILIZAR EL MISMO PUERTO - DEBUG POR SCRIPT
     int opt = 1;
     if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
          error("setsockopt");
     }

     bzero((char *) &serv_addr, sizeof(serv_addr));
     // ASIGNA EL PUERTO PASADO POR ARGUMENTO
     // ASIGNA LA IP EN DONDE ESCUCHA (SU PROPIA IP)
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);

	// VINCULA EL FILE DESCRIPTOR CON LA DIRECCION Y EL PUERTO
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
			  
	// SETEA LA CANTIDAD QUE PUEDEN ESPERAR MIENTRAS SE MANEJA UNA CONEXION		  
     listen(sockfd,5);
	 
	// SE BLOQUEA A ESPERAR UNA CONEXION
     clilen = sizeof(cli_addr);
     newsockfd = accept(sockfd, 
                 (struct sockaddr *) &cli_addr, 
                 &clilen);
     
				 
     //DEVUELVE UN NUEVO DESCRIPTOR POR EL CUAL SE VAN A REALIZAR LAS COMUNICACIONES
	if (newsockfd < 0) 
         error("ERROR on accept");
     bzero(buffer,buffer_size);

     // LEE EL MENSAJE DEL CLIENTE
     int buffer_read = 0;
     do {
          n = read(newsockfd, &buffer[buffer_read], buffer_size - buffer_read);
          if (n < 0) {
               error("ERROR reading from socket");
          }
          buffer_read += n;
          } while (buffer_read != buffer_size);
     
     // Agregado: CALUCLA E IMPRIME CHECKSUM
     calculate_checksum(buffer, buffer_size, checksum);
     
     // printf("Checksum after receiving: ");
     //    for(int i = 0; i < SHA512_DIGEST_LENGTH; i++) {
     //    printf("%02x", checksum[i]);
     // }
     
     //    printf("\n");
     
     // Agregado: IMPRIME CANTIDAD DE BYTES RECIBIDOS
     printf("%d\t", buffer_read);
	 
	//RESPONDE AL CLIENTE
     n = write(newsockfd,"I got your message",18);
     if (n < 0) error("ERROR writing to socket");

     // Agregado: CERRAR SOCKET Y LIBERAR BUFFER
     close(sockfd);
     free(buffer);
     return 0; 
}
