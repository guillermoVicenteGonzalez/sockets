/**
 * servidor.c
 * Autores:
 * Guillermo Vicente Gonzalez DNI 70908566A
 * Jorge Prieto de la Cruz DNI
 **/ 

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

#define TIMEOUT 5
#define RETRIES 5       /* number of times to retry before givin up */
#define PUERTO 17278
#define ADDRNOTFOUND	0xffffffff	/* return address for unfound host */
#define BUFFERSIZE	1024	/* maximum size of packets to be received */
#define TAM_BUFFER 512
#define MAXHOST 128

extern int errno;

int FIN = 0;         
void finalizar(){ FIN = 1; }
int comprobarCorreo(char *sender);
int comprobarCorchetes(char *sender);
void serverTCP(int s, struct sockaddr_in clientaddr_in);
void serverUDP(int s, char * buffer, struct sockaddr_in clientaddr_in);

//alarma
void handler()
{
 printf("Alarma recibida \n");
}

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
	int quit;
}mailState;

typedef struct mail{
	char emisor[TAM_BUFFER];
	char mensaje[TAM_BUFFER];
	char receptores[10][TAM_BUFFER];
	int nReceptores;
}mail;

int main(int argc, char  *argv[])
{
	FILE *log;
	//sListen es el socket TCP
	int sListen, s, s_UDP, l_UDP; //listenUdp como socket udp adicional 
	int port = PUERTO;

	//clientData i serverData sin myAddr_in y clientaddr_in
	struct sockaddr_in serverData, clientData;
	char buffer[TAM_BUFFER];

	//para ignorar SIGCHLD
	struct sigaction sa = {.sa_handler = SIG_IGN}; /* used to ignore SIGCHLD */
	struct sigaction sAlarm; //para la alarma?
	int cc;				    /* contains the number of bytes read */

	int addrlen;	

    fd_set readmask;
    int numfds,s_mayor;

    struct sigaction vec;
	//pongo los flags a 0


	fprintf(stderr, "empieza\n");
	//limpieza de estructuras
	printf("limpio estructuras\n");
	memset ((char *)&clientData, 0, sizeof(struct sockaddr_in));
	memset ((char *)&serverData, 0, sizeof(struct sockaddr_in));

	//creacion socket de escucha
	printf("creo el socket\n");
	if(-1 == (sListen= socket (AF_INET, SOCK_STREAM, 0))){
		perror("socket");
		exit(EXIT_FAILURE);
	}

	//rellenar estructuras para trabajar con ipv4 y el puerto definido
	serverData.sin_family = AF_INET;
	serverData.sin_addr.s_addr = INADDR_ANY;
	serverData.sin_port = htons(PUERTO);
	addrlen = sizeof(struct sockaddr_in);

	//bindeamos el socket de escucha tcp a nuestra dirección
	printf("bindeo el socket\n");
	if (-1 == bind(sListen, (const struct sockaddr *) &serverData, sizeof(serverData))) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	//inicio el socket . 5 es el numero maximo de usuarios simultaneos
	printf("clientes en espera\n");
	if (-1 == listen(sListen, 5)) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	//Creo el socket UDP
	s_UDP = socket (AF_INET, SOCK_DGRAM, 0);
	if (s_UDP == -1) {
		perror(argv[0]);
		printf("%s: unable to create socket UDP\n", argv[0]);
		exit(1);
	}

	//bindeamos nuestra direccion al socket udp
	if (bind(s_UDP, (struct sockaddr *) &serverData, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
		printf("%s: unable to bind address UDP\n", argv[0]);
		exit(1);
	}

	//para disociar del terminal el proceso.
	setpgrp();


	//creo un proceso nuevo y mato al antiguo para conseguir un demonio
	switch(fork()){
		case -1:
			perror(argv[0]);
			fprintf(stderr, "%s: unable to fork daemon\n", argv[0]);
			exit(1);

		case 0:
			//esto se cierra porque se supone que todo va a un log.
			//fclose(stdin);
			//fclose(stderr);

			//para prevenir la acumulacion de zombies
			if ( sigaction(SIGCHLD, &sa, NULL) == -1) {
	            perror(" sigaction(SIGCHLD)");
	            fprintf(stderr,"%s: unable to register the SIGCHLD signal\n", argv[0]);
	            exit(1);
            }


            
		    /* Registrar SIGTERM para la finalizacion ordenada del programa servidor */
        	vec.sa_handler = (void *) finalizar;
        	vec.sa_flags = 0;
        	if ( sigaction(SIGTERM, &vec, (struct sigaction *) 0) == -1) {
            	perror(" sigaction(SIGTERM)");
            	fprintf(stderr,"%s: unable to register the SIGTERM signal\n", argv[0]);
            	exit(1);
            }

            //registro la alarma
            sAlarm.sa_handler = (void *) handler;
            sAlarm.sa_flags = 0;
            if(sigaction(SIGALRM, &sAlarm, (struct sigaction * ) 0) == -1){
            	perror("sigaction(SIGALRM");
            	fprintf(stderr, "%s: unable to register the SIGALRM\n",argv[0]);
            	exit(1);
            }


            while(!FIN){
            	//printf("\nvuelvo al bucle\n------------------------------\n");
            	/* Meter en el conjunto de sockets los sockets UDP y TCP */
            	FD_ZERO(&readmask);
            	FD_SET(sListen, &readmask);
            	FD_SET(s_UDP, &readmask);

	           	/* 
	            Seleccionar el descriptor del socket que ha cambiado. Deja una marca en 
	            el conjunto de sockets (readmask)
	            */
	            if (sListen > s_UDP) s_mayor=sListen;
    			else s_mayor=s_UDP;

	            if ( (numfds = select(s_mayor+1, &readmask, (fd_set *)0, (fd_set *)0, NULL)) < 0) {
	                if (errno == EINTR) {
	                    FIN=1;
			            close (sListen);
			            close (s_UDP);
	                    perror("\nFinalizando el servidor. SeÃal recibida en elect\n "); 
	                }
	            }else{
	            	//se comprueba si se ha seleccionado TCP o UDP
	            	//TCP
                	if (FD_ISSET(sListen, &readmask)) {
						if(-1 == (s= accept(sListen, (struct sockaddr *) &clientData, &addrlen))){
							perror("accept");
							exit(1);
						}

						switch(fork()){
							case -1:
								perror("fork: ");
								exit(1);

							case 0:
								close(sListen);
								serverTCP(s,clientData);
								exit(0);
								//break; //?

							default:
								//cuidado con los nombres de los sockets
								close(s);
						}
                	}

                	//si es udp
                	if(FD_ISSET(s_UDP, &readmask)){
		                cc = recvfrom(s_UDP, buffer, BUFFERSIZE - 1, 0,
		                   (struct sockaddr *)&clientData, &addrlen);
		                if ( cc == -1) {
		                    perror(argv[0]);
		                    printf("%s: recvfrom error\n", argv[0]);
		                    exit (1);
		                }
		                /* Make sure the message received is
		                * null terminated.
		                */
		                buffer[cc]='\0';
		                switch(fork()){
		                	case -1:
		                		perror("fork: ");
		                		exit(1);

		                	case 0:
		                		//proceso hijo
		                		//creo el socket adicional para concurrencia
		                		if(-1 == (l_UDP = socket(AF_INET, SOCK_DGRAM,0))){
		                			perror("socket: ");
		                			exit(1);
		                		}
		                		close(sListen);
		                		close(s_UDP);
		                		//relleno las estructuras
		                		serverData.sin_family = AF_INET;
		                		serverData.sin_addr.s_addr = INADDR_ANY;
		                		serverData.sin_port = 0; //puerto efimero

		                		//bindeo el socket
		                		if(-1 == bind(l_UDP, (const struct sockaddr *) &serverData, sizeof(serverData))){
		                			perror("bind: ");
		                			exit(1);
		                		}

				                serverUDP (l_UDP, buffer, clientData);
				                exit(0);

				            default:
				            	                		
		                }               		
                	}	            	
	            } 
            } //fin del bucle infinito
            close(sListen);
        	close(s_UDP);
    
        	printf("\nFin de programa servidor!\n");

        default:
        	exit(0);
	}

	return 0;
}


/**************************************************************************************
 * FUNCIONES
 * ************************************************************************************/




void serverTCP(int s, struct sockaddr_in clientaddr_in){
/**********************************************************
 * VARIABLES
 * ********************************************************/

	FILE *log;
	int reqcnt = 0;		//numero de requests
	char buf[TAM_BUFFER];		/* This example uses TAM_BUFFER byte messages. */
	char hostname[MAXHOST];		//nombre del remote host
	char serverHostname[MAXHOST]; //nombre de nuestra maquina
	char stream[TAM_BUFFER];	//para guardar cadenas
	char *temp; //para quitarle el "\n" a las cadenas
	char address[INET_ADDRSTRLEN];
	char datosCliente[1000];

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
    //primero abro el fichero de log
	if(NULL == (log = fopen("peticiones.log","a"))){
		fprintf(stderr,"no se puede abrir el fichero");
		exit(1);
	}

    state.helo=0; state.mail=0; state.data=0; state.rcpt=0; state.quit = 0;

	status = getnameinfo((struct sockaddr *)&clientaddr_in,sizeof(clientaddr_in),
                           hostname,MAXHOST,NULL,0,0);

	if(status){
		//lainformacion no esta disponible. 
		if (inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostname, MAXHOST) == NULL)
            	perror(" inet_ntop \n");
	}

	time (&timevar);
	sprintf(stream,"\nStartup from %s port %u at %s-------------------------------------------------------------------\n\n",
		hostname, ntohs(clientaddr_in.sin_port), (char *) ctime(&timevar));
	//printf("-----------------------------------------------------------\n\n");
	fprintf(stdout,stream);
	fputs(stream,log);

	//formateo la ip del cliente en una "string"
	inet_ntop(AF_INET, &clientaddr_in.sin_addr,address,sizeof(address));


	//creo una cadena con los datos de los procesos para imprimirla recurrentemente
	sprintf(datosCliente,"hostname: %s address: %s Protocol: TCP port: %u",hostname, address, ntohs(clientaddr_in.sin_port));

	linger.l_onoff  =1;
	linger.l_linger =1;

	if (setsockopt(s, SOL_SOCKET, SO_LINGER, &linger,sizeof(linger)) == -1){
		errout(hostname);
	}

	strcpy(buf,"220 Servicio de transferencia simple de correo preparado.");
	fprintf(stdout, "enviando: %s\n\n",buf);
	if (send(s, buf, TAM_BUFFER, 0) != TAM_BUFFER){
		fprintf(stderr,"error");
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
		fprintf(stdout,"cadena recibida: %s",buf);
		time(&timevar);
		sprintf(stream,"MESSAGE RECEIVE: %s FROM: %s time: %s\n\n",buf, datosCliente, (char *) ctime(&timevar));
		//temp=strtok(stream,"\n");
		fputs(stream,log);

		//compruebo la primera palabra del mensaje enviado.
		palabra = strtok(buf," ");
		if(strcmp(palabra,"HELO") == 0 && state.helo == 0){
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
					state.helo = 1;
					fprintf(stdout,"\nHelo state: %d\n",state.helo);
					strcpy(buf,"250 OK");
				}
			}
			//solo ha de tener 2 campos.

		//MAIL	
		//comprobamos que se realiza todo en orden
		}else if(strcmp(palabra,"MAIL") == 0 && state.helo && !state.rcpt && !state.data && !state.mail){
			fprintf(stdout,"MAIL recibido\n");
			palabra = strtok(NULL, " ");
			if(strcmp(palabra,"FROM:") == 0){
				fprintf(stdout, "From recibido, bien\n");
				palabra = strtok(NULL, " ");
				if(palabra != NULL){
					if(comprobarCorchetes(palabra) == 1 && comprobarCorreo(palabra)){
						strcpy(miMail.emisor,palabra);
						fprintf(stdout, "emisor: %s",miMail.emisor);
						strcpy(buf,"250 OK\n");
						fprintf(stdout,buf);
						state.mail = 1;
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
		}else if(strcmp(palabra,"RCPT") == 0 && state.helo && state.mail && !state.data){
			fprintf(stdout,"RCPT recibido\n");
			palabra = strtok(NULL," ");
			if(strcmp(palabra,"TO:") == 0){
				palabra = strtok(NULL, " ");

				if(comprobarCorchetes(palabra)  == 1 && comprobarCorreo(palabra) == 1){
					//no haria falta guardarlos pero..
					//strcpy(miMail.receptores[miMail.nReceptores],palabra);
					strcpy(buf,"250 OK\n");
					fprintf(stdout,buf);
					state.rcpt = 1;
				}else{
					strcpy(buf,"500 Error de sintaxis\n");
					fprintf(stdout,buf);
				}

			}else{
				strcpy(buf,"500 Error de sintaxis\n");
				fprintf(stdout,buf);
			}
		///DATA
		}else if(strcmp(palabra,"DATA\n") == 0 && state.helo && state.mail && state.rcpt && !state.quit){
			fprintf(stdout,"DATA recibido\n");

			//envio respuesta
			strcpy(buf,"354 Comenzando con el texto del correo, finalice con .");
			fprintf(stdout, "enviando respuesta: %s\n\n",buf);
			state.data = 1;
			
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

				//registro el mensaje en el log
				time(&timevar);
				sprintf(stream,"MESSAGE RECEIVE: %s FROM: %s time: %s\n\n",buf, datosCliente, (char *) ctime(&timevar));


				//incrementamos el nuero de requests y espero 
				reqcnt++;
				sleep(1);
				fprintf(stdout, "data:%s",buf);

				if(strcmp(buf, ".\n") == 0){
					strcpy(buf,"250 Data finalizado");
					state.quit = 1;
					break;
				}else{
					if (send(s, "250 Data OK", TAM_BUFFER, 0) != TAM_BUFFER){
						fprintf(stderr,"error");
						errout(hostname);
					}

					time(&timevar);
					sprintf(stream,"MESSAGE SEND: %s TO: %s time: %s\n\n",buf, datosCliente, (char *) ctime(&timevar));

				}
			}


		//QUIT
		//cuando llega . en data ponemos state.quit = 1 simbolizando que ya se puede hacer quit. no antes
		}else if(strcmp(palabra, "QUIT") == 0 && state.helo && state.mail && state.rcpt && state.data && state.quit){
			fprintf(stdout,"QUIT recibido\n");
			state.helo = 0;
			state.mail = state.rcpt = state.data = state.quit = 0;
			strcpy(buf,"221 Cerrando servicio");
			//break;

		//ERROR	
		}else if(strcmp(palabra,"\n") == 0){

		}else{
			fprintf(stdout,"error generico\n");
			fprintf(stdout, "Error de sintaxis\n");
			strcpy(buf,"500 Error de sintaxis");		
		}


		//strcpy(buf,"500");
		//strcpy(buf,"respuesta");

		fprintf(stdout, "enviando respuesta: %s\n",buf);
		time (&timevar);
		sprintf(stream,"\nMESSAGE SEND: %s TO: %s time: %s\n\n",buf, datosCliente, (char *) ctime(&timevar));
		//temp=strtok(stream,"\n");
		fputs(stream,log);
		if (send(s, buf, TAM_BUFFER, 0) != TAM_BUFFER) fprintf(stderr,"error");//errout(hostname);
	}


	//cuando acaba el loop significa que no hay mas requests
	//cerramos el socket
	close(s);

		/* Log a finishing message. */
	time (&timevar);
	sprintf(stream,"Completed %s port %u, %d requests, at %s------------------------------------------------------------------------------\n",
		hostname, ntohs(clientaddr_in.sin_port), reqcnt, (char *) ctime(&timevar));
	fprintf(stdout,stream);
	fputs(stream,log);
	fclose(log);
}



