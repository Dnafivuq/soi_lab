#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/prctl.h>



//<======================================================================>
//      shared memeory allocation implementation
//<======================================================================>
void* shmalloc(size_t size) {
  int protection = PROT_READ | PROT_WRITE;
  int visibility = MAP_SHARED | MAP_ANONYMOUS;
  return mmap(NULL, size, protection, visibility, -1, 0);
}
//<======================================================================>



//<======================================================================>
//      definition of fifoqueue, buffer, consumer and producer
//<======================================================================>

typedef struct{
  int max_size;
  int current_size;
  char* content;
} fifoqueue_t;

typedef struct{
  sem_t mutex;
  sem_t empty;
  sem_t full;
  char name;
  fifoqueue_t fifoqueue;
} buffer_t;

typedef struct{
  char name;
  int n_buffers;
  buffer_t** wt_buffers;
} producer_t;

typedef struct{
  char name;
  int n_buffers;
  buffer_t** rf_buffers;
} consumer_t;
//<======================================================================>



//<======================================================================>
//               Highly inefficient fifo queue implementation
//<======================================================================>

char pop(fifoqueue_t* queue) {
  if (queue->current_size < 1)
    __THROW "IndexError, pop on empty queue";

  char element = queue->content[0];
  queue->current_size -= 1;
  //shift content to "front" of the queue
  if (queue->current_size > 0)
    memcpy(queue->content, queue->content + 1, sizeof(char)*queue->current_size);
  return element;
}

void insert(fifoqueue_t* queue, char value){
  if (queue->current_size == queue->max_size)
    __THROW "IndexError, insert on full queue";
  queue->content[queue->current_size] = value;
  queue->current_size+=1;
}
//<======================================================================>



//<======================================================================>
// Semaphores up and down implementation (wrapper on sys implementations)
//<======================================================================>
void down(sem_t* sem){
  if (sem_wait(sem) !=0)
    __THROW "sem_wait: failed";
}

void up(sem_t* sem){
  if (sem_post(sem) !=0)
    __THROW "sem_post: failed";
}
//<======================================================================>



//<======================================================================>
// implementation of helper function (called in producer and consumer)
//<======================================================================>

void produce_item(producer_t* producer){
  printf("\t-producer: %c, produced item\n", producer->name);
}

void enter_item(buffer_t* buffer, char producer_name){
  insert(&(buffer->fifoqueue), producer_name);
  printf("=>buffer: %c just got item from producer: %c\n", buffer->name, producer_name);
}

void consume_item(consumer_t* consumer){
  printf("\t-consumer: %c, consumed item\n", consumer->name);
}

void remove_item(buffer_t* buffer, char consumer_name){
  printf("=>buffer: %c just got item: %c removed by consumer: %c\n", buffer->name, pop(&(buffer->fifoqueue)) ,consumer_name);
}
//<======================================================================>



//<======================================================================>
// constructors for buffer, consumer, producer
//<======================================================================>

//allocates memory for itself and its inner fifoqueue using shmalloc to avoid further copying from local to shared memory 
buffer_t* buffer_constr(int buffer_size, int share, char name){
  buffer_t* instance = shmalloc(sizeof(buffer_t));
  char* queue_content = shmalloc(sizeof(char)*buffer_size);

  sem_init(&(instance->mutex), share, 1);
  sem_init(&(instance->empty), share, buffer_size);
  sem_init(&(instance->full), share, 0);
  instance->name = name;
  instance->fifoqueue.current_size = 0;
  instance->fifoqueue.max_size = buffer_size;
  instance->fifoqueue.content = queue_content;
  return instance;
}

producer_t* producer_const(char name, int n_buffers, buffer_t* wt_buffers[]){
  producer_t* instance = malloc(sizeof(producer_t));
  //allocate memory for array of pointers that points to buffers
  buffer_t** buffers = malloc(sizeof(buffer_t*)*n_buffers);
  buffers = wt_buffers;

  instance->n_buffers = n_buffers;
  instance->name = name;
  instance->wt_buffers = buffers;

  return instance; 
}

consumer_t* consumer_const(char name, int n_buffers, buffer_t* rf_buffers[]){
  consumer_t* instance = malloc(sizeof(consumer_t));
  buffer_t** buffers = malloc(sizeof(buffer_t*)*n_buffers);
  buffers = rf_buffers;

  instance->n_buffers = n_buffers;
  instance->name = name;
  instance->rf_buffers = buffers;

  return instance; 
}
// consumer and producer can use standard malloc since they are located in each seperate process instance
// wheras buffers are by nature shared across processes
//<======================================================================>

//<======================================================================>
// implementation of producer run and consumer run
// main methods of child processes
//<======================================================================>

void producer_run(producer_t* producer){
  while(1)
    for (int i = 0; i < producer->n_buffers; i++){
      produce_item(producer);
      down(&(producer->wt_buffers[i]->empty));
      down(&(producer->wt_buffers[i]->mutex));
      enter_item(producer->wt_buffers[i], producer->name);
      up(&(producer->wt_buffers[i]->mutex));
      up(&(producer->wt_buffers[i]->full));

      //kill child process after main exited
      if(prctl(PR_SET_PDEATHSIG, SIGHUP) == -1)
        exit(0);
      
      //sleep to avoid messaging spam
      sleep(1);
    }
}

void consumer_run(consumer_t* consumer){
  while(1)
    for(int i = 0; i < consumer->n_buffers; i++){
      down(&(consumer->rf_buffers[i]->full));
      down(&(consumer->rf_buffers[i]->mutex));
      remove_item(consumer->rf_buffers[i], consumer->name);
      up(&(consumer->rf_buffers[i]->mutex));
      up(&(consumer->rf_buffers[i]->empty));
      //consume item commented to lower amount of messages spam
      //consume_item(consumer);

      //kill child process after main exited
      if(prctl(PR_SET_PDEATHSIG, SIGHUP) == -1)
        exit(0);

      // sleep to avoid messaging spam
      sleep(1);
    }
}
//<======================================================================>

int main() {
  // define child processes for each consumer/producer
  pid_t p1, p2, p3;
  pid_t c1, c2, c3, c4;

  // define buffers
  buffer_t* buffer1 = buffer_constr(1, 1, '1');
  buffer_t* buffer2 = buffer_constr(2, 1, '2');
  buffer_t* buffer3 = buffer_constr(3, 1, '3');
  buffer_t* buffer4 = buffer_constr(4, 1, '4');
  
  buffer_t* buffers[] = {buffer1, buffer2, buffer3, buffer4};

  
  // allow different code execution based on process
  if((p1 = fork()) == 0){
    producer_t* pA = producer_const('A', 1, &buffers[0]);
    producer_run(pA);
  }
  if((p2 = fork()) == 0){
    producer_t* pB = producer_const('B', 4, buffers);
    producer_run(pB);
  }
  if((p3 = fork()) == 0){
    producer_t* pC = producer_const('C', 1, &buffers[3]);
    producer_run(pC);
  }
  if((c1 = fork()) == 0){
    consumer_t* cA = consumer_const('A', 1, &buffers[0]);
    consumer_run(cA);
  }
  if((c2 = fork()) == 0){
    consumer_t* cB = consumer_const('B', 1, &buffers[1]);
    consumer_run(cB);
  }
  if((c3 = fork()) == 0){
    consumer_t* cC = consumer_const('C', 1, &buffers[2]);
    consumer_run(cC);
  }
  if((c4 = fork()) == 0){
    consumer_t* cD = consumer_const('D', 1, &buffers[3]);
    consumer_run(cD);
  }

  while(1){
    // keep main process alive
  }
}