#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <semaphore.h>
#include <pthread.h>
#include "xprintf.h"

//una estructura que contiene un entero y un NTString (lo que produce el productor)
struct cadena{
    char linea[100];
    int valor;
};

//una estructura que maneja el buffer circular
struct queue{
    struct cadena** buff1;
    int tambuff1;
    int poner;  //indice del campo donde vamos a poner cosas producidas
    int sacar;  //indice del campo de donde vamos a sacar cosas para consumir
};

//variables globales
char nombrein[20];
char nombreout[20];
int tambuff;
struct queue *q=NULL;
struct queue *q2=NULL;
bool threadProdRunning=true;
bool threadConsRunning=true;
int numCleaners;

sem_t full;
sem_t empty;
sem_t fullq2;
sem_t emptyq2;
sem_t mutexsacarq;
sem_t mutexponerq2;

void createQueue(struct queue* q,int tambuff);
void push(struct cadena* c, struct queue* q);
struct cadena* pop(struct queue* q);
bool isEmpty(struct queue* q); //si esta vacia la cola
bool isFull(struct queue* q);  //si esta llena la cola
void printQueue(struct queue* q);
void deleteQueue(struct queue* q);
void consume(struct cadena* c);

void deleteQueue(struct queue* q){
    int i;
    for(i=0; i<q->tambuff1; i++){
        if(q->buff1[i]!=NULL){
            free(q->buff1[i]);
        }
    }
    free(q);
}

void consume(struct cadena* c){
    const char *PATTERN = "AAAAA";  //mientras este pattern hay DNA

    char *target;
    char *start, *end;

    if(strpbrk(c->linea, "BDEFHIJKLMNOPQRSUVWXYZ")!=NULL){ //si hay una de esas letras
    //STRPBRK: USO: Finds the first character in the string str1 that matches any character specified in str2.
    //This function returns a pointer to the character in str1 that matches one of the characters in str2,
    // or NULL if no such character is found.
        strcpy(c->linea, "ALIEN DNA");  //imprime alien dna
                printf("%s\n",c->linea);
    } else {
        if ( (start = strstr( c->linea, PATTERN )) ) //si encontramos el primero
            {
                start += strlen( PATTERN );         //movemos strlen(PATTERN) sitios desde el primero encontrado posiciones
                if ( (end = strstr( start, PATTERN )) )   //si encontramos el segundo
                {
                    target = ( char * )malloc( end - start + 1 );
                    memcpy( target, start, end - start );
                    target[end - start] = '\0';
                    strcpy(c->linea, target);
                    free(target);
                    xprintf(c->linea);
                    printf("\n");
                }else{
                    strcpy(c->linea, "NO PRIMERS");
                    printf("%s\n",c->linea);
                }
            }else{
                    strcpy(c->linea, "NO PRIMERS");
                    printf("%s\n",c->linea);
            }
    }
}

void createQueue(struct queue* q,int tambuff){
    int i;
    q->poner=0;
    q->sacar=0;
    q->tambuff1=tambuff;
    q->buff1=malloc(tambuff*sizeof(struct cadena*));
    for(i=0; i<tambuff; i++){
        q->buff1[i]=NULL;
    }

}
void push(struct cadena* c, struct queue* q){
    assert(!isFull(q));
    q->buff1[q->poner]=c;
    q->poner=((q->poner+1)%q->tambuff1);
}

struct cadena* pop(struct queue* q){
    assert(!isEmpty(q));
    struct cadena* c= q->buff1[q->sacar];
    q->buff1[q->sacar]=NULL;
    q->sacar=((q->sacar+1)%q->tambuff1);
    return c;
}

bool isFull(struct queue* q){
    if(q->poner==q->sacar && (q->buff1[q->poner])!=NULL) return true;
    else return false;
}

bool isEmpty(struct queue* q){
    if(q->poner==q->sacar && (q->buff1[q->poner])==NULL ) return true;
    else return false;
}
void printQueue(struct queue* q){
    int i;
    for(i=0;i<q->tambuff1;i++){
        if(q->buff1[i]==NULL){
            printf("queue[%d] = null\n",i);
        }else{
        printf("queue[%d] = linea : %d contenido : %s\n",i,q->buff1[i]->valor,q->buff1[i]->linea);
        }
    }
}