//comprueba que los correos lleven corchetes
int comprobarCorchetes(char *sender){

	char aux[TAM_BUFFER];
	int i=0;
	for(i=0;sender[i]!='\0';++i){
		//fprintf(stdout,"%d: %c",i,sender[i]);
	}

	strcpy(aux,sender);
	strtok(aux,"\n");
	//fprintf(stdout,"\n[0]:%c [%d]:%c\n",sender[0],i-2,sender[i-2]);
	//-2 porque los ultimos caracteres son \0 y \n
	if(sender[0] == '<' && sender[i-2] == '>')
		return 1;
	else
		return 0;
}


//comprueba que los correos tengan @ y . en el orden adecuado
int comprobarCorreo(char *sender){
   int flagArroba = 0;
   int flagPunto = 0;
   int resultado = 0;

   for(int i=0;sender[i]!='\0';++i){
      //printf("\n%c",sender[i]);
      if(sender[i] == '@' && flagArroba == 0 && flagPunto == 0){
         flagArroba = 1;
         //printf("encontrada arroba\n");
      }else if(sender[i] == '@' && flagArroba == 1){
         //printf("ya habia una arroba antes\n");
         return 0;
      }

      if(sender[i] == '.' && flagArroba == 1 && flagPunto == 0){
         flagPunto = 1;
         //printf("encontrado punto\n");
         resultado = 1;
      }else if((sender[i] == '.' && flagArroba == 0) || (sender[i] == '.' && flagPunto == 1)){
         flagPunto = 1;
         return 0;
         //printf("ya habia punto o no habia arroba antes");
      }
   }
   //fprintf(stdout,"\nresultado correo: %d\n",resultado);
   return resultado;
   //printf("\n[0]:%c [%d]:%c\n",sender[0],i-2,sender[i-2]);
   //-2 porque los ultimos caracteres son \0 y \n
}

