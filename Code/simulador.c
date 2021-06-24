/*	Universidade da Madeira - Unidade Curricular de Sistemas Operativos 2020-2021
	Docentes: Eduardo Marques e Luis Gaspar
	Programador: Pedro Sousa 

	Processo representativo do simulador (server)

	Ficheiro: simulador.c
*/

// Bibliotecas
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include "fileFunctions.h"
#include "unix.h"

#define VELSIM 1       // Cada minuto corresponde a um segundo da simulação
#define MAX_USERS 1000 // Número máximo de utilizadores a serem criados
// Cores
#define CNORMAL "\x1B[0m"
#define RED "\x1B[91m"
#define LIGHTGREEN "\x1B[92m"
#define BLUE "\x1B[94m"
#define CYAN "\x1B[96m"
#define WHITE "\x1B[97m"
#define VIOLET "\x1B[35m"
#define YELLOW "\033[0;33m"
#define GREEN "\x1B[32m"
#define YEC "\x1B[34m"

// Assinaturas de funções
void sendMsg(int par, int msg);
void sendSimInfo(int par);
bool testaProbabilidade(char prob);
void rotina_simulador();
void funcPacienteA(int id);
void hospitalA();
void funcPacienteB(int id);
void hospitalB();
void funcPacienteC(int id);
void hospitalC();

// Variáveis Globais
int minutos; // Minutos que passaram desde o inicio da simulação
int sockfd;  // Ponteiro global para o socket

int pessoas_filaHospital_A, pessoas_filaHospital_B, pessoas_filaHospital_Alta, pessoas_filaHospital_Baixa;            // Variáveis usadas para contar o número de threads à espera nas filas
int pacientes_HospitalA, pacientes_HospitalB, pacientes_HospitalC, positivos, negativos, internados, curados, mortes; // Variáveis usadas para registar valores de simulação

// threads
pthread_attr_t thread_size;
pthread_t taskpessoa[MAX_USERS];
pthread_t task_hospitalA, task_hospitalB, task_hospitalC;

// Semáforos
sem_t sem_filaespA, sem_filaespB, sem_filaA, sem_filaB; // Semaforos para controlar filas de espera dos hospitais

sem_t sem_filaespAlta, sem_filaespBaixa, sem_filaAlta, sem_filaBaixa; // Semaforos para controlar filas de espera dos hospitais

sem_t sem_HospA, sem_HospB; // Semaforos para controlar entrada nos centros de testagem

sem_t sem_HospAlta, sem_HospBaixa; // Semaforos para controlar entrada nos centros de testagem

sem_t sem_resa, sem_resB, sem_resC; // Semaforos para controlar o tempo de testagem

sem_t sem_seguranca_alta, sem_seguranca_baixa, sem_seguranca_testagem; //semáforo usado exclusivamente no hospital C para reforçar a zona de EXCLUSÃO MÚTUA

sem_t sem_testa, sem_testB, sem_testC; // Semáforos para controlar saida dos centros de testagem

// Trincos
pthread_mutex_t trinco_min; // Trinco para acesso a variável de minutos

pthread_mutex_t trinco_p_filaespA, trinco_p_filaespB, trinco_p_filaespAlta, trinco_p_filaespBaixa; // Trincos para filas de espera

pthread_mutex_t trinco_p_HospA, trinco_p_HospB, trinco_p_HospC; // Trincos para testagem

pthread_mutex_t trinco_p_positivosA, trinco_p_positivosB; // Trincos para variáveis de covid positivos

pthread_mutex_t trinco_p_negativosA, trinco_p_negativosB; // Trincos para variáveis de covid negativos

pthread_mutex_t trinco_p_internadosA, trinco_p_internadosB; // Trincos para variável internados

pthread_mutex_t trinco_p_curadosA, trinco_p_curadosB; // Trincos para variável curados

pthread_mutex_t trinco_p_mortesA, trinco_p_mortesB; // Trincos para variável mortes

pthread_mutex_t trinco_socket; // Trinco para o socket (escrita/leitura)

config *conf_ptr; // Ponteiro para as configurações do ficheiro
int num_conf;     // Número de configurações lidas

// variáveis int que guardarão os valores de configuração
int tempo_simul, prob_pessoa, prob_cura, prob_positivo, prob_hospitalA, prob_hospitalB, prob_hospitalC, prob_prioAlta, prob_prioBaixa, tamanho_fila, pessoa_triagem, enfermeiros, tempo_analise, tempo_cura;

int enfermeiros_alta, enfermeiros_baixa; // variável que atribui o nº de enfermeiros a cada tipo de prioridade

