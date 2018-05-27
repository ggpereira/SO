#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>

#define BILLION 1E9

//T1 - Sistemas Operacionais 
//Gabriel Gomes Pereira - Sistemas de Informação

int createFork()
{
    int pid;
    pid = fork();
    if(pid < 0)
    {
        printf("O Fork falhou\n");
        exit(1);
    }
    return pid;
}

void createBranch(int h, int h_bound, int root_pid)
{
    pid_t cLeft;
    pid_t cRight;
    if(getpid() == root_pid)//Começa imprimindo "nó" raiz | Como a função é recursiva preciso criar condição para raiz não ser impressa cada vez que
    {                       //for feita a chamada
         printf("n = %d Raiz PID = %d\n", h, getpid());
         h += 1; //h(altura) passa a valer 1, altura do primeiro filho da esquerda
    }
    if(h <= h_bound)
    {
        cLeft = createFork();//cria primeiro filho da esquerda
        if(cLeft == 0)//Filho da esquerda
        {
            //printf("Sou o filho da esquerda n = %d C[%d, %d]\n", h,  getpid(), getppid()); 
            printf("n = %d C[%d, %d]\n", h, getpid(), getppid());
            createBranch(h + 1, h_bound, root_pid); //O primeiro filho sempre vai ser o da esquerda, cria a esquerda até atingir o limite*/
        }
        else
        {//Pai
            wait(NULL); //Pai espera o filho da esquerda terminar
            cRight = createFork(); //Cria o filho da direita
            if(cRight == 0)//Filho da direita
            {
                //printf("Sou o filho da direita n = %d C[%d, %d]\n", h, getpid(), getppid());
                printf("n = %d C[%d, %d]\n", h, getpid(), getppid());
                createBranch(h + 1,  h_bound, root_pid); //Filho da direita também pode ter mais 2 filhos, faz recursão primeiramente criando e finalizando 
            }                                            //Os ramos da esquerda  
            else//Filho da direita terminou
            {
                wait(NULL);
                if(getpid() != root_pid)//A raiz não termina
                {
                    printf("      T[%d, %d]\n", getpid(), getppid());
                    _exit(0);//Força o término dos filhos, limitando o processo a executar somente o que está dentro da função
                }
            }
        }
    }
    else //Chegou no nível definido
    {
        //printf("Cheguei na altura!\n");
        printf("      T[%d, %d]\n", getpid(), getppid());
        _exit(0);//Força o término, limitando o processo a executar somente o que está dentro da função
    }
}


void createFree(int h, int h_bound, int root_pid)
{
    pid_t cLeft; 
    pid_t cRight;

    if(getpid() == root_pid)
    {
        printf("n = %d Raiz PID = %d\n", h, getpid());
        h += 1;
    }
    if(h <= h_bound)
    {
        cLeft = createFork();
        if(cLeft == 0)
        {
            printf("n = %d C[%d, %d]\n", h, getpid(), getppid());
            createFree(h + 1, h_bound, root_pid);
        }
        else
        {
            cRight = createFork();
            if(cRight == 0)
            {
                printf("n = %d C[%d, %d]\n", h, getpid(), getppid());
                createFree(h + 1, h_bound, root_pid);
            }
            else{
                while(wait(NULL) > 0); //Espera todos os filhos acabarem
                if(getpid() != root_pid) //A raiz ainda não termina
                {
                    printf("      T[%d, %d]\n", getpid(), getppid());
                    _exit(0);
                }
            }
        }
    }
    else//Chegou no fim da árvore
    {
        printf("      T[%d, %d]\n", getpid(), getppid());
        _exit(0);
    }
}


double execTime(struct timespec start, struct timespec stop) //Calcula e retorna o tempo de execução
{
    return (stop.tv_sec - start.tv_sec) + (stop.tv_nsec - start.tv_nsec) / BILLION;
}



int main(int argc, char *argv[])
{
    int h_max;
    int root_pid = getpid();
    double time_branch;
    double time_free;
    
    struct timespec start, stop; 
    
    if(argc == 2)
    {
        h_max = atoi(argv[1]);
        if(h_max <=  0)
        {
            printf("Erro! Altura invalida!\n");
            exit(1);
        }
    }
    else
    {
        printf("Argumentos invalidos!\n");
        exit(1);
    }
    //--------------------------Branch----------------------------------------
    printf("-------------Branch------------------\n");
    //-------------------Tempo inicio branch------------------------------------
    if(clock_gettime(CLOCK_REALTIME, &start) == -1)
    {
        printf("ERRO no clock_gettime!\n");
        exit(1);
    }
    //------------------------------------------------------------------------
    createBranch(0, h_max, root_pid);
    //--------tempo final branch------------------------------------------------------------
    if(clock_gettime(CLOCK_REALTIME, &stop) == -1)
    {
        printf("ERRO no clock_gettime!\n");
        exit(1);
    }
    time_branch = execTime(start, stop);
    printf("----Fim da criacao por branch----\n\n");
    //--------------------------Fim do Branch---------------------------------

    //--------------------------Começa livre ---------------------------------
    printf("--------------------Livre-------------------\n");
    //-----------------------Tempo inicio livre-----------------------------------------
    if(clock_gettime(CLOCK_REALTIME, &start) == -1)
    {
        printf("Erro no clock_gettime!\n");
        exit(1);
    }
    //printf("n = 0 Raiz PID = %d\n", getpid());
    createFree(0, h_max, root_pid);
    //-------------------Tempo final livre---------------------------------------------
    if(clock_gettime(CLOCK_REALTIME, &stop) == -1)
    {
        printf("Erro no clock_gettime!\n");
        exit(1);
    }
    time_free = execTime(start, stop);
    //------------------------------------------------------------------------------------------
     printf("----------Fim da criacao livre--------------\n\n");
    //--------------------------Fim do livre-----------------------------------
   
    printf("Tempo execucao 'Branch': %lf\n", time_branch);
    printf("Tempo execucao 'Livre': %lf\n",  time_free);
    printf("Fim PID = %d (root)\n", getpid());
    return 0;
}
