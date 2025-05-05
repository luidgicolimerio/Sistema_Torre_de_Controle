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
#include <math.h>

#define SHM_KEY_1 1234
#define SHM_KEY_2 7890
#define SHM_KEY_3 4554
#define SHM_KEY_4 2314
#define SHM_KEY_5 3615

#define QTD_AERONAVE 5

typedef struct{
    float posx;
    float posy;
    char  lado;
    int   voando;
    int   pista;
    int pid;
} aeronave;

int processos_ativos;
int shm_ids[QTD_AERONAVE];

int busca_pid(int pid, pid_t* pids) {
    for (int i = 0; i < processos_ativos; i++) {
        if (pids[i] == pid) {
            return i;  // encontrado
        }
    }
    return -1;
}

int verifica_colisao(aeronave* a1,aeronave* a2, pid_t* pids, int id){
    float distancia_x = fabsf(a1->posx - a2->posx);
    float distancia_y = fabsf(a1->posy - a2->posy);
    if (!(a1->voando)){
        int ind = busca_pid(a1->pid, pids);
        kill(a1->pid, SIGKILL);
        if(ind != -1){
            int temp = pids[processos_ativos];
            pids[processos_ativos] = pids[ind];
            pids[ind] = temp;
            processos_ativos--;
        }
    }
    else if((distancia_y < 0.01) && (distancia_x < 0.01) && (a1->lado == a2->lado) && (a1->pista == a2->pista)){
        return 1;
    } else if((distancia_x < 0.01) && (a1->lado == a2->lado) && (a1->pista == a2->pista)){
        return 2;
    }
    return 0;
}

int main() {
    int i = 0;

    pid_t pids[QTD_AERONAVE];
    int shm_keys[QTD_AERONAVE] = {SHM_KEY_1, SHM_KEY_2, SHM_KEY_3, SHM_KEY_4, SHM_KEY_5};

    aeronave* as[QTD_AERONAVE];

    for (int i = 0; i < 5; i++) {
        pids[i] = fork();
        shm_ids[i] = shmget(shm_keys[i], sizeof(aeronave), IPC_CREAT | 0666);
        as[i] = (aeronave*)shmat(shm_ids[i], NULL, 0);
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
    
    int colidiu;
    processos_ativos = QTD_AERONAVE;
    while(processos_ativos > 0) {
        printf("\nAeronaves no ar: %d\n", processos_ativos);
        if (processos_ativos == 0){
            break;
        }
        for (int i = 0;i<processos_ativos; i++){
            kill(pids[i], SIGCONT);
            sleep(1);
            kill(pids[i], SIGSTOP);
        }

        for (int j = 0; j<QTD_AERONAVE; j++){
            for (int k = 0; k<QTD_AERONAVE; k++){
                if (j == k){
                    continue;
                }
                colidiu = verifica_colisao(as[j], as[k], pids, j);
                if (colidiu == 1){
                    printf("COLISAO!\nAs aeronaves %d e %d colidiram!\n", as[j]->pid, as[k]->pid);
                }else if(colidiu == 2){
                    printf("Alerta!\nAs aeronaves %d e %d estão em rotas de colisão!\n", as[j]->pid, as[k]->pid);
                    //tratar com sinais aeronaves em rota de colisão
                }
            }
        }
    }

    printf("Processos finalizados\n");
    
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