int main(int argc, char *argv[])
{
    int clilen, childpid, servlen; //variáveis de apoio à socket

    srand(time(NULL)); // Gerar nova seed para números pseudo-aleatórios.

    if (argc == 2) // Se for passado um ficheiro de configuração como argumento na incialização do simulador então entramos no ciclo de leitura
    {
        printf("Lendo ficheiro: %s \n\n", argv[1]); // Avisa que o ficheiro será lido e indica o nome do ficheiro de configuração

        num_conf = lerConfigurations(&conf_ptr, argv[1]); // Lê o ficheiro e guarda o numero de configurações lidas

        if (num_conf != 0) //Se exisirem configurações, imprime no ecrã o sucesso de leitura e o número de configs lidas
        {
            printf("Ficheiro de configuracão lido com sucesso!\nNumero de configuracões lidas: %d \n", num_conf);
        }
        else //Se não existirem configurações imprime mensagem de erro
        {
            printf("Erro ao ler o ficheiro! Provavelmente não tem configurações definidas, defina configurações.\n");
            exit(0);
        }

        char index_tempo_simul = obterIndiceParametro(conf_ptr, num_conf, "tempo_simul");     //Guarda o index de cada parametro ou em caso de não existir / tiver defeito
        char index_prob_pessoa = obterIndiceParametro(conf_ptr, num_conf, "prob_pessoa");     //guarda o valor do código de erro (-1) para posteriormente realizar tratamento de
        char index_prob_positivo = obterIndiceParametro(conf_ptr, num_conf, "prob_positivo"); //de excepções (para parametros em falta ou com defeito atribuir valores default)
        char index_prob_hospitalA = obterIndiceParametro(conf_ptr, num_conf, "prob_hospitalA");
        char index_prob_hospitalB = obterIndiceParametro(conf_ptr, num_conf, "prob_hospitalB");
        char index_prob_hospitalC = obterIndiceParametro(conf_ptr, num_conf, "prob_hospitalC");
        char index_prob_prioAlta = obterIndiceParametro(conf_ptr, num_conf, "prob_prioAlta");
        char index_prob_prioBaixa = obterIndiceParametro(conf_ptr, num_conf, "prob_prioBaixa");
        char index_prob_cura = obterIndiceParametro(conf_ptr, num_conf, "prob_cura");
        char index_tamanho_fila = obterIndiceParametro(conf_ptr, num_conf, "tamanho_fila");
        char index_pessoa_triagem = obterIndiceParametro(conf_ptr, num_conf, "pessoa_triagem");
        char index_enfermeiros = obterIndiceParametro(conf_ptr, num_conf, "enfermeiros");
        char index_tempo_analise = obterIndiceParametro(conf_ptr, num_conf, "tempo_analise");
        char index_tempo_cura = obterIndiceParametro(conf_ptr, num_conf, "tempo_cura");

        if (index_tempo_simul == -1) //Tratamento de expeções para cada parâmetro. -1 significa não existir o parâmetro ou existir com nome errado...atribuir valores default
        {
            tempo_simul = 240;
            float temp_horas = (float)tempo_simul / 60; //Informa sempre o user de qual o valor lido e guardado e em caso de ser default avisa de tal atribuição
            printf("Parâmetro tempo de simulação inexistente: tempo de simulação default: %d minutos (segundos em tempo real) ou %.2f horas virtuais.\n", tempo_simul, temp_horas);
        }
        else
        {
            tempo_simul = atoi(conf_ptr[index_tempo_simul].dados);
            float temp_horas = (float)tempo_simul / 60;
            printf("Parâmetro tempo de simulação lido: tempo de simulação: %d minutos (segundos em tempo real) ou %.2f horas virtuais.\n", tempo_simul, temp_horas);
        }
        if (index_prob_pessoa == -1)
        {
            prob_pessoa = 80;
            printf("Parâmetro probabilidade de criar pessoa inexistente: probabilidade de criar pessoa default: %d . \n", prob_pessoa);
        }
        else
        {
            prob_pessoa = atoi(conf_ptr[index_prob_pessoa].dados);
            printf("Parâmetro probabilidade de criar pessoa lido: probabilidade de criar pessoa: %d . \n", prob_pessoa);
        }
        if (index_prob_cura == -1)
        {
            prob_cura = 40;
            printf("Parâmetro probabilidade do paciente se curar inexistente: probabilidade do paciente se curar default: %d . \n", prob_cura);
        }
        else
        {
            prob_cura = atoi(conf_ptr[index_prob_cura].dados);
            printf("Parâmetro probabilidade do paciente se curar lido: probabilidade do paciente se curar: %d . \n", prob_cura);
        }
        if (index_prob_positivo == -1)
        {
            prob_positivo = 40;
            printf("Parâmetro probabilidade de ser positivo para covid inexistente: probabilidade de ser covid positivo default: %d .\n", prob_positivo);
        }
        else
        {
            prob_positivo = atoi(conf_ptr[index_prob_positivo].dados);
            printf("Parâmetro probabilidade de ser positivo para covid lido: probabilidade de ser covid positivo: %d .\n", prob_positivo);
        }
        if (index_prob_hospitalA == -1)
        {
            prob_hospitalA = 40;
            printf("Parâmetro probabilidade de ser encaminhado para o Hospital A inexistente: probabilidade de ser encaminhado para o Hospital A default: %d .\n", prob_hospitalA);
        }
        else
        {
            prob_hospitalA = atoi(conf_ptr[index_prob_hospitalA].dados);
            printf("Parâmetro probabilidade de ser encaminhado para o Hospital A lido: probabilidade de ser encaminhado para o Hospital A: %d .\n", prob_hospitalA);
        }
        if (index_prob_hospitalB == -1)
        {
            prob_hospitalB = 30;
            printf("Parâmetro probabilidade de ser encaminhado para o Hospital B inexistente: probabilidade de ser encaminhado para o Hospital B default: %d .\n", prob_hospitalB);
        }
        else
        {
            prob_hospitalB = atoi(conf_ptr[index_prob_hospitalB].dados);
            printf("Parâmetro probabilidade de ser encaminhado para o Hospital B lido: probabilidade de ser encaminhado para o Hospital B: %d .\n", prob_hospitalB);
        }
        if (index_prob_hospitalC == -1)
        {
            prob_hospitalC = 30;
            printf("Parâmetro probabilidade de ser encaminhado para o Hospital C inexistente: probabilidade de ser encaminhado para o Hospital C default: %d .\n", prob_hospitalC);
        }
        else
        {
            prob_hospitalC = atoi(conf_ptr[index_prob_hospitalC].dados);
            printf("Parâmetro probabilidade de ser encaminhado para o Hospital C lido: probabilidade de ser encaminhado para o Hospital C: %d .\n", prob_hospitalC);
        }
        if (index_prob_prioAlta == -1)
        {
            prob_prioAlta = 30;
            printf("Parâmetro probabilidade de ser paciente de prioridade alta inexistente: probabilidade de ser paciente de prioridade alta default: %d .\n", prob_prioAlta);
        }
        else
        {
            prob_prioAlta = atoi(conf_ptr[index_prob_prioAlta].dados);
            printf("Parâmetro probabilidade de ser paciente de prioridade alta lido: probabilidade de ser paciente de prioridade alta: %d .\n", prob_prioAlta);
        }
        if (index_prob_prioBaixa == -1)
        {
            prob_prioBaixa = 30;
            printf("Parâmetro probabilidade de ser paciente de prioridade baixa inexistente: probabilidade de ser paciente de prioridade baixa default: %d .\n", prob_prioBaixa);
        }
        else
        {
            prob_prioBaixa = atoi(conf_ptr[index_prob_prioBaixa].dados);
            printf("Parâmetro probabilidade ser paciente de prioridade baixa lido: probabilidade de ser paciente de prioridade baixa: %d .\n", prob_prioBaixa);
        }
        if (index_tamanho_fila == -1)
        {
            tamanho_fila = 30;
            printf("Parâmetro tamanho da fila inexistente: tamanho da fila default: %d pessoas.\n", tamanho_fila);
        }
        else
        {
            tamanho_fila = atoi(conf_ptr[index_tamanho_fila].dados);
            printf("Parâmetro tamanho da fila lido: tamanho da fila: %d pessoas.\n", tamanho_fila);
        }
        if (index_pessoa_triagem == -1)
        {
            pessoa_triagem = 1;
            printf("Parâmetro número de pessoas na triagem inexistente: Nº de pessoas na triagem default: %d .\n", pessoa_triagem);
        }
        else
        {
            pessoa_triagem = atoi(conf_ptr[index_pessoa_triagem].dados);
            printf("Parâmetro número de pessoas na triagem lido: Nº de pessoas na triagem: %d .\n", pessoa_triagem);
        }
        if (index_enfermeiros == -1)
        {
            enfermeiros = 10;
            printf("Parâmetro número de enfermeiros por Hospital inexistente: Nº de enfermeiros default: %d .\n", enfermeiros);
        }
        else
        {
            enfermeiros = atoi(conf_ptr[index_enfermeiros].dados);
            printf("Parâmetro número de enfermeiros por Hospital lido: Nº de enfermeiros: %d .\n", enfermeiros);
        }
        if (index_tempo_analise == -1)
        {
            tempo_analise = 10;
            printf("Parâmetro tempo de análise inexistente: tempo de análise default: %d .\n", tempo_analise);
        }
        else
        {
            tempo_analise = atoi(conf_ptr[index_tempo_analise].dados);
            printf("Parâmetro tempo de análise lido: tempo de análise: %d .\n", tempo_analise);
        }
        if (index_tempo_cura == -1)
        {
            tempo_cura = 20;
            printf("Parâmetro tempo de cura inexistente: tempo de cura default: %d .\n\n\n", tempo_cura);
        }
        else
        {
            tempo_cura = atoi(conf_ptr[index_tempo_cura].dados);
            printf("Parâmetro tempo de cura lido: tempo de cura: %d .\n\n\n", tempo_cura);
        }
    }
    else //Caso o simulador seja incializado sem ser passado um config file como argumento imprime mensagem de erro e termina programa
    {
        printf("Necessita inicializar o Simulador com o ficheiro de configuração.\n\nPor favor execute o seguinte comando: ./simulador configurations.conf\n");
        exit(0);
    }

    struct sockaddr_un cli_addr, serv_addr;

    /* Cria socket stream */

    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        err_dump("server: can't open stream socket");

    /* Primeiro uma limpeza preventiva!
       Dados para o socket stream: tipo + nome do ficheiro.
       O ficheiro serve para que os clientes possam identificar o servidor */

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, UNIXSTR_PATH);

    /* O servidor é quem cria o ficheiro que identifica o socket.
         Elimina o ficheiro, para o caso de algo ter ficado pendurado.
         Em seguida associa o socket ao ficheiro. 
         A dimensão a indicar ao bind não é a da estrutura, pois depende
         do nome do ficheiro */

    servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
    unlink(UNIXSTR_PATH);
    if (bind(sockfd, (struct sockaddr *)&serv_addr, servlen) < 0)
        err_dump("server, can't bind local address");

    /* Servidor pronto a aceitar 5 clientes para o socket stream */

    listen(sockfd, 1);

    printf(RED "À espera de monitores!\n" CNORMAL);

    clilen = sizeof(cli_addr);
    sockfd = accept(sockfd, (struct sockaddr *)&cli_addr,
                    &clilen);

    if (sockfd < 0)
        err_dump("server: accept error");

    rotina_simulador();

    close(sockfd);

    return 0;
}

