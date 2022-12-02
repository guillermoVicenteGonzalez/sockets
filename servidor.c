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
#define TAM_BUFFER 512
#define MAXHOST 128

extern int errno;

int comprobarCorreo(char *sender);
int comprobarCorchetes(char *sender);
void serverTCP(int s, struct sockaddr_in clientaddr_in);
void errout(char *hostname)
{
	printf("Connection with %s aborted on error\n", hostname);
	perror("send: ");
	exit(1);     
}

typedef struct mailState{
	int helo;
	int mail;
	int data;
	int rcpt;
}mailState;

typedef struct mail{
	char emisor[TAM_BUFFER];
	char mensaje[TAM_BUFFER];
	char receptores[10][TAM_BUFFER];
	int nReceptores;
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
    int receptores = 0;
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
		//falta hacer que no se repitan pasos y que se hagan todos en orden con las flags

		//primero compruebo que puñetas he recibido
		fprintf(stdout,"cadena recibida: %s\n",buf);


		//compruebo la primera palabra del mensaje enviado.
		palabra = strtok(buf," ");
		if(strcmp(palabra,"HELO") == 0){
			fprintf(stdout,"HELO recibido\n");
			//fprintf(stdout,"cadena recibida: %s\n",buf);
			palabra = strtok(NULL, " ");
			if(palabra != NULL){
				//deberia comprobar si el sender es valido
				fprintf(stdout,"palabra 2: %s",palabra);
				//comprobamos si hay mas palabras, en cuyo caso mandamos error de sintaxis
				palabra = strtok(NULL, " ");
				if(palabra != NULL){
					//error. Solo se espera un campo.
					//limpio estructuras y devuelvo error.

					strcpy(buf,"500 Error de sintaxis\n");
					fprintf(stdout,buf);

				}else{
					strcpy(buf,"250 OK");
				}
			}
			//solo ha de tener 2 campos.

		//MAIL	
		}else if(strcmp(palabra,"MAIL") == 0){
			fprintf(stdout,"MAIL recibido\n");
			palabra = strtok(NULL, " ");
			if(strcmp(palabra,"FROM:") == 0){
				fprintf(stdout, "From recibido, bien\n");
				palabra = strtok(NULL, " ");
				if(palabra != NULL){
					if(comprobarCorchetes(palabra) == 1){
						strcpy(miMail.emisor,palabra);
						fprintf(stdout, "emisor: %s",miMail.emisor);
						//falta comprobar arrobas
						strcpy(buf,"250 OK\n");
						fprintf(stdout,buf);
					}else{
						strcpy(buf,"500 Error de sintaxis\r\n");
						fprintf(stdout,buf);
					}
				}
			}else{
				strcpy(buf,"500 Error de sintaxis\r\n");
				fprintf(stdout,buf);
			}

		//RCPT
		}else if(strcmp(palabra,"RCPT") == 0){
			fprintf(stdout,"RCPT recibido\n");
			palabra = strtok(NULL," ");
			if(strcmp(palabra,"TO:") == 0){
				palabra = strtok(NULL, " ");
				if(comprobarCorchetes(palabra)){
					//no haria falta guardarlos pero..
					//strcpy(miMail.receptores[miMail.nReceptores],palabra);
					strcpy(buf,"250 OK\n");
					fprintf(stdout,buf);
				}else{
					strcpy(buf,"500 Error de sintaxis\n");
					fprintf(stdout,buf);
				}

			}else{
				strcpy(buf,"500 Error de sintaxis\n");
				fprintf(stdout,buf);
			}
		///DATA
		}else if(strcmp(palabra,"DATA\n") == 0){
			fprintf(stdout,"DATA recibido\n");

			//envio respuesta
			strcpy(buf,"354 Comenzando con el texto del correo, finalice con .");
			fprintf(stdout, "enviando respuesta: %s\n\n",buf);
			
			if (send(s, buf, TAM_BUFFER, 0) != TAM_BUFFER){
				fprintf(stderr,"error");
				errout(hostname);
			}

			while(len = recv(s, buf, TAM_BUFFER, 0)){
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
				fprintf(stdout, "data:%s",buf);

				if(strcmp(buf, ".\n") == 0){
					strcpy(buf,"250 Data finalizado");
					break;
				}else{
					if (send(s, "250 Data OK", TAM_BUFFER, 0) != TAM_BUFFER){
						fprintf(stderr,"error");
						errout(hostname);
					}
				}
			}


		//QUIT
		}else if(strcmp(palabra, "QUIT") == 0){
			fprintf(stdout,"QUIT recibido\n");
			strcpy(buf,"221 Cerrando servicio");
			//break;

		//ERROR	
		}else if(strcmp(palabra,"\n") == 0){

		}else{
			fprintf(stdout, "Error de sintaxis\n");
			strcpy(buf,"500 Error de sintaxis");		
		}


		//strcpy(buf,"500");
		//strcpy(buf,"respuesta");
		fprintf(stdout, "enviando respuesta: %s\n\n",buf);
		if (send(s, buf, TAM_BUFFER, 0) != TAM_BUFFER) fprintf(stderr,"error");//errout(hostname);
	}


	//cuando acaba el loop significa que no hay mas requests
	//cerramos el socket
	close(s);

		/* Log a finishing message. */
	time (&timevar);
	printf("Completed %s port %u, %d requests, at %s\n",
		hostname, ntohs(clientaddr_in.sin_port), reqcnt, (char *) ctime(&timevar));
}

//comprueba que los correos lleven corchetes
int comprobarCorchetes(char *sender){
	int i=0;
	for(i=0;sender[i]!='\0';++i);
	//printf("\n[0]:%c [%d]:%c\n",sender[0],i-2,sender[i-2]);
	//-2 porque los ultimos caracteres son \0 y \n
	if(sender[0] == '<' && sender[i-2] == '>')
		return 1;
	else
		return 0;
}

int comprobarCorreo(char *sender){
	int flagArroba = 0;
	int flagPunto = 0;
	int resultado = 0;

	for(int i=0;sender[i]!='\0';i++){
		if(sender[i] == '@' && flagPunto == 0 && flagArroba == 0){
			flagArroba =1;
		}
		if(sender == '.' ){
			if(flagArroba == true && flagPunto == false && sender[i+1] != '\0')
				resultado=1;
			else if(flagPunto == true)
				resultado=0;
		}
	}
}