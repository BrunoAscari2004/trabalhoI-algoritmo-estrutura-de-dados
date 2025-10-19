#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_LINHA 1024

//Este programa serve somente para criar os arquivos binarios quando não estão presentes

typedef struct prod
{
    long long id;
    char categoria[20];
    float preco;
} Produto;

typedef struct ped{
    long long id;
    long long id_produto;
    char data[11];
} Pedido;


#define DELIMITADOR ","

// Retorna 1 (true) se a string inteira representa um número válido (int ou float),
// ignorando espaços em volta. Ex.: "98", "98.71", "  0.5  " → true; "jewelry", "" → false.
static int eh_numero(const char *s) {
    if (!s || !*s) return 0;          // nulo ou vazio
    char *end;
    strtod(s, &end);                   // tenta converter o começo da string
    while (*end == ' ' || *end == '\t') end++; // ignora espaços finais
    return *end == '\0';               // se consumiu tudo, é número
}

/*
Processa uma linha do CSV no formato: id,categoria,preco
- Mantém strtok (simples e rápido).
- Trata casos problemáticos como:
    1) "id,,preco"          -> categoria faltou   -> "Sem categoria", preço lido
    2) "id,categoria"       -> preço faltou       -> "Sem categoria"? NÃO: aqui setamos preço=0.0 e mantemos a categoria
    3) "id,preco,"          -> interpretamos 2º token: se for número, é porque a categoria faltou e sobrou só o preço
    4) "id" ou linha vazia  -> categoria="Sem categoria", preco=0.0
Observação: strtok remove campos vazios consecutivos (",,") do fluxo normal,
por isso capturamos APENAS até 3 tokens e inferimos o que faltou pelo n de tokens
e pelo teste "eh_numero" no token do meio.
*/
Produto processar_linha(char *linha) {
    Produto p = (Produto){0};

    // Remove \r\n do fim (robustece para arquivos Windows/Unix)
    linha[strcspn(linha, "\r\n")] = '\0';

    // Captura até 3 tokens: t[0]=id, t[1]=categoria OU preco, t[2]=preco (quando existe)
    char *t[3] = {0};
    int n = 0;
    for (char *tok = strtok(linha, DELIMITADOR); tok && n < 3; tok = strtok(NULL, DELIMITADOR)) {
        t[n++] = tok;
    }

    // 1) ID (se faltar, deixa 0)
    if (n >= 1 && t[0] && *t[0]) {
        p.id = strtoll(t[0], NULL, 10);
    }

    if (n == 3) {
        // Caso padrão: "id,categoria,preco"
        // Categoria: se vazia -> "Sem categoria"
        if (t[1] && *t[1]) {
            strncpy(p.categoria, t[1], sizeof(p.categoria) - 1);
            p.categoria[sizeof(p.categoria) - 1] = '\0';
        } else {
            strcpy(p.categoria, "Sem categoria");
        }
        // Preço: se vazio -> 0.0
        p.preco = (t[2] && *t[2]) ? (float)strtod(t[2], NULL) : 0.0f;

    } else if (n == 2) {
        // Temos "id,ALGUMA_COISA"
        // Essa ALGUMA_COISA pode ser:
        //   - um número (ex.: "id,,preco" virou "id,preco" por causa do strtok) => categoria faltou
        //   - um texto (ex.: "id,categoria") => preço faltou
        if (eh_numero(t[1])) {
            // Interpreta como "categoria faltou" e "t[1] é o preço"
            strcpy(p.categoria, "Sem categoria");
            p.preco = (float)strtod(t[1], NULL);
        } else {
            // Interpreta como "categoria presente" e "preço faltou"
            strncpy(p.categoria, t[1], sizeof(p.categoria) - 1);
            p.categoria[sizeof(p.categoria) - 1] = '\0';
            p.preco = 0.0f;
        }

    } else {
        // n == 1 (só id) ou n == 0 (linha ruim)
        // Define defaults simples e seguros:
        strcpy(p.categoria, "Sem categoria");
        p.preco = 0.0f;
    }

    return p;
}

