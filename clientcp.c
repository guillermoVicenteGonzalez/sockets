/*
**	Cliente tcp
**	Autores:
**	Guillermo Vicente Gonzalez DNI 70908566A
**	Jorge Prieto de la Cruz DNI
**
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <time.h>

#define PUERTO 17278
#define TAM_BUFFER 512

typedef struct respuesta{
	int codigo;
	char *mensaje;
}respuesta;

int main(int argc, char  *argv[])
{
	FILE *fp;
	char linea[100];
	char * ruta = argv[2]; //ruta del fichero a abrir
	int s; //descriptor del socket
	struct sockaddr_in myaddr_in; //local socket address
	struct sockaddr_in servaddr_in; //server socket addres
	struct addrinfo hints, *res;
	char buf[TAM_BUFFER];
	int addrlen, i, j, errcode;
	long timevar;

	if(argc != 3){
		fprintf(stderr, "Uso: ./%s <server> <ordenes.txt>\n",argv[0]);
		exit(1);
	}

	//abro el fichero
	if(NULL == (fp = fopen(ruta,"r"))){
		fprintf(stderr,"no se puede abrir el fichero");
		exit(1);
	}

	//creacion del socket
	s = socket(AF_INET, SOCK_STREAM, 0);
	if(s == -1){
		perror(argv[0]);
		fprintf(stderr, "%s: error creando socket", argv[0]);
		exit(1);
	}

	//limpio las estructuras de datos
	memset((char *) &myaddr_in, 0, sizeof(struct sockaddr_in));
	memset((char *) &servaddr_in, 0, sizeof(struct sockaddr_in));

	//indicamos que trabajamos con ipv4 -> AF_INET
	servaddr_in.sin_family = AF_INET;

	memset (&hints, 0, sizeof (hints)); //ponemos a 0 cada byte del puntero
    hints.ai_family = AF_INET; //

    errcode = getaddrinfo (argv[1], NULL, &hints, &res); 
    if(errcode !=0){
    	fprintf(stderr, "%s: No es posible resolver la ip");
    	exit(1);
    }else{
    	servaddr_in.sin_addr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
    }
    freeaddrinfo(res);

    //puerto del servidor en orden de red
    servaddr_in.sin_port = htons(PUERTO);

    if (connect(s, (const struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: incapaz de conectar\n", argv[0]);
		exit(1);
	}

	addrlen = sizeof(struct sockaddr_in);
	if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to read socket address\n", argv[0]);
		exit(1);
	}

	time(&timevar);
	printf("Connected to %s on port %u at %s\n",
	argv[1], ntohs(myaddr_in.sin_port), (char *) ctime(&timevar));


	//bucle de lectura de fichero
	do{
		fgets(linea,100,fp);
		//mando el mensaje
		if(strcmp(linea,"\n") == 0){
			printf("solo en blanco. No envio"); 
		}
		fprintf(stdout,"mandando: %s",linea);
		if(send(s,linea,TAM_BUFFER,0) != TAM_BUFFER){
			fprintf(stderr, "%s: Conexion abortada al producirse un error", argv[0]);
			exit(1);
		}

		//espero una respuesta
		//fprintf(stdout, "espero respuesta\n");
		recv(s, buf, TAM_BUFFER, 0);
		fprintf(stdout,"respuesta: %s\n\n",buf);
		if (i == -1) {
	        perror(argv[0]);
			fprintf(stderr, "%s: error reading result\n", argv[0]);
			exit(1);
		}

		//verifico que resultado me ha dado y si es de error salgo
		

	}while(!feof(fp));

	if (shutdown(s, 1) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to shutdown socket\n", argv[0]);
		exit(1);
	}


	/*
	//for(int i=0;i<2;i++){
		printf("mando hola\n");
		if(send(s,"HELO",TAM_BUFFER,0) != TAM_BUFFER){
			fprintf(stderr, "%s: Conexion abortada al producirse un error", argv[0]);
			exit(1);
		}

		while (i = recv(s, buf, TAM_BUFFER, 0)) {
			if (i == -1) {
	            perror(argv[0]);
				fprintf(stderr, "%s: error reading result\n", argv[0]);
				exit(1);
			}

			//para errores
			while (i < TAM_BUFFER) {
				j = recv(s, &buf[i], TAM_BUFFER-i, 0);
				if (j == -1) {
	                     perror(argv[0]);
				         fprintf(stderr, "%s: error reading result\n", argv[0]);
				         exit(1);
	               }
				i += j;
			}

			//printf("\n%d\n",strcmp(buf,"respuesta"));
			if(strcmp(buf,buf) == 0){
				printf("recibida respuesta: %s\n",buf);
				if(send(s,"HELO",TAM_BUFFER,0) != TAM_BUFFER){
					fprintf(stderr, "%s: Conexion abortada al producirse un error", argv[0]);
					exit(1);
				}
				printf("mando otro mensaje\n");
				//Print out message indicating the identity of this reply.
			}else{
				fprintf(stdout,"Sintaxis: %s",buf);
			}
		}

		if (shutdown(s, 1) == -1) {
			perror(argv[0]);
			fprintf(stderr, "%s: unable to shutdown socket\n", argv[0]);
			exit(1);
		}
	//}*/

	printf("\n");
	return 0;
}