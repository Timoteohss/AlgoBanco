#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>

#define tipoQuantidadeRecurso 3
#define quantidadeProcessos 5

int i = 0;
int j = 0;

//Lock Mutex para variaveis globais
pthread_mutex_t mutex;

//estruturas dos processos
int vetorRecursosDisponiveis [tipoQuantidadeRecurso];
int matrixAlocacao [quantidadeProcessos][tipoQuantidadeRecurso] = {{1,1,0},{1,3,0},{0,0,2},{0,1,1},{0,2,0}};
int matrixMaxima [quantidadeProcessos][tipoQuantidadeRecurso] = {{5,5,5},{3,3,6},{3,5,3},{7,1,4},{7,2,2}};
int matrixPedidos [quantidadeProcessos][tipoQuantidadeRecurso];

int requisitaRecurso(int idProcesso,int vetorPedido[]);
int desaegaRecurso(int idProcesso,int vetorDesapega[]);
int seMaiorQueNecessario(int idProcesso,int vetorPedido[]);
int sePodeDesapegar(int idProcesso, int vetorDesapega[]);
int seModoSeguro();
int seSuficienteParaDesapegar(int []);
void imprimeMatrixDePedidos();
void imprimeMatrixAlocacao();
void imprimeDisponiveis();
void imprimeVetor(int vec[]);
void *cliente(void* idCliente);

int main(){
	//quantidade de recursos disponiveis
	int argv[] = {8,5,3};
	for(i = 0; i < tipoQuantidadeRecurso; i++){
		vetorRecursosDisponiveis[i] = argv[i];
	}
	
	//inicializa matrizPedidos
	for (i = 0; i < quantidadeProcessos; ++i){
		for (j = 0; j < tipoQuantidadeRecurso; ++j){
			matrixPedidos[i][j] = matrixMaxima[i][j] - matrixAlocacao[i][j];
		}
	}
	printf("Vetor de disponibilidade inicial:\n");
	imprimeDisponiveis();
	printf("Alocacao inicial da matrix:\n");
	imprimeMatrixAlocacao();
	printf("Matrix de necessidade inicial\n");
	imprimeMatrixDePedidos();

	
	//Inicia Multi Thread
	pthread_mutex_init(&mutex,NULL);
	pthread_attr_t attrDefault;
	pthread_attr_init(&attrDefault);
	pthread_t *tid = (pthread_t*)malloc(sizeof(pthread_t) * quantidadeProcessos);
	//id do cliente
	int *pid = (int*)malloc(sizeof(int)*quantidadeProcessos);
	
	//inicializa pid e cria threads
	for(i = 0; i < quantidadeProcessos; i++){
		*(pid + i) = i;
		pthread_create((tid+i), &attrDefault, cliente, (pid+i));
	}
	
	//join
	for(i = 0; i < quantidadeProcessos; i++){
		pthread_join(*(tid+i),NULL);
	}
	return 0;
}

//Simula pedidos e desapegos
void *cliente(void* idCliente){

	//pega um id de um cliente
	int idProcesso = *(int*)idCliente;
	int contador = 2;
	while(contador--){
		sleep(1);
		//requisita um numero aleatorio de recursos
		int vetorPedido[tipoQuantidadeRecurso];
		//dá um lock no mutex para acessar a variavel global
		pthread_mutex_lock(&mutex);
		
		for(i = 0; i < tipoQuantidadeRecurso; i++){
			if(matrixPedidos[idProcesso][i] != 0){
				vetorPedido[i] = rand() % matrixPedidos[idProcesso][i];
			}
			else{
				vetorPedido[i] = 0;
			}
		}
		printf("\nProcesso %d Pedindo recursos:\n",idProcesso);
		imprimeVetor(vetorPedido);
		requisitaRecurso(idProcesso,vetorPedido);
		//solta o mutex
		pthread_mutex_unlock(&mutex);
		
		//desapega de um numero aleatorio de recursos e espera um segundo
		sleep(1);
		int vetorDesapega[tipoQuantidadeRecurso];

		//lock mutex
		pthread_mutex_lock(&mutex);

		//inicializa vetorDesapega
		for(i = 0; i < tipoQuantidadeRecurso; i++){
			if(matrixAlocacao[idProcesso][i] != 0){
				vetorDesapega[i] = rand() % matrixAlocacao[idProcesso][i];
			}
			else{
				vetorDesapega[i] = 0;
			}
		}
		printf("\nProcesso %d Desapegando de recursos:\n",idProcesso);
		imprimeVetor(vetorDesapega);
		desaegaRecurso(idProcesso,vetorDesapega);
		
		pthread_mutex_unlock(&mutex);
	}
	return NULL;
}

