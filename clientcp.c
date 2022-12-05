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
	char linea[TAM_BUFFER];
	char * ruta = argv[3]; //ruta del fichero a abrir
	int protocolo; //0 = tcp 1 = udp
	int s; //descriptor del socket
	struct sockaddr_in myaddr_in; //local socket address
	struct sockaddr_in servaddr_in; //server socket addres
	struct addrinfo hints, *res;
	char buf[TAM_BUFFER];
	int addrlen, i, j, errcode;
	long timevar;

	if(argc != 4){
		fprintf(stderr, "Uso: ./%s <server> <Protocolo> <ordenes.txt>\n",argv[0]);
		exit(1);
	}

	//abro el fichero
	if(NULL == (fp = fopen(ruta,"r"))){
		fprintf(stderr,"no se puede abrir el fichero");
		exit(1);
	}

	//verifico el protocolo
	if(strcmp(argv[2], "TCP") == 0){
		protocolo = 0;
	}else if(strcmp(argv[2],"UDP") == 0){
		protocolo = 1;
	}else{
		fprintf(stderr,"se ha de especificar el protocolo\n Uso: ./%s <server> <Protocolo> <ordenes.txt>\n",argv[0]);
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

    //obtengo la ip
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

	//antes de empezar tengo que recibir el ready del servidor
	recv(s, buf, TAM_BUFFER, 0);
	fprintf(stdout,"respuesta: %s\n\n",buf);

	//bucle de lectura de fichero
	do{
		fgets(linea,TAM_BUFFER,fp);
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
	
	printf("\n");
	return 0;
}