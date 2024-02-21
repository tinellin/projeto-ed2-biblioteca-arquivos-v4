#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <math.h>

#define ARQ_INSERE "arquivos/insere.bin"
#define ARQ_DADOS "arquivos/dados.bin"
#define ARQ_HASH "arquivos/hash.bin"
#define ARQ_BUSCA "arquivos/busca.bin"
#define ARQ_REMOVE "arquivos/remove.bin"

#define QTD_ENDERECOS 31
#define TAM_CHAVE 14

typedef struct {
    char isbn[TAM_CHAVE];
    char titulo[50];
    char autor[50];
    char ano[5];
} Livro;

typedef enum {
    VAZIO = 0,
    INDISPONIVEL,
    DISPONIVEL
} Status;

#define TAM_HASH (2 * sizeof(int) + sizeof(char) * TAM_CHAVE)

FILE *abrirArquivo(char *, char *);
void fecharArquivo(FILE *);
void obterRegistro(FILE *, Livro *, char[TAM_CHAVE], char);
void inicializarArquivoHash(FILE *);
int calcularEnderecoHash(char[TAM_CHAVE]);
void inserir(FILE *, FILE *, FILE *);
void remover(FILE *, FILE *);
void buscar(FILE *, FILE *, FILE *);

int main(void) {
    setlocale(LC_ALL, "");

    FILE *arqInserir = NULL, *arqDados = NULL, *arqHash = NULL, *arqBusca = NULL, *arqRemover = NULL;
    int op;

    printf("*-------- BIBLIOTECA --------*\n");

    do
    {
        printf("*______________________________________________*\n");
        printf("|  Opcao      | Funcao                         |\n");
        printf("|    1.       | Inserir                        |\n");
        printf("|    2.       | Remover                        |\n");
        printf("|    3.       | Buscar                         |\n");
        printf("|    0.       | Sair do programa               |\n");
        printf("*----------------------------------------------*\n");
        printf("Digite a opcao: ");
        scanf("%d", &op);

        switch (op)
        {
            case 1:
            {
                inserir(arqInserir, arqDados, arqHash);
                break;
            }
            case 2:
            {
                arqRemover = abrirArquivo(ARQ_REMOVE, "r+b");
                arqHash = abrirArquivo(ARQ_HASH, "r+b");

                remover(arqRemover, arqHash);

                fecharArquivo(arqRemover);
                fecharArquivo(arqHash);

                break;
            }
            case 3:
            {
                buscar(arqBusca, arqHash, arqDados);
                break;
            }
            case 0:
            {
                fecharArquivo(arqRemover);
                exit(0);
            }
            default:
                printf("Digite uma das opcoes\n");
        }
    } while (op != 0);
    return 0;
}


FILE *abrirArquivo(char *ch, char *tipoAbertura)
{
    FILE *arq;

    if (((arq = fopen(ch, tipoAbertura)) == NULL))
    {
        printf("ERRO: Falha ao abrir o arquivo\n%s", ch);
        return arq;
    }

    return arq;
}

void fecharArquivo(FILE *arq)
{
    fclose(arq);
}

void obterRegistro(FILE *arq, Livro *livro, char chave[TAM_CHAVE], char tipoArq)
{
    int ct = 1;

    if (fgetc(arq) == '@')
    {
        fread(&ct, sizeof(int), 1, arq);

        // Pular para posicao desejada e ler o registro
        if (tipoArq == 'i')
            fseek(arq, sizeof(Livro) * ct, SEEK_SET);
        else if (tipoArq == 'r' || tipoArq == 'b')
            fseek(arq, TAM_CHAVE * ct, SEEK_SET);

        if (tipoArq == 'i')
            fread(livro, sizeof(Livro), 1, arq);
        else if (tipoArq == 'r' || tipoArq == 'b')
            fread(chave, TAM_CHAVE * ct, 1, arq);

        // Salvar o proximo registro a ser lido
        rewind(arq);
        ct++;

        fseek(arq, 1, SEEK_SET);
        fwrite(&ct, sizeof(int), 1, arq);
    }
    else
    {
        rewind(arq);

        if (tipoArq == 'i')
            fread(livro, sizeof(Livro), 1, arq);
        else if (tipoArq == 'r' || tipoArq == 'b')
            fread(chave, TAM_CHAVE, 1, arq);

        rewind(arq);

        fwrite("@", 1, sizeof(char), arq);
        fwrite(&ct, sizeof(int), 1, arq);
    }
}