void rotina_simulador() //Função da rotina do simulador
{

    // Inicializa as filas de espera

    size_t minSize = 0x4000, mySize;
    pthread_attr_setstacksize(&thread_size, minSize);

    system("clear");
    printf(LIGHTGREEN "Simulacao iniciada\n" CNORMAL);
    printf("Mensagens de Simulação:\n");
    printf("----------------------------------------------------\n");
    int p = 1; // Contador de total de pacientes criados

    // Iniciar Simulação

    sendSimInfo(1); // Envia código de Mensagem de inicio de simulação
    sendMsg(201, enfermeiros);

    // Inicialização dos hospitais

    if (pthread_create(&task_hospitalA, NULL /*&thread_size*/, hospitalA, NULL))
    {
        perror("Erro criar thread Hospital A:");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf(CYAN "Hospital A " LIGHTGREEN "inicializado.\n" CNORMAL);
    }

    if (pthread_create(&task_hospitalB, NULL /*&thread_size*/, hospitalB, NULL))
    {
        perror("Erro criar thread Hospital B:");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf(VIOLET "Hospital B " LIGHTGREEN "inicializado.\n" CNORMAL);
    }
    if (pthread_create(&task_hospitalC, NULL /*&thread_size*/, hospitalC, NULL))
    {
        perror("Erro criar thread Hospital C:");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf(GREEN "Hospital C " LIGHTGREEN "inicializado.\n" CNORMAL);
    }

    //Cáculo do número de enfermeiros para cada tipo de prioridade
    if (enfermeiros % 2 == 0)
    {
        enfermeiros_alta = enfermeiros / 2;
        enfermeiros_baixa = enfermeiros / 2;
    }
    else
    {
        enfermeiros_baixa = (enfermeiros - 1) / 2;
        enfermeiros_alta = enfermeiros - enfermeiros_baixa;
    }

    // inicialização de semaforos
    sem_init(&sem_filaespA, 0, tamanho_fila);
    sem_init(&sem_filaespB, 0, tamanho_fila);
    sem_init(&sem_filaespAlta, 0, tamanho_fila);
    sem_init(&sem_filaespBaixa, 0, tamanho_fila);

    sem_init(&sem_filaA, 0, tamanho_fila);
    sem_init(&sem_filaB, 0, tamanho_fila);
    sem_init(&sem_filaAlta, 0, tamanho_fila);
    sem_init(&sem_filaBaixa, 0, tamanho_fila);

    sem_init(&sem_HospA, 0, enfermeiros);
    sem_init(&sem_HospB, 0, enfermeiros);
    sem_init(&sem_HospAlta, 0, enfermeiros_alta);
    sem_init(&sem_HospBaixa, 0, enfermeiros_baixa);

    sem_init(&sem_resa, 0, 1);
    sem_init(&sem_resB, 0, 1);
    sem_init(&sem_resC, 0, 1);

    sem_init(&sem_seguranca_alta, 0, 1);
    sem_init(&sem_seguranca_baixa, 0, 1);
    sem_init(&sem_seguranca_testagem, 0, 1);

    sem_init(&sem_testa, 0, 0);
    sem_init(&sem_testB, 0, 0);
    sem_init(&sem_testC, 0, 0);

    // Até atingir o tempo máximo de simulação
    while (minutos <= tempo_simul)
    {

        if (testaProbabilidade(prob_pessoa)) //Testa probabilidade de criar nova pessoa
        {
            if (testaProbabilidade(prob_hospitalA)) //Testa probabilidade da pessoa a ser criada ser paciente do hospital A
            {
                if (pthread_create(&taskpessoa[p - 1], NULL /*&thread_size*/, funcPacienteA, (void *)p) != 0) //Inicializa nova thread paciente do hospital A
                {
                    perror("Erro criar thread paciente Hospital A:");
                    exit(EXIT_FAILURE);
                }
                else
                {
                    p++; // Se a thread for inicializada com sucesso adiciona ao total pacientes
                }
            }

            if (testaProbabilidade(prob_hospitalB)) //Testa probabilidade da pessoa a ser criada ser paciente do hospital B
            {
                if (pthread_create(&taskpessoa[p - 1], NULL /*&thread_size*/, funcPacienteB, (void *)p) != 0) //Inicializa nova thread paciente do hospital B
                {
                    perror("Erro criar thread paciente Hospital B:");
                    exit(EXIT_FAILURE);
                }
                else
                {
                    p++; // Se a thread for inicializada com sucesso adiciona ao total pacientes
                }
            }

            if (testaProbabilidade(prob_hospitalC)) //Testa probabilidade da pessoa a ser criada ser paciente do hospital C
            {
                if (pthread_create(&taskpessoa[p - 1], NULL /*&thread_size*/, funcPacienteC, (void *)p) != 0) //Inicializa nova thread paciente do hospital C
                {
                    perror("Erro criar thread paciente Hospital C:");
                    exit(EXIT_FAILURE);
                }
                else
                {
                    p++; // Se a thread for inicializada com sucesso adiciona ao total pacientes
                }
            }
        }

        sleep(VELSIM); // Dorme 1
        minutos++;     // Incrementa minutos
    }

    // Terminar Simulação

    sendSimInfo(2); //Envia código de término de simulação

    printf(RED "Simulacao Terminada\n" CNORMAL);
}

