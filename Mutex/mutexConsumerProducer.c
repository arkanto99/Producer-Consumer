/*Resolucion del problema del productor-consumidor utilizando mutexes y variables de condicion*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h> //Libreria que incluye sleep()

//Definimos colores para facilitar la lectura de  resultados
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

#define MAX 10 //Numero de elementos a producir por cada productor
#define N 10 //Tamanho del buffer

pthread_mutex_t mutex; //Declaracion del mutex que control el acceso a la región crítica y garantiza la exclusion mutua
pthread_cond_t condConsumer; //Declaracion de la variable de condicion asociada al consumidor
pthread_cond_t condProducer; //Declaracion de la variable de condicion asociada al productor

int buffer[N]; //Declaracion del buffer de tamanho N
//Las siguientes variables permiten que el buffer funcione como una cola FIFO
int cuenta=0;//Indica el numero de elementos validos en el buffer (numero de posiciones ocupadas)
int posConsumir=0;//Indica la posicion en la cual se debe consumir
int posProducir=0;//Indica la posicion en la cual se debe producir
//Cadena que contiene los colores disponibles
char *colors[]={GREEN,WHITE,BLUE,YELLOW,RED,MAGENTA,CYAN,GRAY,LIGHTRED,LIGHTGREEN,LIGHTBLUE,LIGHTMAGENTA,LIGHTCYAN,DARKGRAY,BLACK,BROWN};
//Struct que almacena el color y el numero de iteraciones asociadas a cada consumidor
struct consum_struct {
  char *color;
  int numeroIteraciones;
};

void *producer(void *ptr_color);
void *consumer(void *ptr_struct);

int main(int argc, char * argv[]){

  int nPr,nCo; //Numero de consumidores, numero de productores
  int i; //Variable auxiliar para bucles


  //Lectura de parametros: numero de productores y numero de consumidores
  if(argc!=3){
    printf("Numero de parametros invalidos: Introduzca el numero de consumidores y el numero de productores");
    exit(-1); //Salimos del programa
  }
  else{
    nPr=atoi(argv[1]);//numero de productores
		nCo=atoi(argv[2]);//numero de consumidores
  }

  if(nPr<0 || nPr>=5 || nCo<0 || nCo>=4){ //Comprobamos si el numero de productores/consumidores es valido
    printf("Ejecucion abortada: recuerde que el numero de productores debe ser inferior a 5 y el de consumidores inferior a 4");
    exit(-1);//Salimos del programa
  }

  for(i=0;i<N;i++){ //Inicializamos el buffer al valor que indica "vacio"
    buffer[i]=-1;
  }


  pthread_t productores[nPr]; //Array de productores
  pthread_t consumidores[nCo]; //Array de consumidores

  //Eliminamos los mutex antes de inicializarlos, por si no se hubiese eliminado correctamente en ejecuciones anteriores
  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&condProducer);
  pthread_cond_destroy(&condConsumer);
  //Inicilizacion del mutex y de las variables de condicion, todos a valor 0
  pthread_mutex_init(&mutex,0);
  pthread_cond_init(&condProducer,0);
  pthread_cond_init(&condConsumer,0);

  //Creacion de productores (y reparto de colores)
  for(i=0;i<nPr;i++){
    pthread_create(&productores[i],0,producer,colors[i]);
  }

  //Reparto de iteraciones y colores entre los diferentes consumidores
  struct consum_struct color_iteraciones[nCo];//A cada consumidor se le pasa su color y el numero de iteraciones a realizar
  int iteracionesConsumidores = (MAX*nPr)/(float)nCo; //Division exacta: Las iteraciones "sobrantes" se asignan al primer consumidor creado
  //Las asignaciones al primer consumidor se hacen fuera pues en ocasiones realiza mas iteraciones que el resto
  color_iteraciones[0].numeroIteraciones=iteracionesConsumidores+(MAX*nPr-iteracionesConsumidores*nCo); //Asignamos las iteraciones restantes
  color_iteraciones[0].color=colors[nPr];
  for(i=1;i<nCo;i++){
    color_iteraciones[i].numeroIteraciones=iteracionesConsumidores;
    color_iteraciones[i].color=colors[nPr+i];//Sumamos nPr para no repetir colores
  }

  //Creacion de consumidores
  for(i=0;i<nCo;i++){
    pthread_create(&consumidores[i],0,consumer,&color_iteraciones[i]);
  }

  //Esperamos a que terminen los consumidores y los productores
  for(i=0;i<nPr;i++){
    pthread_join(productores[i],0);
  }
  for(i=0;i<nCo;i++){
    pthread_join(consumidores[i],0);
  }
  //Eliminamos las variables de condicion y el mutex
  pthread_cond_destroy(&condConsumer);
  pthread_cond_destroy(&condProducer);
  pthread_mutex_destroy(&mutex);

  return EXIT_SUCCESS;
}


int  produce_item(){//Crea un item
	int item = (rand()%N)+1;//Obtenemos un numero aleatorio entre 1 y 10
  return item;
}

void insert_item(int item){//Introduce un item en el buffer

	buffer[posProducir]=item;//"producimos" un item en la posicion count
  //Indicamos que el siguiente elemento a producir sea en la posicion posterior del buffer (o al inicio si estamos en la ultima)
  posProducir=(posProducir+1)%N; //Nos aseguramos que, al llegar a N, la cuenta vuelva a 0 (ya que el buffer tiene N posiciones, comenzando en la 0)
}

/*La siguiente funcion es invocada por el consumidor desde fuera de la zona que garantiza la exclusion mutua, ya que no es necesaria garantizarla.
Sin embargo, esto provoca que la impresion en el terminal puede no corresponderse correctamente (bien sea el color o el tabulado), ya que
otro hilo puede estar imprimiendo ya cuando se ejecute. Esto, si bien puede dificultar en momentos muy puntuales la lectura de resultados,
no tiene una mayor complejidad*/