int calcularEnderecoHash(char chave[TAM_CHAVE]) {
    double soma = 0, res;
    char *ptr;
    soma = strtod(chave, &ptr);

    res = fmod(soma, QTD_ENDERECOS);

    return (int) res;
}

void inicializarArquivoHash(FILE *arqHash) {
    char isbn[TAM_CHAVE];
    int rrn;
    Status status;

    int i, j;

    for(i = 0; i < QTD_ENDERECOS; i++) {
        status = VAZIO;
        for (j = 0; j < TAM_CHAVE - 1; j++) isbn[j] = '@';
        isbn[TAM_CHAVE - 1] = '\0';
        rrn = -1;

        fwrite(&status, sizeof(int), 1, arqHash);
        fwrite(isbn, sizeof(char), TAM_CHAVE, arqHash);
        fwrite(&rrn, sizeof(int), 1, arqHash);
    }

    rewind(arqHash);

    printf("Arquivo hash inicializado com sucesso!");
}

void inserir(FILE *arqInserir, FILE *arqDados, FILE *arqHash) {
    int endereco, ctColisao = 1, ctFim = 0;

    unsigned long offset, rrn;
    Status status;

    Livro livro;

    char buffer[sizeof(Livro)];

    arqInserir = abrirArquivo(ARQ_INSERE, "r+b");
    arqDados = abrirArquivo(ARQ_DADOS, "r+b");

    if (!(arqHash = abrirArquivo(ARQ_HASH, "r+b"))) {
        arqHash = abrirArquivo(ARQ_HASH, "w+b");
        inicializarArquivoHash(arqHash);
    }

    // Inserir registro de tamanho variado no arq. de dados

    obterRegistro(arqInserir, &livro, NULL, 'i');

    sprintf(buffer, "%s#%s#%s#%s#", livro.isbn, livro.titulo, livro.autor, livro.ano);

    fseek(arqDados, 0, SEEK_END);

    fwrite(buffer, sizeof(Livro), 1, arqDados);

    /*
     * Função hash:
     * Arquivo hash: Chave + RRN + Status (disp. ou vazio) (14 + 2 * sizeof(int));
     * Inserir em espaços vazios ou disponíveis
    */

    endereco = calcularEnderecoHash(livro.isbn);
    printf("Endereco %d \n", endereco);
    offset = endereco * TAM_HASH;

    //Calcular RRN
    fseek(arqDados, 0, SEEK_END);
    rrn = (ftell(arqDados) / sizeof(Livro)) - 1;

    fseek(arqHash, (long) offset, SEEK_SET);

    while (1) {
        if(ftell(arqHash) == (TAM_HASH * QTD_ENDERECOS)) {
            ctFim++;

            printf("ctFim: %d\n", ctFim);

            if(ctFim == 2) {
                printf("ERRO: Nao foi possivel inserir o registro.\n");
                break;
            }

            fseek(arqHash, 0, SEEK_SET);
            continue;
        }

        fread(&status, sizeof(int), 1, arqHash);

        if(status == INDISPONIVEL) {
            printf("Colisao\n");
            printf("Tentativa %d\n", ctColisao++);
            fseek(arqHash, TAM_HASH - sizeof(int), SEEK_CUR);
            continue;
        }

        fseek(arqHash, -4, SEEK_CUR);

        // Inserir hash no arquivo: Status (int) + Isbn (TAM_CHAVE) + rrn (int)
        status = INDISPONIVEL;
        fwrite(&status, sizeof(int), 1, arqHash);
        fwrite(livro.isbn, TAM_CHAVE, 1, arqHash);
        fwrite(&rrn, 1, sizeof (int), arqHash);

        printf("Chave %s inserida com sucesso \n", livro.isbn);

        break;
    }

    fecharArquivo(arqInserir);
    fecharArquivo(arqDados);
    fecharArquivo(arqHash);
}