void funcPacienteA(int id) //Função Paciente do hospital A (FILA COM DESISTÊNCIAS)
{
    int min_chegada;              //Variável que guardará o tempo de chegada do paciente ao hospital
    int min_desistir;             //Variável que guardará o tempo de desistência do paciente
    int p_filaesp, p_filahosp;    //Variáveis de apoio para contabilizar nº de pacientes/threads em espera ativa
    int temp_sair = (rand() % 5); //variável aleatória do tempo que um paciente leva para sair do hospital

    sem_wait(&sem_filaespA);

    // Paciente chega ao hospital

    // obter o tempo de chegada ao hospital
    pthread_mutex_lock(&trinco_min);
    min_chegada = minutos;
    pthread_mutex_unlock(&trinco_min);

    // obter o numero de pessoas na fila de espera

    // cálculo do tempo até desistir em espera ativa
    min_desistir = min_chegada + (rand() % 6) + 1;

    // envia mensagem para o ecrã e para o monitor
    printf(RED "[Min: %d]" CNORMAL " O paciente %d chegou ao Hospital A! \n", minutos, id);
    sendMsg(1, id);

    // espera ativa até poder entrar na fila para o hospital
    pthread_mutex_lock(&trinco_p_filaespA);
    p_filahosp = pessoas_filaHospital_A;
    pthread_mutex_unlock(&trinco_p_filaespA);

    while (p_filahosp >= tamanho_fila) //Enquanto não houver espaço na fila para testagem, espera na rua do hospital
    {

        pthread_mutex_lock(&trinco_min);
        if (minutos > min_desistir) // Se o tempo passado for superior ao de desistência, paciente desiste
        {
            pthread_mutex_unlock(&trinco_min);
            printf(RED "[Min: %d]" CNORMAL " O paciente %d desistiu de esperar tanto tempo para entrar na fila do Hospital A! \n", minutos, id);
            sendMsg(4, id);
            sem_post(&sem_filaespA);
            pthread_exit(NULL);
        }
        else
            pthread_mutex_unlock(&trinco_min);

        pthread_mutex_lock(&trinco_p_filaespA); //caso contrário aguarda
        p_filahosp = pessoas_filaHospital_A;
        pthread_mutex_unlock(&trinco_p_filaespA);
    }

    sem_wait(&sem_filaA); // Entra na fila do hospital

    // Aumenta o nº de pessoas na fila do hospital
    pthread_mutex_lock(&trinco_p_filaespA);
    pessoas_filaHospital_A++;
    pthread_mutex_unlock(&trinco_p_filaespA);

    sem_post(&sem_filaespA); // Sai da lista pessoas em espera para entrar na fila do hospital

    printf(RED "[Min: %d]" CNORMAL " O paciente %d entrou na fila do Hospital A! \n", minutos, id);
    sendMsg(6, id);

    sem_wait(&sem_HospA); //Paciente aguarda para entrar no centro de testagem

    pthread_mutex_lock(&trinco_p_filaespA);
    pessoas_filaHospital_A--;
    pthread_mutex_unlock(&trinco_p_filaespA); // Paciente entra no centro de testagem e decrementa o nº de pessoa na fila do hospital

    printf(RED "[Min: %d]" CNORMAL " O paciente %d entrou na sala de testagem do Hospital A! \n", minutos, id);
    sendMsg(11, id);

    pthread_mutex_lock(&trinco_p_HospA);
    pacientes_HospitalA++;
    pthread_mutex_unlock(&trinco_p_HospA); // Incrementa o nº de pessoa em testagem

    sem_post(&sem_filaA); // Sai da fila do hospital

    sem_wait(&sem_testa); // Aguarda testes para poder sair do centro de testagem

    printf(RED "[Min: %d]" CNORMAL " O paciente %d saiu da sala de testagem do Hospital A! \n", minutos, id);
    sendMsg(13, id);

    pthread_mutex_lock(&trinco_p_HospA); // Saiu do centro de testagem (post do semáforo estão na funcão do Hospital correspondente)
    pacientes_HospitalA--;               // Decrementa o nº de pessoas em testagem
    pthread_mutex_unlock(&trinco_p_HospA);

    sleep(temp_sair); // Simula o tempo que o paciente leva a sair das instalações (Sair do hospital)

    printf(RED "[Min: %d]" CNORMAL " O paciente %d saiu do Hospital A! \n", minutos, id); // Paciente sai do hospital
    sendMsg(16, id);

    sleep(tempo_analise); // Simula o tempo que demora a receber a resposta das análises

    if (testaProbabilidade(prob_positivo)) //Testa probabilidade de estar positivo
    {
        pthread_mutex_lock(&trinco_p_positivosA); // Se tiver positivo incrementa o nº de positivos e de internados
        positivos++;
        pthread_mutex_unlock(&trinco_p_positivosA);

        printf(RED "[Min: %d]" CNORMAL " O paciente %d do Hospital A testou positivo para COVID! Será Hopitalizado\n", minutos, id);
        sendMsg(17, id);

        pthread_mutex_lock(&trinco_p_internadosA);
        internados++;
        pthread_mutex_unlock(&trinco_p_internadosA);

        sleep(tempo_cura); // Simula o tempo que leva a curar ou falecer no pior dos casos

        if (testaProbabilidade(prob_cura)) // Testa a probabilidade de cura. Se o paciente se curar decrementa positivos e internados e incrementa curados
        {
            pthread_mutex_lock(&trinco_p_positivosA);
            positivos--;
            pthread_mutex_unlock(&trinco_p_positivosA);

            pthread_mutex_lock(&trinco_p_internadosA);
            internados--;
            pthread_mutex_unlock(&trinco_p_internadosA);

            pthread_mutex_lock(&trinco_p_curadosA);
            curados++;
            pthread_mutex_unlock(&trinco_p_curadosA);

            printf(RED "[Min: %d]" CNORMAL " O paciente %d do Hospital A curou-se do COVID!\n", minutos, id);
            sendMsg(18, id);
        }
        else // Se o paciente não for curado, decrementa positivos e internados e incrementa mortes
        {
            pthread_mutex_lock(&trinco_p_positivosA);
            positivos--;
            pthread_mutex_unlock(&trinco_p_positivosA);

            pthread_mutex_lock(&trinco_p_internadosA);
            internados--;
            pthread_mutex_unlock(&trinco_p_internadosA);

            pthread_mutex_lock(&trinco_p_mortesA);
            mortes++;
            pthread_mutex_unlock(&trinco_p_mortesA);

            printf(RED "[Min: %d]" CNORMAL " Infelizmente O paciente %d do Hospital A sucumbiu ao COVID!\n", minutos, id);
            sendMsg(19, id);
        }
    }
    else //Caso contrário, resultado negativo, incrementa o nº de negativos para covid
    {
        pthread_mutex_lock(&trinco_p_negativosA);
        negativos++;
        pthread_mutex_unlock(&trinco_p_negativosA);

        printf(RED "[Min: %d]" CNORMAL " O paciente %d do Hospital A testou negativo para COVID! Reencaminhado para casa \n", minutos, id);
        sendMsg(20, id);
    }

    pthread_exit(NULL); // Termina thread
}

