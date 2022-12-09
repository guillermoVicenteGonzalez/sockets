#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

extern int errno;

#define ADDRNOTFOUND    0xffffffff  /* value returned for unknown host */
#define RETRIES 5       /* number of times to retry before givin up */
#define BUFFERSIZE  1024    /* maximum size of packets to be received */
#define PUERTO 17278
#define TIMEOUT 5
#define MAXHOST 512
#define TAM_BUFFER 512
/*
 *          H A N D L E R
 *
 *  This routine is the signal handler for the alarm signal.
 */
void handler()
{
 printf("Alarma recibida \n");
}

void clienTCP(char *ruta, char *servidor);
void clientUDP(char *ruta, char *servidor);

int main(int argc, char  *argv[])
{
	char *servidor = argv[1]; //servidor al que me conecto
	char *ruta = argv[3]; //ruta del fichero
	if(argc != 4){
		fprintf(stderr, "Uso: ./%s <direccion> <Protocolo> <ordenes.txt>\n",argv[0]);
		exit(1);
	}

	if(strcmp(argv[2],"TCP") == 0){
		clienTCP(ruta,servidor);
	}else{	
		clientUDP(ruta, servidor);
	}
	return 0;
}

void clienTCP(char *ruta, char *servidor){
	FILE *log;
	FILE *fp;
	char linea[TAM_BUFFER];
	//char * ruta = argv[3]; //ruta del fichero a abrir
	int protocolo; //0 = tcp 1 = udp
	int s; //descriptor del socket
	struct sockaddr_in myaddr_in; //local socket address
	struct sockaddr_in servaddr_in; //server socket addres
	struct addrinfo hints, *res;
	char buf[TAM_BUFFER];
	int addrlen, i, j, errcode;
	long timevar;
	char puerto[TAM_BUFFER];
	char etiqueta[TAM_BUFFER];


	//abro el fichero
	if(NULL == (fp = fopen(ruta,"r"))){
		fprintf(stderr,"no se puede abrir el fichero");
		exit(1);
	}

	//creacion del socket
	s = socket(AF_INET, SOCK_STREAM, 0);
	if(s == -1){
		fprintf(stderr,"error creando socket");
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
    errcode = getaddrinfo (servidor, NULL, &hints, &res); 
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
		//perror(argv[0]);
		fprintf(stderr, "incapaz de conectar\n");
		exit(1);
	}

	addrlen = sizeof(struct sockaddr_in);
	if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1) {
		//perror(argv[0]);
		fprintf(stderr, "unable to read socket address\n");
		exit(1);
	}

	time(&timevar);
	printf("Connected to %s on port %u at %s\n",
	servidor, ntohs(myaddr_in.sin_port), (char *) ctime(&timevar));

	//abro el fichero de log
	sprintf(puerto,"%u.txt",ntohs(myaddr_in.sin_port));
	log = fopen(puerto,"a");


	//antes de empezar tengo que recibir el ready del servidor
	recv(s, buf, TAM_BUFFER, 0);
	sprintf(etiqueta,"respuesta: %s\n\n",buf);
	fprintf(stdout,etiqueta);
	fputs(etiqueta,log);

	//bucle de lectura de fichero
	do{
		fgets(linea,TAM_BUFFER,fp);
		//mando el mensaje
		if(strcmp(linea,"\n") == 0){
			printf("solo en blanco. No envio"); 
		}
		fprintf(stdout,"mandando: %s",linea);
		if(send(s,linea,TAM_BUFFER,0) != TAM_BUFFER){
			sprintf(etiqueta, "Conexion abortada al producirse un error");
			fprintf(stderr,etiqueta);
			fputs(etiqueta,log);
			fclose(log);
			fclose(fp);
			exit(1);
		}

		//espero una respuesta
		//fprintf(stdout, "espero respuesta\n");
		recv(s, buf, TAM_BUFFER, 0);
		sprintf(etiqueta,"respuesta: %s\n\n",buf);
		fprintf(stdout,etiqueta);
		fputs(etiqueta,log);
		if (i == -1) {
	        //perror(argv[0]);
			sprintf(etiqueta, "error reading result\n");
			fprintf(stderr,etiqueta);
			fputs(etiqueta,log);
			fclose(log);
			fclose(fp);
			exit(1);
		}

		//verifico que resultado me ha dado y si es de error salgo?
		

	}while(!feof(fp));

	if (shutdown(s, 1) == -1) {
		//perror(argv[0]);
		sprintf(etiqueta, "unable to shutdown socket\n");
		fprintf(stderr,etiqueta);
		fputs(etiqueta,log);
		fclose(log);
		fclose(fp);
		exit(1);
	}
	
	printf("\n");
	exit(0);
}