void consume_item(int itemize){//Muestra un item
	    printf("	-Item consumido: %d\n",itemize);

}

int remove_item(){//Elimina un item del buffer
  int item;
	item=buffer[posConsumir];//"Consumimos" el item que se encuentra en la posicion count
	buffer[posConsumir]=-1; //-1 indica que esta vacio el buffer
  //Indicamos que el siguiente elemento a consumir sea en la posicion posterior del buffer (o al inicio si estamos en la ultima)
  posConsumir=(posConsumir+1)%N; //Nos aseguramos que, al llegar a N, la cuenta vuelva a 0 (ya que el buffer tiene N posiciones, comenzando en la 0)
	return item;
}

void *producer(void *ptr_color){
  int i;consumir
  int item;
  char *color;
  color=(char *)ptr_color;//Obtenemos el color
  srand(163);//Fijamos una semilla para la generacion de numeros aleatorios

  for(i=0;i<MAX;i++){
    item=produce_item();
    sleep(rand()%5);//Sleep aleatorio entre 0 y 4 segundos
    pthread_mutex_lock(&mutex); //El productor obtiene acceso exclusivo a la region critica
    while(cuenta>=N) pthread_cond_wait(&condProducer,&mutex); //Esperamos a que se reciba la señal de que se puede producir
    insert_item(item);
    ++cuenta;//Incrementamos el numero de elementos para consumir que contiene el buffer
      printf("%s",color);//fijamos el color de este productor. Lo hacemos dentro de la region de acceso exclusivo para que ningun otro proceso cambie el color
      printf("Productor: ");
      for(int j=0;j<N;j++){ //Imprimimos el buffer para que sea mas sencillo de interpretar
        printf(" %d ",buffer[j]);
      }
      printf("\n");
      printf("		-Item producido: %d\n",item);
    pthread_cond_signal(&condConsumer);//Enviamos una señal al consumidor para despertarlo
    pthread_mutex_unlock(&mutex); //Salimos de la region critica
  }
  pthread_exit(0);
}

void *consumer(void *ptr_struct){
  int i;
  int item;
  struct consum_struct *color_iteraciones;
  color_iteraciones=(struct consum_struct *)ptr_struct;//Obtenemos el color
  srand(24); //Fijamos una semilla para la generacion de numeros aleatorios

  for(i=0;i<color_iteraciones->numeroIteraciones;i++){ //El consumidor realiza las iteraciones que le marca el campo del struct, segun el reparto hecho en el main
    pthread_mutex_lock(&mutex); //El productor obtiene acceso exclusivo a la region critica
    while(cuenta==0) pthread_cond_wait(&condConsumer,&mutex); //Esperamos a que se reciba la señal de que se puede producir
    --cuenta; //Decrementamos el numero de elementos para consumir que contiene el buffer
    item=remove_item();
      printf("%s",color_iteraciones->color);//fijamos el color de este consumidor. Lo hacemos dentro de la region de acceso exclusivo para que ningun otro proceso cambie el color mientras
      printf("Consumidor: ");
      for(int j=0;j<N;j++){ //Imprimimos el buffer para que sea mas sencillo de interpretar
        printf(" %d ",buffer[j]);
      }
      printf("\n");
    pthread_cond_signal(&condProducer);//Enviamos una señal al productor para despertarlo
    pthread_mutex_unlock(&mutex); //Salimos de la region critica
    consume_item(item);
    sleep(rand()%5);//Sleep aleatorio entre 0 y 4 segundos
  }
  pthread_exit(0);
}
