/*	Universidade da Madeira - Cadeira de Sistemas Operativos 2020-2021
	Docentes: Eduardo Marques e Luis Gaspar
	Programador: Pedro Sousa 

	Biblioteca de funções para tratamento de operações com ficheiros

	Ficheiro: fileFunctions.h
*/

#ifndef FILEFUNCTIONS_H
#define FILEFUNCTIONS_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Constantes

#define TAM_LINHA 50 // Tamanho predefinido para uma linha do ficheiro config
#define MAX_NAME 20	 // Tamanho maximo para um parametro e para um dado do ficheiro config

#define CODIGO_SUCESSO 0 // Codigo de sucesso na leitura ou escrita do ficheiro
#define CODIGO_ERRO 1	 // Codigo de erro na leitura ou escrita do ficheiro

// Tipo de estrutura criado e utilizado para armazenar os atributos do ficheiro config

typedef struct config
{
	char *parametro; //apontador do tipo char que armazena o parametro lido
	char *dados;	 //apontador do tipo char que armazena o dado lido
} config;

FILE *relatorio; //Apontador para o ficheiro de relatório

// Assinaturas de métodos

char obterIndiceParametro(config *conf, char num_conf, const char *par);
char lerConfigurations(config **conf, const char *ficheiro);
int contaLinhas(const char *ficheiro);
char abrirRelatorio(const char *ficheiro);
char inserirRelatorio(const char *s);
void fecharRelatorio();
int my_strcmp(char *strg1, char *strg2);

#endif