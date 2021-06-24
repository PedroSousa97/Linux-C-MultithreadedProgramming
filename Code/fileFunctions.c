/*	Universidade da Madeira - Unidade Curricular de Sistemas Operativos 2020-2021
	Docentes: Eduardo Marques e Luis Gaspar
	Programador: Pedro Sousa 

	Biblioteca de funções para tratamento de operações com ficheiros

	Ficheiro: fileFunctions.c
*/

#include "fileFunctions.h"
#include "unix.h"

int contaLinhas(const char *ficheiro) //Função utilizada para contar o numero de linhas do ficheiro
{
    int linhas = 0; //Variável de apoio usada para guardar o numero de linhas do ficheiro

    FILE *fp;

    if ((fp = fopen(ficheiro, "r")) == NULL) // Abre o ficheiro para leitura ("r")
        return CODIGO_ERRO;                  // Retorna codigo de erro se houver erro a abrir o ficheiro para leitura

    char ch;                       // Variável que guarda o caracter
    while ((ch = getc(fp)) != EOF) // Enquanto não chegar ao fim do ficheiro lê o próximo caracter
    {
        if (ch == '\n') // Se o caracter lido for o de nova linha:
            linhas++;   // incrementa número de linhas
    }
    fclose(fp); // fecha o ficheiro

    return linhas; // devolve o número de linhas
}

char lerConfigurations(config **conf, const char *ficheiro) // Função utilizada para ler o ficheiro config
{
    int linhas = contaLinhas(ficheiro); // Numero de linhas a ler no ficheiro config
    if (linhas == 0)                    //Verifica se o ficheiro config está vazio, se sim print de aviso e exit do processo
    {
        printf("Ficheiro de configuração vazio! Por favor introduza as suas configurações.");
        exit(0);
    }

    config *file_conf = (config *)malloc(linhas * sizeof(config)); // Alocar espaço (malloc) para as configurações, neste caso 6 linhas * 2 atributos

    FILE *fp;

    if ((fp = fopen(ficheiro, "r")) == NULL)
    { // Abre o ficheiro para leitura ("r")
        printf("Nome do ficheiro de configuração introduzido não existe.\n\nPor favor execute o seguinte comando: ./simulador configurations.conf\n");
        exit(1);
    }
    char str[TAM_LINHA + 1]; // Aloca espaço para leitura de cada linha do ficheiro

    int i = 0; // Variável de apoio ao while

    while (fgets(str, TAM_LINHA, fp) != NULL) // lê uma linha e guarda em str (enquanto houver linhas para ler)
    {
        // se não retornar null continua o ciclo
        if (str[0] == '#')
        { //Se o primeiro char da string for um # significa que se trata de um comentário, passar ao próximo ciclo (continue)
            continue;
        }

        char *par = (char *)malloc(MAX_NAME + 1);  // Aloca espaço para a string do parâmetro
        char *dado = (char *)malloc(MAX_NAME + 1); // "               " a string de dados

        strcpy(par, strtok(str, " :\n"));   // copia para a string do parâmetro, o "token" antes dos ":"
        strcpy(dado, strtok(NULL, " :\n")); // "                               " o "token" depois dos ":"

        if (par != NULL && dado != NULL) // Se não for nullpointer (apontar para o vazio)...ou seja se apontar para alguma variável
        {
            if (atoi(dado) == 0) //Se o atributo dado estiver vazio, o seu valor int é 0...e para estes casos atribuimos valores default
            {
                //tratamento de erros / excepções - atribuição de valores default
                if (my_strcmp(par, "tempo_simul") == 0)
                {
                    file_conf[i].parametro = par;                                                                            // guarda o parâmetro lido do ficheiro config na nova configuração
                    file_conf[i].dados = "150";                                                                              // guarda o valor default do dado, para este tipo de parametro, na nova config
                    printf("O parâmetro %s não continha dados. Por esta razão foi atribuído o valor default: 150\n\n", par); //Avisa user da falta de dados e da atribuição de valor default
                }
                if (my_strcmp(par, "prob_pessoa") == 0)
                {
                    file_conf[i].parametro = par;                                                                            // guarda o parâmetro lido do ficheiro config na nova config
                    file_conf[i].dados = "50";                                                                               // guarda o valor default do dado, para este tipo de parametro, na nova config
                    printf("O parâmetro %s não continha dados. Por esta razão foi atribuí­do o valor default: 50\n\n", par); //Avisa user da falta de dados e da atribuição de valor default
                }
                if (my_strcmp(par, "prob_positivo") == 0)
                {
                    file_conf[i].parametro = par;                                                                            // guarda o parâmetro lido do ficheiro config na nova config
                    file_conf[i].dados = "20";                                                                               // guarda o valor default do dado, para este tipo de parametro, na nova config
                    printf("O parâmetro %s não continha dados. Por esta razão foi atribuí­do o valor default: 20\n\n", par); //Avisa user da falta de dados e da atribuição de valor default
                }
                if (my_strcmp(par, "tamanho_fila") == 0)
                {
                    file_conf[i].parametro = par;                                                                            // guarda o parâmetro lido do ficheiro config na nova config
                    file_conf[i].dados = "10";                                                                               // guarda o valor default do dado, para este tipo de parametro, na nova config
                    printf("O parâmetro %s não continha dados. Por esta razão foi atribuí­do o valor default: 10\n\n", par); //Avisa user da falta de dados e da atribuição de valor default
                }
                if (my_strcmp(par, "tempo_resultados") == 0)
                {
                    file_conf[i].parametro = par;                                                                           // guarda o parâmetro lido do ficheiro config na nova config
                    file_conf[i].dados = "5";                                                                               // guarda o valor default do dado, para este tipo de parametro, na nova config
                    printf("O parâmetro %s não continha dados. Por esta razão foi atribuí­do o valor default: 5\n\n", par); //Avisa user da falta de dados e da atribuição de valor default
                }
                if (my_strcmp(par, "tempo_cura") == 0)
                {
                    file_conf[i].parametro = par;                                                                           // guarda o parãmetro lido do ficheiro config na nova config
                    file_conf[i].dados = "5";                                                                               // guarda o valor default do dado, para este tipo de parametro, na nova config
                    printf("O parâmetro %s não continha dados. Por esta razão foi atribuí­do o valor default: 5\n\n", par); //Avisa user da falta de dados e da atribuição de valor default
                }
            }
            else //Se existerem valores no atributo dado, então:
            {
                file_conf[i].parametro = par; // guarda o parâmetro lido do ficheiro config na nova configuração
                file_conf[i].dados = dado;    // guarda o dado lido do ficheiro config na nova configuração
            }
        }
        i++; //incrementa variavel de apoio ao loop
    }

    fclose(fp); // Encerra/fecha o ficheiro

    *conf = file_conf; // Aponta o apontador de configuração para a nova configuração
    return (char)i;    // Devolve o número de configurações lidas (em formato char)
}

