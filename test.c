#include<stdio.h>
#include<string.h>
#include<stdlib.h>

//fgets(palabra,10000,fp); lee una linea completa

int main(int argc, char *argv[])
{
	FILE *fp;
   char *ruta = argv[1];
   char linea[100];
   char *palabra;
   char delimiter[1] = " ";

   fp = fopen(ruta,"r");
   fgets(linea,100,fp);

   printf("%s\n",linea);
   palabra = strtok(linea, " ");
   printf("%s",palabra);
   
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

   fclose(fp);
   printf("\n");
   return 0;
}