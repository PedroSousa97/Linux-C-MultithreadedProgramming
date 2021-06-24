

/*	Universidade da Madeira - Unidade Curricular de Sistemas Operativos 2020-2021
	Docentes: Eduardo Marques e Luis Gaspar
	Programador: Pedro Sousa 

	Processo representativo do monitor (client)

	Ficheiro: monitor.c
*/

// Bibliotecas
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "fileFunctions.h"
#include "unix.h"

#define MAXLINE 100 // Tamanho máximo de cada linha do buffer
#define MAXMSG 20   // Máximo de linhas ou ultimos eventos a serem imprimidos

// Códigos de cores para as letras
#define CNORMAL "\x1B[0m"
#define LIGHTRED "\x1B[91m"
#define LIGHTGREEN "\x1B[92m"
#define BLUE "\x1B[94m"
#define CYAN "\x1B[96m"
#define WHITE "\x1B[97m"
#define VIOLET "\x1B[35m"
#define YELLOW "\033[0;33m"
#define GREEN "\x1B[32m"

#define MAX_USERS 1000 // Número máximo de utilizadores a serem criados nesta simulação

char buffer_mens[MAXMSG][MAXLINE]; // Buffer que guardará os últimos MAXMSG (10) eventos

time_t tempo_inicial_sim;    // Variável que regista o tempo de início da simulação
time_t init_time[MAX_USERS]; // Array de timestamps (início/chegada) de casa user
time_t end_time[MAX_USERS];  // Array que guarda o timestamp de saí­da de cada user

// Variáveis globais para registar valores estatí­sticos da simulação
int nr_testes, nr_testesb, nr_testesc, nr_saidas_b, nr_saidas_a, nr_saidas_c, nr_des_a, nr_pacientesB, nr_pacientesA, nr_pacientesC;
int n_fila_hosp_a, n_fila_hosp_b, n_pacientes_HospA, n_pacientes_HospB, nr_a_fila, nr_b_fila, n_fila_hosp_c, n_pacientes_HospC, nr_c_fila;
int n_internados, nr_positivos, n_negativos, n_curados, n_mortes, n_enfermeiros;
int n_internados_b, nr_positivos_b, n_negativos_b, n_curados_b, n_mortes_b;
int n_internados_c, nr_positivos_c, n_negativos_c, n_curados_c, n_mortes_c;

char estado_sim = 0; // Variável que guarda o estado da simulação
int i;               // Variável de apoio a loop

// Assinaturas de funções
void rotina_monitor(FILE *fp, int sockfd);
void desenharEcra();
void mens_insert(char *str);
void mens_print();
float calc_temp();

main()
{
    int sockfd, servlen;
    struct sockaddr_un serv_addr;

    /* Cria socket stream */
    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        err_dump("client: can't open stream socket");

    /* Primeiro uma limpeza preventiva!
    Dados para o socket stream: tipo + nome do ficheiro.
    O ficheiro identifica o servidor */

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, UNIXSTR_PATH);
    servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);

    /* Tenta estabelecer uma ligação. Só funciona se o servidor tiver 
    sido lançado primeiro (o servidor tem de criar o ficheiro e associar
    o socket ao ficheiro) */

    if (connect(sockfd, (struct sockaddr *)&serv_addr, servlen) < 0)
        err_dump("client: can't connect to server");

    rotina_monitor(stdin, sockfd);

    /* Fecha o socket e termina */
    close(sockfd);
    exit(0);

    return 0;
}

