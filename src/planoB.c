#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "proc_grafico.h"
#include "mouse.h"
//#include "acelerometro.c"
#include <unistd.h>
#include <stdint.h>

extern void fecha_dev_mem();
extern void inicializa_fpga();
extern void escreve_bloco(uint16_t posicao, uint16_t cor);
extern void apaga_bloco(uint16_t posicao);
extern void altera_cor_bg(uint16_t cor, uint8_t registrador);
extern void apaga_cor_bg(uint8_t registrador);
extern void exibe_sprite(uint8_t sp, uint32_t xy, uint16_t offset, uint8_t registrador);
extern int acess_btn();
extern int abre_mouse();
extern int le_mouse_orientacao(int fd);
extern int le_mouse_direcao(int fd, uint16_t *x, uint16_t *y);
extern int teste_leitura(int fd);


#define LARGURA 80 // Largura do labirinto (deve ser ímpar)
#define ALTURA 60  // Altura do labirinto (deve ser ímpar)
#define ESPESSURA 4 // Espessura das paredes e caminhos
char labirinto[ALTURA][LARGURA];

void inicializaLabirinto();
void imprimeLabirinto();
int validaPosicao(int x, int y);
void geraLabirinto(int x, int y);
void imprimeLabirintoVGA();
void apagaLabirinto();
void move_sprite();
void colisao_labirinto();
int main();

// Função para inicializar o labirinto com paredes
void inicializaLabirinto() {
   for (int i = 0; i < ALTURA; i++) {
       for (int j = 0; j < LARGURA; j++) {
           labirinto[i][j] = '#';
       }
   }
}




// Função para imprimir o labirinto
void imprimeLabirintoTerminal() {
   for (int i = 0; i < ALTURA; i++) {
       for (int j = 0; j < LARGURA; j++) {
           printf("%c", labirinto[i][j]);
       }
       printf("\n");
   }
}




// Função para verificar se a posição é válida e se pode ser cavada
int validaPosicao(int x, int y) {
   if (x < ESPESSURA || y < ESPESSURA || x >= ALTURA - ESPESSURA || y >= LARGURA - ESPESSURA) {
       return 0; // Fora dos limites
   }
   return labirinto[x][y] == '#'; // Cavar apenas se a posição for parede
}




// Função recursiva para gerar o labirinto
void geraLabirinto(int x, int y) {
   for (int i = 0; i < ESPESSURA; i++) {
       for (int j = 0; j < ESPESSURA; j++) {
           labirinto[x + i][y + j] = ' '; // Marca como espaço
       }
   }




   // Embaralhar as direções
   int direcoes[] = {0, 1, 2, 3};
   for (int i = 0; i < 4; i++) {
       int r = rand() % 4;
       int temp = direcoes[i];
       direcoes[i] = direcoes[r];
       direcoes[r] = temp;
   }




   // Tentar cada direção
   for (int i = 0; i < 4; i++) {
       int nx = x + dx[direcoes[i]] * ESPESSURA * 2;
       int ny = y + dy[direcoes[i]] * ESPESSURA * 2;




       if (validaPosicao(nx, ny)) {
           for (int j = 1; j < ESPESSURA * 2; j++) {
               for (int k = 0; k < ESPESSURA; k++) {
                   labirinto[x + dx[direcoes[i]] * j + k][y + dy[direcoes[i]] * j + k] = ' '; // Remove parede intermediária
               }
           }
           geraLabirinto(nx, ny);
       }
   }
}




// void imprimeLabirintoVGA() {
//     altera_cor_bg(BRANCO, 0); //pintando fundo




//     int i, j;
//     for(i=0;i < ALTURA;i++) { //linhas
//         for(j=0; j < LARGURA; j++) { //colunas
//             if(labirinto[i][j] == '#'){
//                 escreve_bloco( j + (i * 80), AZUL_ESCURO);
//             } else if(labirinto[i][j] == 'K'){
//                 escreve_bloco( j + (i * 80), PRETO);
//             }
//         }
//     }
// }




// void apagaLabirinto() {
//     apaga_cor_bg(0);
//     int i, j;
//     for(i=0;i < ALTURA;i++) { //linhas
//         for(j=0; j < LARGURA; j++) { //colunas
//                 apaga_bloco( j + (i * 80));
         
//         }
//     }
// }




