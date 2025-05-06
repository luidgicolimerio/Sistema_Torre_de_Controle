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

// Automatizar criação de chaves
#define SHM_KEY_1 1234
#define SHM_KEY_2 7890
#define SHM_KEY_3 4554
#define SHM_KEY_4 2314
#define SHM_KEY_5 3615
#define SHM_KEY_P 5162

#define QTD_AERONAVE 10

typedef struct{
    float posx;
    float posy;
    char  lado;
    int   voando;
    int   pista;
    int pid;
    int prioridade;
    int status;
} aeronave;

int shm_ids[QTD_AERONAVE];

int busca_pid(int processos_ativos, int pid, pid_t* pids) {
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
    if((distancia_y < 0.01) && (distancia_x < 0.01) && (a1->lado == a2->lado) && (a1->pista == a2->pista)){
        return 1;
    } else if((distancia_x < 0.01) && (a1->lado == a2->lado) && (a1->pista == a2->pista)){
        return 2;
    }
    return 0;
}

int verifica_pouso(int processos, aeronave** as, pid_t* pids){
    for(int i = 0; i<processos; i++){
        if(as[i]->voando == 0){
            int ind = busca_pid(processos, as[i]->pid, pids);
            int temp = pids[processos-1];
            pids[processos - 1] = pids[ind];
            pids[ind] = temp;
            processos--;
        }
    }
    return processos;
}


int main() {
    pid_t pids[QTD_AERONAVE];
    int shm_keys[QTD_AERONAVE] = {SHM_KEY_1, SHM_KEY_2, SHM_KEY_3, SHM_KEY_4, SHM_KEY_5};

    int shm_id = shmget(SHM_KEY_P, sizeof(int), IPC_CREAT | 0666);

    int processos_ativos = QTD_AERONAVE;

    aeronave* as[QTD_AERONAVE];

    for (int i = 0; i < QTD_AERONAVE; i++) {
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
    while(processos_ativos > 0) {
        printf("\nAeronaves no ar: %d\n", processos_ativos);

        for(int i=0; i<processos_ativos; i++){
            kill(pids[i], SIGCONT);
            sleep(1);
            kill(pids[i], SIGSTOP);
        }
        sleep(1);
        processos_ativos = verifica_pouso(processos_ativos, as, pids);
        int troca = 1;
        for (int j = 0; j<processos_ativos; j++){
            for (int k = j; k<processos_ativos; k++){
                if (j == k){
                    continue;
                }
                colidiu = verifica_colisao(as[j], as[k], pids, j);
                if (colidiu == 1){
                    printf("COLISAO!\nAs aeronaves %d e %d colidiram!\n", as[j]->pid, as[k]->pid);
                    // kill(as[j]->pid, SIGKILL);
                }else if(colidiu == 2){
                    printf("Alerta!\nAs aeronaves %d e %d estão em rotas de colisão!\n", as[j]->pid, as[k]->pid);
                    int tr = 1;
                    int ind = -1;
                    for(int t = 0; t<QTD_AERONAVE; t++){
                        if (t == k)
                            continue;
                        if(as[t]->lado != as[j]->lado)  //Verifica pois as pistas dos lado W são diferntes da do E
                            continue;
                        if((as[j]->status == 0) && (as[t]->status == 0)){ // Nenhuma das duas entrou em espera
                            if(as[j]->pista != as[t]->pista){
                                tr = 0;
                            }
                            else{
                                if(tr){
                                    tr = 1;
                                }
                                ind = j;
                            }
                                // if(as[t]->prioridade > as[j]->prioridade)
                                //     kill(as[j]->pid, SIGUSR1);
                                // else
                                //     kill(as[t]->pid, SIGUSR1);
                                // troca = 0;
                                // break;
                            
                        }
                    }
                    if(tr){
                        kill(as[j]->pid, SIGUSR2);
                    }
                    else{
                        if(as[ind]->prioridade > as[j]->prioridade)
                             kill(as[j]->pid, SIGUSR1);
                        else
                             kill(as[ind]->pid, SIGUSR1);
                                // troca = 0;
                    }
                }
            }
        }
    }

    printf("Processos finalizados\n");
    
    for (int i = 0; i < QTD_AERONAVE; i++) {
        shmdt(as[i]);
        shmctl(shm_ids[i], IPC_RMID, 0);
    }
    
    return 0;
}