void rotina_monitor(FILE *fp, int sockfd) //Função da rotina do monitor
{
    printf("Conectado ao Simulador\n");

    int com, par1, par2; // Variáveis usadas para receber parâmetros/mensagens da socket

    abrirRelatorio("relatorio.log"); // Abre o ficheiro de relatório para posterior escrita dos dados de simulação

    for (;;) // Enquanto o simulador rodar (for vazio)
    {
        int user, acao; // Variáveis usadas para receber comandos de simulação e id do user via socket

        if (readn(sockfd, (char *)&com, sizeof(int)) != sizeof(int))
            err_dump("erro: leitura do tipo de mensagem");

        if (com == 0)
        {

            // Comandos para o Monitor
            if (readn(sockfd, (char *)&par1, sizeof(int)) != sizeof(int))
                err_dump("erro: leitura do comando");

            if (par1 == 1)
            {
                // Inicio de simulação
                estado_sim = 1;
                tempo_inicial_sim = time(NULL);
                inserirRelatorio("Tempo\tUser\tEvento\n");
                desenharEcra();
            }

            if (par1 == 2) //Fim simulação
            {
                estado_sim = 2;
                desenharEcra();
                fecharRelatorio();

                return;
            }
        }

        if (com == 1)
        {
            // Comandos da Simulação
            if (readn(sockfd, (char *)&par1, sizeof(int)) != sizeof(int))
                err_dump("erro: leitura do primeiro par");

            if (readn(sockfd, (char *)&par2, sizeof(int)) != sizeof(int))
                err_dump("erro: leitura do segundo par");

            int user, acao;

            user = par2; // Guarda em variáveis os parâmetros "acao" e "userid" enviados pela socket
            acao = par1;

            // Buffers que irão guardar as mensagens a serem inseridas (tanto no ecrã como no relatório)
            char temp_str[MAXLINE];
            char log_str[MAXLINE];

            switch (acao) //Para cada código de ação corresponde o seu print de monitor
            {
            case 201:
                n_enfermeiros = user; // Código usado para guardar o número de enfermeiros de cada hospital

                break;

            case 1: //Para cada caso incrementa as variáveis associadas
                nr_pacientesA++;
                nr_a_fila++;

                init_time[user - 1] = time(NULL); //guarda o tempo de chegada do user

                sprintf(temp_str, "O paciente " YELLOW "%d " LIGHTGREEN "chegou" CNORMAL " ao Hospital A! \n", user);
                sprintf(log_str, "%d\t%d\tChegou ao Hospital A\n", time(NULL) - tempo_inicial_sim, user);

                inserirRelatorio(log_str);
                mens_insert(temp_str);

                break;

            case 2:

                nr_pacientesB++;
                nr_b_fila++;

                init_time[user - 1] = time(NULL); //guarda o tempo de chegada do user

                sprintf(temp_str, "O paciente " YELLOW "%d " LIGHTGREEN "chegou" CNORMAL " ao Hospital B! \n", user);
                sprintf(log_str, "%d\t%d\tChegou ao Hospital B\n", time(NULL) - tempo_inicial_sim, user);

                inserirRelatorio(log_str);
                mens_insert(temp_str);

                break;

            case 3:
                sprintf(temp_str, "O paciente " YELLOW "%d " LIGHTGREEN "entrou" CNORMAL " na fila do Hospital B! \n", user);
                sprintf(log_str, "%d\t%d\tEntrou na fila do Hospital B \n", time(NULL) - tempo_inicial_sim, user);
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                n_fila_hosp_b++;
                nr_b_fila--;

                break;

            case 4:
                sprintf(temp_str, "O paciente " YELLOW "%d do Hospital A " LIGHTRED "desistiu" CNORMAL "! \n", user);
                sprintf(log_str, "%d\t%d\tDesistiu \n", time(NULL) - tempo_inicial_sim, user);
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                nr_des_a++;
                nr_a_fila--;
                break;

            case 5:
                sprintf(temp_str, "O paciente " YELLOW "%d " LIGHTGREEN "entrou" CNORMAL " no centro de testagem do Hospital B! \n", user);
                sprintf(log_str, "%d\t%d\tEntrou no Hospital B \n", time(NULL) - tempo_inicial_sim, user);
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                n_fila_hosp_b--;
                n_pacientes_HospB++;
                break;

            case 6:
                sprintf(temp_str, "O paciente " YELLOW "%d " LIGHTGREEN "entrou" CNORMAL " na fila do Hospital A! \n", user);
                sprintf(log_str, "%d\t%d\tEntrou na fila do Hospital A \n", time(NULL) - tempo_inicial_sim, user);
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                n_fila_hosp_a++;
                nr_a_fila--;

                break;

            case 7:
                sprintf(temp_str, "O paciente " YELLOW "%d " LIGHTRED "saiu" CNORMAL " do centro de testagem do Hospital B! \n", user);
                sprintf(log_str, "%d\t%d\tSaiu do Hospital B \n", time(NULL) - tempo_inicial_sim, user);
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                n_pacientes_HospB--;
                break;
            case 8:
                sprintf(temp_str, "O paciente " YELLOW "%d " LIGHTRED "saiu" CNORMAL " do Hospital B! \n", user);
                sprintf(log_str, "%d\t%d\tO paciente saiu do Hospital B\n", time(NULL) - tempo_inicial_sim, user);
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                end_time[user - 1] = time(NULL); //Guarda o tempo de saí­da do user
                nr_saidas_b++;
                break;
            case 9:
                sprintf(temp_str, "Os pacientes do Hospital A estão a ser " LIGHTGREEN "testados!" CNORMAL "\n", user);
                sprintf(log_str, "%d\t%s\tOs pacientes do Hospital A estÃ£o a ser testados \n", time(NULL) - tempo_inicial_sim, "-");
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                break;

            case 10:
                nr_testes = nr_testes + n_enfermeiros;
                sprintf(temp_str, "Os pacientes do Hospital A foram " LIGHTGREEN "testados!" CNORMAL "\n", user);
                sprintf(log_str, "%d\t%s\tOs pacientes do Hospital A foram testados \n", time(NULL) - tempo_inicial_sim, "-");
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                break;

            case 11:
                sprintf(temp_str, "O paciente " YELLOW "%d " LIGHTGREEN "entrou" CNORMAL " no centro de testagem do Hospital A! \n", user);
                sprintf(log_str, "%d\t%d\tEntrou no Hospital A \n", time(NULL) - tempo_inicial_sim, user);
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                n_fila_hosp_a--;
                n_pacientes_HospA++;
                break;

            case 12:
                sprintf(temp_str, "O paciente " YELLOW "%d do Hospital B testou " LIGHTRED "positivo" CNORMAL " para COVID! SerÃ¡ hospitalizado. \n", user);
                sprintf(log_str, "%d\t%d\tPositivo para COVID. Foi Internado \n", time(NULL) - tempo_inicial_sim, user);
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                n_internados_b++;
                nr_positivos_b++;
                break;

            case 13:
                sprintf(temp_str, "O paciente " YELLOW "%d " LIGHTRED "saiu" CNORMAL " do centro de testagem do Hospital A! \n", user);
                sprintf(log_str, "%d\t%d\tSaiu do Hospital A \n", time(NULL) - tempo_inicial_sim, user);
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                n_pacientes_HospA--;
                break;

            case 14:
                sprintf(temp_str, "O paciente " YELLOW "%d do Hospital B " LIGHTGREEN "curou-se" CNORMAL " do COVID! \n", user);
                sprintf(log_str, "%d\t%d\tCurou-se do COVID \n", time(NULL) - tempo_inicial_sim, user);
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                n_internados_b--;
                nr_positivos_b--;
                n_curados_b++;
                break;

            case 15:
                sprintf(temp_str, "O paciente " YELLOW "%d do Hospital B " LIGHTRED "sucumbiu" CNORMAL " ao COVID!\n", user);
                sprintf(log_str, "%d\t%d\tFaleceu com COVID \n", time(NULL) - tempo_inicial_sim, user);
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                n_internados_b--;
                nr_positivos_b--;
                n_mortes_b++;
                break;

            case 16:
                sprintf(temp_str, "O paciente " YELLOW "%d " LIGHTRED "saiu" CNORMAL " do Hospital A! \n", user);
                sprintf(log_str, "%d\t%d\tO paciente saiu do Hospital A \n", time(NULL) - tempo_inicial_sim, user);
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                end_time[user - 1] = time(NULL); //Guarda o tempo de saí­da do user
                nr_saidas_a++;
                break;

            case 17:
                sprintf(temp_str, "O paciente " YELLOW "%d do Hospital A testou " LIGHTRED "positivo" CNORMAL " para COVID! SerÃ¡ hospitalizado. \n", user);
                sprintf(log_str, "%d\t%d\tPositivo para COVID. Foi Internado \n", time(NULL) - tempo_inicial_sim, user);
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                n_internados++;
                nr_positivos++;
                break;

            case 18:
                sprintf(temp_str, "O paciente " YELLOW "%d do Hospital A " LIGHTGREEN "curou-se" CNORMAL " do COVID! \n", user);
                sprintf(log_str, "%d\t%d\tCurou-se do COVID \n", time(NULL) - tempo_inicial_sim, user);
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                n_internados--;
                nr_positivos--;
                n_curados++;
                break;

            case 19:
                sprintf(temp_str, "O paciente " YELLOW "%d do Hospital A " LIGHTRED "sucumbiu" CNORMAL " ao COVID!\n", user);
                sprintf(log_str, "%d\t%d\tFaleceu com COVID \n", time(NULL) - tempo_inicial_sim, user);
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                n_internados--;
                nr_positivos--;
                n_mortes++;
                break;

            case 20:
                sprintf(temp_str, "O paciente " YELLOW "%d do Hospital A testou " LIGHTGREEN "negativo" CNORMAL " para COVID! Será enviado para casa.\n", user);
                sprintf(log_str, "%d\t%d\tNegativo para COVID. Reencaminhado para casa. \n", time(NULL) - tempo_inicial_sim, user);
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                n_negativos++;
                break;

            case 21:
                sprintf(temp_str, "O paciente " YELLOW "%d do Hospital B testou " LIGHTGREEN "negativo" CNORMAL " para COVID! Será enviado para casa.\n", user);
                sprintf(log_str, "%d\t%d\tNegativo para COVID. Reencaminhado para casa. \n", time(NULL) - tempo_inicial_sim, user);
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                n_negativos_b++;
                break;
            case 22:
                sprintf(temp_str, "Os pacientes do Hospital B estão a ser " LIGHTGREEN "testados!" CNORMAL "\n", user);
                sprintf(log_str, "%d\t%s\tOs pacientes do Hospital B estão a ser testados \n", time(NULL) - tempo_inicial_sim, "-");
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                break;

            case 23:
                nr_testesb = nr_testesb + n_enfermeiros;
                sprintf(temp_str, "Os pacientes do Hospital B foram " LIGHTGREEN "testados!" CNORMAL "\n", user);
                sprintf(log_str, "%d\t%s\tOs pacientes do Hospital B foram testados \n", time(NULL) - tempo_inicial_sim, "-");
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                break;
            case 24: //Para cada caso incrementa as variáveis associadas
                nr_pacientesC++;
                nr_c_fila++;

                init_time[user - 1] = time(NULL); //guarda o tempo de chegada do user

                sprintf(temp_str, "O paciente " YELLOW "%d " CNORMAL "de prioridade alta " LIGHTGREEN "chegou" CNORMAL " ao Hospital C! \n", user);
                sprintf(log_str, "%d\t%d\t[Prioridade Alta] Chegou ao Hospital C\n", time(NULL) - tempo_inicial_sim, user);

                inserirRelatorio(log_str);
                mens_insert(temp_str);

                break;

            case 25:

                sprintf(temp_str, "O paciente " YELLOW "%d " CNORMAL "de prioridade alta " LIGHTGREEN "entrou" CNORMAL " na fila do Hospital C! \n", user);
                sprintf(log_str, "%d\t%d\t[Prioridade Alta] Entrou na fila do Hospital C \n", time(NULL) - tempo_inicial_sim, user);
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                n_fila_hosp_c++;
                nr_c_fila--;

                break;

            case 26:
                sprintf(temp_str, "O paciente " YELLOW "%d " CNORMAL "de prioridade alta " LIGHTGREEN "entrou" CNORMAL " no centro de testagem do Hospital C! \n", user);
                sprintf(log_str, "%d\t%d\t[Prioridade Alta] Entrou no Hospital C \n", time(NULL) - tempo_inicial_sim, user);
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                n_fila_hosp_c--;
                n_pacientes_HospC++;
                break;

            case 27:
                sprintf(temp_str, "O paciente " YELLOW "%d " CNORMAL "de prioridade alta " LIGHTRED "saiu" CNORMAL " do centro de testagem do Hospital C! \n", user);
                sprintf(log_str, "%d\t%d\t[Prioridade Alta] Saiu do Hospital C \n", time(NULL) - tempo_inicial_sim, user);
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                n_pacientes_HospC--;
                break;

            case 28:
                sprintf(temp_str, "O paciente " YELLOW "%d " CNORMAL "de prioridade alta " LIGHTRED "saiu" CNORMAL " do Hospital C! \n", user);
                sprintf(log_str, "%d\t%d\tO paciente de prioridade alta saiu do Hospital C\n", time(NULL) - tempo_inicial_sim, user);
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                end_time[user - 1] = time(NULL); //Guarda o tempo de saí­da do user
                nr_saidas_c++;
                break;

            case 29:
                sprintf(temp_str, "O paciente " YELLOW "%d do Hospital C testou " LIGHTRED "positivo" CNORMAL " para COVID! Será hospitalizado. \n", user);
                sprintf(log_str, "%d\t%d\tPositivo para COVID. Foi Internado \n", time(NULL) - tempo_inicial_sim, user);
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                n_internados_c++;
                nr_positivos_c++;
                break;

            case 30:
                sprintf(temp_str, "O paciente " YELLOW "%d do Hospital C " LIGHTGREEN "curou-se" CNORMAL " do COVID! \n", user);
                sprintf(log_str, "%d\t%d\tCurou-se do COVID \n", time(NULL) - tempo_inicial_sim, user);
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                n_internados_c--;
                nr_positivos_c--;
                n_curados_c++;
                break;
            case 31:
                sprintf(temp_str, "O paciente " YELLOW "%d do Hospital C " LIGHTRED "sucumbiu" CNORMAL " ao COVID!\n", user);
                sprintf(log_str, "%d\t%d\tFaleceu com COVID \n", time(NULL) - tempo_inicial_sim, user);
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                n_internados_c--;
                nr_positivos_c--;
                n_mortes_c++;
                break;
            case 32:
                sprintf(temp_str, "O paciente " YELLOW "%d do Hospital C testou " LIGHTGREEN "negativo" CNORMAL " para COVID! Será enviado para casa.\n", user);
                sprintf(log_str, "%d\t%d\tNegativo para COVID. Reencaminhado para casa. \n", time(NULL) - tempo_inicial_sim, user);
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                n_negativos_c++;
                break;

            case 33:
                nr_pacientesC++;
                nr_c_fila++;

                init_time[user - 1] = time(NULL); //guarda o tempo de chegada do user

                sprintf(temp_str, "O paciente " YELLOW "%d " CNORMAL "de prioridade baixa " LIGHTGREEN "chegou" CNORMAL " ao Hospital C! \n", user);
                sprintf(log_str, "%d\t%d\t[Prioridade Baixa] Chegou ao Hospital C\n", time(NULL) - tempo_inicial_sim, user);

                inserirRelatorio(log_str);
                mens_insert(temp_str);

                break;

            case 34:
                sprintf(temp_str, "O paciente " YELLOW "%d " CNORMAL "de prioridade baixa " LIGHTGREEN "entrou" CNORMAL " na fila do Hospital C! \n", user);
                sprintf(log_str, "%d\t%d\t[Prioridade Baixa] Entrou na fila do Hospital C \n", time(NULL) - tempo_inicial_sim, user);
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                n_fila_hosp_c++;
                nr_c_fila--;

                break;

            case 35:
                sprintf(temp_str, "O paciente " YELLOW "%d " CNORMAL "de prioridade baixa " LIGHTGREEN "entrou" CNORMAL " no centro de testagem do Hospital C! \n", user);
                sprintf(log_str, "%d\t%d\t[Prioridade Baixa] Entrou no Hospital C \n", time(NULL) - tempo_inicial_sim, user);
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                n_fila_hosp_c--;
                n_pacientes_HospC++;
                break;

            case 36:
                sprintf(temp_str, "O paciente " YELLOW "%d " CNORMAL "de prioridade baixa " LIGHTRED "saiu" CNORMAL " do centro de testagem do Hospital C! \n", user);
                sprintf(log_str, "%d\t%d\t[Prioridade Baixa] Saiu do Hospital C \n", time(NULL) - tempo_inicial_sim, user);
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                n_pacientes_HospC--;
                break;

            case 37:
                sprintf(temp_str, "O paciente " YELLOW "%d " CNORMAL "de prioridade baixa " LIGHTRED "saiu" CNORMAL " do Hospital C! \n", user);
                sprintf(log_str, "%d\t%d\tO paciente de prioridade baixa saiu do Hospital C\n", time(NULL) - tempo_inicial_sim, user);
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                end_time[user - 1] = time(NULL); //Guarda o tempo de saí­da do user
                nr_saidas_c++;
                break;

            case 38:
                sprintf(temp_str, "Os pacientes do Hospital C estão a ser " LIGHTGREEN "testados!" CNORMAL "\n", user);
                sprintf(log_str, "%d\t%s\tOs pacientes do Hospital C estão a ser testados \n", time(NULL) - tempo_inicial_sim, "-");
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                break;

            case 39:
                nr_testesc = nr_testesc + n_enfermeiros;
                sprintf(temp_str, "Os pacientes do Hospital C foram " LIGHTGREEN "testados!" CNORMAL "\n", user);
                sprintf(log_str, "%d\t%s\tOs pacientes do Hospital C foram testados \n", time(NULL) - tempo_inicial_sim, "-");
                inserirRelatorio(log_str);
                mens_insert(temp_str);
                break;
            }

            desenharEcra(); //Após cada ciclo volta a imprimir os dados no ecrã, neste caso, os dados de simulação os úlltimos 10 eventos
        }
    }
}