Pedido processar_linha_pedido(char *linha) {
    Pedido p = {0}; // Inicializa toda a struct com zero
    char *token;
    int indice_coluna = 0;

    // Remove o '\n' se presente
    //linha[strcspn(linha, "\n")] = 0; 

    token = strtok(linha, DELIMITADOR);


    while (token != NULL) {
         printf("\n%s\n",token);
        
        // Se a string tem tamanho zero (ou seja, é um campo vazio)
        int is_empty = (token[0] == '\0');
        
        switch (indice_coluna) {
            case 0: // ID (long long)
                if (is_empty) {
                    p.id = 0; // Atribui 0 se estiver vazio
                } else {
                    p.id = strtoll(token, NULL, 10);
                }
                break;

            case 1: // ID Produto (long long)
                if (is_empty) {
                    p.id_produto = 0; // Atribui 0 se estiver vazio
                } else {
                    p.id_produto = strtoll(token, NULL, 10);
                }
                break;

            case 2: // Data (char[11])
                if (is_empty) {
                    strcpy(p.data, " "); 
                } else {
                    // O campo não está vazio, copia normalmente
                    strncpy(p.data, token, sizeof(p.data) - 1);
                    p.data[sizeof(p.data) - 1] = '\0'; 
                }
                break;
            
            default:
                break;
        }

        token = strtok(NULL, DELIMITADOR);
        indice_coluna++;
    }

    return p;
}

void salvar_pedido(FILE * arquivo, const Pedido *pedido) {

    size_t itens_escritos;

    itens_escritos = fwrite(pedido, sizeof(Pedido), 1, arquivo);

    if (itens_escritos != 1) {
        fprintf(stderr, "Erro na escrita: Apenas %zu de 1 item foram escritos.\n", itens_escritos);
    } else {
        printf("Pedido ID %lld salvo com sucesso no arquivo binário.\n", pedido->id);
    }

}


void salvar_produto(FILE * arquivo, const Produto *produto) {

    size_t itens_escritos;


    // Usar fwrite para escrever a struct inteira
    // Escreve 1 item do tamanho de 'Produto', começando no endereço de 'produto'
    itens_escritos = fwrite(produto, sizeof(Produto), 1, arquivo);

    if (itens_escritos != 1) {
        fprintf(stderr, "Erro na escrita: Apenas %zu de 1 item foram escritos.\n", itens_escritos);
    } else {
        printf("Produto ID %lld salvo com sucesso no arquivo binário.\n", produto->id);
    }

}

Produto ler_produto(FILE *arquivo, int quant){

    Produto produto;
    for (int i = 0; i < quant; i++)
    {
        fread(&produto, sizeof(Produto),1, arquivo);
        printf("\n%lld - %s - %.2f", produto.id, produto.categoria, produto.preco);
    }
    
}

Pedido ler_pedido(FILE *arquivo, int quant){



    Pedido pedido;
    for (int i = 0; i < quant; i++)
    {
        fread(&pedido, sizeof(Pedido),1, arquivo);
        printf("\n%lld - %lld - %s", pedido.id, pedido.id_produto, pedido.data);
    }
    

}



int main(){

    
    FILE *produtoscsv = fopen("produtos.csv","r");
    FILE *produtosbin = fopen("produtos.bin","wb");

    char buffer[MAX_LINHA];
    int count = 0;

    //utilizando o fgets para ler até o \n
    while(fgets(buffer, MAX_LINHA, produtoscsv) != NULL){
        Produto prod = processar_linha(buffer);
        salvar_produto(produtosbin, &prod);
        count++; 
    }


    fclose(produtosbin);

    produtosbin = fopen("produtos.bin", "rb");

    ler_produto(produtosbin, 50);

    
 
    FILE *pedidoscsv = fopen("pedidos.csv","r");
    FILE *pedidosbin = fopen("pedidos.bin","wb");

    char bufferPed[MAX_LINHA];
    int countPed = 0;

    //utilizando o fgets para ler até o \n
    while(fgets(bufferPed, MAX_LINHA, pedidoscsv) != NULL){
        Pedido ped = processar_linha_pedido(bufferPed);

        printf("\n%lld - %lld - %s\n", ped.id, ped.id_produto, ped.data);
        salvar_pedido(pedidosbin, &ped);
        countPed++; 
    }

    fclose(pedidosbin);
    

  /*   FILE *produtos = fopen("produtos.bin", "rb");

    ler_produto(produtos, 70);

    fclose(produtos);*/
    //fclose(pedidoscsv);

    return 0;
}