void clientUDP(char *ruta, char *servidor){
	//sustituyo servidor por argv2 y nameserver por argv1

	FILE *log;
    FILE *fp;
    char string[TAM_BUFFER];
    int i, errcode;
    int retry = RETRIES;        /* holds the retry count */
    int s;              /* socket descriptor */
    long timevar;                       /* contains time returned by time() */
    struct sockaddr_in myaddr_in;   /* for local socket address */
    struct sockaddr_in servaddr_in; /* for server socket address */
    struct in_addr reqaddr;     /* for returned internet address */
    int addrlen, n_retry;
    struct sigaction vec;
    char hostname[MAXHOST];
    char nameserver[TAM_BUFFER]; //lo que antes era argv[1];
    struct addrinfo hints, *res;
    char puerto[TAM_BUFFER]; //para almacenar el puerto en string
    char etiqueta[TAM_BUFFER]; //para copiar las respuestas al log

    strcpy(nameserver,servidor);
    
    //abro el archivo con las ordenes
    if(NULL == (fp = fopen(ruta,"r"))){
        fprintf(stderr, "error al abrir el fichero de ordenes\n");
        exit(1);
    }

    //creo el socket
    s = socket (AF_INET, SOCK_DGRAM, 0);
    if (s == -1) {
        //perror(argv[0]);
        fprintf(stderr, "unable to create socket\n");
        exit(1);
    }
    
    //limpio las estructuras del cliente y del servidor
    memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
    memset ((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));
    

    //establezco estructuras del cliente
    myaddr_in.sin_family = AF_INET;
    myaddr_in.sin_port = 0;
    myaddr_in.sin_addr.s_addr = INADDR_ANY;


    //bindeo el socket a mi direccion local
    if (bind(s, (const struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
        //perror(argv[0]);
        fprintf(stderr, "unable to bind socket\n");
        exit(1);
    }

    //adquireo los datos
    addrlen = sizeof(struct sockaddr_in);
    if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1) {
            //perror(argv[0]);
            fprintf(stderr, "unable to read socket address\n");
            exit(1);
    }

    //imprimo por pantalla los datos de la conexion
    time(&timevar);
    printf("Connected to %s on port %u at %s", nameserver, ntohs(myaddr_in.sin_port), (char *) ctime(&timevar));

    //relleno las estructuras del server
    servaddr_in.sin_family = AF_INET;
    memset (&hints, 0, sizeof (hints));
    hints.ai_family = AF_INET;
    errcode = getaddrinfo (nameserver, NULL, &hints, &res); 
    if (errcode != 0){
        fprintf(stderr, "No es posible resolver la IP de %s\n", nameserver);
        exit(1);
    }
    else {
        servaddr_in.sin_addr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
    }
    freeaddrinfo(res);
     /* puerto del servidor en orden de red*/
    servaddr_in.sin_port = htons(PUERTO);

   /* Registrar SIGALRM para no quedar bloqueados en los recvfrom */
    vec.sa_handler = (void *) handler;
    vec.sa_flags = 0;
    if ( sigaction(SIGALRM, &vec, (struct sigaction *) 0) == -1) {
            perror(" sigaction(SIGALRM)");
            fprintf(stderr,"unable to register the SIGALRM signal\n");
            exit(1);
        }
    
    n_retry=RETRIES;
    
    //creo el fichero de log ahora que conozco mi puerto efimero
        //fprintf(stdout,"mi puerto es el: %u",ntohs(myaddr_in.sin_port));
    sprintf(puerto,"%u.txt",ntohs(myaddr_in.sin_port));
    log = fopen(puerto,"a");

    //primera llamada de la conexion
    while (n_retry > 0) {
        /* Send the request to the nameserver. */
        //aqui era argv2
        if (sendto (s, servidor, strlen(servidor), 0, (struct sockaddr *)&servaddr_in,
                sizeof(struct sockaddr_in)) == -1) {
                //perror(argv[0]);
                fprintf(stderr, "unable to send request\n");
                exit(1);
        }

        //timeout para que no se quede bloqueado
        alarm(TIMEOUT);

        //recv bloqueante. Espera la respuesta.
        printf("recibo\n");
        if (recvfrom (s, &reqaddr, sizeof(struct in_addr), 0,
                        (struct sockaddr *)&servaddr_in, &addrlen) == -1) {
            if (errno == EINTR) {
                    /* Alarm went off and aborted the receive.
                     * Need to retry the request if we have
                     * not already exceeded the retry limit.
                     */
                 sprintf(etiqueta,"attempt %d (retries %d).\n", n_retry, RETRIES);
                 fprintf(stdout,etiqueta);
                 fputs(etiqueta,log);
                 n_retry--; 
            }else {
                sprintf(etiqueta,"Unable to get response ");
                fprintf(stdout,etiqueta);
                fputs(etiqueta,log);
                exit(1); 
            }
        }else{
            alarm(0);
            /* Print out response. */
            if (reqaddr.s_addr == ADDRNOTFOUND){
            	sprintf(etiqueta,"Host %s unknown by nameserver %s\n", servidor, nameserver);
            	fprintf(stdout,etiqueta);
            	fputs(etiqueta,log);
            }else{
                /* inet_ntop para interoperatividad con IPv6 */
                if (inet_ntop(AF_INET, &reqaddr, hostname, MAXHOST) == NULL)
                   perror(" inet_ntop \n");
                sprintf(etiqueta,"Address for %s is %s\n", servidor, hostname);
            	fprintf(stdout,etiqueta);
            	fputs(etiqueta,log);
            }   
            break;  
        }

    }

    //restablezco el numero de retries
    n_retry=RETRIES;

    printf("empiezo a leer el fichero\n");
    do{
        fgets(string,TAM_BUFFER,fp);
        fprintf(stdout,"enviando: %s",string);
        while(n_retry > 0){
            //envio mensaje
            if (sendto (s, string, TAM_BUFFER, 0, (struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in)) == -1) {
                //perror(argv[0]);
                sprintf(etiqueta, "unable to send request\n");
                fprintf(stderr,etiqueta);
                fputs(etiqueta,log);
                exit(1);
            }

            //timeout para que no se quede bloqueado
            alarm(TIMEOUT);

            //inicia la recepcion
            if (recvfrom (s, string, TAM_BUFFER, 0,(struct sockaddr *)&servaddr_in, &addrlen) == -1) {
                if (errno == EINTR) {
                    //ha saltado la alarma
                    sprintf(etiqueta,"attempt %d (retries %d).\n", n_retry, RETRIES);
                    fprintf(stderr,etiqueta);
                    fputs(etiqueta,log);
                    n_retry--; 
                }else {
                    sprintf(etiqueta,"Unable to get response");
                    fprintf(stderr,etiqueta);
                    fputs(etiqueta,log);
                    exit(1); 
                }
            }else{
                alarm(0);
                /* Print out response. */
                sprintf(etiqueta,"recibido:%s\n\n",string);
                fprintf(stdout,etiqueta);
                fputs(etiqueta,log);
                break;  
            }

        }
    }while(!feof(fp));


    if (n_retry == 0) {
       sprintf(etiqueta,"Unable to get response");
       fprintf(stdout,etiqueta);
       fputs(etiqueta,log);

       sprintf(etiqueta," %s after %d attempts.\n", nameserver, RETRIES);
       fprintf(stdout,etiqueta);
       fputs(etiqueta,log);
    }

    fclose(log);
    fclose(fp);
}