void hospitalA() // Função Hospital A que irá receber pacientes do tipo A
{

    int min_atual;        // variável que regista minuto atual de simulação
    int pessoas_testagem; // Variável que guarda o nº de pessoas em testagem

    pthread_mutex_lock(&trinco_min); // Minuto atual = variável partilhada dos minutos de simulação
    min_atual = minutos;
    pthread_mutex_unlock(&trinco_min);

    while (min_atual <= tempo_simul) // Enquanto o tempo atual não ultrapassar o tempo de simulação
    {

        int iter; // Variável de apoio a loops

        pthread_mutex_lock(&trinco_p_HospA);
        pessoas_testagem = pacientes_HospitalA; // Verifica quantas pessoas tem no centro de testagem
        pthread_mutex_unlock(&trinco_p_HospA);

        while (pessoas_testagem < enfermeiros) // Enquanto o centro de testagem não estiver preenchido (equanto houverem enfermeiros disponíveis), aguarda pacientes
        {
            pthread_mutex_lock(&trinco_p_HospA);
            pessoas_testagem = pacientes_HospitalA;
            pthread_mutex_unlock(&trinco_p_HospA);
        }

        sem_wait(&sem_resa); //Semáforo para simular o tempo de testagem

        printf(RED "[Min: %d]" CNORMAL " Os pacientes do Hospital A estão a ser testados!\n", minutos);
        sendMsg(9, 0);

        sleep(tempo_analise); // Pausar durante o tempo da testagem

        printf(RED "[Min: %d]" CNORMAL " Os pacientes do Hospital A foram testados!\n", minutos);
        sendMsg(10, 0);

        sem_post(&sem_resa);

        for (iter = 0; iter < enfermeiros; iter++) //Enquanto houverem pessoas dentro do centro de testagem esvazia todas essas pessoas, de forma a poderem sair do hospital
        {
            sem_post(&sem_testa);
        }

        pthread_mutex_lock(&trinco_p_HospA);
        pessoas_testagem = pacientes_HospitalA; //Atualiza nº de pessoas em testagem, porque as pessoas estão a sair do centro de testagem e a diminuir este valor
        pthread_mutex_unlock(&trinco_p_HospA);

        while (pessoas_testagem > 0) // Enquanto ainda houverem pessoas em centro de testagem, mais nenhuma ronda de testes se pode iniciar
        {
            pthread_mutex_lock(&trinco_p_HospA);
            pessoas_testagem = pacientes_HospitalA; // Aguarda que todasd as pessoas acabem os testes e saiam dos centros de testagem
            pthread_mutex_unlock(&trinco_p_HospA);
        }

        for (iter = 0; iter < enfermeiros; iter++) // Após todos os pacientes sairem do centro de testagem, assinala que o centro de testagem está vazio
            sem_post(&sem_HospA);                  // De forma a que as pessoas que estão a aguardar para entrar no centro de testagem possam avançar

        pthread_mutex_lock(&trinco_min);
        min_atual = minutos; //atualiza minuto atual de simulação
        pthread_mutex_unlock(&trinco_min);
    }

    pthread_exit(NULL); // Termina thread do hospital após terminar simulação
}
void funcPacienteB(int id) // Função Paciente do hospital B (fila sem desistências)
{
    int min_chegada;              // Variável que guardará o tempo de chegada do paciente ao hospital
    int min_desistir;             // Variável que guardará o tempo de desistência do paciente
    int p_filaesp, p_filahosp;    // Variáveis de apoio para contabilizar nº de pacientes/threads em espera ativa
    int temp_sair = (rand() % 5); // Variável aleatória do tempo que um paciente leva para sair do hospital

    sem_wait(&sem_filaespB);

    // Paciente chega ao hospital

    // Obter o tempo de chegada ao hospital
    pthread_mutex_lock(&trinco_min);
    min_chegada = minutos;
    pthread_mutex_unlock(&trinco_min);

    // Envia mensagem para o ecrã e para o monitor
    printf(RED "[Min: %d]" CNORMAL " O paciente %d chegou ao Hospital B! \n", minutos, id);
    sendMsg(2, id);

    // Espera ativa até poder entrar na fila para o hospital
    pthread_mutex_lock(&trinco_p_filaespB);
    p_filahosp = pessoas_filaHospital_B;
    pthread_mutex_unlock(&trinco_p_filaespB);

    while (p_filahosp >= tamanho_fila)
    {
        pthread_mutex_lock(&trinco_p_filaespB);
        p_filahosp = pessoas_filaHospital_B;
        pthread_mutex_unlock(&trinco_p_filaespB);
    }

    sem_wait(&sem_filaB);

    // Entra na fila do hospital

    // Aumenta o nº de pessoas na fila do hospital
    pthread_mutex_lock(&trinco_p_filaespB);
    pessoas_filaHospital_B++;
    pthread_mutex_unlock(&trinco_p_filaespB);

    sem_post(&sem_filaespB); // Sai da lista pessoas em espera para entrar na fila do hospital

    printf(RED "[Min: %d]" CNORMAL " O paciente %d entrou na fila do Hospital B! \n", minutos, id);
    sendMsg(3, id);

    sem_wait(&sem_HospB); // Paciente aguarda para entrar no centro de testagem

    pthread_mutex_lock(&trinco_p_filaespB);
    pessoas_filaHospital_B--; // Decrementa pessoas em espera na fila do hospital
    pthread_mutex_unlock(&trinco_p_filaespB);

    printf(RED "[Min: %d]" CNORMAL " O paciente %d entrou na sala de testagem do Hospital B! \n", minutos, id);
    sendMsg(5, id);

    pthread_mutex_lock(&trinco_p_HospB); // Paciente entra no centro de testagem e decrementa o nº de pessoa na fila do hospital
    pacientes_HospitalB++;               // Incrementa o nº de pessoa em testagem
    pthread_mutex_unlock(&trinco_p_HospB);

    sem_post(&sem_filaB); // Sai da fila do hospital

    sem_wait(&sem_testB); // Aguarda testes para poder sair do centro de testagem

    printf(RED "[Min: %d]" CNORMAL " O paciente %d saiu da sala de testagem do Hospital B! \n", minutos, id);
    sendMsg(7, id);

    pthread_mutex_lock(&trinco_p_HospB); // Saiu do centro de testagem (post do semáforo está na funcão do Hospital correspondente)
    pacientes_HospitalB--;               // Decrementa o nº de pessoas em testagem
    pthread_mutex_unlock(&trinco_p_HospB);

    sleep(temp_sair); // Simula o tempo que o paciente leva a sair das instalações (Sair do hospital)

    printf(RED "[Min: %d]" CNORMAL " O paciente %d saiu do Hospital B! \n", minutos, id); // Paciente sai do hospital
    sendMsg(8, id);

    sleep(tempo_analise); //Simula o tempo que demora a receber a resposta das análises

    if (testaProbabilidade(prob_positivo)) //Testa probabilidade de estar positivo
    {
        pthread_mutex_lock(&trinco_p_positivosB); //Se tiver positivo incrementa o nº de positivos e de internados
        positivos++;
        pthread_mutex_unlock(&trinco_p_positivosB);

        printf(RED "[Min: %d]" CNORMAL " O paciente %d do Hospital B testou positivo para COVID! Será Hopitalizado\n", minutos, id);
        sendMsg(12, id);

        pthread_mutex_lock(&trinco_p_internadosB);
        internados++;
        pthread_mutex_unlock(&trinco_p_internadosB);

        sleep(tempo_cura); // Simula o tempo que leva a curar ou falecer no pior dos casos

        if (testaProbabilidade(prob_cura)) // Testa a probabilidade de cura. Se o paciente se curar decrementa positivos e internados e incrementa curados
        {
            pthread_mutex_lock(&trinco_p_positivosB);
            positivos--;
            pthread_mutex_unlock(&trinco_p_positivosB);

            pthread_mutex_lock(&trinco_p_internadosB);
            internados--;
            pthread_mutex_unlock(&trinco_p_internadosB);

            pthread_mutex_lock(&trinco_p_curadosB);
            curados++;
            pthread_mutex_unlock(&trinco_p_curadosB);

            printf(RED "[Min: %d]" CNORMAL " O paciente %d do Hospital B curou-se do COVID!\n", minutos, id);
            sendMsg(14, id);
        }
        else // Se o paciente não se curar decrementa positivos e internados e incrementa mortes
        {
            pthread_mutex_lock(&trinco_p_positivosB);
            positivos--;
            pthread_mutex_unlock(&trinco_p_positivosB);

            pthread_mutex_lock(&trinco_p_internadosB);
            internados--;
            pthread_mutex_unlock(&trinco_p_internadosB);

            pthread_mutex_lock(&trinco_p_mortesB);
            mortes++;
            pthread_mutex_unlock(&trinco_p_mortesB);

            printf(RED "[Min: %d]" CNORMAL " Infelizmente O paciente %d do Hospital B sucumbiu ao COVID!\n", minutos, id);
            sendMsg(15, id);
        }
    }
    else // Caso contrário, resultado negativo, incrementa o nº de negativos para covid
    {
        pthread_mutex_lock(&trinco_p_negativosB);
        negativos++;
        pthread_mutex_unlock(&trinco_p_negativosB);

        printf(RED "[Min: %d]" CNORMAL " O paciente %d do Hospital B testou negativo para COVID! Reenchaminhado para casa \n", minutos, id);
        sendMsg(21, id);
    }

    pthread_exit(NULL); //Termina thread
}

