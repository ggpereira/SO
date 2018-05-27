#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define BILLION 1E9

/*
  Classifying numbers with threads
  Gabriel Gomes Pereira
*/

/*Structure to save results*/
typedef struct
{
    int idThread;
    int d;
    int a;
    int p;
    int work;
}class_res;

/*Structure to save arguments of threads*/
typedef struct
{
  int idThread;
  int chunkSize;
  int start;
  int end;
  int inc;
}thread_args;

//Returns the time execution in seconds
double execTime(struct timespec start, struct timespec stop)
{
  return (stop.tv_sec - start.tv_sec) + (stop.tv_nsec - start.tv_nsec)/BILLION;
}

/*
    Classify based the numbers interval passed in parameters

    Inc = Increment parameter, the classNum function is recycled in all classifications

    Sparse classification explanation:
    ex: 4 Threads -> Inc = 4   t_worksize = 16

    t1 n1 = 1              t2 n1 = 2              t3 n1 = 3              t4 n1 = 4
    t1 n2 = 1 + 4 = 5      t2 n2 = 2 + 4 = 6      t3 n2 = 4 + 3 = 7      t4 n2 = 4 + 4 = 8
    t1 n3 = 5 + 4 = 9      t2 n3 = 6 + 4 = 10     t3 n3 = 7 + 4 = 11     t4 n3 = 8 + 4 = 12
    t1 n4 = 9 + 4 = 13     t2 n4 = 10 + 4 = 14    t3 n4 = 11 + 4 = 15    t4 n4 = 12 + 4 = 16

    return reu struct class_res
*/
class_res *classNum(int ini, int end, int inc)
{
  class_res *res = malloc(sizeof(class_res));
  int sumDiv;
  res->d = 0;
  res->a = 0;
  res->p = 0;
  res->work = 0;
  for(int k = ini; k <= end; k+=inc)
  {
      res->work += 1;
      sumDiv = 0;

      //Divide number
      for(int n = 1; n < k; n++)
      {
        if(!(k % n))
        {
          sumDiv += n;
        }
      }
      //Classify number
      //Defective
      if(sumDiv < k)
      {
        res->d++;
      }
      //Excessive
      else if(sumDiv > k)
      {
        res->a++;
      }
      //Perfect
      else if(sumDiv == k)
      {
        res->p++;
      }
  }
  return res;
}

//No threads
double classNumSeq(int t_worksize)
{
    class_res *res;
    struct timespec start, stop;
    double t;

    if(clock_gettime(CLOCK_REALTIME, &start) == -1)
    {
      printf("Error in clock_gettime!\n");
      exit(-1);
    }

    res = classNum(1, t_worksize, 1);

    if(clock_gettime(CLOCK_REALTIME, &stop) == - 1)
    {
      printf("Error in clock_gettime!\n");
      exit(-1);
    }

    printf("--------------------------------------------------------------------\n");
    printf("\t\t[D]\t[A]\t[P]\t\t[Wtot]\n");
    printf("* Sequential:\t%d\t%d\t%d\t\t%d\n", res->d, res->a, res->p, t_worksize);

    printf("--------------------------------------------------------------------\n");

    free(res);

    return execTime(start, stop);
}

void *classThread(void *arg)
{
  thread_args *args = (thread_args *) arg;
  class_res *res;
  res = classNum(args->start, args->end, args->inc);
  res->idThread = args->idThread;
  free(args);
  pthread_exit(res);
}

//Return arguments
//Fill arguments
thread_args* fillArgThread(int k, int ini, int chunkSize, int end, int inc)
{
  thread_args *args = (thread_args *) malloc(sizeof(thread_args));
  if(args == NULL)
  {
    printf("Ocorreu um erro ao alocar memória!\n");
    exit(-1);
  }
  args->idThread = k;
  args->start = ini;
  args->chunkSize = chunkSize;
  args->end = end;
  args->inc = inc;
  return args;
}

