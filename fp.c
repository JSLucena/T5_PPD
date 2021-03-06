#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

#define ARRAY_SIZE 1000000
#define FACTOR 0.1 // % do vetor local, para espaco de tocas
//#define DEBUG 1
//#define DEBUG2 1
//#define DEBUG3 1

//ACESSO AO LAD - ssh portoalegre\\17103269@sparta.pucrs.br
//ssh -o PasswordAuthentication=yes ppd59007@grad.lad.pucrs.br

void bs(int n, int * vetor)
{
    int c=0, d, troca, trocou =1;

    while (c < (n-1) & trocou )
        {
        trocou = 0;
        for (d = 0 ; d < n - c - 1; d++)
            if (vetor[d] > vetor[d+1])
                {
                troca      = vetor[d];
                vetor[d]   = vetor[d+1];
                vetor[d+1] = troca;
                trocou = 1;
                }
        c++;
        }
}

int *interleaving(int vetor[],int tam)
{
    int *vetor_aux;
    int i1,i2,i_aux;

    vetor_aux = (int *)malloc(sizeof(int)*tam);

    i1 = 0;
    i2 = tam/2;
    for(i_aux = 0; i_aux < tam;i_aux++)
    {
        if(((vetor[i1]<=vetor[i2]) && (i1 < (tam/2))) || (i2 == tam)) 
        {
            vetor_aux[i_aux] = vetor[i1++];
        }
        else
        {
            vetor_aux[i_aux] = vetor[i2++];
        }
        
    }
    return vetor_aux;
}

int main(int argc , char **argv)
{
    int i;
    double t1,t2;
    int proc_n;
    int my_rank;
    int exchange_size, local_array_size;
    int pronto = 0;
    int *local_array;
    int *readys;
    int message;
    int *exchange_array;
    int *interleave;
    int *to_interleave;
    MPI_Status status;


    MPI_Init (&argc , & argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank); // pega pega o numero do processo atual (rank)
    MPI_Comm_size(MPI_COMM_WORLD, &proc_n);  // pega informacao do numero de processos (quantidade total)

    local_array_size = ARRAY_SIZE/proc_n;
    exchange_size = ARRAY_SIZE/proc_n * FACTOR;


    local_array = (int *)malloc(sizeof(int)*(local_array_size + exchange_size));

    readys = (int *) malloc(sizeof(int) * proc_n);
    for(i = 0; i < local_array_size+exchange_size; i++)
    {
        local_array[i] = 0;
    }

    //inicializacao do vetor de prontos
    for(i = 0; i < proc_n; i++)
    {
        readys[i] = 0;
    }

    t1 = MPI_Wtime(); //inicio de medicao

    for(i=0;i<local_array_size;i++)
        {
            local_array[i] = ARRAY_SIZE - i - local_array_size*my_rank; //ordenacao do array pelo pior caso
        }

    #ifdef DEBUG
    printf("\nVetor %d: ",my_rank);
    for (i=0 ; i<local_array_size; i++)    
    {          /* print unsorted array */
            printf("[%03d] ", local_array[i]);
        printf("\n");
    }
    #endif

    while (!pronto)
    {
        bs(local_array_size,local_array);

        message = local_array[local_array_size-1]; //envia o maior elemento para o processo vizinho testar
        if(my_rank != proc_n-1)
        {
            MPI_Send(&message, 1, MPI_INT, my_rank+1, 0 ,MPI_COMM_WORLD);
        }
        if(my_rank != 0)
        {
            MPI_Recv(&message, 1, MPI_INT, my_rank-1, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            if(message > local_array[0]) //se o valor recebido for maior que o meu menor elemento
                readys[my_rank] = 0; //nao estamos ordenados
            else
            {
                readys[my_rank] = 1;
            }
            
        }
        if(my_rank == 0)
            readys[0] = 1; //primeiro processo sempre ordenado com a esquerda dele

        for(i = 0; i < proc_n;i++)
        {
            MPI_Bcast(&readys[i],1,MPI_INT,i,MPI_COMM_WORLD); //broadcast dos prontos de cada processo
        }

        pronto = 1;
        
        for (i=0 ; i<proc_n; i++)    
        {
            #ifdef DEBUG          
            printf(" %d ", readys[i]);
            #endif 
            if(readys[i] == 0) //se algum deles nao esta pronto
                pronto = 0; //continuaremos executando
            
        }
        

        if(my_rank != 0)
        {
            MPI_Send(&local_array[0], exchange_size, MPI_INT, my_rank-1, 0 ,MPI_COMM_WORLD); //envia menores valores para a esquerda
        }
        if(my_rank != proc_n -1)
        {
            MPI_Recv(&local_array[local_array_size], exchange_size, MPI_INT, my_rank+1, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

                #ifdef DEBUG2
            printf("\nVetor %d: ",my_rank);
            for (i=0 ; i<local_array_size+exchange_size; i++)    
            {          /* print unsorted array */
                printf("[%03d] ", local_array[i]);
                printf("\n");
            }
            #endif
            to_interleave = &local_array[local_array_size-exchange_size]; //vetor auxiliar com a parte alta do vetor local

            interleave = interleaving(to_interleave,2*exchange_size); //intercala os valores recebidos com a parte alta do vetor
            
            #ifdef DEBUG3
            printf("\nVetor %d: ",my_rank);
            for (i=0 ; i<2*exchange_size; i++)    
            {          
                printf("[%03d] ", interleave[i]);
                printf("\n");
            }
            #endif

            for(i = 0; i < exchange_size*2;i++)
            {
                local_array[local_array_size-exchange_size+i] = interleave[i];
            }

            MPI_Send(&local_array[local_array_size], exchange_size, MPI_INT, my_rank+1, 0 ,MPI_COMM_WORLD); //manda os valores mais altos depois do interleave de volta
        }
        if(my_rank != 0)
        {
            MPI_Recv(&local_array[0], exchange_size, MPI_INT, my_rank-1, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        }

        
        #ifdef DEBUG2
        printf("\nInterleave Vetor %d: ",my_rank);
        for (i=0 ; i<local_array_size+exchange_size; i++)    
        {          /* print unsorted array */
            printf("[%03d] ", local_array[i]);
            printf("\n");
        }
        #endif

         
        

        //pronto = 1;
    }
    #ifdef DEBUG2
    printf("\nVetor %d: ",my_rank);
        for (i=0 ; i<local_array_size; i++)    
        {          /* print unsorted array */
            printf("[%03d] ", local_array[i]);
            printf("\n");
        }
    #endif   

    t2 = MPI_Wtime(); //inicio de medicao

    if(my_rank == 0)
        printf("\nTempo de execucao: %f\n\n",t2-t1);
    MPI_Finalize();
    return 0;
}