void hospitalB() // Função Hospital B que irá receber pacientes do tipo B
{

    int min_atual;        // Variá¡vel que regista minuto atual de simulação
    int pessoas_testagem; // Variável que guarda o nº de pessoas em testagem

    pthread_mutex_lock(&trinco_min);
    min_atual = minutos; // Minuto atual = variável partilhada dos minutos de simulação
    pthread_mutex_unlock(&trinco_min);

    while (min_atual <= tempo_simul) // Enquanto o tempo atual nÃo ultrapassar o tempo de simulação
    {

        int iter; // Variável de aopio a loops

        pthread_mutex_lock(&trinco_p_HospB);
        pessoas_testagem = pacientes_HospitalB; // Verifica quantas pessoas tem no centro de testagem
        pthread_mutex_unlock(&trinco_p_HospB);

        while (pessoas_testagem < enfermeiros) // Enquanto o centro de testagem não estiver preenchido (equanto houverem enfermeiros disponí­veis), aguarda pacientes
        {
            pthread_mutex_lock(&trinco_p_HospB);
            pessoas_testagem = pacientes_HospitalB;
            pthread_mutex_unlock(&trinco_p_HospB);
        }

        sem_wait(&sem_resB); //Semáforo para simular o tempo de testagem

        printf(RED "[Min: %d]" CNORMAL " Os pacientes do Hospital B estÃ£o a ser testados!\n", minutos);
        sendMsg(22, 0);

        sleep(tempo_analise); // Pausar durante o tempo da testagem

        printf(RED "[Min: %d]" CNORMAL " Os pacientes do Hospital B foram testados!\n", minutos);
        sendMsg(23, 0);

        sem_post(&sem_resB);

        for (iter = 0; iter < enfermeiros; iter++) // Enquanto houverem pessoas dentro do centro de testagem esvazia todas essas pessoas, de forma a poderem sair do hospital
        {
            sem_post(&sem_testB);
        }

        pthread_mutex_lock(&trinco_p_HospB);
        pessoas_testagem = pacientes_HospitalB; // Atualiza nº de pessoas em testagem, porque as pessoas estão a sair do centro de testagem e a diminuir este valor
        pthread_mutex_unlock(&trinco_p_HospB);

        while (pessoas_testagem > 0) // Enquanto ainda houverem pessoas em centro de testagem, mais nenhuma ronda de testes se pode iniciar
        {
            pthread_mutex_lock(&trinco_p_HospB);
            pessoas_testagem = pacientes_HospitalB; // Aguarda que todas as pessoas acabem os testes e saiam dos centros de testagem
            pthread_mutex_unlock(&trinco_p_HospB);
        }

        for (iter = 0; iter < enfermeiros; iter++) // Após todos os pacientes sairem do centro de testagem, assinala que o centro de testagem estar vazio
            sem_post(&sem_HospB);                  // De forma a que as pessoas que estão a aguardar para entrar no centro de testagem possam avançar

        pthread_mutex_lock(&trinco_min); // Atualiza minuto atual de simulação
        min_atual = minutos;
        pthread_mutex_unlock(&trinco_min);
    }

    pthread_exit(NULL); // Termina thread do hospital após terminar simulação
}

