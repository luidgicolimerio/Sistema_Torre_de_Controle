#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <math.h>
#include <sys/types.h>

typedef struct{
    float posx;
    float posy;
    char  lado;
    int   voando;
    int   pista;
    int pid;
    int prioridade;
    int status;
    int andei;
} aeronave;

int qtd_aeronave;
int processos_ativos;
int colisao, reduzir, troca_pista, muda_aeroporto;

// Função de busca do indíce do PID na lista de PIDs ativos
// Recebe o número de processos ativos, o PID que deverá ser encontrado e a lista de PIDs
// Retorna o índice ou -1 caso não o encontre
int busca_pid(int processos_ativos, int pid, pid_t* pids) {
    for (int i = 0; i < processos_ativos; i++) {
        if (pids[i] == pid) {
            return i;  // encontrado
        }
    }
    return -1;
}

// Função que verifica se duas aeronaves colidiram
// Recebe 2 estruturas de aeronave e compara seus estados
int verifica_colisao(aeronave* a1,aeronave* a2){
    float distancia_x = fabsf(a1->posx - a2->posx);
    float distancia_y = fabsf(a1->posy - a2->posy);
    if ((a1->voando == 1) && (a2->voando == 1)){
        if((distancia_y < 0.01) && (distancia_x < 0.01) && (a1->lado == a2->lado) && (a1->pista == a2->pista)){
            return 1;
        } else if((distancia_x < 0.01) && (a1->lado == a2->lado) && (a1->pista == a2->pista)){
            return 2;
        }
    }
    return 0;
}

// Função para verificar se alguma aeronave já pousou
// Recebe o número de processos ativos, a lista de todas as aeronaves e a lista de PIDs
// Tem como objetivo atualizar o número de aeronaves que ainda estão voando e mover o PID para uma zona morta da lista
// Define também o trocou da estrutura da aeronave como 1, para que em uma próxima verificação não acaba eliminando um processo ativo.
int verifica_pouso(int processos, aeronave** as, pid_t* pids){
    for(int i = 0; i<processos; i++){
        if(as[i]->voando == 0){
            int ind = busca_pid(processos, as[i]->pid, pids);
            int temp = pids[processos-1];
            pids[processos - 1] = pids[ind];
            pids[ind] = temp;
            processos--; i--;
        }
    }
    return processos;
}

// Função que remove aeronaves que já colidiram
// Recebe a quantidade de processos ativos, as duas aeronaves que colidiram e a lista de PIDs
// É necessário fazer o swap dos processos para uma zona morta da lista
int mata_colisao(int processos, aeronave* a1,aeronave* a2, pid_t* pids){
    int ind = busca_pid(processos, a1->pid, pids);
    int temp = pids[processos-1];
    pids[processos - 1] = pids[ind];
    pids[ind] = temp;
    processos--;

    ind = busca_pid(processos, a2->pid, pids);
    temp = pids[processos-1];
    pids[processos - 1] = pids[ind];
    pids[ind] = temp;
    processos--;
    
    return processos;
}

// Função para mandar a aeronave para outro aeroporto caso ela surja no mesmo local que outra aeronave
// Recebe processos, a aeronave que será enviada para outro aeroporto, a lista de aeronaves e a lista de PIDs
// Coloca o processo para uma zona morta da lista e também faz o swap da posição da aeronave na lista
int troca_aeroporto(int processos, aeronave* a1, aeronave** as,  pid_t* pids) {
    int ind = busca_pid(processos, a1->pid, pids);
    if (ind != -1) {
        pid_t temp = pids[processos - 1];
        pids[processos - 1] = pids[ind];
        pids[ind] = temp;

        aeronave* temp_a = as[processos - 1];
        as[processos - 1] = as[ind];
        as[ind] = temp_a;

        processos--;
    }
    return processos;
}

