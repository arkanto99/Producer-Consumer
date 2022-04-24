//Implementacion del problema del productor-consumidor usando procesos
//Compilar con -pthread
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
#include <pthread.h>
#include <semaphore.h>
#include <sys/wait.h> //Libreria en la cual se encuentra la funcion wait

//Definimos el tamaño del buffer
#define N 5
//Definimos el booleano TRUE para facilitar la lectura del codigo
#define TRUE 1

void consumer();

//Semaforos utilizados en la solucion de Tanenbaum
sem_t *full;//Toma valores entre 0 y N. Indica los espacios ocupados del buffer
sem_t *empty;//Toma valores entre 0 y N. Indica los espacios libres del buffer
sem_t *mutex;//Toma los valores 0 o 1.Garantiza la exclusion mutua
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
  -El primer argumento de mmap indica la direccion de memoria donde se va a comenzar a proyectar el archivo.
  Si es NULL, el kernel elige esta direccion
	-El segundo argumento indica la cantidad de memoria que se reserva: En este caso, el tamanho del fichero
  -El tercer argumento hace refrencia a los permisos que tendran las paginas de memoria donde se proyecta el FICHERO: En este caso, permisos de lectura y escritura
  .El cuarto argumento (flags) indican la visibilidad del mapa por otros procesos: En este caso, memoria compartida
  -El quinto argumento es el "objeto de memoria" que queremos proyectar
  -El sexto argumento indica el desplazamiento dentro de la zona de memoria indicada en el primer argumento*/
	if((proyeccion=(int*)mmap(NULL,informacion.st_size,PROT_READ | PROT_WRITE,MAP_SHARED,fichero,0))==((void *)-1)){
    perror("Error al proyectar el fichero en el mapa de memoria: ejecucion abortada\n");
    exit(-1);
  }

	//Abrimos los semaforos creados por el productor
	//sem_open. Argumentos:
	/*1.-Nombre		2.-Flags*/
	full= sem_open("FULL",0);
	empty= sem_open("EMPTY",0);
	mutex= sem_open("MUTEX",0);

	if(full==SEM_FAILED || empty==SEM_FAILED || mutex==SEM_FAILED){//Comprobamos si los semaforos fueron abiertos correctamente
		perror("Error al abrir los semáforos: ejecución abortada");
		exit(-1);
	}

	consumer();

  if(munmap(proyeccion,informacion.st_size)==-1){//Cerramos el acceso a la zona de memoria compartida
    perror("Error al cerrar la proyeccion de memoria: Ejecucion abortada\n");
    exit(-1);
  }
	close(fichero);//Cerramos el fichero
	sem_close(full); //Cierra al acceso al semaforo desde el proceso que ejecuta la llamada
	sem_close(empty);
	sem_close(mutex);

	return EXIT_SUCCESS;
}


void consume_item(int itemize){//Muestra un item
	    printf("	-Item consumido: %d\n",itemize); //Existe la posilidad de que se imprima en este color!

}

int remove_item(int count){//Elimina un item del buffer
  int item;
	item=proyeccion[count];//"Consumimos" el item que se encuentra en la posicion count
	proyeccion[count]=-1; //-1 indica que esta vacio el buffer
	return item;
}

void consumer(){

	int item;
	int iteraciones=10;
	int *count=proyeccion+0; //Count se encuentra en la posicion 0 de la zona de memoria compartida
	srand(3.14);

	while(iteraciones-- > 0){
		sleep(rand()%5);//Sleep aleatorio entre 0 y 4 segundos
		sem_wait(full); //Down
		sem_wait(mutex); //Down
		printf("Consumidor en región crítica\n");
		printf("Buffer antes de consumir: ");
		for(int i=1;i<=N;i++){
			printf(" %d ",*(proyeccion+i));
		}
		item=remove_item(*count);
		--(*count);
		printf("\n-Count: %d",*count);
    sem_post(mutex); //Up
		sem_post(empty); //Up
		consume_item(item);
	}
}