void remover (FILE *arqRemover, FILE *arqHash) {
    char *chave = (char *)calloc(sizeof(char), TAM_CHAVE);
    char *chaveArqHash = (char *)calloc(sizeof(char), TAM_CHAVE);

    int endereco, offset, ctFim = 0;

    int rrn = 0;
    Status status;

    obterRegistro(arqRemover, NULL, chave, 'r');

    endereco = calcularEnderecoHash(chave);
    offset = endereco * TAM_HASH;

    fseek(arqHash, offset, SEEK_SET);

    while (1) {
        printf("%ld\n", ftell(arqHash));
        if(ftell(arqHash) == (TAM_HASH * QTD_ENDERECOS)) {
            ctFim++;

            printf("ctFim: %d\n", ctFim);

            if(ctFim == 2) {
                printf("ERRO: Nao foi possivel inserir o registro.\n");
                break;
            }

            fseek(arqHash, 0, SEEK_SET);
            continue;
        }


        fseek(arqHash, sizeof (int), SEEK_CUR);

        fread(chaveArqHash, TAM_CHAVE, 1, arqHash);

        if(strcmp(chave, chaveArqHash) != 0) {
            fseek(arqHash, sizeof(int), 1);
            continue;
        }

        fseek(arqHash, -18, SEEK_CUR);

        status = DISPONIVEL;
        rrn = -1;

        fwrite(&status, sizeof(int), 1, arqHash);
        fwrite("@@@@@@@@@@@@@", sizeof(char), TAM_CHAVE, arqHash);
        fwrite(&rrn, sizeof(int), 1, arqHash);

        break;
    }
}

void buscar(FILE *arqBuscar, FILE *arqHash, FILE *arqDados) {
    char *chave = (char *)calloc(sizeof(char), TAM_CHAVE);
    char *chaveArqHash = (char *)calloc(sizeof(char), TAM_CHAVE);

    int offset, endereco, ctAcessos, ctFim = 0;

    char *token;
    char buffer[sizeof(Livro)];

    Livro livro;
    Status status;
    int rrn = 0;

    arqBuscar = abrirArquivo(ARQ_BUSCA, "r+b");
    arqHash = abrirArquivo(ARQ_HASH, "r+b");
    arqDados = abrirArquivo(ARQ_DADOS, "r+b");

    obterRegistro(arqBuscar, NULL, chave, 'b');

    endereco = calcularEnderecoHash(chave);
    offset = endereco * TAM_HASH;

    fseek(arqHash, offset, SEEK_SET);
    printf("Offset = %d\n", offset);

    ctAcessos = 1;
    while (1) {
        printf("%d\n", ftell(arqHash));
        if(ftell(arqHash) >= (TAM_HASH * QTD_ENDERECOS)) {
            ctFim++;

            if(ctFim == 2) {
                printf("ERRO: Nao foi possivel buscar o registro.\n");
                break;
            }

            fseek(arqHash, 0, SEEK_SET);
            continue;
        }

        fseek(arqHash, sizeof(int), SEEK_CUR);
        fread(chaveArqHash, TAM_CHAVE, 1, arqHash);

        printf("%s \n", chaveArqHash);

        if((strcmp(chave, chaveArqHash)) == 0) {
            fread(&rrn, sizeof(int), 1, arqHash);

            ctAcessos > 1 ?
            printf("Chave %s encontrada, endereco %d, %d acessos\n", chave, endereco, ctAcessos)
            : printf("Chave %s encontrada, endereco %d, %d acesso\n", chave, endereco, ctAcessos);

            fseek(arqDados, rrn * sizeof(Livro),SEEK_SET);
            fread(buffer, sizeof(Livro), 1, arqDados);

            token = strtok(buffer, "#");
            strcpy(livro.isbn, token);
            token = strtok(NULL, "#");
            strcpy(livro.titulo, token);
            token = strtok(NULL, "#");
            strcpy(livro.autor, token);
            token = strtok(NULL, "#");
            strcpy(livro.ano, token);
            printf("\n*---------------------------------------------*\n");
            printf("ISBN: %s\nTITULO: %s\nAUTOR: %s\nANO: %s\n", livro.isbn, livro.titulo, livro.autor, livro.ano);
            printf("*---------------------------------------------*\n");

            break;
        } else {
            fseek(arqHash, sizeof(int), SEEK_CUR);
            ctAcessos++;
        }
    }

    fecharArquivo(arqHash);
    fecharArquivo(arqBuscar);
    fecharArquivo(arqDados);
}