// Função principal
int main() {

    //Interface inicial
    printf("Este é um simulador de torre de controle\n");
    printf("Entre com a quantidade de aeronaves que você deseja simular e pressione ENTER para iniciar\n");
    printf("Numero de aeronaves: ");
    scanf("%d", &qtd_aeronave);
    printf("\n");

    int shm_ids[qtd_aeronave];
    pid_t pids[qtd_aeronave];



    key_t shm_keys[qtd_aeronave];

    // Inicia contadores
    colisao = 0;
    reduzir = 0;
    troca_pista = 0;
    muda_aeroporto = 0;


    aeronave* as[qtd_aeronave];
    processos_ativos = qtd_aeronave;

    // Criação de arquivo para geração de chaves aleatórias
    const char *key_file = "chaves.txt";
    FILE *file = fopen(key_file, "w");
    //Gera chaves aleatórias
    for (int i = 0; i < qtd_aeronave; i++) {
        shm_keys[i] = ftok(key_file, 'A' + i);
    }

    // Inicia os processos(aeronaves) de forma dinâmica
    for (int i = 0; i < qtd_aeronave; i++) {
        pids[i] = fork();
        shm_ids[i] = shmget(shm_keys[i], sizeof(aeronave), IPC_CREAT | 0666);
        as[i] = (aeronave*)shmat(shm_ids[i], NULL, 0);
        if (pids[i] == 0) {
            char key[12];
            snprintf(key, sizeof(key), "%d", shm_keys[i]);
            execl("./aeronave", "aeronave", key, NULL); //Passa a chave de cada aeronave no comando de execução
            perror("execl");
            exit(1);
        } else if (pids[i] < 0) {
            perror("fork");
            exit(1);
        }
        sleep(1); // Dorme para dar tempo da aeronave iniciar suas variáveis
        kill(pids[i], SIGSTOP);

    }


    int colidiu = 0;
    // Verifica se alguma aeronave nasceu no mesmo lugar que outra
    // Percorre todas as aeronaves da lista
    for (int j = 0; j < processos_ativos; j++) {
        for (int k = j + 1; k < processos_ativos; k++) { // Começa de j+1 para evitar comparar mesmo índice
            colidiu = verifica_colisao(as[j], as[k]);
            if (colidiu == 1) {
                muda_aeroporto++;
                printf("A Aeronave: %d foi direcionada para outro aeroporto\n", as[j]->pid);
                processos_ativos = troca_aeroporto(processos_ativos, as[j], as, pids);
                j--;   // Exclui a verificação do último índice visto que ocorreu o swap
                break; 
            }
        }
    }
    
    // Loop enquanto houverem aeronaves voando
    while(processos_ativos > 0) {
        printf("\nAeronaves no ar: %d\n", processos_ativos);

        //Round Robin
        for(int i=0; i<processos_ativos; i++){
            kill(pids[i], SIGCONT);
            as[i]->andei = 1;
            sleep(1);
            kill(pids[i], SIGSTOP);
            as[i]->andei = 0;
        }
        sleep(1); // Dorme para dar tempo dos processos receberem os sinais
        processos_ativos = verifica_pouso(processos_ativos, as, pids); // Atualiza processos ativos caso alguma aeronave já tenha pousado
        
        // Verificação de tragetória da aeronave/colisão
        for (int j = 0; j<processos_ativos; j++){
            for (int k = j; k<processos_ativos; k++){
                if (j == k){
                    continue;
                }
                colidiu = verifica_colisao(as[j], as[k]);
                // Colidiu
                if (colidiu == 1){
                    printf("COLISAO!\nAs aeronaves %d e %d colidiram!\n", as[j]->pid, as[k]->pid);
                    colisao++;
                    processos_ativos = mata_colisao(processos_ativos, as[j], as[k], pids);
                //Pode Colidir
                }else if(colidiu == 2){
                    printf("Alerta!\nAs aeronaves %d e %d estão em rotas de colisão!\n", as[j]->pid, as[k]->pid);
                    int troca = 1;
                    int ind = -1;
                    for(int t = 0; t<processos_ativos; t++){
                        if (t == k)
                            continue;
                        if(as[t]->lado != as[j]->lado)  //Verifica pois as pistas dos lado W são diferntes da do E
                            continue;
                        if((as[j]->status == 0) && (as[t]->status == 0)){ // Nenhuma das duas entrou em espera
                            if(as[j]->pista != as[t]->pista){// verifica se existe algum avião do mesmo lado na outra pista
                                troca = 0;// Se já existe, não vai ter a troca de pista, o avião vai precisar aguardar (reduzir a velocidade))
                            }
                            else{// Pega o indice do avião que está voando para a mesma pista
                                if(troca){ // para manter o valor da variável e pegar o indice do ultimo avião na mesma rota
                                    troca = 1;
                                }
                                ind = j;
                            }
                        }
                    }
                    if(troca){// A troca de pista é feita quando não tem avião na outra pista, é enviado um sinal para esse avião permitindo a troca de pista
                        troca_pista++;
                        kill(as[j]->pid, SIGUSR2);
                    }
                    else{// Já existe avião na outra pista, então é verificada a prioridade de dois aviões que estão na mesma pista
                        if(as[ind]->prioridade > as[j]->prioridade){// Se o avião já precisou aguardar antes, sua prioridade em pousar aumenta
                             kill(as[j]->pid, SIGUSR1); 
                             reduzir++;
                        }
                        else{
                             kill(as[ind]->pid, SIGUSR1); 
                             reduzir++;
                        }
                    }
                }
            }
        }
    }
    // Fim da simulação
    printf("Processos finalizados\n");
    printf("| Registros da Torre\n| Colisões: %d\n| Reduções: %d\n| Trocas de Pista:%d\n| Mudança de Aeroporto: %d\n", colisao, reduzir, troca_pista, muda_aeroporto);   
    
    // Libera a memória compartilhada
    for (int i = 0; i < qtd_aeronave; i++) {
        shmdt(as[i]);
        shmctl(shm_ids[i], IPC_RMID, 0);
    }
    
    return 0;
}