/*A função que se segue é importante à posteriori para calcular o indice de cada parametro e
para verificar se o parametro não foi incluido ficheiro config, ou se foi incluído mas está mal escrito ou outro erro do género.
Para estas situações, fizemos exception handling (através do valor de erro/return -1), e no processo do simulador, são atribuidos valores 
default para parâmetros inexistentes ou com defeito/erro */

char obterIndiceParametro(config *conf, char num_conf, const char *par) // Função que calcula o index de cada parâmetro guardado no ponteiro para variaveis de configuração
{

    int indexParametro;                                                   // Variável de apoio ao loop (for) e que igualmente representa o index do parâmetro (a ser procurado)
    for (indexParametro = 0; indexParametro < num_conf; indexParametro++) // Ciclo inicializado a 0 e que percorre todos os parâmetros de configuração existentes
    {
        if (strcmp(conf[indexParametro].parametro, par) == 0) // Se o parâmetro (a ser procurado) existe na lista de parâmetros de configuração, então:
        {
            return indexParametro; // Devolve o indice exato desse parâmetro
        }
    }

    return -1; // Se o parâmetro procurado não existir, devolve código de erro (representado pelo valor -1)
}

char abrirRelatorio(const char *ficheiro) // Função utilizada para abrir/criar ficheiro de relatório
{

    relatorio = fopen(ficheiro, "w"); // Abre/cria o ficheiro especificado em modo de escrita

    if (relatorio != NULL) // Se não houver erros a abrir/criar o ficheiro então retorna código de sucesso
        return CODIGO_SUCESSO;

    return CODIGO_ERRO; // Caso contrário retorna código de erro
}

char inserirRelatorio(const char *s) // Função utilizada para inserir dados no ficheiro de relatório
{
    int res = fputs(s, relatorio); // insere os dados passados no relatorio e a variavel "res" guarda o sucesso ou insucesso do processo de escrita no ficheiro

    if (res < 0) // Se res < 0 significa que houve um erro de escrita e devolve codigo de erro
        return CODIGO_ERRO;

    return CODIGO_SUCESSO; // Caso contrario devolve codigo de sucesso
}

void fecharRelatorio() // Função para fechar o ficheiro relatorio.log
{
    fclose(relatorio); // Fecha o ficheiro
}

int my_strcmp(char *strg1, char *strg2) // Função utilizada para comparar strings de dois apontadores do tipo char (compara caracter a caracter)
{
    while ((*strg1 != '\0' && *strg2 != '\0') && *strg1 == *strg2) // Enquanto nenhuma das strings chegar ao fim e o caracter comparado for igual:
    {
        strg1++; // incrementa os apontadores... ou seja apontam para o próximo caracter/posição de memória
        strg2++;
    }

    if (*strg1 == *strg2) // Chegando ao fim da palavra, se os caracteres apontados forem iguais significa que a palavra é igual e retorna codigo de sucesso
    {
        return CODIGO_SUCESSO;
    }

    else // Caso contrario retorna codigo de erro, ou seja as strings são diferentes
    {
        return CODIGO_ERRO;
    }
}