void funcPacienteC(int id) //Função Paciente do hospital A (FILA COM DESISTÊNCIAS)
{
    int min_chegada;                                                        //Variável que guardará o tempo de chegada do paciente ao hospital
    int p_filaesp_alta, p_filahosp_alta, p_filaesp_baixa, p_filahosp_baixa; //Variáveis de apoio para contabilizar nº de pacientes/threads em espera ativa
    int temp_sair = (rand() % 5);                                           //variável aleatória do tempo que um paciente leva para sair do hospital

    if (testaProbabilidade(prob_prioAlta)) //Testa probabilidade da pessoa a ser criada ser paciente do hospital A
    {
        sem_wait(&sem_filaespAlta);

        // Paciente chega ao hospital

        // Obter o tempo de chegada ao hospital
        pthread_mutex_lock(&trinco_min);
        min_chegada = minutos;
        pthread_mutex_unlock(&trinco_min);

        // Envia mensagem para o ecrã e para o monitor
        printf(RED "[Min: %d]" CNORMAL " O paciente %d de prioridade alta chegou ao Hospital C! \n", minutos, id);
        sendMsg(24, id);

        // ESPERA ATIVA até poder entrar na fila para o hospital
        pthread_mutex_lock(&trinco_p_filaespAlta);
        p_filahosp_alta = pessoas_filaHospital_Alta;
        pthread_mutex_unlock(&trinco_p_filaespAlta);

        while (p_filahosp_alta >= tamanho_fila)
        {
            pthread_mutex_lock(&trinco_p_filaespAlta);
            p_filahosp_alta = pessoas_filaHospital_Alta;
            pthread_mutex_unlock(&trinco_p_filaespAlta);
        }

        sem_wait(&sem_filaAlta);

        // Entra na fila do hospital
        sem_wait(&sem_seguranca_alta);
        // Aumenta o nº de pessoas na fila do hospital
        pthread_mutex_lock(&trinco_p_filaespAlta);
        pessoas_filaHospital_Alta++;
        pthread_mutex_unlock(&trinco_p_filaespAlta);
        sem_post(&sem_seguranca_alta);

        sem_post(&sem_filaespAlta); // Sai da lista pessoas em espera para entrar na fila do hospital

        printf(RED "[Min: %d]" CNORMAL " O paciente %d entrou na fila de alta prioridade do Hospital C! \n", minutos, id);
        sendMsg(25, id);

        sem_wait(&sem_HospAlta); // Paciente aguarda para entrar no centro de testagem

        sem_wait(&sem_seguranca_alta);
        pthread_mutex_lock(&trinco_p_filaespAlta);
        pessoas_filaHospital_Alta--; // Decrementa pessoas em espera na fila do hospital
        pthread_mutex_unlock(&trinco_p_filaespAlta);
        sem_post(&sem_seguranca_alta);

        printf(RED "[Min: %d]" CNORMAL " O paciente %d de prioridade alta entrou na sala de testagem do Hospital C! \n", minutos, id);
        sendMsg(26, id);

        sem_wait(&sem_seguranca_testagem);
        pthread_mutex_lock(&trinco_p_HospC); // Paciente entra no centro de testagem e decrementa o nº de pessoa na fila do hospital
        pacientes_HospitalC++;               // Incrementa o nº de pessoa em testagem
        pthread_mutex_unlock(&trinco_p_HospC);
        sem_post(&sem_seguranca_testagem);

        sem_post(&sem_filaAlta); // Sai da fila do hospital

        sem_wait(&sem_testC); // Aguarda testes para poder sair do centro de testagem

        printf(RED "[Min: %d]" CNORMAL " O paciente %d de prioridade alta saiu da sala de testagem do Hospital C! \n", minutos, id);
        sendMsg(27, id);

        sem_wait(&sem_seguranca_testagem);
        pthread_mutex_lock(&trinco_p_HospC); // Saiu do centro de testagem (post do semáforo está na funcão do Hospital correspondente)
        pacientes_HospitalC--;               // Decrementa o nº de pessoas em testagem
        pthread_mutex_unlock(&trinco_p_HospC);
        sem_post(&sem_seguranca_testagem);

        sleep(temp_sair); // Simula o tempo que o paciente leva a sair das instalações (Sair do hospital)

        printf(RED "[Min: %d]" CNORMAL " O paciente %d de prioridade alta saiu do Hospital C! \n", minutos, id); // Paciente sai do hospital
        sendMsg(28, id);

        sleep(tempo_analise); //Simula o tempo que demora a receber a resposta das análises

        if (testaProbabilidade(prob_positivo)) //Testa probabilidade de estar positivo
        {
            pthread_mutex_lock(&trinco_p_positivosB); //Se tiver positivo incrementa o nº de positivos e de internados
            positivos++;
            pthread_mutex_unlock(&trinco_p_positivosB);

            printf(RED "[Min: %d]" CNORMAL " O paciente %d do Hospital C testou positivo para COVID! Será Hopitalizado\n", minutos, id);
            sendMsg(29, id);

            pthread_mutex_lock(&trinco_p_internadosB);
            internados++;
            pthread_mutex_unlock(&trinco_p_internadosB);

            sleep(tempo_cura); // Simula o tempo que leva a curar ou falecer no pior dos casos

            if (testaProbabilidade(prob_cura)) // Testa a probabilidade de cura. Se o paciente se curar decrementa positivos e internados e incrementa curados
            {
                pthread_mutex_lock(&trinco_p_positivosB);
                positivos--;
                pthread_mutex_unlock(&trinco_p_positivosB);

                pthread_mutex_lock(&trinco_p_internadosB);
                internados--;
                pthread_mutex_unlock(&trinco_p_internadosB);

                pthread_mutex_lock(&trinco_p_curadosB);
                curados++;
                pthread_mutex_unlock(&trinco_p_curadosB);

                printf(RED "[Min: %d]" CNORMAL " O paciente %d do Hospital C curou-se do COVID!\n", minutos, id);
                sendMsg(30, id);
            }
            else // Se o paciente não se curar decrementa positivos e internados e incrementa mortes
            {
                pthread_mutex_lock(&trinco_p_positivosB);
                positivos--;
                pthread_mutex_unlock(&trinco_p_positivosB);

                pthread_mutex_lock(&trinco_p_internadosB);
                internados--;
                pthread_mutex_unlock(&trinco_p_internadosB);

                pthread_mutex_lock(&trinco_p_mortesB);
                mortes++;
                pthread_mutex_unlock(&trinco_p_mortesB);

                printf(RED "[Min: %d]" CNORMAL " Infelizmente O paciente %d do Hospital C sucumbiu ao COVID!\n", minutos, id);
                sendMsg(31, id);
            }
        }
        else // Caso contrário, resultado negativo, incrementa o nº de negativos para covid
        {
            pthread_mutex_lock(&trinco_p_negativosB);
            negativos++;
            pthread_mutex_unlock(&trinco_p_negativosB);

            printf(RED "[Min: %d]" CNORMAL " O paciente %d do Hospital C testou negativo para COVID! Reencaminhado para casa \n", minutos, id);
            sendMsg(32, id);
        }

        pthread_exit(NULL); //Termina thread
    }

    if (testaProbabilidade(prob_prioBaixa)) //Testa probabilidade da pessoa a ser criada ser paciente do hospital B
    {
        sem_wait(&sem_filaespBaixa);

        // Paciente chega ao hospital

        // Obter o tempo de chegada ao hospital
        pthread_mutex_lock(&trinco_min);
        min_chegada = minutos;
        pthread_mutex_unlock(&trinco_min);

        // Envia mensagem para o ecrã e para o monitor
        printf(RED "[Min: %d]" CNORMAL " O paciente %d de baixa prioridade chegou ao Hospital C! \n", minutos, id);
        sendMsg(33, id);

        // ESPERA ATIVA até poder entrar na fila para o hospital
        pthread_mutex_lock(&trinco_p_filaespBaixa);
        p_filahosp_baixa = pessoas_filaHospital_Baixa;
        pthread_mutex_unlock(&trinco_p_filaespBaixa);

        while (p_filahosp_baixa >= tamanho_fila)
        {
            pthread_mutex_lock(&trinco_p_filaespBaixa);
            p_filahosp_baixa = pessoas_filaHospital_Baixa;
            pthread_mutex_unlock(&trinco_p_filaespBaixa);
        }

        sem_wait(&sem_filaBaixa);

        // Entra na fila do hospital
        sem_wait(&sem_seguranca_baixa);
        // Aumenta o nº de pessoas na fila do hospital
        pthread_mutex_lock(&trinco_p_filaespBaixa);
        pessoas_filaHospital_Baixa++;
        pthread_mutex_unlock(&trinco_p_filaespBaixa);
        sem_post(&sem_seguranca_baixa);

        sem_post(&sem_filaespBaixa); // Sai da lista pessoas em espera para entrar na fila do hospital

        printf(RED "[Min: %d]" CNORMAL " O paciente %d entrou na fila de prioridade baixa do Hospital C! \n", minutos, id);
        sendMsg(34, id);

        sem_wait(&sem_HospBaixa); // Paciente aguarda para entrar no centro de testagem

        sem_wait(&sem_seguranca_baixa);
        pthread_mutex_lock(&trinco_p_filaespBaixa);
        pessoas_filaHospital_Baixa--; // Decrementa pessoas em espera na fila do hospital
        pthread_mutex_unlock(&trinco_p_filaespBaixa);
        sem_post(&sem_seguranca_baixa);

        printf(RED "[Min: %d]" CNORMAL " O paciente %d de prioridade baixa entrou na sala de testagem do Hospital C! \n", minutos, id);
        sendMsg(35, id);

        sem_wait(&sem_seguranca_testagem);
        pthread_mutex_lock(&trinco_p_HospC); // Paciente entra no centro de testagem e decrementa o nº de pessoa na fila do hospital
        pacientes_HospitalC++;               // Incrementa o nº de pessoa em testagem
        pthread_mutex_unlock(&trinco_p_HospC);
        sem_post(&sem_seguranca_testagem);

        sem_post(&sem_filaBaixa); // Sai da fila do hospital

        sem_wait(&sem_testC); // Aguarda testes para poder sair do centro de testagem

        printf(RED "[Min: %d]" CNORMAL " O paciente %d de prioridade baixa saiu da sala de testagem do Hospital C! \n", minutos, id);
        sendMsg(36, id);

        sem_wait(&sem_seguranca_testagem);
        pthread_mutex_lock(&trinco_p_HospC); // Saiu do centro de testagem (post do semáforo está na funcão do Hospital correspondente)
        pacientes_HospitalC--;               // Decrementa o nº de pessoas em testagem
        pthread_mutex_unlock(&trinco_p_HospC);
        sem_post(&sem_seguranca_testagem);

        sleep(temp_sair); // Simula o tempo que o paciente leva a sair das instalações (Sair do hospital)

        printf(RED "[Min: %d]" CNORMAL " O paciente %d de prioridade baixa saiu do Hospital C! \n", minutos, id); // Paciente sai do hospital
        sendMsg(37, id);

        sleep(tempo_analise); //Simula o tempo que demora a receber a resposta das análises

        if (testaProbabilidade(prob_positivo)) //Testa probabilidade de estar positivo
        {
            pthread_mutex_lock(&trinco_p_positivosB); //Se tiver positivo incrementa o nº de positivos e de internados
            positivos++;
            pthread_mutex_unlock(&trinco_p_positivosB);

            printf(RED "[Min: %d]" CNORMAL " O paciente %d do Hospital C testou positivo para COVID! Será Hopitalizado\n", minutos, id);
            sendMsg(29, id);

            pthread_mutex_lock(&trinco_p_internadosB);
            internados++;
            pthread_mutex_unlock(&trinco_p_internadosB);

            sleep(tempo_cura); // Simula o tempo que leva a curar ou falecer no pior dos casos

            if (testaProbabilidade(prob_cura)) // Testa a probabilidade de cura. Se o paciente se curar decrementa positivos e internados e incrementa curados
            {
                pthread_mutex_lock(&trinco_p_positivosB);
                positivos--;
                pthread_mutex_unlock(&trinco_p_positivosB);

                pthread_mutex_lock(&trinco_p_internadosB);
                internados--;
                pthread_mutex_unlock(&trinco_p_internadosB);

                pthread_mutex_lock(&trinco_p_curadosB);
                curados++;
                pthread_mutex_unlock(&trinco_p_curadosB);

                printf(RED "[Min: %d]" CNORMAL " O paciente %d do Hospital C curou-se do COVID!\n", minutos, id);
                sendMsg(30, id);
            }
            else // Se o paciente não se curar decrementa positivos e internados e incrementa mortes
            {
                pthread_mutex_lock(&trinco_p_positivosB);
                positivos--;
                pthread_mutex_unlock(&trinco_p_positivosB);

                pthread_mutex_lock(&trinco_p_internadosB);
                internados--;
                pthread_mutex_unlock(&trinco_p_internadosB);

                pthread_mutex_lock(&trinco_p_mortesB);
                mortes++;
                pthread_mutex_unlock(&trinco_p_mortesB);

                printf(RED "[Min: %d]" CNORMAL " Infelizmente O paciente %d do Hospital C sucumbiu ao COVID!\n", minutos, id);
                sendMsg(31, id);
            }
        }
        else // Caso contrário, resultado negativo, incrementa o nº de negativos para covid
        {
            pthread_mutex_lock(&trinco_p_negativosB);
            negativos++;
            pthread_mutex_unlock(&trinco_p_negativosB);

            printf(RED "[Min: %d]" CNORMAL " O paciente %d do Hospital C testou negativo para COVID! Reencaminhado para casa \n", minutos, id);
            sendMsg(32, id);
        }

        pthread_exit(NULL); //Termina thread
    }
}

