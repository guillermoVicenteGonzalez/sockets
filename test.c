#include<stdio.h>
#include<string.h>
#include<stdlib.h>

//fgets(palabra,10000,fp); lee una linea completa

int comprobarCorreo(char *sender);
int main(int argc, char *argv[])
{
	FILE *fp;
   char *ruta = argv[1];
   char linea[100];
   char *palabra;
   char delimiter[1] = " ";

   fp = fopen(ruta,"a");
   fgets(linea,100,fp);

   printf("%s\n",linea);
   palabra = strtok(linea, " ");
   printf("%s",palabra);
   /*
   printf("\n");
   while(palabra != NULL){
      printf("\n");
      printf("%s", palabra);
      palabra = strtok(NULL, " ");
   }

   while(!feof(fp)){
      fgets(linea,100,fp);
      printf("\n%s\n",linea);
      if(strcmp(linea,"DATA\n") == 0){
         printf("encontrado data");
      }
      if(strcmp(linea,".\n")==0){
         printf("\npunto\n");
      }
   }
*/
   printf("%d\n",comprobarCorreo("guillermo@usal.es"));
   printf("%d\n",comprobarCorreo("guillermousales"));
   printf("%d\n",comprobarCorreo("guill.ermo@usal.es"));
   printf("%d\n",comprobarCorreo("guillermo@usal@españa.com"));
   printf("%d\n",comprobarCorreo("guillermo.usal@es"));

   fclose(fp);
   printf("\n");
   return 0;
}

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
   return resultado;
   //printf("\n[0]:%c [%d]:%c\n",sender[0],i-2,sender[i-2]);
   //-2 porque los ultimos caracteres son \0 y \n
}

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