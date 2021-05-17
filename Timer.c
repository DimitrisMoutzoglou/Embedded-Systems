#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <time.h>
#define NumPro 1
#define NumCon 3
#define QUEUESIZE 10
#define PI 3.1415926535
void *producer (void *args);
void *consumer (void *args);
void *divider(void *p);
void *siner(void *p);
void *printer(void *p);
typedef void * (*work)(void *);
typedef struct workFunction {
void * (*work)(void *);
void * arg;
struct timeval tv;
}workFunction;
work functions[3] = {divider,siner,printer};
typedef struct {
long head, tail;
int full, empty;
workFunction buff[QUEUESIZE];
pthread_mutex_t *mut;
pthread_cond_t *notFull, *notEmpty;
} queue;
queue *queueInit (void);
void queueDelete (queue *q);
void queueAdd (queue *q, struct workFunction in);
void queueDel (queue *q, struct workFunction *out);
typedef struct {
pthread_t tid;
int TimerPeriod;
unsigned int TasksToExecute;
unsigned int StartDelay;
work TimerFcn;
int *UserData;
queue *thequeue;
int *calc1,*calc2;
} timer;
timer *TimeInit (int Period, unsigned int Tasks, unsigned int Delay, void * (*work)(void *), queue *q, pthread_t id);
void start(timer *t1);
void StartFcn (long p);
int ErrorFcn (long p);
void StopFcn (long p);
void dpfiler (timer *t);
void cfiler(unsigned int totaltasks);
void *function();
int clear1=0;
int ErrCount = 0;
double *claps;
int k=0;
int main(){
pthread_t proThreads[NumPro];
pthread_t conThreads[NumCon];
queue *fifo;
fifo = queueInit ();
if (fifo == NULL) {
fprintf (stderr, "main: Queue Init failed.\n");
exit (1);
}
timer *t1,*t2,*t3;
t1 = TimeInit(10,360000,1,divider,fifo,proThreads[0]);
t2 = TimeInit(100,36000,1,siner,fifo,proThreads[1]);
t3 = TimeInit(1000,3600,1,printer,fifo,proThreads[2]);
claps = (double *) malloc(sizeof(double)*(t1->TasksToExecute+ t2->TasksToExecute+ t3->TasksToExecute));
long c;
for(c=0; c<NumCon;c++){
printf("In main: creating consumer thread %ld\n",c);
pthread_create (&conThreads[c], NULL, consumer, fifo);
}
start(t1);
start(t2);
start(t3);
pthread_join(t1->tid,NULL);
pthread_join(t2->tid,NULL);
pthread_join(t3->tid,NULL);
printf("Done joining the pro threads\n");
dpfiler(t1); // kanonika katw apo join twn con threads
dpfiler(t2);
dpfiler(t3);
cfiler(t1->TasksToExecute+ t2->TasksToExecute+ t3->TasksToExecute); //+ t2->TasksToExecute
for (c=0;c<NumCon;c++){
pthread_join(&conThreads[c],NULL);
}
queueDelete (fifo);
free(t1);
free(t2);
free(t3);
free(claps);
printf("Total Number of Errors %d\n",ErrCount);
return 0;
}
void *producer (void *q)
{
timer *data = (timer *)q;
queue *fifo;
workFunction job;
struct timeval tv,send;
fifo = data->thequeue;
job.work=data->TimerFcn;
StartFcn (data->tid);
long double sleeptime = data->TimerPeriod*1000;
long double last=0;
long double drift=0;
long double sent=0;
usleep(data->StartDelay*1000*1000);
printf("slept my assigned delay time.\n");
int i;
for (i = 0; i < data->TasksToExecute; i++){
gettimeofday(&send,NULL); // keeping tabs on time just before we try to store a function pointer
pthread_mutex_lock (fifo->mut);
while (fifo->full) {
ErrorFcn(data->tid);
pthread_cond_wait (fifo->notFull, fifo->mut);
}
queueAdd (fifo, job);
gettimeofday(&tv,NULL);// keeping tabs on time right after we stored a function pointer
sent = tv.tv_sec *1000000 + tv.tv_usec;
pthread_mutex_unlock (fifo->mut);
pthread_cond_signal (fifo->notEmpty);
data->calc2[i] = sent - (send.tv_sec*1000000 + send.tv_usec);
if (i > 0){
drift = (tv.tv_sec*1000000+tv.tv_usec -last) - (data->TimerPeriod*1000);
sleeptime = sleeptime - drift;
printf("Drift = %Lf.\n",drift);
}
last = sent;
data->calc1[i] = drift;
if (sleeptime < 0){
sleeptime = 0; // enallaktika data_>TimerPeriod
}
usleep(sleeptime);
}
clear1=1;
return (NULL);
}
void *consumer (void *q)
{
queue *fifo;
fifo = (queue *)q;
long tid = pthread_self();
struct timeval feed,beg;
long elapsed;
workFunction job;
while(1) {
pthread_mutex_lock (fifo->mut);
while (fifo->empty) {
//printf ("consumer: queue EMPTY.\n"); //testing
pthread_cond_wait (fifo->notEmpty, fifo->mut);
}
job = fifo->buff[fifo->head];
gettimeofday(&beg, NULL);//keeping tabs on time just before execution
(job.arg) = (rand()%1000);
job.work(job.arg);
feed=job.tv;
queueDel (fifo, &job);
pthread_mutex_unlock (fifo->mut);
pthread_cond_signal (fifo->notFull);
if (clear1==1){
StopFcn(tid); // afotou ektelesei thn teleutaia
}
elapsed = (beg.tv_sec - feed.tv_sec)*1000000 + beg.tv_usec - feed.tv_usec;
*(claps+k)=elapsed;
k++;
}
return (NULL);
}
queue *queueInit (void)
{
queue *q;
q = (queue *)malloc (sizeof (queue));
if (q == NULL) return (NULL);
q->empty = 1;
q->full = 0;
q->head = 0;
q->tail = 0;
q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
pthread_mutex_init (q->mut, NULL);
q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
pthread_cond_init (q->notFull, NULL);
q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
pthread_cond_init (q->notEmpty, NULL);
return (q);
}
void queueDelete (queue *q)
{
pthread_mutex_destroy (q->mut);
free (q->mut);
pthread_cond_destroy (q->notFull);
free (q->notFull);
pthread_cond_destroy (q->notEmpty);
free (q->notEmpty);
free (q);
}
void queueAdd (queue *q, struct workFunction in)
{
gettimeofday(&in.tv,NULL);//keeping tabs on time just before storing our function pointer in fifo
q->buff[q->tail] = in;
q->tail++;
if (q->tail == QUEUESIZE)
q->tail = 0;
if (q->tail == q->head)
q->full = 1;
q->empty = 0;
return;
}
void queueDel (queue *q, struct workFunction *out)
{
*out = q->buff[q->head];
q->head++;
if (q->head == QUEUESIZE)
q->head = 0;
if (q->head == q->tail)
q->empty = 1;
q->full = 0;
return;
}
timer *TimeInit (int Period, unsigned int Tasks, unsigned int Delay, void * (*work)(void *), queue *q, pthread_t id)
{
timer *t;
t = (timer *)malloc (sizeof (timer));
if (t == NULL) return (NULL);
t->TimerPeriod = Period;
t->TasksToExecute = Tasks;
t->StartDelay = Delay;
t->TimerFcn = work;
t->thequeue=(queue *)q;
t->tid = id;
int *c1 = (int *) malloc(sizeof(int)*Tasks);
int *c2 = (int *) malloc(sizeof(int)*Tasks);
int i = 0;
for (i=0; i<Tasks;i++){
c1[i]=0;
c2[i]=0;
}
t->calc1 = c1;
t->calc2 = c2;
return (t);
}
void start (timer *t1){
pthread_create(&(t1->tid),NULL,producer,t1);
}
void StartFcn (long p){
printf ("producer %ld: Work,Work,Work just like Rihanna.\n",p);
}
int ErrorFcn (long p){
printf ("producer %ld: queue FULL.\n",p);
ErrCount++;
return 0;
}
void StopFcn (long p){
printf("producer %ld: My job here is done.\n",p);
clear1=0;
}
void startat(timer *t1,int y,int m,int d,int h,int min,int sec){
time_t futt;
struct tm fut;
fut.tm_year = y - 1900;
fut.tm_mon = m - 1;
fut.tm_mday = d;
fut.tm_hour = h;
fut.tm_min = min;
fut.tm_sec = sec;
fut.tm_isdst = -1;
futt = mktime(&fut);
time_t now;
time(&now);
printf("Difference is %.2f seconds",difftime(futt,now));
sleep(difftime(futt,now)-120); //sleeps for the bulk of time in seconds apart from a whole minute to compensate for sleep function inaccuracy and another for our time delay
time(&now);
sleep(difftime(futt,now)-60); //sleeps for the rest of time in seconds except for a minute that corresponds to our delay
pthread_create(&(t1->tid),NULL,producer,t1);
}
void *divider(void *p){
if ((long int)p % 3 == 0){
printf("The long integer %ld is a multiple of 3.\n",(long int)p);
}
else {
printf ("The long integer %ld is not a multiple of 3.\n",(long int)p);
}
}
void *siner(void *p){
double ret, val;
long int x = ( long int) p;
val = PI/180;
ret = sin(val* (double) x);
printf("The sine value of %ld is %lf degrees\n",x,ret);
}
void *printer(void *p){
printf("Printer called and printer printed a random integer %ld.\n",(long int)p);
}
void dpfiler (timer *t){
char namea[20];
printf("Enter the name of the first file to be created:\n");
fgets(namea,30,stdin);
FILE *fptra = fopen(namea,"a");
char nameb[20];
printf("Enter the name of the second file to be created:\n");
fgets(nameb,30,stdin);
FILE *fptrb = fopen(nameb,"a");
int j;
for (j=0;j<t->TasksToExecute;j++){
fprintf(fptra,"%d\n",t->calc1[j]);
fprintf(fptrb,"%d\n",t->calc2[j]);
}
fclose(fptra);
fclose(fptrb);
printf("Couple of drift and producer files created\n");
free(t->calc1);
free(t->calc2);
}
void cfiler(unsigned int totaltasks){
char name[20];
printf("Enter the name of the consumer file to be created:\n");
fgets(name,30,stdin);
FILE *fptr = fopen(name,"a");
int j;
for (j=0;j<totaltasks;j++){
fprintf(fptr,"%lf\n",claps[j]);
}
fclose(fptr);
printf("Total consumer file created\n");
}
