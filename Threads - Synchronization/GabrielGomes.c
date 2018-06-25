#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <math.h>

//Estados para o atendente
#define RESTING  0
#define ATTENDING  1
#define IDLE  2

//Estrutura para a fila
typedef struct {
    int max;
    int waiting;
    int ini;
    int end;
    int *queue;
} lines;

//Argumentos para as threads
typedef struct {
    int id_thread;
} thread_arg;

//Estrutura para controlar estados dos atendentes,
//Saber com qual cliente o atendente esta falando
//Saber se o atendente pode descansar
typedef struct {
    int st;
    int client_id;
    int n;
    int rest;
} attendant_state;

/*Variáveis globais*/
lines *lines_st;
attendant_state *att_st;

//Guarda para qual atendente deve ser direcionada a chamada no momento
int call = 0;

int attendants;

sem_t mutex;
sem_t *service;
sem_t wait;
sem_t mutex2;

/*-----------------------------------------------------------------*/
//Insere na fila
void push_queue(int id_thread)
{
    lines_st->end = (lines_st->ini + lines_st->waiting)% lines_st->max;
    lines_st->queue[lines_st->end] = id_thread;
    lines_st->waiting++;
}
//Retira da fila
void pull_queue()
{
    lines_st->ini = (lines_st->ini + 1) % lines_st->max;
    lines_st->waiting--;
}
//Mostra a fila
void print_queue()
{
    int aux_ini = lines_st->ini;
    int aux_waiting = lines_st->waiting;
    if(lines_st->waiting > 0)
    {
        printf("\n\n------Clientes na fila------\n");
        while(aux_waiting > 0)
        {
            printf("%d ", lines_st->queue[aux_ini]);
            aux_ini = (aux_ini + 1) % lines_st->max;
            aux_waiting--;
        }
        printf("\n\n");
    }
    else {
        printf("Nao ha clientes na fila..\n");
    }
}
//Verifica se pode descansar
bool canRest()
{
    //Se já houver algum atendente em descanso retorna false
    for(int i = 0; i < attendants; i++)
    {
        if(att_st[i].st == RESTING)
        {
            return false;
        }
    }
    //Chegando aqui não há atendentes descansando 
    //Retorna true sinalizando que o atendente já pode descansar
    return true;
} 


//Descanso do atendente
void rest(int id_thread)
{
    //Testa se o atendente possui a possibilidade de descansar
    if(att_st[id_thread].rest <= 0)
    {
        sem_wait(&mutex2);  //Região crítica, entra 1 por vez
        //Chama função que  testa se  já não há algum atendente descansando 
        //Só pode descasar um atendente por vez
        if(canRest())   
        {
            printf("Atendente %d esta descansando!\n", id_thread);
            //Muda estado do atendente
            att_st[id_thread].st = RESTING;
            sem_post(&mutex2); //Sai da região crítica e descansa fora do semáforo
            sleep(2 + (rand()%4));
            printf("O atendente %d terminou seu descanso e esta de volta a mesa\n", id_thread);
            //Reseta o contador para nova contagem
            att_st[id_thread].rest = 4 + (rand() %10);
            //Atendente está disponível para receber chamadas na sua mesa
            att_st[id_thread].st = IDLE;
        }
        sem_post(&mutex2);  //Sai da região crítica
    }
  
}

//Procura distribuir as chamadas entre os atendentes de forma circular
void redirectCall(int id_thread)
{
    //Verifica se o atendente escolhido está disponível
    if(att_st[call].st == IDLE)
    {
        //Guarda o ID do cliente para saber quem o atendente vai atender
        att_st[call].client_id = id_thread;
        //Faz a chamada ao atendente
        sem_post(&service[call]);
        call = (call + 1) % attendants;
    }
    else {
        //Caso atendente escolhido não esteja disponível procura pelo primeiro disponível
        for(int k = 0; k < attendants; k++)
        {
            if(att_st[k].st == IDLE)
            {
                //Guarda o ID do cliente para saber quem o atendente vai atender
                att_st[k].client_id = id_thread;
                //Faz a chamada ao atendente
                sem_post(&service[k]);
                break;  
            }
        }
    }
}

//Gerenciador de chamadas
void call_management(int id_thread)
{   
    print_queue();
    //Se wait for 0 -> Não há atendentes disponíveis
    //Cliente aguarda na fila
    sem_wait(&wait);
    printf("Cliente %d sera atendido! Ha %d chamadas em espera\n", id_thread, lines_st->waiting);
    //Designa a chamada para o primeiro atendente disponível
    //Cliente saiu da fila para ser atendido
    pull_queue();
    //Redireciona a chamada para algum atendente disponível
    redirectCall(id_thread);
}