void serverUDP(int s, char * buffer, struct sockaddr_in clientaddr_in){
	int n_retry = RETRIES;        /* holds the retry count */
	FILE *log;
	struct in_addr reqaddr;	/* for requested host's address */
    struct hostent *hp;		/* pointer to host info for requested host */
    int nc, errcode;
    char string[TAM_BUFFER];
    char logString[TAM_BUFFER];
    char *palabra;
    int final = 0;
    long timevar;
    char hostname[MAXHOST];
    int status;
	int reqcnt = 0;		//numero de requests
	char datosCliente[1000];
	char address[INET_ADDRSTRLEN];

	mailState state;

    struct addrinfo hints, *res;

	int addrlen;
    
	//pongo la estructura de control del mail a 0
	state.helo=0; state.mail=0; state.data=0; state.rcpt=0;

    //abro el fichero de log
    log = fopen("peticiones.log","a");
   	addrlen = sizeof(struct sockaddr_in);

   	status = getnameinfo((struct sockaddr *)&clientaddr_in,sizeof(clientaddr_in),
                           hostname,MAXHOST,NULL,0,0);

	if(status){
		//lainformacion no esta disponible. 
		if (inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostname, MAXHOST) == NULL)
            	perror(" inet_ntop \n");
	}

    memset (&hints, 0, sizeof (hints));
    hints.ai_family = AF_INET;
    //obtengo la ip a partir del hostname
    errcode = getaddrinfo (buffer, NULL, &hints, &res); 
    if (errcode != 0){
		/* Name was not found.  Return a
		 * special value signifying the error. */
		reqaddr.s_addr = ADDRNOTFOUND;
      }
    else {
		/* Copy address of host into the return buffer. */
		reqaddr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
	}
    
    freeaddrinfo(res);

    //preparo una cadena con los datos de los procesos para uso recurrente
 	sprintf(datosCliente,"hostname: %s address: %s Protocol: UDP port: %u",hostname, address, ntohs(clientaddr_in.sin_port));
	
	//imprimo cadena indicando que se ha iniciado el proceso
	sprintf(logString,"\nStartup from %s port %u at %s-------------------------------------------------------------------\n\n",
		hostname, ntohs(clientaddr_in.sin_port), (char *) ctime(&timevar));
	fprintf(stdout,logString);
	fputs(logString,log);

	nc = sendto (s, &reqaddr, sizeof(struct in_addr),
			0, (struct sockaddr *)&clientaddr_in, addrlen);

	if ( nc == -1) {
         perror("serverUDP");
         printf("%s: sendto error\n", "serverUDP");
         return;
    }

