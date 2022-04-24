//Implementacion del problema del productor-consumidor usando procesos
/*Estructura ficheroMemoria:
	Posicion 0:  Count
	Posiciones 1-N: Buffer
*/
//EJECUTAR EN PRIMER LUGAR (antes que el consumidor)

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

void producer();
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
    directorio="ficheroMemoria.txt"; ///Valor por defecto
  }

  if((fichero=open(directorio,O_RDWR))==-1){ //Abrimos el fichero con permisos de lectura y escritura
    perror("Error al abrir el fichero: ejecucion abortada\n");
    exit(-1);
  }
  fstat(fichero,&informacion); //Obtenemos informacion sobre el fichero

  if(ftruncate(fichero,(1+N)*sizeof(int))==-1){ //Tomamos como tamanho del fichero N+1 enteros, para almacenar el buffer y el count
    perror("Ampliacion del fichero erronea: ejecucion abortada\n");
    exit(-1);
  }
  fstat(fichero,&informacion);//Obtenemos la nueva informacion sobre el fichero tras la ampliacion de tamanho

	/*MMAP:
  -El primer argumento de mmap indica la direccion de memoria donde se va a comenzar a proyectar el archivo.
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
	//Inicializacion de la zona de memoria compartida
	proyeccion[0]=0;//Inicializamos count a 0
	for(int i=1;i<=N;i++){
		proyeccion[i]=-1; //-1 indica que el buffer esta vacio.
	}

	producer();

  if(munmap(proyeccion,informacion.st_size)==-1){//Cerramos el acceso a la zona de memoria compartida
    perror("Error al cerrar la proyeccion de memoria: Ejecucion abortada\n");
    exit(-1);
  }
  close(fichero);//Cerramos el fichero

	return EXIT_SUCCESS;
}


int  produce_item(){//Crea un item
  int item = (rand()%N)+1;//Obtenemos un numero aleatorio entre 1 y 10
  printf("Productor --> Item producido: %d ",item);
  return item;
}

void insert_item(int item,int *count){//Introduce un item en el buffer
	//"producimos" un item en la posicion count
  proyeccion[*count+1]=item; //Sumamos 1 pues en la posicion 0 de la memoria compartida se encuentra el contador

}

void producer(){ //Productor realizado no teniendo en cuenta las carreras criticas que pueden suceder

	int item;
  int *count;
	count=proyeccion+0;//Count se encuentra en la posicion 0 de la memoria compartida
	srand(3.14);
	while(TRUE){
		item=produce_item();
		while(*count==N); //Esperamos mientras en buffer este lleno
    insert_item(item,count);
		++(*count);//Incrementamos el contador
		printf("  Count:%d \n",*count);
		sleep(rand()%3);//Sleep aleatorio entre 0 y 2 segundos
	}
}