void hospitalC() // Função Hospital A que irá receber pacientes do tipo A
{

    int min_atual;        // Variável que regista minuto atual de simulação
    int pessoas_testagem; // Variável que guarda o nº de pessoas em testagem
    int pessoas_prio_alta;
    int pessoas_prio_baixa;

    pthread_mutex_lock(&trinco_min); // Minuto atual = variável partilhada dos minutos de simulação
    min_atual = minutos;
    pthread_mutex_unlock(&trinco_min);

    pthread_mutex_lock(&trinco_p_filaespAlta);
    pessoas_prio_alta = pessoas_filaHospital_Alta;
    pthread_mutex_unlock(&trinco_p_filaespAlta);

    pthread_mutex_lock(&trinco_p_filaespBaixa);
    pessoas_prio_baixa = pessoas_filaHospital_Baixa;
    pthread_mutex_unlock(&trinco_p_filaespBaixa);

    while (min_atual <= tempo_simul) // Enquanto o tempo atual não ultrapassar o tempo de simulação
    {

        int iter; // Variável de apoio a loops

        pthread_mutex_lock(&trinco_p_filaespAlta);
        pessoas_prio_alta = pessoas_filaHospital_Alta; //Atualiza nº de pessoas em espera na fila de prioridade alta
        pthread_mutex_unlock(&trinco_p_filaespAlta);

        pthread_mutex_lock(&trinco_p_filaespBaixa);
        pessoas_prio_baixa = pessoas_filaHospital_Baixa; //Atualiza nº de pessoas em espera na fila de prioridade baixa
        pthread_mutex_unlock(&trinco_p_filaespBaixa);

        pthread_mutex_lock(&trinco_p_HospC);
        pessoas_testagem = pacientes_HospitalC; // Verifica quantas pessoas tem no centro de testagem
        pthread_mutex_unlock(&trinco_p_HospC);

        while (pessoas_testagem < enfermeiros) // Enquanto o centro de testagem não estiver preenchido (enquanto houverem enfermeiros disponíveis), aguarda pacientes
        {
            sem_wait(&sem_seguranca_testagem);
            pthread_mutex_lock(&trinco_p_HospC);
            pessoas_testagem = pacientes_HospitalC;
            pthread_mutex_unlock(&trinco_p_HospC);
            sem_post(&sem_seguranca_testagem);
        }

        sem_wait(&sem_resC); //Semáforo para simular o tempo de testagem

        printf(RED "[Min: %d]" CNORMAL " Os pacientes do Hospital C estão a ser testados!\n", minutos);
        sendMsg(38, 0);

        sleep(tempo_analise); // Pausar durante o tempo da testagem

        printf(RED "[Min: %d]" CNORMAL " Os pacientes do Hospital C foram testados!\n", minutos);
        sendMsg(39, 0);

        sem_post(&sem_resC);

        for (iter = 0; iter < enfermeiros; iter++) //Enquanto houverem pessoas dentro do centro de testagem esvazia todas essas pessoas, de forma a poderem sair do hospital
        {
            sem_post(&sem_testC);
        }

        pthread_mutex_lock(&trinco_p_HospC);
        pessoas_testagem = pacientes_HospitalC; //Atualiza nº de pessoas em testagem, porque as pessoas estão a sair do centro de testagem e a diminuir este valor
        pthread_mutex_unlock(&trinco_p_HospC);

        while (pessoas_testagem > 0) // Enquanto ainda houverem pessoas em centro de testagem, mais nenhuma ronda de testes se pode iniciar
        {
            pthread_mutex_lock(&trinco_p_HospC);
            pessoas_testagem = pacientes_HospitalC; // Aguarda que todasd as pessoas acabem os testes e saiam dos centros de testagem
            pthread_mutex_unlock(&trinco_p_HospC);
        }

        pthread_mutex_lock(&trinco_p_filaespAlta);
        pessoas_prio_alta = pessoas_filaHospital_Alta; //Atualiza nº de pessoas em espera na fila de prioridade alta
        pthread_mutex_unlock(&trinco_p_filaespAlta);

        pthread_mutex_lock(&trinco_p_filaespBaixa);
        pessoas_prio_baixa = pessoas_filaHospital_Baixa; //Atualiza nº de pessoas em espera na fila de prioridade baixa
        pthread_mutex_unlock(&trinco_p_filaespBaixa);

        for (iter = 0; iter < enfermeiros_alta; iter++)
        { // Após todos os pacientes sairem do centro de testagem, assinala que o centro de testagem está vazio
            // De forma a que as pessoas que estão a aguardar na fila de prioridade alta para entrar no centro de testagem possam avançar
            pthread_mutex_lock(&trinco_p_filaespAlta);
            pessoas_prio_alta = pessoas_filaHospital_Alta;
            pthread_mutex_unlock(&trinco_p_filaespAlta);

            pthread_mutex_lock(&trinco_p_filaespBaixa);
            pessoas_prio_baixa = pessoas_filaHospital_Baixa;
            pthread_mutex_unlock(&trinco_p_filaespBaixa);

            sem_post(&sem_HospAlta);
        }
        for (iter = 0; iter < enfermeiros_baixa; iter++)
        { // Após todos os pacientes sairem do centro de testagem, assinala que o centro de testagem está vazio
            // De forma a que as pessoas que estão a aguardar na fila de prioridade baixa para entrar no centro de testagem possam avançar
            pthread_mutex_lock(&trinco_p_filaespAlta);
            pessoas_prio_alta = pessoas_filaHospital_Alta;
            pthread_mutex_unlock(&trinco_p_filaespAlta);

            pthread_mutex_lock(&trinco_p_filaespBaixa);
            pessoas_prio_baixa = pessoas_filaHospital_Baixa;
            pthread_mutex_unlock(&trinco_p_filaespBaixa);

            sem_post(&sem_HospBaixa);
        }
        pthread_mutex_lock(&trinco_min);
        min_atual = minutos; //atualiza minuto atual de simulação
        pthread_mutex_unlock(&trinco_min);
    }

    pthread_exit(NULL); // Termina thread do hospital após terminar simulação
}

void sendMsg(int par, int msg) // Função para envio de mensagens relativas à simulação para o simulador (REGIAO CRÍTICA)
{
    pthread_mutex_lock(&trinco_socket);
    int com = 1;

    if (writen(sockfd, &com, sizeof(int)) != sizeof(int))
        err_dump("str_echo: writen error");

    if (writen(sockfd, &par, sizeof(int)) != sizeof(int))
        err_dump("str_echo: writen error");

    if (writen(sockfd, &msg, sizeof(int)) != sizeof(int))
        err_dump("str_echo: writen error");

    pthread_mutex_unlock(&trinco_socket);
}

void sendSimInfo(int par) // Função para envio de mensagens de estado de execução (REGIAO CRÍTICA)
{

    pthread_mutex_lock(&trinco_socket);
    int com = 0;

    if (writen(sockfd, &com, sizeof(int)) != sizeof(int))
        err_dump("str_echo: writen error");

    if (writen(sockfd, &par, sizeof(int)) != sizeof(int))
        err_dump("str_echo: writen error");

    pthread_mutex_unlock(&trinco_socket);
}

bool testaProbabilidade(char prob) // Testa as probabilidades
{
    if (prob > 100)
        prob = 100;

    char random = (rand() % 101); // Obter número entre 0 e 100

    if (random < prob)
        return true;

    return false;
}