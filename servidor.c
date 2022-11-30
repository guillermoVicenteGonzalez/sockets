#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define PUERTO 17278
#define ADDRNOTFOUND	0xffffffff	/* return address for unfound host */
#define BUFFERSIZE	1024	/* maximum size of packets to be received */
#define TAM_BUFFER 150
#define MAXHOST 128

extern int errno;

void serverTCP(int s, struct sockaddr_in clientaddr_in);
void errout(char *hostname)
{
	printf("Connection with %s aborted on error\n", hostname);
	exit(1);     
}

typedef struct mailState{
	int helo;
	int mail;
	int data;
	int rcpt;
}mailState;

typedef struct mail{
	char emisor[100];
	char mensaje[100];
}mail;

int main(int argc, char  *argv[])
{
	int sListen, s;
	int port = PUERTO;
	int sockaddrSize;
	struct sockaddr_in serverData, clientData;
	

	//pongo los flags a 0


	fprintf(stderr, "empieza\n");

	//limpieza de estructuras
	printf("limpio estructuras\n");
	memset (&clientData, 0, sizeof(clientData));
	memset (&serverData, 0, sizeof(serverData));

	//creacion socket
	printf("creo el socket\n");
	if(-1 == (sListen= socket (AF_INET, SOCK_STREAM, 0))){
		perror("socket");
		exit(EXIT_FAILURE);
	}

	//rellenar estructuras para trabajar con ipv4 y el puerto definido
	serverData.sin_family = AF_INET;
	serverData.sin_addr.s_addr = INADDR_ANY;
	serverData.sin_port = htons(port);

	//bindeamos el socket a nuestra dirección
	printf("bindeo el socket\n");
	if (-1 == bind(sListen, (const struct sockaddr *) &serverData, sizeof(serverData))) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	printf("clientes en espera\n");
	if (-1 == listen(sListen, 5)) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	while(1){
		if(-1 == (s= accept(sListen, (struct sockaddr *) &clientData, &sockaddrSize))){
			perror("accept");
			exit(EXIT_FAILURE);
		}
		//printf("en el bucle");
		serverTCP(s,clientData);
	}

	return 0;
}

void serverTCP(int s, struct sockaddr_in clientaddr_in){
/**********************************************************
 * VARIABLES
 * ********************************************************/

	int reqcnt = 0;		//numero de requests
	char buf[TAM_BUFFER];		/* This example uses TAM_BUFFER byte messages. */
	char hostname[MAXHOST];		//nombre del remote host

	int len, len1, status;
    struct hostent *hp;		/* pointer to host info for remote host */
    long timevar;			/* contains time returned by time() */
    
    struct linger linger;

    //estado del mail
    mailState state;
    //contenido del mail
    mail miMail;

    //tratamiento de cadenas
    int contador = 0;
    char linea[100];
    char *palabra;
    char delimiter[1] = " ";

/**********************************************************
 * FUNCION
 * ********************************************************/

    state.helo=0; state.mail=0; state.data=0; state.rcpt=0;

	status = getnameinfo((struct sockaddr *)&clientaddr_in,sizeof(clientaddr_in),
                           hostname,MAXHOST,NULL,0,0);

	if(status){
		//lainformacion no esta disponible. 
		if (inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostname, MAXHOST) == NULL)
            	perror(" inet_ntop \n");
	}
	time (&timevar);
	printf("Startup from %s port %u at %s",
		hostname, ntohs(clientaddr_in.sin_port), (char *) ctime(&timevar));
	printf("-----------------------------------------------------------\n\n");
	linger.l_onoff  =1;
	linger.l_linger =1;

	if (setsockopt(s, SOL_SOCKET, SO_LINGER, &linger,sizeof(linger)) == -1){
		errout(hostname);
	}

	//este bucle lo tiene que ejecutar un hijo
	//cuando sea la condicion de salida el hijo muere?
	while (len = recv(s, buf, TAM_BUFFER, 0)) {
		if (len == -1) errout(hostname); 

		//posibilidad remota de error
		while (len < TAM_BUFFER) {
			len1 = recv(s, &buf[len], TAM_BUFFER-len, 0);
			if (len1 == -1) errout(hostname);
			len += len1;
		}

		//incrementamos el nuero de requests y espero 
		reqcnt++;
		sleep(1);

		//primero dividimos la cadena

/***************************************************************
 * TRATAMIENTO DE CADENAS
 * *************************************************************/
		palabra = strtok(buf," ");
		if(strcmp(palabra,"HELO") == 0){
			fprintf(stdout,"HELO recibido\n");
			fprintf(stdout,"cadena recibida: %s\n",buf);
			palabra = strtok(NULL, " ");
			if(palabra != NULL){
				//deberia comprobar si el sender es valido
				fprintf(stdout,"palabra 2: %s",palabra);
				strcpy(miMail.emisor,palabra);
				palabra = strtok(NULL, " ");
				if(palabra != NULL){
					//error. Solo se espera un campo.
					//limpio estructuras y devuelvo error.
				}else{
					strcpy(buf,"500");
				}
			}
			//solo ha de tener 2 campos.
		}else if(strcmp(palabra,"MAIL") == 0){
			fprintf(stdout,"MAIL recibido\n");
			fprintf(stdout,"cadena recibida: %s\n",buf);
			strcpy(buf,"200");
		}else if(strcmp(palabra,"RCPT") == 0){
			fprintf(stdout,"RCPT recibido\n");
			fprintf(stdout,"cadena recibida: %s\n",buf);
			strcpy(buf,"200");
		}else if(strcmp(palabra,"DATA") == 0){
			fprintf(stdout,"DATA recibido\n");
			fprintf(stdout,"cadena recibida: %s\n",buf);
			strcpy(buf,"200");
		}else if(strcmp(palabra, "QUIT") == 0){
			fprintf(stdout,"QUIT recibido\n");
			fprintf(stdout,"cadena recibida: %s\n",buf);
			strcpy(buf,"200");			
		}else{
			fprintf(stdout, "Error de sintaxis\n");
			fprintf(stdout,"cadena recibida: %s\n",buf);
			strcpy(buf,"500");		
		}


		//strcpy(buf,"500");
		//strcpy(buf,"respuesta");
		fprintf(stdout, "enviando respuesta\n\n");
		if (send(s, buf, TAM_BUFFER, 0) != TAM_BUFFER) fprintf(stderr,"error");//errout(hostname);
	}


	//cuando acaba el loop significa que no hay mas requests
	//cerramos el socket
	close(s);

		/* Log a finishing message. */
	time (&timevar);
		/* The port number must be converted first to host byte
		 * order before printing.  On most hosts, this is not
		 * necessary, but the ntohs() call is included here so
		 * that this program could easily be ported to a host
		 * that does require it.
		 */
	printf("Completed %s port %u, %d requests, at %s\n",
		hostname, ntohs(clientaddr_in.sin_port), reqcnt, (char *) ctime(&timevar));
}