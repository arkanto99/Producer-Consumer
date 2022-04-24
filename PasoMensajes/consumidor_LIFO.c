/*Resolucion del problema del productor-consumidor utilizando paso de mensajes*/
//Version LIFO (este codigo es optativo, funciona perfectamente utilizando el consumidor de la version FIFO)
//Compilar con -lrt

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> //Libreria que incluye sleep() y fork
#include <mqueue.h> //Libreria que incluye las funciones de paso de mensajes
#include <fcntl.h>  //Incluye las flags del tipo O_*


#define MAX_BUFFER 6
#define DATOS_A_PRODUCIR 20
#define MSG_CHAR_SIZE 15
/* tamaño del buffer */
/* número de datos a producir/consumir */

/* cola de entrada de mensajes para el productor */
mqd_t almacen1;
/* cola de entrada de mensajes para el consumidor */
mqd_t almacen2;

void consumidor();

int main() {

  /* Apertura de los buffers */
  almacen1 = mq_open("/ALMACEN1", O_WRONLY);
  almacen2 = mq_open("/ALMACEN2", O_RDONLY);
  if ((almacen1 == -1) || (almacen2 == -1)) {
    perror ("mq_open");
    exit(EXIT_FAILURE);
  }
  //Invocamos la función que realiza el consumo de items
  consumidor();
  //Cerramos el acceso a las colas de entrada de consumidor y productor desde este proceso
  mq_close(almacen1);
  mq_close(almacen2);

  exit(EXIT_SUCCESS);
}

void consume_item(char *item,int prioridad){//Muestra un item
	    printf("Item consumido: %s, Prioridad: %d\n",item,prioridad);
      sleep(rand()%6);
}

void consumidor(){
  int i;
  char mensaje[MSG_CHAR_SIZE];
  char mensajeVacio[0]; //La solucion de Tanenbaum implica el envio de mensajes vacios
  unsigned int prioridad=0;
  srand(618);//Fijamos una semilla para el numero aleatorio
  
  for(i=0;i<MAX_BUFFER;i++){//Enviamos tantos mensajes vacios como quepan en el buffer
    mq_send(almacen1,mensajeVacio,sizeof(mensajeVacio),0);
  }
  for(i=0;i<DATOS_A_PRODUCIR-MAX_BUFFER;i++){ //Realizamos este numero de iteraciones para compensar las peticiones de items realizadas en el bucle anterior (para más detalle, ver el informe anexo al código)
    mq_receive(almacen2,mensaje,sizeof(mensaje),&prioridad); //Esperamos a que llegue un mensaje
    mq_send(almacen1,mensajeVacio,sizeof(mensajeVacio),0);//Enviamos un mensaje vacio
    consume_item(mensaje,prioridad);
  }
  for(i=0;i<MAX_BUFFER;i++){ //Extraemos los mensajes restantes del buffer, sin efectuar nuevas peticiones de items
    mq_receive(almacen2,mensaje,sizeof(mensaje),&prioridad); //Esperamos a que llegue un mensaje
    consume_item(mensaje,prioridad);
  }

}