//Thread para clientes
void *client(void *args)
{
    //casting para thread_arg
    thread_arg *client_arg = (thread_arg *) args;
    //Região crítica
    sem_wait(&mutex);
    //Verifica se há linhas disponíveis
    if(lines_st->waiting < lines_st->max)
    {
        printf("Cliente %d esta ligando! Havia %d chamadas em espera\n", client_arg->id_thread, lines_st->waiting);
        push_queue(client_arg->id_thread);
        //Sai da região crítica);
        sem_post(&mutex);
        //Chamada é realizada
        call_management(client_arg->id_thread);
    }  
    else {
        printf("Cliente %d nao conseguiu realizar a chamada. Todas as linhas ocupadas.\n", client_arg->id_thread);
        sem_post(&mutex);
    }
}


//Thread para atendentes
void *attendant(void *args)
{
    //casting para thread_arg
    thread_arg *attendant_arg = (thread_arg *) args;
    //Atendente inicia disponível
    att_st[attendant_arg->id_thread].st = IDLE;
    //Contador para o número de atendimentos realizados
    att_st[attendant_arg->id_thread].n = 0;
    
    while(true)
    {
        sem_wait(&service[attendant_arg->id_thread]); //Atendente aguarda por alguma chamada
        //Passa o estado do atendente de Ocioso para Atendendo
        //A partir de agora nenhuma chamada pode ser designada a essa mesa  
        att_st[attendant_arg->id_thread].st = ATTENDING;
        //Contador de atendimentos é incrementado se for diferente de zero
        if(att_st[attendant_arg->id_thread].rest > 0)
        {
            att_st[attendant_arg->id_thread].rest--;
        }
        //Realiza o atendimento
        printf("O atendente %d esta atendendo o cliente %d\n", attendant_arg->id_thread, att_st[attendant_arg->id_thread].client_id);
        sleep(4 + (rand()%10));
        printf("O atendente %d terminou o atendimento. Sao necessarios %d atendimentos para a folga.\n", attendant_arg->id_thread, att_st[attendant_arg->id_thread].rest);
        //Atendente terminou o atendimento

        //Verifica se o atendente pode descansar
        rest(attendant_arg->id_thread);
        //Passa seu estado para Ocioso, a partir daqui espera novas chamadas
        att_st[attendant_arg->id_thread].st = IDLE;
        //post no wait sinalizando que algum atendente esta livre para receber a chamada
        sem_post(&wait);
    }
}


int main(int argc, char *argv[])
{   
    lines_st = (lines *) malloc(sizeof(lines));

    pthread_t *client_thread;
    pthread_t *attendant_thread;

    thread_arg *arg_client;
    thread_arg *arg_attendant;

    int id = 0;

    lines_st->max = atoi(argv[2]);
    attendants = atoi(argv[1]);

    //Aloca vetor para a fila
    lines_st->queue = (int *) malloc(lines_st->max * sizeof(int));
    //Inicia a fila em 0
    lines_st->end = 0;
    lines_st->ini = 0;
    lines_st->waiting = 0;
    //Verifica a consistência das entradas
    if(argc != 3 || lines_st->max <= 0 || attendants <=0)
    {
        printf("Argumentos inválidos!\n");
        exit(-1);
    }

    //Aloca vetor de semáforos
    service = (sem_t *) malloc(attendants * sizeof(sem_t));

    //Vetor para controle dos estados dos atendentes
    att_st = (attendant_state *) malloc(attendants * sizeof(attendant_state));

    //Inicia semáforos dos atendentes
    for(int j = 0; j < attendants; j++)
    {
        sem_init(&service[j], 0, 0);
    }   

    //Semáforo de exclusão mútua
    sem_init(&mutex, 0, 1);

    sem_init(&wait, 0, attendants);

    sem_init(&mutex2, 0, 1);

    printf("--------------------------------------------------------\n");
    printf("------------------Call Center----------------\n");
    printf("Numero de linhas: %d          Numero de Atendentes:%d\n", lines_st->max, attendants);
    printf("-----------------------------------------------------------\n");

    attendant_thread = (pthread_t *) malloc(attendants * sizeof(pthread_t)); 

    srand(time(NULL));

    for(int j = 0; j < attendants; j++)
    {
        arg_attendant = (thread_arg *) malloc(sizeof(thread_arg));
        arg_attendant->id_thread = j;
        att_st[arg_attendant->id_thread].rest = (j+1) + (4 + (rand()%8));
        if(pthread_create(&attendant_thread[j], NULL, attendant, (void *)arg_attendant))
        {
            printf("Algum atendente pediu demissao!\n");
            exit(-1);
        }
    }

    client_thread = (pthread_t *) malloc(sizeof(pthread_t));

    while(true)
    {
        arg_client = (thread_arg *) malloc(sizeof(thread_arg));
        arg_client->id_thread = id;
        sleep(1 + (rand()%2));

        if(pthread_create(client_thread, NULL, client, (void *) arg_client))
        {
            printf("Um erro ocorreu ao criar a thread(cliente)!\n");
            exit(-1);
        }
        id++;
    }


    return 0;
}