void productor(){
    assert(q!=NULL);

    FILE * f;

    f = fopen(nombrein, "r");

    //COMPRUEBA QUE EL FICHERO SE HABRE CORRECTAMENTE
   if(f==NULL){
      perror( "Errorfopeninput: ");
      exit(EXIT_FAILURE);
    }

    int i=0;
    //SI HAY LINEA EN EL FICHERO...
    while(!feof(f)){

        struct cadena *c=(struct cadena*)malloc(sizeof(struct cadena));
        //fgets sirve para copiar lineas de un fichero
        //USO: fgets (variable, length, file)
        fgets(c->linea, 100, f);    //lee la linea del fichero input
        c->valor=i;                 //lee el numero de la linea del fichero de la cadena
        sem_wait(&empty);       //espera al espacio en buffer1 para poner cadenas
        push(c, q);             //pone cadena en el buffer1
        sem_post(&full);        //aumenta el numero de los elementos en el buffer1
        ++i;                    //vamos a la linea proxima
// comprobar if size of whats remaining ,100
    }

   // threadProdRunning=0;

    if(fclose(f)!=0){
        perror("ERRORfcloseinput: ");
        exit(EXIT_FAILURE);
    }
}

void consumidor(){
    assert(q!=NULL && q2!=NULL);
    while(!(threadProdRunning==false)){
        struct cadena* c;
        sem_wait(&full);    //esperamos hasta que haya algo para sacar del buffer1
        sem_wait(&mutexsacarq);
        c=pop(q);           //sacamos c del q
        sem_post(&mutexsacarq);
        sem_post(&empty);   //incrementamos el numero de las celdas libres en el buffer1
        consume(c);
        sem_wait(&emptyq2); //espera al espacio en buffer2 para poner cadenas
        sem_wait(&mutexponerq2);
        push(c,q2);         //pone la cadena
        sem_post(&mutexponerq2);
        sem_post(&fullq2);  //incrementa el numero de los elementos en el buffer2
    }
    while(!isEmpty(q)){    //cuando se termina el hilo Productor quitamos todo que era en la q
        struct cadena* c;
        sem_wait(&full);    //esperamos hasta que haya algo para sacar del buffer1
        sem_wait(&mutexsacarq);
        c=pop(q);           //sacamos c del q
        sem_post(&mutexsacarq);
        sem_post(&empty);   //incrementamos el numero de las celdas libres en el buffer1
        consume(c);
        sem_wait(&emptyq2); //espera al espacio en buffer2 para poner cadenas
        sem_wait(&mutexponerq2);
        push(c,q2);         //pone la cadena
        sem_post(&mutexponerq2);
        sem_post(&fullq2);  //incrementa el numero de los elementos en el buffer2
    }

   // threadConsRunning=0;
}

void consumidorFinal(){
    assert(q2!=NULL && q!=NULL);
    struct cadena* c;

    FILE * f;

    f = fopen(nombreout, "w");

        //COMPRUEBA QUE EL FICHERO SE HABRE CORRECTAMENTE
    if(f==NULL){
        perror( "Errorfopenoutput: ");
        exit(EXIT_FAILURE);
    }

    while( !(threadConsRunning==false)){
        sem_wait(&fullq2);      //esperamos hasta que haya algo para sacar del buffer2
        c=pop(q2);              //saca la cadena del buffer2
        sem_post(&emptyq2);     //incrementa el numero de los elementos en el buffer2
        fprintf(f, "%s", c->linea);   //lo imprime en el output file
        fprintf(f, "\n");
        free(c);                //desalocar cadena
        }

    while(!isEmpty(q2)){        //cuando se termina hilo Consumidor quitamos lo que queda en el q2
        sem_wait(&fullq2);      //esperamos hasta que haya algo para sacar del buffer2
        c=pop(q2);              //saca la cadena del buffer2
        sem_post(&emptyq2);     //incrementa el numero de los elementos en el buffer2
        fprintf(f, "%s", c->linea);   //lo imprime en el output file
        fprintf(f, "\n");
        free(c);                //desalocar cadena
    }

    if(fclose(f)!=0){
        perror("ERRORfcloseoutput: ");
        exit(EXIT_FAILURE);
    }

}

