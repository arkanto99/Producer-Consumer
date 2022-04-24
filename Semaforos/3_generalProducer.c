//Generalizacion del problema del productor-consumidor usando procesos
//Compilar con -pthread
/*Estructura ficheroMemoria:
	Posicion 0:  Count
	Posiciones 1-N: Buffer
*/
//EJECUTAR EN PRIMER LUGAR (antes que el consumidor)

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h> //Biblioteca necesaria para realizar las proyecciones en memoria (mman, munmap)
#include <unistd.h> //Libreria que incluye fork()
#include <sys/stat.h> //Biblioteca necesaria pra fstat. Tambien para los modos de creacion del semafoto
#include <fcntl.h> //Biblioteca donde se encuentran open. Tambien las flags necesarias para crear el semaforo
#include <pthread.h>//Libreria necesaria para el uso de up y wait con los semaforos
#include <semaphore.h>
#include <sys/wait.h> //Libreria en la cual se encuentra la funcion wait

//DEFINIMOS COLORES PARA FACILITAR A LECTURA DOS PRODUCTORES
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

void producer(char *color);//Le pasamos "al productor" el color con el que se identificara en el terminal

//Semaforos utilizados en la solucion de Tanenbaum
sem_t *full;//Toma valores entre 0 y N. Indica los espacios ocupados del buffer
sem_t *empty;//Toma valores entre 0 y N. Indica los espacios libres del buffer
sem_t *mutex;//Toma los valores 0 o 1.Garantiza la exclusion mutua
//Representacion de la zona de memoria compartida. La declaramos aqui para poder acceder desde las diferentes funciones
int *proyeccion;
//Cadena que conten as cores disponibles para asignalas facilmente
char *colors[]={GREEN,WHITE,BLUE,YELLOW,RED,MAGENTA,CYAN,GRAY,LIGHTRED,LIGHTGREEN,LIGHTBLUE,LIGHTMAGENTA,LIGHTCYAN,DARKGRAY,BLACK,BROWN};

int main(int argc, char * argv[]){//Argumentos: ficheroMapeoMemoria, nºProductores


	char *directorio;//Ubicacion del fichero que se proyectara en memoria
  int fichero;
	int estado; //Variable para almacenar o estado devolto polo wait do fillo
  struct stat informacion;//Esta variable almacena informacion sobre el fichero. Necesaria para realizar el truncate

	int hijo; //PID do proceso creado con fork
	int m; //Numero de productores
	int i; //Variable auxiliar para bucles


	//Obtencion de parametros
	if(argc!=3){
    printf("Numero de parametros invalidos: Introduzca el fichero de mapeo de memoria y el numero de productores");
  }
  else{
    directorio=argv[1];//Ubicacion del fichero
		m=atoi(argv[2]);//Convertimos en entero el numero de productores pasado como parametro
  }

  if((fichero=open(directorio,O_RDWR))==-1){ //Abrimos el fichero con permisos de lectura y escritura
    perror("Error al abrir el fichero: ejecucion abortada\n");
    exit(-1);
  }

  if(ftruncate(fichero,(N+1)*sizeof(int))==-1){ //Tomamos como tamanho del fichero N+1 enteros, para almacenar el buffer y el count
    perror("Ampliacion del fichero erronea: ejecucion abortada\n");
    exit(-1);
  }
  fstat(fichero,&informacion);//Obtenemos la nueva informacion sobre el fichero tras el cambio de tamanho
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

	//Inicializacion de la zona de memoria compartida
	proyeccion[0]=0;//Inicializamos count a 0
  for(int i=1;i<=N;i++){
    proyeccion[i]=-1; //-1 indica que el buffer esta vacio.
  }

	//CREACION DE LOS SEMAFOROS. Eliminamos posibles semaforos con el mismo nombre que existan en el kernel
	sem_unlink("FULL");
	sem_unlink("EMPTY");
	sem_unlink("MUTEX");
	//sem_open. Argumentos:
	/*1.-Nombre		2.-Flag		3.-Modo de creacion		4.-Valor de inicializacion del semaforo*/
	full= sem_open("FULL",O_CREAT,0600,0);
	empty= sem_open("EMPTY",O_CREAT,0600,N);
	mutex= sem_open("MUTEX",O_CREAT,0600,1);

	if(full==SEM_FAILED || empty==SEM_FAILED || mutex==SEM_FAILED){//Comprobamos si los semaforos fueron creados correctamente
		perror("Error al crear los semáforos: ejecución abortada");
		exit(-1);
	}

	for(i=0;i<m;i++){//Se crearan m productores
    if((hijo=fork())<0){ //Creamos los procesos
      printf("Error en la creacion de uno de los procesos: Ejecucion abortada\n");
      exit(-1);
    }
    else if(hijo==0){ //Proceso productor
      producer(colors[i%15]); //Hay 15 colores disponibles. Si hay mas productores, se repetiran
      exit(1); //Salimos do proceso
    }
    else //Proceso padre
      ; //Continuamos con el bucle de creacion de procesos productores
  }
  //Esperamos por los m productores
  for(i=0;i<m;i++){
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


int  produce_item(){//Crea un item
	int item = (rand()%N)+1;//Obtenemos un numero aleatorio entre 1 y 10
  return item;
}

void insert_item(int item,int count){//Introduce un item en el buffer

	*(proyeccion+count)=item;//"producimos" un item en la posicion count
}

void producer(char *color){

	int item;
	int iteraciones=10;
	int *count=proyeccion+0;//Count se encuentra en la posicion 0 de la zona de memoria compartida
	int i;
	srand(12);

	while(iteraciones-- > 0){

		item=produce_item();
		sleep(rand()%5);//Sleep aleatorio entre 0 y 4 segundos
		sem_wait(empty); //down
		sem_wait(mutex); //down
		printf("%s",color);//fijamos el color de este productor. Lo hacemos dentro de la region con mutex para que ningun otro proceso cambie el color mientras
		printf("Productor en región crítica\n");
		printf("	Buffer antes de producir: ");
		for(i=1;i<=N;i++){
			printf(" %d ",*(proyeccion+i));
		}
		++(*count); //Incrementamos en primer lugar pues el buffer comienza en la posicion 1, no en la 0
		insert_item(item,*count);
		printf("\n	-Count: %d",*count);
		printf("		-Item producido: %d\n",item);
		sem_post(mutex); //up
		sem_post(full); //up

	}
}