void desenharEcra() //Função para desenhar no ecrã
{
    system("clear");

    if (estado_sim == 1)
    {
        printf("Estado Atual: " LIGHTGREEN "Execução!\n" CNORMAL);
    }
    else if (estado_sim == 2)
    {
        printf("Estado Atual: " LIGHTRED "Terminada!\n" CNORMAL);
    }
    else
    {
        printf("Estado Atual: Indeterminado!\n");
    }

    printf(LIGHTRED "-> " WHITE "Utilizadores:" CYAN " [HOSPITAL A] " WHITE "total pacientes:" BLUE " %d" CNORMAL "\n", nr_pacientesA);
    printf(LIGHTRED "-> " WHITE "Utilizadores:" VIOLET " [HOSPITAL B] " WHITE "total pacientes:" BLUE " %d" CNORMAL "\n", nr_pacientesB);
    printf(LIGHTRED "-> " WHITE "Utilizadores:" GREEN " [HOSPITAL C] " WHITE "total pacientes:" BLUE " %d" CNORMAL "\n", nr_pacientesC);
    printf(LIGHTRED "-> " WHITE "Utilizadores:" WHITE " total pacientes:" BLUE " %d" CNORMAL "\n", (nr_pacientesA + nr_pacientesB + nr_pacientesC));
    printf(LIGHTRED "-> " WHITE "Desistências:" WHITE " total:" BLUE " %d" CNORMAL " \n", nr_des_a);
    printf(LIGHTRED "-->" CYAN "[HOSPITAL A] " WHITE "Pacientes a aguardar lugar na fila de espera:" BLUE " %d\n" CNORMAL, nr_a_fila);
    printf(LIGHTRED "-->" VIOLET "[HOSPITAL B] " WHITE "Pacientes a aguardar lugar na fila de espera:" BLUE " %d\n" CNORMAL, nr_b_fila);
    printf(LIGHTRED "-->" GREEN "[HOSPITAL C] " WHITE "Pacientes a aguardar lugar na fila de espera:" BLUE " %d\n" CNORMAL, nr_c_fila);
    printf(LIGHTRED "-> " WHITE "Número total de testes realizados:" BLUE " %d" CNORMAL " \n", (nr_testes + nr_testesb + nr_testesc));
    printf(LIGHTRED "-> " WHITE "Número de casos ativos para COVID:" BLUE " %d" CNORMAL " \n", (nr_positivos + nr_positivos_b + nr_positivos_c));
    printf(LIGHTRED "-> " WHITE "Número de testes negativos para COVID:" BLUE " %d" CNORMAL " \n", (n_negativos + n_negativos_b + n_negativos_c));
    printf(LIGHTRED "-> " WHITE "Número de internados:" BLUE " %d" CNORMAL " \n", (n_internados + n_internados_b + n_internados_c));
    printf(LIGHTRED "-> " WHITE "Número de pacientes curados:" BLUE " %d" CNORMAL " \n", (n_curados + n_curados_b + n_curados_c));
    printf(LIGHTRED "-> " WHITE "Número de mortes por COVID:" BLUE " %d" CNORMAL " \n", (n_mortes + n_mortes_b + n_mortes_c));

    printf(LIGHTRED "@@" YELLOW " TEMPO MÉDIO DE ESPERA: " BLUE "%.2f" WHITE " minutos. \n" CNORMAL, calc_temp());

    printf("\nOs ultimos %d eventos: \n", MAXMSG);
    mens_print(); // Imprime os últimos MAXMSG eventos guardados no buffer
}

float calc_temp() // Calcula tempo
{
    int soma = 0;
    int div = 0;
    for (i = 0; i < (nr_pacientesB + nr_pacientesA + nr_pacientesC); i++) // Para cada user que chegou e saiu, calcula todos os tempos (chegada -> saí­da) e incrementa div para poder calcular média
    {
        if (end_time[i] != NULL)
        {
            soma += (end_time[i] - init_time[i]);
            div++;
        }
    }

    if ((nr_saidas_b + nr_saidas_a + nr_saidas_c) > 0) //Se já houve saí­das, calcula a média (total de tempos/total de saidas)
    {
        return (float)soma / div;
    }
    else
    {
        return 0;
    }
}

void mens_insert(char *str) // Insere dados no buffer
{
    for (i = 0; i < MAXMSG; i++)
    {
        strcpy(buffer_mens[i], buffer_mens[i + 1]);
    }
    strcpy(buffer_mens[MAXMSG - 1], str);
}

void mens_print() // Imprime dados do buffer
{
    for (i = 0; i < MAXMSG; i++)
    {
        if (buffer_mens[i] != NULL)
            printf(buffer_mens[i]);
    }
}