void atualizaMatriz(char jogador, int novaX, int novaY, int antigaX, int antigaY) {
   // Limpa a posição antiga
   for (int i = 0; i < 2; i++) {
       for (int j = 0; j < 2; j++) {
           labirinto[antigaX + i][antigaY + j] = ' ';
       }
   }




   // Atualiza a nova posição
   for (int i = 0; i < 2; i++) {
       for (int j = 0; j < 2; j++) {
           labirinto[novaX + i][novaY + j] = jogador;
       }
   }
}



void imprimeLabirintoVGA() {
   altera_cor_bg(BRANCO, 0); //pintando fundo




   int i, j;
   for(i=0;i < ALTURA;i++) { //linhas
       for(j=0; j < LARGURA; j++) { //colunas
           if(labirinto[i][j] == '#'){
               escreve_bloco( j + (i * 80), AZUL_ESCURO);
           } else if(labirinto[i][j] == 'K'){
               escreve_bloco( j + (i * 80), PRETO);
           }
       }
   }
}




void apagaLabirinto() {
   apaga_cor_bg(0);
   int i, j;
   for(i=0;i < ALTURA;i++) { //linhas
       for(j=0; j < LARGURA; j++) { //colunas
               apaga_bloco( j + (i * 80));
         
       }
   }
}


void move_sprite() {

    //posicoes iniciais da matriz - jogador 1
    //bloco superior esquerda
    int posx1 = ESPESSURA;
    int posy1 = 0;
    //bloco superior direita
    int posx2 = ESPESSURA;
    int posy2 = 1;
    //bloco inferior esquerda
    int posx3 = ESPESSURA + 1;
    int posy3 = 0;
    //bloco inferior direita
    int posx4 = ESPESSURA + 1;
    int posy4 = 1;

   #define INICIO_Y 358410
   #define LIMITE_Y 358850
   int x = 0, y = 0;

   #define mascara_10bits 0b1111111111
   uint16_t pos_x = 350;
   uint16_t pos_y = 240;

   pos_x &= mascara_10bits;
   pos_y &= mascara_10bits;
 
   uint32_t pos_xy_20b;
   pos_xy_20b = (pos_x << 10 | pos_y);
   uint32_t pos_xy_20b_ant = (pos_xy_20b); //inicia com posicao anterior igual a posicao atual

   int fd = abre_mouse();

   while (1) {
       int direcao = le_mouse_direcao(fd, &pos_x, &pos_y);
       int orientacao = le_mouse_orientacao(fd);

       if(direcao == 0) { //horizontal
           if(orientacao > 0) { //direita
                if(labirinto[posx2][posy2 + 1] != '#' && labirinto[posx4][posy4+1] != '#'){
                    //limpando posicao anterior
                    labirinto[posx1][posy1] = ' ';
                    labirinto[posx2][posy2] = ' ';
                    labirinto[posx3][posy3] = ' ';
                    labirinto[posx4][posy4] = ' ';

                    //atualizando posicao
                    posy1 += 1;
                    posy2 += 1;
                    posy3 += 1;
                    posy4 += 1;
                    labirinto[posx1][posy1] = 1;
                    labirinto[posx2][posy2] = 1;
                    labirinto[posx3][posy3] = 1;
                    labirinto[posx4][posy4] = 1;

                    //atualizando sprit
                    

                } else {
                    printf("Não é possivel mexer para essa posição \n");
                }
           } else { //esquerda
                if(labirinto[posx1][posy1 - 1] != '#' && labirinto[posx3][posy3-1] != '#'){
                    //limpando posicao anterior
                    labirinto[posx1][posy1] = ' ';
                    labirinto[posx2][posy2] = ' ';
                    labirinto[posx3][posy3] = ' ';
                    labirinto[posx4][posy4] = ' ';

                    //atualizando posicao
                    posy1 -= 1;
                    posy2 -= 1;
                    posy3 -= 1;
                    posy4 -= 1;
                    labirinto[posx1][posy1] = 1;
                    labirinto[posx2][posy2] = 1;
                    labirinto[posx3][posy3] = 1;
                    labirinto[posx4][posy4] = 1;
                } else {
                    printf("Não é possivel mexer para essa posição \n");
                }
           }

       }else{ //vertical
            if(orientacao > 0) { //cima
                if(labirinto[posx2-1][posy2] != '#' && labirinto[posx1-1][posy1] != '#'){
                    //limpando posicao anterior
                    labirinto[posx1][posy1] = ' ';
                    labirinto[posx2][posy2] = ' ';
                    labirinto[posx3][posy3] = ' ';
                    labirinto[posx4][posy4] = ' ';

                    //atualizando posicao
                    posx1 -= 1;
                    posx2 -= 1;
                    posx3 -= 1;
                    posx4 -= 1;
                    labirinto[posx1][posy1] = 1;
                    labirinto[posx2][posy2] = 1;
                    labirinto[posx3][posy3] = 1;
                    labirinto[posx4][posy4] = 1;
                } else {
                    printf("Não é possivel mexer para essa posição \n");
                }
           } else { //baixo
                if(labirinto[posx4+1][posy4] != '#' && labirinto[posx3+1][posy4] != '#'){
                    //limpando posicao anterior
                    labirinto[posx1][posy1] = ' ';
                    labirinto[posx2][posy2] = ' ';
                    labirinto[posx3][posy3] = ' ';
                    labirinto[posx4][posy4] = ' ';

                    //atualizando posicao
                    posy1 += 1;
                    posy2 += 1;
                    posy3 += 1;
                    posy4 += 1;
                    labirinto[posx1][posy1] = 1;
                    labirinto[posx2][posy2] = 1;
                    labirinto[posx3][posy3] = 1;
                    labirinto[posx4][posy4] = 1;
                } else {
                    printf("Não é possivel mexer para essa posição \n");
                }
           }
       }

       // apaga o sprite exibido na posicao anterior
       exibe_sprite(0, pos_xy_20b_ant, 5, 1); // sp = 0 -> desabilita sprite




     
       pos_x &= mascara_10bits;
       pos_y &= mascara_10bits;
       pos_xy_20b = (pos_x << 10 | pos_y);
     
        // exibe o sprite na posicao atual
       exibe_sprite(1, pos_xy_20b, 5, 1); // sp = 1 -> habilita sprite
       pos_xy_20b_ant = pos_xy_20b;




   }
}




