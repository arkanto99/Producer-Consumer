/*Resolucion del problema del productor-consumidor utilizando paso de mensajes*/
//Version LIFO
//Compilar con -lrt

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> //Libreria que incluye sleep()
#include <mqueue.h> //Libreria que incluye las funciones de paso de mensajes
#include <fcntl.h>  //Incluye las flags del tipo O_*
#include <string.h> //Libreria para manejo de strings


#define MAX_BUFFER 6
#define DATOS_A_PRODUCIR 20
#define MSG_CHAR_SIZE 15
/* tamaño del buffer */
/* número de datos a producir/consumir */
/*Numero de chars enviados en un mensaje*/

/* cola de entrada de mensajes para el productor */
mqd_t almacen1;
/* cola de entrada de mensajes para el consumidor */
mqd_t almacen2;

void productor();


int main() {

  struct mq_attr attr;
  /* Atributos de la cola */
  attr.mq_maxmsg = MAX_BUFFER; //Tamaño de la cola
  attr.mq_msgsize = sizeof (char)*MSG_CHAR_SIZE; //Tamaño de cada mensaje enviado
  /* Borrado de los buffers de entrada por si existían de una ejecución previa*/
  mq_unlink("/ALMACEN1");
  mq_unlink("/ALMACEN2");
  /* Apertura y creacion de los buffers */
  almacen1 = mq_open("/ALMACEN1", O_CREAT|O_RDONLY, 0777, &attr);
  almacen2 = mq_open("/ALMACEN2", O_CREAT|O_WRONLY, 0777, &attr);
  if ((almacen1 == -1) || (almacen2 == -1)) {
    perror ("mq_open");
    exit(EXIT_FAILURE);
  }
  //Invocamos la función que realiza la producción de items
  productor();
  //Cerramos el acceso a las colas de entrada de consumidor y productor desde este proceso
  mq_close(almacen1);
  mq_close(almacen2);

  exit(EXIT_SUCCESS);
}

void  produce_item(char *item){//Crea un item, que en este caso es una cadena

  int numero=rand()%DATOS_A_PRODUCIR; //Generamos un numero aleatorio
  sprintf(item,"%d",numero); //Pasamos el numero a la cadena
  strcat(item,"-Hola"); //Concatenamos un pequeño mensaje
  sleep(rand()%3);
}


void productor() {
  int i;
  char mensaje[MSG_CHAR_SIZE];
  char vacio[0]; //Cadena 'vacia' para almacenar los mensajes vacios que envia el consumidor
  srand(314);//Fijamos una semilla para el numero aleatorio
  
  //Esperamos a que el consumidor llegue el buffer del productor con mensajes vacios
  struct mq_attr attr;
  attr.mq_curmsgs=0; //Inicializamos este valor para entrar correctamente en el bucle while
  while(attr.mq_curmsgs!=MAX_BUFFER)   mq_getattr(almacen1,&attr); //Esperamos a que el consumidor llene el buffer del productor con mensajes vacios

  for(i=0;i<DATOS_A_PRODUCIR;i++){ //Realizamos tantas iteraciones como elementos se quieren producir
    produce_item(mensaje);
    mq_receive(almacen1,vacio,sizeof(mensaje),0); //Esperamos a que llegue un mensaje vacio
    mq_send(almacen2,mensaje,sizeof(mensaje),0); //Enviamos el mensaje al consumidor
    printf("Item producido: %s\n",mensaje); //Mostramos el item producido
  }
}
