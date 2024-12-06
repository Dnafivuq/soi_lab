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
  char c;
  int i;
} item_t;

typedef struct{
  int max_size;
  int current_size;
  item_t* content;
} fifoqueue_t;

typedef struct{
  sem_t mutex;
  sem_t empty;
  sem_t full;
  char name;
  fifoqueue_t queue;
} buffer_t;

typedef struct{
  item_t* produced_item;
  int n_buffers;
  buffer_t** wt_buffers;
  char name;
} producer_t;

typedef struct{
  item_t* received_item;
  int n_buffers;
  buffer_t** rf_buffers;
  char name;
} consumer_t;
//<======================================================================>



//<======================================================================>
//               Highly inefficient fifo queue implementation
//<======================================================================>

item_t pop(fifoqueue_t* queue) {
  if (queue->current_size < 1)
    __THROW;

  item_t element = queue->content[0];
  queue->current_size -= 1;
  //shift content to "front" of the queue
  if (queue->current_size > 0)
    memcpy(queue->content, queue->content + 1, sizeof(item_t)*queue->current_size);
  return element;
}

void insert(fifoqueue_t* queue, item_t value){
  if (queue->current_size == queue->max_size)
    __THROW;
  queue->content[queue->current_size] = value;
  queue->current_size+=1;
}
//<======================================================================>



//<======================================================================>
// Semaphores up and down implementation (wrapper on sys implementations)
//<======================================================================>
void down(sem_t* sem){
  if (sem_wait(sem) !=0)
    __THROW;
}

void up(sem_t* sem){
  if (sem_post(sem) !=0)
    __THROW;
}
//<======================================================================>



//<======================================================================>
// implementation of helper function (called in producer and consumer)
//<======================================================================>

void produce_item(producer_t* producer){
  if (producer->produced_item != NULL){
    __THROW;
  }
  producer->produced_item->c=producer->name;
  producer->produced_item->i=rand();
  printf("\t- producer: %c, produced item %d\n", producer->produced_item->c, producer->produced_item->i);
}

void enter_item(buffer_t* buffer, item_t* producer_item){
  if (producer_item == NULL){
    __THROW;
  }
  insert(&(buffer->queue), *producer_item);
  producer_item = NULL;
  item_t* buffer_item = &(buffer->queue.content[buffer->queue.current_size-1]);
  printf("=> buffer: %c just got item %d from producer: %c\n", buffer->name, buffer_item->i, buffer_item->c);
}

void consume_item(consumer_t* consumer){
  if (consumer->received_item == NULL){
    __THROW;
  }
  printf("\t- consumer: %c, consumed item %d from producer %c\n", consumer->name, consumer->received_item->i, consumer->received_item->c);
  consumer->received_item = NULL;
}

item_t remove_item(buffer_t* buffer, char consumer_name){
  item_t poped_item = pop(&(buffer->queue));
  printf("  => buffer: %c just got item: %d from producer %c removed by consumer: %c\n", buffer->name, poped_item.i, poped_item.c , consumer_name);
  return poped_item;
}
//<======================================================================>



//<======================================================================>
// constructors for buffer, consumer, producer
//<======================================================================>

//allocates memory for itself and its inner fifoqueue using shmalloc to avoid further copying from local to shared memory 
buffer_t* buffer_constr(int buffer_size, int share, char name){
  buffer_t* instance = shmalloc(sizeof(buffer_t));
  item_t* queue_content = shmalloc(sizeof(item_t)*buffer_size);

  sem_init(&(instance->mutex), share, 1);
  sem_init(&(instance->empty), share, buffer_size);
  sem_init(&(instance->full), share, 0);
  instance->name = name;
  instance->queue.current_size = 0;
  instance->queue.max_size = buffer_size;
  instance->queue.content = queue_content;
  return instance;
}

producer_t* producer_const(char name, int n_buffers, buffer_t* wt_buffers[]){
  producer_t* instance = malloc(sizeof(producer_t));
  item_t* item = malloc(sizeof(item_t));

  instance->n_buffers = n_buffers;
  instance->name = name;
  instance->wt_buffers = wt_buffers;
  instance->produced_item = item;
  return instance; 
}

consumer_t* consumer_const(char name, int n_buffers, buffer_t* rf_buffers[]){
  consumer_t* instance = malloc(sizeof(consumer_t));
  item_t* item = malloc(sizeof(item_t));

  instance->n_buffers = n_buffers;
  instance->name = name;
  instance->rf_buffers = rf_buffers;
  instance->received_item = item;

  return instance; 
}
// consumer and producer can use standard malloc since they are located in each seperate process instance
// wheras buffers are by nature shared across processes
//<======================================================================>

//<======================================================================>
// implementation of producer run and consumer run -
// - main methods of child processes
//<======================================================================>

void producer_run(producer_t* producer){
  while(1)
    for (int i = 0; i < producer->n_buffers; i++){
      produce_item(producer);
      down(&(producer->wt_buffers[i]->empty));
      down(&(producer->wt_buffers[i]->mutex));
      enter_item(producer->wt_buffers[i], producer->produced_item);
      up(&(producer->wt_buffers[i]->mutex));
      up(&(producer->wt_buffers[i]->full));

      //kill child process after main exited
      if(prctl(PR_SET_PDEATHSIG, SIGHUP) == -1)
        exit(0);
      
      //sleep to avoid messaging spam
      // sleep(1);
    }
}

void consumer_run(consumer_t* consumer){
  while(1)
    for(int i = 0; i < consumer->n_buffers; i++){
      down(&(consumer->rf_buffers[i]->full));
      down(&(consumer->rf_buffers[i]->mutex));
      *(consumer->received_item) = remove_item(consumer->rf_buffers[i], consumer->name);
      up(&(consumer->rf_buffers[i]->mutex));
      up(&(consumer->rf_buffers[i]->empty));
      //consume item commented to lower amount of messages spam
      //consume_item(consumer);

      //kill child process after main exited
      if(prctl(PR_SET_PDEATHSIG, SIGHUP) == -1)
        exit(0);

      // sleep to avoid messaging spam
      // sleep(1);
    }
}
//<======================================================================>

int main() {
  // seed rand
  // srand(time(NULL));

  // define child processes for each consumer/producer
  pid_t p1, p2, p3;
  pid_t c1, c2, c3, c4;

  // define buffers
  buffer_t* buffer1 = buffer_constr(5, 1, '1');
  buffer_t* buffer2 = buffer_constr(1, 1, '2');
  buffer_t* buffer3 = buffer_constr(2, 1, '3');
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