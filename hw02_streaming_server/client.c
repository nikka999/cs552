#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>

int cliConn (char *host, int port) {
 
  struct sockaddr_in name;
  struct hostent *hent;
  int sd;
 
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("(cliConn): socket() error");
    exit (-1);
  }
 
  if ((hent = gethostbyname (host)) == NULL)
    fprintf (stderr, "Host %s not found.\n", host);
  else
    bcopy (hent->h_addr, &name.sin_addr, hent->h_length);
 
  name.sin_family = AF_INET;
  name.sin_port = htons (port);
 
  /* connect port */
  if (connect (sd, (struct sockaddr *)&name, sizeof(name)) < 0) {
    perror("(cliConn): connect() error");
    exit (-1);
  }
 
  return (sd);
}


int main (int argc, char const *argv[])
{
	size_t len;
	char buffer[20];
	char *msg = "Just doing some random tests...";
  	int sd = cliConn ("localhost", 5050);
	len = strlen(msg);
	len = htonl(len);
	while(1) {
		while(fgets(buffer, 20, stdin)) {
			if (!strcmp(buffer, "s\n")) {
				write (sd, &len, sizeof(size_t));
				printf("the msg is: %s\n", msg);
				write (sd, msg, strlen(msg));			
			}
			else if(!strcmp(buffer, "q\n")) {
				close(sd);
				exit(0);
			}
		}
	}
  	return 0;
}