void configLabirinto() {
   inicializaLabirinto();
   geraLabirinto(ESPESSURA, ESPESSURA);




   // Coloca as saídas
   for (int i = 0; i <= ESPESSURA; i++) {
       labirinto[ESPESSURA][i] = ' ';
       labirinto[ESPESSURA + 1][i] = ' ';
       labirinto[ALTURA - ESPESSURA - 1][LARGURA - ESPESSURA - i] = ' ';
       labirinto[ALTURA - ESPESSURA - 2][LARGURA - ESPESSURA - i] = ' ';
   }




   // Posiciona o jogador 1 como uma peça
   //pra pegar a pos x: espessura * 10,6666667 e pos y: 0 * 6
   labirinto[ESPESSURA][0] = '1';
   labirinto[ESPESSURA][1] = '1';
   labirinto[ESPESSURA + 1][0] = '1';
   labirinto[ESPESSURA + 1][1] = '1';




   // Posiciona o jogador 2 como uma peça 2x2
   labirinto[ALTURA - ESPESSURA - 2][LARGURA - ESPESSURA] = '2';
   labirinto[ALTURA - ESPESSURA - 2][LARGURA - ESPESSURA - 1] = '2';
   labirinto[ALTURA - ESPESSURA - 1][LARGURA - ESPESSURA] = '2';
   labirinto[ALTURA - ESPESSURA - 1][LARGURA - ESPESSURA - 1] = '2';




 
   for(int i = 0; i < ALTURA; i++) {
       for(int j = 0; j < LARGURA; j++) {
           labirinto[i][LARGURA - 1] = '#';
           labirinto[i][LARGURA - 2] = '#';
           labirinto[i][LARGURA - 3] = '#';
       }
   }
}




int main() {
   //inicializa_fpga();
   //configurar_acelerometro();




   configLabirinto();
   imprimeLabirintoTerminal();
   
   while(1) {
       move_sprite();
   }



   // #define mascara_10bits 0b1111111111
   // uint16_t pos_x = 590;
   // uint16_t pos_y = 430;
   // pos_x &= mascara_10bits;
   // pos_y &= mascara_10bits;
   // uint32_t pos_xy_20b;
   // pos_xy_20b = (pos_x << 10 | pos_y);


   // for(int i = 0; i < ALTURA; i++) {
   //     for(int j = 0; j < LARGURA; j++) {
   //         if(labirinto[i][j] == '1'){
   //             exibe_sprite(3, pos_xy_20b, 1, 5);
   //         }
   //         else if(labirinto[i][j] == '2'){
   //             exibe_sprite(4, pos_xy_20b, 2, 6);
   //         }
   //     }
   // }


   // for (int i = 0; i < 1500; i++) {
   //         imprimeLabirintoVGA();
   //         //apagaLabirinto();
   // }
 
   //move_sprite();


   // desmapear_memoria();
   // fecha_dev_mem();
   return 0;
}