int main(int argc, char *argv[]) {
        //gestion de la entrada
            if(argc != 5){
                printf("[+] Usage : ./<program> <inputFile> <outputFile> <tamBuffer> <numCleaners>\n");
                exit(EXIT_FAILURE);
            }
            FILE* f;
            if((f=fopen(argv[1],"r"))==NULL){
                perror(argv[1]);
                fclose(f);
                exit(EXIT_FAILURE);
            }else{
                fclose(f);
                strncpy(nombrein, argv[1], sizeof(nombrein));
            }
            if((f=fopen(argv[2],"w"))==NULL){
                perror(argv[2]);
                fclose(f);
                exit(EXIT_FAILURE);
            }else{
                fclose(f);
                strncpy(nombreout, argv[2], sizeof(nombreout));
            }
            if((tambuff = atoi(argv[3]))<1){
                printf("[+] tambuff debe ser mayor que 0\n");
            }
            if((numCleaners = atoi(argv[4]))<1){
                printf("[+] numCleaners debe ser mayor que 0\n");
            }


    //declaracion de hilos
      pthread_t Productor, ConsumidorFinal;

      pthread_t* Consumidor;
      Consumidor=malloc(numCleaners*sizeof(pthread_t));

    //inicializacion de semaforos
      sem_init(&empty, 0, tambuff); //semaforos del buffer1
      sem_init(&full, 0, 0);    //semaforos del buffer1
      sem_init(&emptyq2, 0, 5); //semaforos del buffer2
      sem_init(&fullq2, 0, 0);  //semaforos del buffer2

      sem_init(&mutexponerq2, 0, 1); //semaforos del buffer2
      sem_init(&mutexsacarq, 0, 1);  //semaforos del buffer2

    //allocar memoria para la cola
    q = malloc(sizeof(struct queue));

    //crear la cola
    createQueue(q, tambuff);

    //allocar memoria para la cola
    q2 = malloc(sizeof(struct queue));  //en cola 2 tenemos buffer2 del huion de la practica

    //crear la cola 2
    createQueue(q2, 5);


    //creacion de los hilos
    if (pthread_create(&ConsumidorFinal, NULL, (void*)consumidorFinal, (void *)NULL))
    {
        perror("pthread_create 3");
        exit(EXIT_FAILURE);
    }

    int i=0;

    for(i=0; i<numCleaners; i++){
        if (pthread_create(&Consumidor[i], NULL, (void*)consumidor, (void *)NULL))
        {
            perror("pthread_create 2");
            exit(EXIT_FAILURE);
        }
    }

    if (pthread_create(&Productor, NULL, (void*)productor, (void *)NULL))
    {
        perror("pthread_create 1");
        exit(EXIT_FAILURE);
    }



    //suspend execution of the calling thread until the target thread terminates,
    //unless the target thread has already terminated.

    if(pthread_join(Productor, NULL)==0) threadProdRunning=false;
    else //threadProdRunning nos dice cuando se acaba el hilo Productor
    {
        perror("ERROR pthread_join Productor: "); //si hay error, pthread_join devuelve otro valor que 0
        exit(EXIT_FAILURE);
    }

    for(i=0; i<numCleaners; i++){
        if(pthread_join(Consumidor[i], NULL)!=0)
        {
            perror("ERROR pthread_join Consumidor: ");  //si hay error pthread_join devuelve otro valor que 0
            exit(EXIT_FAILURE);
        }
    }
    threadConsRunning=false;  //threadConsRunning nos dice cuando se aca el hilo Consumidor


    if(pthread_join(ConsumidorFinal, NULL)!=0)
    {
        perror("ERROR pthread_join Consumidor: "); //si hay error pthread_join devuelve otro valor que 0
        exit(EXIT_FAILURE);
    }

    //destruccion de las colas
    deleteQueue(q);
    deleteQueue(q2);

    //destruccion de los semaforos
    sem_destroy(&emptyq2);
    sem_destroy(&fullq2);
    sem_destroy(&empty);
    sem_destroy(&full);
    sem_destroy(&mutexponerq2);
    sem_destroy(&mutexsacarq);

    free(Consumidor);

    return 0;
}
