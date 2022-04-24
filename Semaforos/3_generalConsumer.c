//Generalizacion del problema del productor-consumidor usando procesos
//Compilar con -pthread
/*Estructura ficheroMemoria:
	Posicion 0:  Count
	Posiciones 1-N: Buffer
*/
//EJECUTAR EL PRODUCTOR ANTES DE ESTE CODIGO

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h> //Biblioteca necesaria para realizar las proyecciones en memoria (mman, munmap)
#include <unistd.h> //Libreria que incluye fork()
#include <sys/stat.h> //Biblioteca necesaria pra fstat
#include <fcntl.h> //Biblioteca donde se encuentran open
#include <pthread.h>
#include <semaphore.h>
#include <sys/wait.h> //Libreria en la cual se encuentra la funcion wait

//DEFINIMOS COLORES PARA FACILITAR A LECTURA DOS CONSUMIDORES
#define BLACK "\033[22;30m"
#define WHITE "\033[01;37m"
#define BROWN "\033[22;33m"
#define YELLOW "\033[01;33m"
#define RED "\033[22;31m"
#define LIGHTRED "\033[01;31m"
#define GREEN "\033[22;32m"
#define LIGHTGREEN "\033[01;32m"
#define BLUE "\033[22;34m"
#define LIGHTBLUE "\033[01;34m"
#define MAGENTA "\033[22;35m"
#define LIGHTMAGENTA "\033[01;35m"
#define CYAN "\033[22;36m"
#define LIGHTCYAN "\033[01;36m"
#define GRAY "\033[22;37m"
#define DARKGRAY "\033[01;30m"
//Definimos el tamaño del buffer
#define N 5
//Definimos el booleano TRUE para facilitar la lectura del codigo
#define TRUE 1


void consumer(char *color);//Le pasamos "al consumidor" el color con el que se identificara en el terminal

//Semaforos utilizados en la solucion de Tanenbaum
sem_t *full;//Toma valores entre 0 y N. Indica los espacios ocupados del buffer
sem_t *empty;//Toma valores entre 0 y N. Indica los espacios libres del buffer
sem_t *mutex;//Toma los valores 0 o 1.Garantiza la exclusion mutua
//Representacion de la zona de memoria compartida. La declaramos aqui para poder acceder desde las diferentes funciones
int *proyeccion;
//Cadena que conten as cores disponibles para asignalas facilmente
char *colors[]={GREEN,WHITE,BLUE,YELLOW,RED,MAGENTA,CYAN,GRAY,LIGHTRED,LIGHTGREEN,LIGHTBLUE,LIGHTMAGENTA,LIGHTCYAN,DARKGRAY,BLACK,BROWN};


int main(int argc, char * argv[]){

	char *directorio;//Ubicacion del fichero que se proyectara en memoria
  int fichero;
	int estado; //Variable para almacenar o estado devolto polo wait do fillo
  struct stat informacion;//Esta variable almacena informacion sobre el fichero. Necesaria para obtener su dimension

	int hijo; //PID do proceso creado con fork
	int n; //Numero de consumidores
	int i; //Variable auxiliar para bucles

	//Obtencion de parametros
	if(argc!=3){
    printf("Numero de parametros invalidos: Introduzca el fichero de mapeo de memoria y el numero de consumidores");
  }
  else{
    directorio=argv[1];//Ubicacion del fichero
		n=atoi(argv[2]);//Convertimos en entero el numero de consumidores pasado como parametro
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

	for(i=0;i<n;i++){//Se crearan n consumidores
    if((hijo=fork())<0){ //Creamos los procesos
      printf("Error en la creacion de uno de los procesos: Ejecucion abortada\n");
      exit(-1);
    }
    else if(hijo==0){ //Proceso consumidor
			consumer(colors[i%15]); //Hay 15 colores disponibles. Si hay mas consumidores, se repetiran
      exit(1); //Salimos do proceso
    }
    else //Proceso padre
      ; //Contiuamos con el bucle de creacion de procesos consumidores
  }
  //Esperamos por los n consumidores
  for(i=0;i<n;i++){
    wait(&estado);
  }
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

void consumer(char *color){

	int item;
	int iteraciones=10;
	int *count=proyeccion+0;//Count se encuentra en la posicion 0 de la zona de memoria compartida
	srand(3.14);

	while(iteraciones-- > 0){
		sleep(rand()%5);//Sleep aleatorio entre 0 y 4 segundos
		sem_wait(full); //Down
		sem_wait(mutex); //Down
		printf("%s",color);//fijamos el color de este productor. Lo hacemos dentro de la region con mutex para que ningun otro proceso cambie el color mientras
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
