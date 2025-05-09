#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>

typedef struct{ //Informações de Controle = Tabela PCB
    float posx;
    float posy;
    char  lado;
    int   voando;
    int   pista;
    int pid;
    int prioridade;
    int status;
    int andei;
    int trocou;
} aeronave;

aeronave* a;

// Tratamento da SIGUSR1
void us1Handler(int sinal)
{
    a->prioridade++;
    a->status = 1;
    printf("Aeronave: %d Em espera\n", a->pid);
    sleep(2);
    a->status = 0;

}
// Tratamento da SIGUSR2
void us2tHandler(int sinal){
    if(a->lado == 'W'){
        if (a->pista == 18){
            a->pista = 3;
        } else {
            a->pista = 18;;
        }
    } else {
        if (a->pista == 27){
            a->pista = 6;
        } else {
            a->pista = 27;;
        }
    }
    printf("Aeronave: %d Trocou para a pista: #%d\n", a->pid, a->pista);
}

int main(int argc, char *argv[]){
    srand(time(NULL) + getpid()); // Garante valores diferentes

    int shm_key = atoi(argv[1]); // Primeiro argumento
    int shm_id = shmget(shm_key, sizeof(aeronave), 0666);
    void (*p)(int);

    p = signal(SIGUSR1, us1Handler);
    p = signal(SIGUSR2, us2tHandler);

    a = (aeronave*)shmat(shm_id, NULL, 0);

    // Define variáveis para togle
    a->voando = 1;
    a->prioridade = 0;
    a->status = 0;
    a->andei = 0;
    a->trocou = 0;

    // Sortei valores
    int lado = rand()%2;
    int pista = rand()%2;
    pid_t pid = getpid();

    a->pid = pid;

    if (lado){ // 1
        a->lado =  'W';
        a->posx = 0.0;
        a->posy = (rand() % 21) * 0.05; // Valor entre 0 e 1 multiplo de 0.05
        if (pista){
            a->pista = 18;
        } else {
            a->pista = 3;
        }
    } else{ // 0
        a->lado = 'E';
        a->posx = 1.0;
        a->posy = (rand() % 21) * 0.05; // Valor entre 0 e 1 multiplo de 0.05
        if (pista){
            a->pista = 27;
        } else {
            a->pista = 6;
        }
    }
    printf("Aeronave: %d / Vinda da Direção: %c / Pista de Preferência:#%d / Altura de Voo:%.2f \n", pid, a->lado, a->pista, a->posy);
    sleep(1);

    //Loop de Voo
    while(a->voando){
        // Tentativa de evitar andar 2 vezes na mesma rodada
        if(!a->andei){
            sleep(1);
            continue;
        }
        // Se aeronave chegou no 0.5 / 0.5 ela finaliza seu processo
        if ((a->posx > 0.49 && a->posx < 0.51) && (a->posy > 0.49 && a->posy < 0.51)){
            printf("Aeronave: %d \n POUSOU!\n", pid);
            a->voando = 0;
            a->trocou = 1;
            exit(0);
        }

        // Condições de locomoção
        if (!(a->posx > 0.49 && a->posx < 0.51)){
            if (a->posx > 0.5){
                a->posx -= 0.05;
            }else if(a->posx < 0.5){
                a->posx += 0.05;
            }
        }
        if (!(a->posy > 0.49 && a->posy < 0.51)){
            if (a->posy > 0.5){
                a->posy -= 0.05;
            }else if(a->posy < 0.5){
                a->posy += 0.05;
            }
        }

        sleep(1);
        // Status Atual
        printf("Aeronave: %d / posx: %.2f  posy: %.2f / Pista Atual: #%d\n", pid, a->posx, a->posy, a->pista);
    }
    exit(0);
}