//aloca recursos para um processo
int requisitaRecurso(int idProcesso,int vetorPedido[]){
	
	if (seMaiorQueNecessario(idProcesso,vetorPedido) == -1){
		printf("quantidade pedida de recursos maior que a necessaria.\n");
		return -1;
	}
	printf("Simula Alocacao\n");

	if(seSuficienteParaDesapegar(vetorPedido) == -1){
		printf("Sem Recursos suficientes...\n");
		return -1;
	}

	//finge alocacao
	for (i = 0; i < tipoQuantidadeRecurso; ++i){
		matrixPedidos[idProcesso][i] -= vetorPedido[i];
		matrixAlocacao[idProcesso][i] += vetorPedido[i];
		vetorRecursosDisponiveis[i] -= vetorPedido[i];
	}
	
	//verifica status
	if (seModoSeguro() == 0){
		printf("Seguro. Alocamento foi um sucesso.\nRecursos disponiveis:\n");
		imprimeDisponiveis();
		printf("Matriz de alocacao:\n");
		imprimeMatrixAlocacao();
		printf("Matrix de necessidade:\n");
		imprimeMatrixDePedidos();
		return 0;
	}
	else{
		printf("Voltando...\n");
		for (i = 0; i < tipoQuantidadeRecurso; ++i){
			matrixPedidos[idProcesso][i] += vetorPedido[i];
			matrixAlocacao[idProcesso][i] -= vetorPedido[i];
			vetorRecursosDisponiveis[i] += vetorPedido[i];
		}
		return -1;
	}
}

int desaegaRecurso(int idProcesso,int vetorDesapega[]){
	if(sePodeDesapegar(idProcesso,vetorDesapega) == -1){
		printf("Processo nao possui recursos suficientes\n");
		return -1;
	}

	//enough to release
	for(i = 0; i < tipoQuantidadeRecurso; i++){
		matrixAlocacao[idProcesso][i] -= vetorDesapega[i];
		matrixPedidos[idProcesso][i] += vetorDesapega[i];
		vetorRecursosDisponiveis[i] += vetorDesapega[i];
	}
	printf("Desapegado!\nRecursos disponiveis:\n");
	imprimeDisponiveis();
	printf("Matriz de alocacao:\n");
	imprimeMatrixAlocacao();
	printf("Matrix de necessidade:\n");
	imprimeMatrixDePedidos();
	return 0;
}

int sePodeDesapegar(int idProcesso,int vetorDesapega[]){
	for (i = 0; i < tipoQuantidadeRecurso; ++i){
		if (vetorDesapega[i] <= matrixAlocacao[idProcesso][i]){
			continue;
		}
		else{
			return -1;
		}
	}
	return 0;
}

int seMaiorQueNecessario(int idProcesso,int vetorPedido[]){
	for (i = 0; i < tipoQuantidadeRecurso; ++i){
		if (vetorPedido[i] <= matrixPedidos[idProcesso][i]){
			continue;
		}
		else{
			return -1;
		}
	}
	return 0;
}

int seSuficienteParaDesapegar(int vetorPedido[]){
	for (i = 0; i < tipoQuantidadeRecurso; ++i){
		if (vetorPedido[i] <= vetorRecursosDisponiveis[i]){
			continue;
		}
		else{
			return -1;
		}
	}
	return 0;
}

void imprimeMatrixDePedidos(){
	for (i = 0; i < quantidadeProcessos; ++i){
		printf("{ ");
		for (j = 0; j < tipoQuantidadeRecurso; ++j){
			printf("%d, ", matrixPedidos[i][j]);
		}
		printf("}\n");
	}
	return;
}

void imprimeMatrixAlocacao(){
	for (i = 0; i < quantidadeProcessos; ++i){
		printf("{ ");
		for (j = 0; j < tipoQuantidadeRecurso; ++j){
			printf("%d, ", matrixAlocacao[i][j]);
		}
		printf("}\n");
	}
	return;
}

void imprimeDisponiveis(){
	for (i = 0; i < tipoQuantidadeRecurso; ++i){
		printf("%d, ",vetorRecursosDisponiveis[i]);
	}
	printf("\n");
	return;
}

void imprimeVetor(int vec[]){
	for (i = 0; i < tipoQuantidadeRecurso; ++i){
		printf("%d, ",vec[i]);
	}
	printf("\n");
	return;
}

int seModoSeguro(){
	int vetAcabou[quantidadeProcessos] = {0};
	int trabalho[tipoQuantidadeRecurso];
	for(i = 0; i < tipoQuantidadeRecurso; i++){
		trabalho[i] = vetorRecursosDisponiveis[i];
	}
	int k;
	for(i = 0; i < quantidadeProcessos; i++){
		if (vetAcabou[i] == 0){
			for(j = 0; j < tipoQuantidadeRecurso; j++){
				if(matrixPedidos[i][j] <= trabalho[j]){
					if(j == tipoQuantidadeRecurso - 1){
						vetAcabou[i] = 1;
						for (k = 0; k < tipoQuantidadeRecurso; ++k){
							trabalho[k] += matrixAlocacao[i][k];
						}
						i = -1;
						break;
					}
					else{
						continue;
					}
				}
				else{
					break;
				}
			}
		}
		else{
			continue;
		}
	}
	for(i = 0; i < quantidadeProcessos; i++){
		if (vetAcabou[i] == 0){
			return -1;
		}
		else{
			continue;
		}
	}
	return 0;
}
