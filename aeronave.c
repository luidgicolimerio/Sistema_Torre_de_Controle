#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>

typedef struct{
    float posx;
    float posy;
    char  lado;
    int   voando;
    int   pista;
    int pid;
} aeronave;

int main(int argc, char *argv[]){
    srand(time(NULL) + getpid()); // garante valores diferentes

    int shm_key = atoi(argv[1]);
    int shm_id = shmget(shm_key, sizeof(aeronave), 0666);

    aeronave* a = (aeronave*)shmat(shm_id, NULL, 0);
    a->voando = 1;

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
    printf("Aeronave: %d / Vinda da Direção: %c / Pista de Preferência:#%d\n", pid, a->lado, a->pista);
    sleep(1);
    while(a->voando){

        if ((a->posx > 0.49 && a->posx < 0.51) && (a->posy > 0.49 && a->posy < 0.51)){
            printf("Aeronave: %d \n POUSOU!\n", pid);
            a->voando = 0;
            exit(0);
        }
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
        printf("Aeronave: %d / posx: %.2f  posy: %.2f\n", pid, a->posx, a->posy);
    }
    exit(0);
}