/***************************************************************
 * BUCLE DE RECEPCIONES Y RESPUESTAS
 * *************************************************************/

    //aqui tiene que haber una condicion con alarmas igual que en el cliente
    //ademas de una condicion con el quit. Cuando llegue quit sal.
    while(final != 1 && n_retry > 0){

    	//timeout para que no quede bloqueado
    	alarm(TIMEOUT);

    	//comienza la recepcion
    	if(-1 == recvfrom(s, string, TAM_BUFFER, 0, (struct sockaddr*)&clientaddr_in, &addrlen)){
    		if(errno == EINTR){
    			//ha saltado la alarma?
                printf("attempt %d (retries %d).\n", n_retry, RETRIES);
                n_retry--;     			
    		}else{
	    		perror("recvfrom");
				fprintf(stderr,"unable to receive");
				exit(1);
    		}
		}else{
			alarm(0);
			fprintf(stdout,"cadena recibida: %s",string);
			time (&timevar);
			sprintf(logString,"MESSAGE RECEIVE: %s FROM: %s time: %s\n",string,datosCliente,(char *) ctime(&timevar));
    		//strtok(logString,"\n");
    		fputs(logString,log);
			reqcnt++;
		}
		/***************************************************************
		 * TRATAMIENTO DE CADENAS
		 * *************************************************************/

		sleep(1);

		//divido la cadena en palabras separadas por espacios
		palabra = strtok(string," ");
		//HELO
		if(strcmp(palabra,"HELO") == 0 && state.helo == 0 &&state.mail == 0 ){
			fprintf(stdout,"HELO recibido\n");
			//descomponemos el HELO para verificar su formato

			//verifico que despues de HELO va el sender
			palabra = strtok(NULL," ");
			if(palabra != NULL){
				//aqui se deberia comprobar el sender de algun modo
				palabra = strtok(NULL, " ");
				if(palabra != NULL){
					//error, solo se espera un campo. Devuelvo error
					strcpy(string,"500 Error de sintaxis");
					fprintf(stdout,string);
				}else{
					state.helo = 1;
					strcpy(string,"250 OK");
				}
			}

		}

		//MAIL
		else if(strcmp(palabra,"MAIL") == 0 && state.helo == 1 && !state.rcpt && !state.data && !state.mail){
			fprintf(stdout,"MAIL recibido\n");
			palabra = strtok(NULL," ");
			if(strcmp(palabra,"FROM:") == 0){
				//se ha recibido from. La sintaxis es correcta (de momento)
				//ahora verificamos el sender
				palabra = strtok(NULL," ");
				if(palabra != NULL){
					//comprobamos el formato del sender
					if(comprobarCorchetes(palabra) == 1 && comprobarCorreo(palabra) == 1){
						fprintf(stdout,"emisor: %s",palabra);
						strcpy(string,"250 OK");
						state.mail = 1;
					}else{
						strcpy(string,"500 Error de sintaxis");
						fprintf(stdout,string);
					}
				}else{
					strcpy(string,"500 Error de sintaxis");
					fprintf(stdout,string);
				}
			}

		//RCPT
		//no se comprueba state.rcpt porque puede indicarse mas de un receptor
		}else if(strcmp(palabra,"RCPT") == 0 && state.helo == 1 && state.mail && !state.data){
			fprintf(stdout,"RCPT recibido\n");
			palabra = strtok(NULL," ");
			if(strcmp(palabra,"TO:") == 0){
				fprintf(stdout,"TO: recibido\n");
				palabra = strtok(NULL," ");
				fprintf(stdout,"correo: %s",palabra);
				if(comprobarCorchetes(palabra) == 1 && comprobarCorreo(palabra) == 1){
					//compruebo si hay mas cosas en cuyo caso la orden es incorrecta
					if(NULL != (palabra = strtok(NULL," "))){
						strcpy(string,"500 Error de sintaxis");
						fprintf(stdout,string);
					}else{
						strcpy(string,"250 OK");
						fprintf(stdout,string);
						state.rcpt = 1;
					}
				}else{
					strcpy(string,"500 Error de sintaxis");
					fprintf(stdout,string);
				}
			}else{
				strcpy(string,"500 Error de sintaxis");
				fprintf(stdout,string);
			}

		//DATA
		}else if(strcmp(palabra,"DATA\n") == 0 && state.helo && state.mail && state.rcpt && !state.quit){
			fprintf(stdout,"DATA recibido\n");
			strcpy(string,"354 Comenzando con el texto del correo, finalice con .");
			fprintf(stdout,string);
			state.data = 1;

			//envio la respuesta
	    	nc = sendto (s, string, TAM_BUFFER,0, (struct sockaddr *)&clientaddr_in, addrlen);
	    	if(nc == -1){
	    		perror("server udp");
	    		fprintf(stderr,"error enviando");
	    		exit(1);
	    	}

	    	//guardo la info en el log
	    	sprintf(logString,"MESSAGE SEND: %s TO: %s time: %s\n",string,datosCliente,(char *) ctime(&timevar));						
			fputs(logString,log);

	    	//comienza el bucle de procesamiento de data
	    	//este bucle esta mal. Se sale con el break como parche.
	    	do{
		    	if(-1 == recvfrom(s, string, TAM_BUFFER, 0, (struct sockaddr*)&clientaddr_in, &addrlen)){
		    		perror("recvfrom");
					fprintf(stderr,"unable to receive");
					exit(1);
				}else{
					fprintf(stdout,"cadena recibida: %s",string);
					reqcnt++;
				}

		    	//guardo la info en el log
		    	sprintf(logString,"MESSAGE RECEIVE: %s FROM: %s time: %s\n",string,datosCliente,(char *) ctime(&timevar));						
				fputs(logString,log);				

				if(strcmp(string,".\n") == 0){
					strcpy(string,"DATA finalizado");
					state.quit = 1;
			    	break;	
				}else{
					strcpy(string,"250 DATA OK");
					fprintf(stdout,"enviando: %s\n",string);
					nc = sendto (s, string, TAM_BUFFER,0, (struct sockaddr *)&clientaddr_in, addrlen);
			    	if(nc == -1){
			    		perror("server udp");
			    		fprintf(stderr,"error enviando");
			    		exit(1);
			    	}

					sprintf(logString,"MESSAGE SEND: %s TO: %s time: %s\n",string,datosCliente,(char *) ctime(&timevar));						
					fputs(logString,log);
				}

	    	}while(!(strcmp(string, ".\n") == 0));
	    	fprintf(stdout,"salgo del bucle");


		//QUIT
		}else if(strcmp(palabra,"QUIT") == 0 && state.helo && state.mail && state.rcpt && state.data && state.quit){
			fprintf(stdout, "QUIT recibido\n");
			state.helo = 0;
			strcpy(string, "221 Cerrando servicio");
			final = 1;
			//<break;

		}else{
			fprintf(stdout,"OTRO\n");
			strcpy(string,"500 Error de sintaxis");
		}
    	
    	/********************************************************
    	* RESPUESTA
    	************************************************************/
    	time (&timevar);
    	sprintf(logString,"MESSAGE SEND: %s TO: %s time: %s\n",string,datosCliente,(char *) ctime(&timevar));
    	//strtok(logString,"\n");
    	fputs(logString,log);
    	fprintf(stdout,"respondiendo %s\n\n",string);
    	nc = sendto (s, string, TAM_BUFFER,0, (struct sockaddr *)&clientaddr_in, addrlen);
    	if(nc == -1){
    		perror("server udp");
    		fprintf(stderr,"error enviando");
    		exit(1);
    	}
    }//acaba el bucle de recepcion

    close(s);
    time (&timevar);
	sprintf(string,"Completed %s port %u, %d requests, at %s------------------------------------------------------------------------------\n",
		hostname, ntohs(clientaddr_in.sin_port), reqcnt, (char *) ctime(&timevar));

	fprintf(stdout,string);
	fputs(string,log);
	fclose(log);

    exit(0);   
}

