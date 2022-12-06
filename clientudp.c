/*
 *          C L I E N T U D P
 *
 *  This is an example program that demonstrates the use of
 *  sockets as an IPC mechanism.  This contains the client,
 *  and is intended to operate in conjunction with the server
 *  program.  Together, these two programs
 *  demonstrate many of the features of sockets, as well as good
 *  conventions for using these features.
 *
 *
 *  This program will request the internet address of a target
 *  host by name from the serving host.  The serving host
 *  will return the requested internet address as a response,
 *  and will return an address of all ones if it does not recognize
 *  the host name.
 *
 */
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


int main(argc, argv)
int argc;
char *argv[];
{
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
    struct addrinfo hints, *res;


    //compruebo los argumentos que se me pasan
    if (argc != 4) {
        fprintf(stderr, "Usage:  %s <nameserver> <target> <file>\n", argv[0]);
        exit(1);
    }
    
    //abro el archivo con las ordenes
    if(NULL == (fp = fopen(argv[3],"r"))){
        fprintf(stderr, "error al abrir el fichero de ordenes\n");
        exit(1);
    }

    //creo el socket
    s = socket (AF_INET, SOCK_DGRAM, 0);
    if (s == -1) {
        perror(argv[0]);
        fprintf(stderr, "%s: unable to create socket\n", argv[0]);
        exit(1);
    }
    
    //limpio las estructuras del cliente y del servidor
    memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
    memset ((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));
    

    //establezco estructuras del cliente
    myaddr_in.sin_family = AF_INET;
    myaddr_in.sin_port = 0;
    myaddr_in.sin_addr.s_addr = INADDR_ANY;

    //bindeo el socket
    if (bind(s, (const struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
        perror(argv[0]);
        fprintf(stderr, "%s: unable to bind socket\n", argv[0]);
        exit(1);
    }

    //adquireo los datos
    addrlen = sizeof(struct sockaddr_in);
    if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1) {
            perror(argv[0]);
            fprintf(stderr, "%s: unable to read socket address\n", argv[0]);
            exit(1);
    }

    //imprimo por pantalla los datos de la conexion
    time(&timevar);
    printf("Connected to %s on port %u at %s", argv[1], ntohs(myaddr_in.sin_port), (char *) ctime(&timevar));

    //relleno las estructuras del server
    servaddr_in.sin_family = AF_INET;
    memset (&hints, 0, sizeof (hints));
    hints.ai_family = AF_INET;
    errcode = getaddrinfo (argv[1], NULL, &hints, &res); 
    if (errcode != 0){
        fprintf(stderr, "%s: No es posible resolver la IP de %s\n",argv[0], argv[1]);
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
            fprintf(stderr,"%s: unable to register the SIGALRM signal\n", argv[0]);
            exit(1);
        }
    
    n_retry=RETRIES;
    
    //mientras haya cosas en el fichero
    //while(!feof(fp))

    //primera llamada de la conexion
    while (n_retry > 0) {
        /* Send the request to the nameserver. */
        if (sendto (s, argv[2], strlen(argv[2]), 0, (struct sockaddr *)&servaddr_in,
                sizeof(struct sockaddr_in)) == -1) {
                perror(argv[0]);
                fprintf(stderr, "%s: unable to send request\n", argv[0]);
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
                 printf("attempt %d (retries %d).\n", n_retry, RETRIES);
                 n_retry--; 
            }else {
                printf("Unable to get response from");
                exit(1); 
            }
        }else{
            alarm(0);
            /* Print out response. */
            if (reqaddr.s_addr == ADDRNOTFOUND) 
               printf("Host %s unknown by nameserver %s\n", argv[2], argv[1]);
            else{
                /* inet_ntop para interoperatividad con IPv6 */
                if (inet_ntop(AF_INET, &reqaddr, hostname, MAXHOST) == NULL)
                   perror(" inet_ntop \n");
                printf("Address for %s is %s\n", argv[2], hostname);
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
                perror(argv[0]);
                fprintf(stderr, "%s: unable to send request\n", argv[0]);
                exit(1);
            }

            //timeout para que no se quede bloqueado
            alarm(TIMEOUT);

            //inicia la recepcion
            if (recvfrom (s, string, TAM_BUFFER, 0,(struct sockaddr *)&servaddr_in, &addrlen) == -1) {
                if (errno == EINTR) {
                    //ha saltado la alarma
                    printf("attempt %d (retries %d).\n", n_retry, RETRIES);
                    n_retry--; 
                }else {
                    printf("Unable to get response from");
                    exit(1); 
                }
            }else{
                alarm(0);
                /* Print out response. */
                fprintf(stdout,"recibido:%s\n\n",string);
                break;  
            }

        }
    }while(!feof(fp));


    if (n_retry == 0) {
       printf("Unable to get response from");
       printf(" %s after %d attempts.\n", argv[1], RETRIES);
       }
}