//Classify with chunks
double classChunks(int t_worksize, int n_threads)
{
  pthread_t *tid = (pthread_t *) malloc(n_threads* sizeof(pthread_t));
  class_res *res;
  class_res total;
  thread_args *args;
  struct timespec start, stop;
  int nChunks = t_worksize / n_threads;
  int r = t_worksize % n_threads;
  int temp_ini = 1;
  int temp_final = 0;
  int k;

  if(clock_gettime(CLOCK_REALTIME, &start) == -1)
  {
    printf("Erro no clock_gettime!\n");
    exit(-1);
  }
  for(k = 0; k < n_threads; k++)
  {
      for(k = k; k < r; k++)
      {
          /*
              Charge distribution
              Case division is not exact
          */
          temp_final = temp_ini + (nChunks + 1) - 1;
          args = fillArgThread(k + 1, temp_ini, nChunks + 1, temp_final, 1);
          if(pthread_create(&tid[k], NULL, classThread, (void *)args))
          {
            printf("An error ocurred to creating thread!\n");
            exit(-1);
          }
          temp_ini += (nChunks + 1) ;
      }
      temp_final = temp_ini + nChunks - 1;
      args = fillArgThread(k + 1, temp_ini, nChunks, temp_final, 1);
      if(pthread_create(&tid[k], NULL, classThread, (void *)args))
      {
        printf("An error ocurred to creating thread!\n");
        exit(-1);
      }
      temp_ini +=  nChunks;
  }
  total.d = 0;
  total.a = 0;
  total.p = 0;
  total.work = 0;
  printf("(Chunk)\t\t[D]\t[A]\t[P]\t\t[Wth]\n");
  for(k = 0; k < n_threads; k++)
  {
    if(pthread_join(tid[k], (void*)&res))
    {
      printf("Error in join!\n");
      exit(-1);
    }
    printf("* Thread %d:\t%d\t%d\t%d\t\t%d\n", res->idThread, res->d,res->a, res->p, res->work);
    total.d += res->d;
    total.a += res->a;
    total.p += res->p;
    total.work += res->work;
    free(res);
  }
  if(clock_gettime(CLOCK_REALTIME, &stop) == - 1)
  {
    printf("Error in clock_gettime!\n");
    exit(-1);
  }
  printf("  [TOTAL]:  \t%d\t%d\t%d\t\t%d\n", total.d, total.a, total.p, total.work);
  printf("--------------------------------------------------------------------\n");

  free(tid);
  return execTime(start, stop);
}

double classSparse(int t_worksize, int n_threads)
{
  pthread_t *tid = (pthread_t *) malloc(n_threads* sizeof(pthread_t));
  class_res *res;
  class_res total;
  thread_args *args;
  struct timespec start, stop;
  int k;
  if(clock_gettime(CLOCK_REALTIME, &start) == -1)
  {
    printf("Error in clock_gettime!\n");
    exit(-1);
  }

  for(k = 0; k < n_threads; k++)
  {
    args = fillArgThread(k+1, k+1 , 0 , t_worksize, n_threads);
    if(pthread_create(&tid[k], NULL, classThread, (void *)args))
    {
      printf("An error ocurred to creating thread!\n");
      exit(-1);
    }
  }

  total.d = 0;
  total.a = 0;
  total.p = 0;
  total.work = 0;
  printf("(Sparse)\t[D]\t[A]\t[P]\t\t[Wth]\n");
  for(k = 0; k < n_threads; k++)
  {
    if(pthread_join(tid[k], (void*)&res))
    {
      printf("Error in join!\n");
      exit(-1);
    }
    printf("* Thread %d:\t%d\t%d\t%d\t\t%d\n", res->idThread, res->d,res->a, res->p, res->work);
    total.d += res->d;
    total.a += res->a;
    total.p += res->p;
    total.work += res->work;
  }
  if(clock_gettime(CLOCK_REALTIME, &stop) == - 1)
  {
    printf("Error in clock_gettime!\n");
    exit(-1);
  }
  printf("  [TOTAL]:  \t%d\t%d\t%d\t\t%d\n", total.d, total.a, total.p, total.work);
  printf("--------------------------------------------------------------------\n");

  free(res);
  free(tid);
  return execTime(start, stop);

}

int main(int argc, char *argv[])
{

  int n_threads;
  int total_worksize;
  double timeClassSeq;
  double timeClassChunks;
  double timeClassSparse;

  if(argc != 3)
  {
    printf("Numbers arguments is invalid!\n");
    exit(-1);
  }

  for(int i = 1; i <= 2; i++)
  {
    if(atoi(argv[i]) <= 0)
    {
      printf("Arguments invalid!\n");
      exit(-1);
    }
    if(i == 1)
    {
      n_threads = atoi(argv[i]);
    }
    else{
      total_worksize = atoi(argv[i]);
    }
  }

  if(n_threads > total_worksize){
    printf("Error: threads greather than worksize!\n");
    exit(-1);
  }

  timeClassSeq = classNumSeq(total_worksize);
  timeClassChunks = classChunks(total_worksize, n_threads);
  timeClassSparse = classSparse(total_worksize, n_threads);

  printf("Time search sequential:  %lf seg\n", timeClassSeq);
  printf("Time search with threads: %lf seg (chunk)\n", timeClassChunks);
  printf("Time search with threads: %lf seg (sparse)\n", timeClassSparse);
  printf("--------------------------------------------------------------------------\n");

  return 0;
}
