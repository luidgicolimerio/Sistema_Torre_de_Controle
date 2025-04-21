/*
O programa deve criar múltiplos processos filhos, cada um representando uma aeronave
que está em rota. Cada aeronave deve executar uma tarefa simples, como "voar" (simular
uma contagem de distância percorrida) e comunicar sua posição ao controlador através da
memória compartilhada. Quando cada processo representando uma aeronave é criada,
escolhe-se randomicamente:
● o lado de entrada (E, W)
● a coordenada Y do oponto de entrada
● sorteio de um atraso (dentre 0 e 2 segundos)) até que a aerovane de fato começe a
entrar no espaço aéreo (por exemplo aeronaves A e D entraram antes do que
aeronaves Ce E)
● qual é a pista de pouco preferencia, de acordo com o lado de entrada
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>

#define SHM_KEY_1 1234
#define SHM_KEY_2 7890
#define SHM_KEY_3 4554
#define SHM_KEY_4 2314
#define SHM_KEY_5 3615

typedef struct{
    float posx;
    float posy;
    char  lado;
    int   voando;
    int   pista;
} aeronave;

int main() {
    int i = 0;

    pid_t pids[5];
    int shm_keys[5] = {SHM_KEY_1, SHM_KEY_2, SHM_KEY_3, SHM_KEY_4, SHM_KEY_5};
    int shm_ids[5];
    aeronave* as[5];

    for (int i = 0; i < 5; i++) {
        pids[i] = fork();
        shmget(shm_keys[i], sizeof(aeronave), IPC_CREAT | 0666);
        as[i] = (aeronave*) shmat(shm_ids[i], NULL, 0);
        if (pids[i] == 0) {
            char key[10];
            snprintf(key, sizeof(key), "%d", shm_keys[i]);
            execl("./aeronave", "aeronave", key, NULL);
            perror("execl");
            exit(1);
        } else if (pids[i] < 0) {
            perror("fork");
            exit(1);
        }
        kill(pids[i], SIGSTOP);
    }
    

    for (int i = 0; i < 5; i++) {
        kill(pids[0], SIGCONT);
        sleep(1);
        kill(pids[1], SIGCONT);
        sleep(1);
        kill(pids[2], SIGCONT);
        sleep(1);
        kill(pids[3], SIGCONT);
        sleep(1);
        kill(pids[4], SIGCONT);
        sleep(1);
        kill(pids[0], SIGSTOP);
        kill(pids[1], SIGSTOP);
        kill(pids[2], SIGSTOP);
        kill(pids[3], SIGSTOP);
        kill(pids[4], SIGSTOP);
    }
    kill(pids[0], SIGKILL);
    kill(pids[1], SIGKILL);
    kill(pids[2], SIGKILL);
    kill(pids[3], SIGKILL);
    kill(pids[4], SIGKILL);

    wait(NULL);
    wait(NULL);
    wait(NULL);
    wait(NULL);
    wait(NULL);

    printf("Processos finalizados\n");

    
    shmdt(as[0]);
    shmdt(as[1]);
    shmdt(as[2]);
    shmdt(as[3]);
    shmdt(as[4]);

    shmctl(shm_ids[0], IPC_RMID, 0);
    shmctl(shm_ids[1], IPC_RMID, 0);
    shmctl(shm_ids[2], IPC_RMID, 0);
    shmctl(shm_ids[3], IPC_RMID, 0);
    shmctl(shm_ids[4], IPC_RMID, 0);

    return 0;
}