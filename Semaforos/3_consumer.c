//Implementacion del problema del productor-consumidor usando procesos
//Implementacion del problema del productor-consumidor usando procesos
/*Estructura ficheroMemoria:
	Posicion 0:  Count
	Posiciones 1-N: Buffer
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h> //Biblioteca necesaria para realizar las proyecciones en memoria (mman, munmap)
#include <unistd.h> //Libreria que incluye fork()
#include <sys/stat.h> //Biblioteca necesaria pra fstat
#include <fcntl.h> //Biblioteca donde se encuentran open
#include <sys/wait.h> //Libreria en la cual se encuentra la funcion wait


//Definimos el tamaño del buffer
#define N 10
//Definimos el booleano TRUE para facilitar la lectura del codigo
#define TRUE 1

void consumer();
//Representacion de la zona de memoria compartida. La declaramos aqui para poder acceder desde las diferentes funciones
int *proyeccion;

int main(int argc, char * argv[]){

	char *directorio;//Ubicacion del fichero que se proyectara en memoria
  int fichero;
	struct stat informacion;//Esta variable almacena informacion sobre el fichero. Necesaria para realizar el truncate

	//Obtencion de parametros
	if(argc==2){
    directorio=argv[1];//Ubicacion del fichero
  }
  else{
    directorio="ficheroMemoria.txt"; //Valor por defecto
  }

  if((fichero=open(directorio,O_RDWR))==-1){ //Abrimos el fichero con permisos de lectura y escritura
    perror("Error al abrir el fichero: ejecucion abortada\n");
    exit(-1);
  }
  fstat(fichero,&informacion); //Obtenemos informacion sobre el fichero. La que nos interesa es su longitud, que se encuentra en informacion.st_size
	/*MMAP:
  -El primer argumento d//Sleep aleatorio entre 0 y 2 segundose mmap indica la direccion de memoria donde se va a comenzar a proyectar el archivo.
  Si es NULL, el kernel elige esta direccion
	-El segundo argumento indica la cantidad de memoria que se reserva: En este caso, el tamanho del fichero
  -El tercer argumento hace refrencia a los permisos que tendran las paginas de memoria donde se proyecta el FICHERO: En este caso, permisos de lectura y escritura
  .El cuarto argumento (flags) indican la visibilidad del mapa por otros procesos: En este caso, memoria compartida
  -El quinto argumento es el "objeto de memoria" que queremos proyectar
  -El sexto argumento indica el desplazamiento dentro de la zona de memoria indicada en el primer argumento*/
	if((proyeccion=mmap(NULL,informacion.st_size,PROT_READ | PROT_WRITE,MAP_SHARED,fichero,0))==((void *)-1)){
    perror("Error al proyectar el fichero en el mapa de memoria: ejecucion abortada\n");
    exit(-1);
  }

	consumer();

  if(munmap(proyeccion,informacion.st_size)==-1){//Cerramos el acceso a la zona de memoria compartida
    perror("Error al cerrar la proyeccion de memoria: Ejecucion abortada\n");
    exit(-1);
  }
  close(fichero);//Cerramos el fichero
	return EXIT_SUCCESS;
}


void consume_item(int item){//Muestra un item
    printf("Consumidor -->Item consumido: %d ",item);
}

int remove_item(int *count){//Elimina un item del buffer
	int item;
	item=proyeccion[*count];//En este caso no se indica que este vacio explicitamente en el buffer ya que este no se va a imprimir
	return item;
}

void consumer(){ //Consumidor realizado no teniendo en cuenta las carreras criticas que pueden suceder

	int item;
  int *count=proyeccion+0;
	srand(12);//Fijamos la semilla de generacion de numeros aleatorios

	while(TRUE){
		while(*count==0); //Esperamos mientras el buffer este vacio
		item=remove_item(count);
    sleep(rand()%3); //Sleep aleatorio entre 0 y 2 segundos
		--(*count);
		consume_item(item);
    printf("  Count:%d \n",*count); //Si count==10->El productor a incrementado contador antes de poder imprimirlo
	}
}
