# Sistemas Operativos - Terceira Fase - Simulação Rastreamento COVID-19

## Descrição do Projeto

> No âmbito da unidade curricular de Sistemas Operativos, foi proposto a implementação de um projeto que simulasse o funcionamento de uma unidade hospitalar destinada ao rastreio do vírus COVID-19.
> Desta forma, foi desenvolvida uma simulação que implementou os principais conceitos de Sistemas Operativos, conceitos estes que foram lecionados ao longo de todo o 1º Semestre do ano letivo 2020/2021.
> Objetivamente, a solução esperada teria que implementar comunicação entre processos distintos, leitura e escrita de ficheiros e o conceito de programação multitarefa (multithread) – e claro, todos os conceitos inerentes à mesma (tratamento de acessos concorrentes a dados partilhados).

## Dados personalizáveis pelo ficheiro de configuração

> Tempo de simulação (tempo_simul);
> Probabilidade de criar novo paciente (prob_pessoa);
> Probabilidade do novo paciente ser utente do Hospital A (prob_hospitalA);
> Probabilidade do novo paciente ser utente do Hospital B (prob_hospitalB);
> Probabilidade do novo paciente ser utente do Hospital C (prob_hospitalC);
> Probabilidade de ser um paciente de prioridade alta (prob_prioAlta);
> Probabilidade de ser um paciente de prioridade baixa (prob_prioBaixa);
> Probabilidade do paciente ser COVID-19 positivo (prob_positivo);
> Probabilidade do paciente se curar do COVID-19 (prob_cura);
> Tamanho das filas de espera para o centro de testagem (tamanho_fila);
> Número de enfermeiros por centro de testagem (enfermeiros);
> Tempo para obter o resultado das análises (tempo_analise);
> Tempo de internamento (tempo_cura);

## Instruções de Execução

> Antes de qualquer execução altere o UNIXSTR_PATH do ficheiro unix.h, ara a diretoria que lhe é correspondente: "/tmp/'o seu nº de aluno'";
> Em seguida basta executar o comando "make" na diretoria onde os ficheiros do projeto se encontram;
> Após ter feito o comando make, todos os ficheiros necessários à execução foram criados com sucesso;
> Para executar o programa, execute o comando "./simulador configurations.conf" numa das suas janelas para incializar o simulador;
> Na segunda janela execute "./monitor";
> Posto isto a simulação é inicializada e vai levar o tempo de simulação até terminar;
> As informações de simulação são guardadas no ficheiro relatorio.log;
> Se quiser eliminar os ficheiros executáveis e .o para recompilar os seus ficheiros, basta executar "make clean", e posteriormente "make" para recompilar;

## Programador

> Pedro Sousa
