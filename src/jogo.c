#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include "proc_grafico.h"
#include "acelerometro.c"
#include "mouse.h"
#include "colisao.c"
#include "sprite.c"
#include "animacao_menu.c"
#include "animacao_win.c"

extern void fecha_dev_mem();
extern void inicializa_fpga();
extern void escreve_bloco(uint16_t posicao, uint16_t cor);
extern void apaga_bloco(uint16_t posicao);
extern void altera_cor_bg(uint16_t cor, uint8_t registrador);
extern void apaga_cor_bg(uint8_t registrador);
extern void exibe_sprite(uint8_t sp, uint32_t xy, uint16_t offset, uint8_t registrador);
extern int acess_btn();

#define ALTURA_LAB 60  // Altura do labirinto
#define LARGURA_LAB 80 // Largura do labirinto
#define ESPESSURA 4    // Espessura das paredes e caminhos
#define mascara_10bits 0b1111111111

extern int abre_mouse();
extern int le_mouse_orientacao(int fd);
extern int le_mouse_direcao(int fd, uint16_t *x, uint16_t *y);
extern int teste_leitura(int fd);

char labirinto[ALTURA_LAB][LARGURA_LAB];

// Vetores para movimentação (cima, baixo, esquerda, direita)
int dx[] = {0, 0, -1, 1};
int dy[] = {-1, 1, 0, 0};

// Dados dos jogadores p1 e p2
typedef struct {
    uint32_t pos_xy_inicial;
    uint32_t pos_xy_20b;
    pthread_mutex_t mutex;
    int ativo;
    int vidas;
} Dados_jogador;

// Dados do elemento passivo 1
typedef struct {
    uint32_t pos_xy_20b;
    pthread_mutex_t mutex;
    int ativo;
} Dados_Estrela;

// Dados do elemento passivo 2
typedef struct {
    uint32_t pos_xy_20b;
    pthread_mutex_t mutex;
    int ativo;
} Dados_Portal;

Dados_jogador p1_acelerometro;
Dados_jogador p2_mouse;
Dados_Estrela p_estrela;
Dados_Portal p_portal;

void inicializaLabirinto();
void imprimeLabirintoTerminal();
int validaPosicao(int x, int y);
void geraLabirinto(int x, int y);
void imprimeLabirintoVGA();
void apagaLabirinto();
int colide(uint16_t prox_pos_x, uint16_t prox_pos_y);
void *move_acelerometro();
void *move_mouse();
void def_saidas_labirinto();
void def_posicao_jogadores();
void def_borda_labirinto();
void posiciona_sprites(uint32_t *pos_xy_20b_p1, uint32_t *pos_xy_20b_p2);
void apaga_sprite();
int main();
int montaJogo();

// Cria as threads
pthread_t thread_acel, thread_mouse, thread_passiva, thread_portal;

// Função para inicializar o labirinto com paredes (caractere '#').
void inicializaLabirinto() {
    for (int i = 0; i < ALTURA_LAB; i++) {
        for (int j = 0; j < LARGURA_LAB; j++) {
            labirinto[i][j] = '#';
        }
    }
}

// Função para verificar se a posição é válida e se pode ser cavada
int validaPosicao(int x, int y) {
    if (x < ESPESSURA || y < ESPESSURA || x >= ALTURA_LAB - ESPESSURA || y >= LARGURA_LAB - ESPESSURA) {
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

void imprimeLabirintoVGA() {
    altera_cor_bg(BRANCO, 0); //pintando fundo
    int i, j;
    for(i=0;i < ALTURA_LAB;i++) { //linhas
        for(j=0; j < LARGURA_LAB; j++) { //colunas
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
    for(i=0; i < ALTURA_LAB; i++) { //linhas
        for(j=0; j < LARGURA_LAB; j++) { //colunas
                apaga_bloco( j + (i * 80));
        }
    }
}

// Apaga todos os sprites da tela.
void apaga_sprite(){
    exibe_sprite(0, 0, 1, 1);
    exibe_sprite(0, 0, 2, 2);
    exibe_sprite(0, 0, 4, 10);
    exibe_sprite(0, 0, 9, 15);
    exibe_sprite(0, 0, 15, 16);
    exibe_sprite(0, 0, 15, 17);
    exibe_sprite(0, 0, 15, 18);
    exibe_sprite(0, 0, 15, 19);
    exibe_sprite(0, 0, 15, 20);
    exibe_sprite(0, 0, 15, 21);
}

void def_vidas_p1(int v1, int v2, int v3){
    uint16_t pos_x_p1, pos_y_p1;
    uint32_t pos_xy_20b_p1;

    converte_labirinto_para_sprite(10, 76, &pos_x_p1, &pos_y_p1);
    pos_x_p1 &= mascara_10bits;
    pos_y_p1 &= mascara_10bits;
    pos_xy_20b_p1 = (pos_x_p1 << 10 | pos_y_p1);
    exibe_sprite(v1, pos_xy_20b_p1, 15, 16);

    converte_labirinto_para_sprite(15, 76, &pos_x_p1, &pos_y_p1);
    pos_x_p1 &= mascara_10bits;
    pos_y_p1 &= mascara_10bits;
    pos_xy_20b_p1 = (pos_x_p1 << 10 | pos_y_p1);
    exibe_sprite(v2, pos_xy_20b_p1, 15, 17);

    converte_labirinto_para_sprite(20, 76, &pos_x_p1, &pos_y_p1);
    pos_x_p1 &= mascara_10bits;
    pos_y_p1 &= mascara_10bits;
    pos_xy_20b_p1 = (pos_x_p1 << 10 | pos_y_p1);
    exibe_sprite(v3, pos_xy_20b_p1, 15, 18);
}

void def_vidas_p2(int v1, int v2, int v3){
    uint16_t pos_x_p2, pos_y_p2;
    uint32_t pos_xy_20b_p2;

    converte_labirinto_para_sprite(35, 76, &pos_x_p2, &pos_y_p2);
    pos_x_p2 &= mascara_10bits;
    pos_y_p2 &= mascara_10bits;
    pos_xy_20b_p2 = (pos_x_p2 << 10 | pos_y_p2);
    exibe_sprite(v1, pos_xy_20b_p2, 15, 19);

    converte_labirinto_para_sprite(40, 76, &pos_x_p2, &pos_y_p2);
    pos_x_p2 &= mascara_10bits;
    pos_y_p2 &= mascara_10bits;
    pos_xy_20b_p2 = (pos_x_p2 << 10 | pos_y_p2);
    exibe_sprite(v2, pos_xy_20b_p2, 15, 20);

    converte_labirinto_para_sprite(45, 76, &pos_x_p2, &pos_y_p2);
    pos_x_p2 &= mascara_10bits;
    pos_y_p2 &= mascara_10bits;
    pos_xy_20b_p2 = (pos_x_p2 << 10 | pos_y_p2);
    exibe_sprite(v3, pos_xy_20b_p2, 15, 21);
}

//Verifica inicio e fim do sprite
int colide(uint16_t prox_pos_x, uint16_t prox_pos_y) { 
    int verifica_x[] = {prox_pos_x, prox_pos_x + 19};
    int verifica_y[] = {prox_pos_y, prox_pos_y + 19};
    int i,j,q,w;
    for( q= 0; q < 2; q++) {
        for( w = 0; w < 2; w++) {
            converte_sprite_para_labirinto(verifica_x[q], verifica_y[w], &i, &j);
            if (labirinto[i][j] == '#') {
                return 1; //Tem parede
            } else if (labirinto[i][j] == 'F') {
                return 2; //Saida Fechada
            } else if (labirinto[i][j] == 'x') {
                return 3; //Estrela
            } else if (labirinto[i][j] == 'v') {
                return 4; //Portal
            } else if (labirinto[i][j] == 'K') {
                return 5; //Borda
            } 
        }
    }
    return 0; //Sem colisao
}

void *move_acelerometro() {
    int direcao_sprite, movimento, colidiu, btn;
    uint16_t pos_x, pos_y, prox_pos_y, prox_pos_x;
    int velocidade = 1;   
    p1_acelerometro.ativo = 1;
    p1_acelerometro.vidas = 3;
    int v1, v2, v3;
    v1 = 1;
    v2 = 1;
    v3 = 1;

    def_vidas_p1(v1, v2, v3);

    uint32_t pos_xy_20b_ant = p1_acelerometro.pos_xy_20b; //inicia com posicao anterior igual a posicao atual

    while (p1_acelerometro.ativo) {
        def_vidas_p1(v1, v2, v3);

        pthread_mutex_lock(&p1_acelerometro.mutex);

        pos_y = (p1_acelerometro.pos_xy_20b & mascara_10bits);
        pos_x = ((p1_acelerometro.pos_xy_20b >> 10) & mascara_10bits);

        direcao_sprite = get_movimento(&velocidade); //8 cima, 2 baixo, 6 direita, 4 esquerda, 0 sem movimento

        //apaga o sprite exibido na posicao anterior
        exibe_sprite(0, pos_xy_20b_ant, 1, 1);//sp = 0 - desabilita sprite
        pos_xy_20b_ant = p1_acelerometro.pos_xy_20b;
    
        //exibe o sprite na posicao atual
        exibe_sprite(1, p1_acelerometro.pos_xy_20b, 1, 1);//sp = 1 - habilita sprite

        movimento = 5*velocidade;

        //descendo
        if ( direcao_sprite == 2 ){
            prox_pos_y = pos_y + movimento;
            colidiu = colide(pos_x, prox_pos_y);
            if( !colidiu ){ //sem parede, pode mover
                pos_y = prox_pos_y;
            }
        }
        //subindo
        else if ( direcao_sprite == 8 ){
            prox_pos_y = pos_y - movimento;
            colidiu = colide(pos_x, prox_pos_y);
            if( !colidiu ){ //sem parede, pode mover
                pos_y = prox_pos_y;
            }
        }
        //direita
        else if ( direcao_sprite == 6 ){
            prox_pos_x = pos_x + movimento;
            colidiu = colide(prox_pos_x, pos_y);
            if( !colidiu ){ //sem parede, pode mover
                pos_x = prox_pos_x; 
            }
        }
        //esquerda
        else if ( direcao_sprite == 4 ){
            prox_pos_x = pos_x - movimento;
            colidiu = colide(prox_pos_x, pos_y);
            if( !colidiu ){ //sem parede, pode mover
                pos_x = prox_pos_x; 
            }
        }
        p1_acelerometro.pos_xy_20b = (pos_x << 10 | pos_y);

        if (colidiu == 3){
            if (p1_acelerometro.vidas == 3){
                v1 = 0;
            } else if (p1_acelerometro.vidas == 2){
                v2 = 0;
            } else if (p1_acelerometro.vidas == 1){
                v3 = 0;
            }

            p1_acelerometro.vidas -= 1;
            p1_acelerometro.pos_xy_20b = p1_acelerometro.pos_xy_inicial;

            def_vidas_p1(v1, v2, v3);
        }
        if (colidiu == 4){
            apaga_sprite();
            animacao_menu_win_1(); //Portal, p1 ganhou o jogo!
        }
        if (p1_acelerometro.vidas == 0) {
            apaga_sprite();
            animacao_menu_win_2();
        } 
        pthread_mutex_unlock(&p1_acelerometro.mutex);

        btn = button();
        printf("%d\n", btn);

        if (btn == 2){
            printf("caiu aq\n");
            while(1){
                btn = button();
                printf("%d\n", btn);
                if(btn == 3){
                    break;
                }
            }
        }   
    }
}

void *move_mouse() {
    int fd, direcao, orientacao, colidiu, movimento, btn;
    uint16_t pos_x, pos_y, prox_pos_y, prox_pos_x;
    int velocidade = 1;  
    fd = abre_mouse();
    p2_mouse.ativo = 1;
    p2_mouse.vidas = 5;
    int v1, v2, v3;
    v1 = 1;
    v2 = 1;
    v3 = 1;

    def_vidas_p2(v1, v2, v3);

    uint32_t pos_xy_20b_ant = p2_mouse.pos_xy_20b;
    
    while (p2_mouse.ativo) {  
        def_vidas_p2(v1, v2, v3);
        // Trava mutex para acesso seguro aos dados  
        pthread_mutex_lock(&p2_mouse.mutex);

        pos_x = (p2_mouse.pos_xy_20b >> 10) & mascara_10bits;
        pos_y = p2_mouse.pos_xy_20b & mascara_10bits;

        direcao = get_movimento_mouse(fd, &velocidade); //8 cima, 2 baixo, 6 direita, 4 esquerda, 0 sem movimento

        //apaga o sprite exibido na posicao anterior
        exibe_sprite(0, pos_xy_20b_ant, 2, 2);//sp = 0 - desabilita sprite
        pos_xy_20b_ant = p2_mouse.pos_xy_20b;
    
        //exibe o sprite na posicao atual
        exibe_sprite(1, p2_mouse.pos_xy_20b, 2, 2);//sp = 1 - habilita sprite

        movimento = 5*velocidade;

        if (direcao == 8) { // cima
            prox_pos_y = pos_y - movimento;
            colidiu = colide(pos_x, prox_pos_y);
            if( !colidiu ){ //sem parede, pode mover
                pos_y = prox_pos_y;
            }
        } else if (direcao == 2) { // baixo
            prox_pos_y = pos_y + movimento;
            colidiu = colide(pos_x, prox_pos_y);
            if( !colidiu ){ //sem parede, pode mover
                pos_y = prox_pos_y;
            }            
        } else if (direcao == 6) { // direita
            prox_pos_x = pos_x + movimento;
            colidiu = colide(prox_pos_x, pos_y);
            if( !colidiu ){ //sem parede, pode mover
                pos_x = prox_pos_x; 
            }
        } else if (direcao == 4) { // esquerda
            prox_pos_x = pos_x - movimento;
            colidiu = colide(prox_pos_x, pos_y);
            if( !colidiu ){ //sem parede, pode mover
                pos_x = prox_pos_x; 
            }
        }
        p2_mouse.pos_xy_20b = (pos_x << 10 | pos_y);

        if (colidiu == 3){
            if (p2_mouse.vidas == 5){
                v1 = 0;
            } else if (p2_mouse.vidas == 3){
                v2 = 0;
            } else if (p2_mouse.vidas == 1){
                v3 = 0;
            }
            p2_mouse.vidas -= 1;
            p2_mouse.pos_xy_20b = p2_mouse.pos_xy_inicial;
            def_vidas_p2(v1, v2, v3);
        }
        if (colidiu == 4){
            apaga_sprite();
            animacao_menu_win_2(); //Portal, p2 ganhou o jogo!
        }
        if (p2_mouse.vidas == 0) {
            apaga_sprite();
            animacao_menu_win_1();
        } 
        usleep(10000);
        pthread_mutex_unlock(&p2_mouse.mutex);

        btn = button();

        if (btn == 2){
            while(1){
                btn = button();
                if(btn == 3){
                    break;
                }
            }
        }   
    }
}

void def_saidas_labirinto(){
    // Coloca as saídas
    for (int i = 0; i <= ESPESSURA; i++) {
        labirinto[ESPESSURA][i] = ' ';
        labirinto[ESPESSURA + 1][i] = ' ';
        labirinto[ESPESSURA + 2][i] = ' ';
        labirinto[ESPESSURA + 3][i] = ' ';
        labirinto[ALTURA_LAB - ESPESSURA - 1][LARGURA_LAB - ESPESSURA - i] = ' '; 
        labirinto[ALTURA_LAB - ESPESSURA - 2][LARGURA_LAB - ESPESSURA - i] = ' '; 
        labirinto[ALTURA_LAB - ESPESSURA - 3][LARGURA_LAB - ESPESSURA - i] = ' '; 
        labirinto[ALTURA_LAB - ESPESSURA - 4][LARGURA_LAB - ESPESSURA - i] = ' '; 
    }
    
    labirinto[ESPESSURA][0] = 'F';
    labirinto[ESPESSURA + 1][0] = 'F';
    labirinto[ESPESSURA + 2][0] = 'F';
    labirinto[ESPESSURA + 3][0] = 'F';

    labirinto[ALTURA_LAB - ESPESSURA - 4][LARGURA_LAB - ESPESSURA] = 'F'; 
    labirinto[ALTURA_LAB - ESPESSURA - 3][LARGURA_LAB - ESPESSURA] = 'F'; 
    labirinto[ALTURA_LAB - ESPESSURA - 2][LARGURA_LAB - ESPESSURA] = 'F'; 
    labirinto[ALTURA_LAB - ESPESSURA - 1][LARGURA_LAB - ESPESSURA] = 'F';
}

void def_posicao_jogadores(){
    // Posiciona o p1 e p2
    labirinto[ESPESSURA][1] = '1';
    labirinto[ALTURA_LAB - ESPESSURA - 4][LARGURA_LAB - ESPESSURA - 4] = '2';
}

void def_borda_labirinto(){
    for(int i = 0; i < ALTURA_LAB; i++) { 
        for(int j = 0; j < LARGURA_LAB; j++) {
            labirinto[i][LARGURA_LAB - 1] = 'K';
            labirinto[i][LARGURA_LAB - 2] = 'K';
            labirinto[i][LARGURA_LAB - 3] = 'K';
        }
    }
}


void posiciona_sprites(uint32_t *pos_xy_20b_p1, uint32_t *pos_xy_20b_p2){
    //posicao inicial p1 e p2
    uint16_t pos_x_p1, pos_y_p1, pos_x_p2, pos_y_p2;

    for(int i = 0; i < ALTURA_LAB; i++) { 
        for(int j = 0; j < LARGURA_LAB; j++) {
            if(labirinto[i][j] == '1'){
                converte_labirinto_para_sprite(i,j, &pos_x_p1, &pos_y_p1);
                pos_x_p1 &= mascara_10bits;
                pos_y_p1 &= mascara_10bits;
                *pos_xy_20b_p1 = (pos_x_p1 << 10 | pos_y_p1);
                p1_acelerometro.pos_xy_inicial = *pos_xy_20b_p1;
            }
            if(labirinto[i][j] == '2'){
                converte_labirinto_para_sprite(i,j, &pos_x_p2, &pos_y_p2);
                pos_x_p2 &= mascara_10bits;
                pos_y_p2 &= mascara_10bits;
                *pos_xy_20b_p2 = (pos_x_p2 << 10 | pos_y_p2);
                p2_mouse.pos_xy_inicial = *pos_xy_20b_p2;
            }
        }
    }
}

// ELEMENTO PASSIVO = SPRITE APARECENDO EM LUGARES ALEATÓRIOS
void *elemento_passivo() {
    uint16_t pos_x_passivo, pos_y_passivo, btn;

    //uint32_t xy_passivo;
    uint32_t pas_xy_ant = (p_estrela.pos_xy_20b); // inicia com posicao anterior igual a posicao atual
    int colidiu, i, j;

    pos_x_passivo = (p_estrela.pos_xy_20b & mascara_10bits);
    pos_y_passivo = ((p_estrela.pos_xy_20b >> 10) & mascara_10bits);

    p_estrela.ativo = 1;

    while(p_estrela.ativo) {
        pthread_mutex_lock(&p_estrela.mutex);
        
        int i_ant, j_ant;

        srand((unsigned)time(NULL));  // Inicializa o gerador de números aleatórios
        pos_x_passivo = (rand() % 640) + 1;  
        pos_y_passivo = (rand() % 480) + 1; 

        colidiu = colide(pos_x_passivo, pos_y_passivo);
        
        if (!colidiu) {
            converte_sprite_para_labirinto(pos_x_passivo, pos_y_passivo, &i, &j);
            labirinto[i_ant][j_ant] = ' ';

            i_ant = i;
            j_ant = j;

            labirinto[i][j] = 'x';
            
            pos_x_passivo &= mascara_10bits;
            pos_y_passivo &= mascara_10bits;
            p_estrela.pos_xy_20b = (pos_x_passivo << 10 | pos_y_passivo);
        
            // exibe o sprite na posicao atual
            exibe_sprite(1, p_estrela.pos_xy_20b, 9, 15);
        }
        pthread_mutex_unlock(&p_estrela.mutex);

        btn = button();

        if (btn == 2){
            printf("caiu aq\n");
            while(1){
                btn = button();
                if(btn == 3){
                    break;
                }
            }
        }   
    }
}

// ELEMENTO PASSIVO 2 = Portal
void *portal() {
    uint16_t pos_x_portal, pos_y_portal;
    uint32_t pos_ant_portal = (p_portal.pos_xy_20b); // inicia com posicao anterior igual a posicao atual

    int colidiu, i, j;

    pos_x_portal = (p_portal.pos_xy_20b & mascara_10bits);
    pos_y_portal = ((p_portal.pos_xy_20b >> 10) & mascara_10bits);

    p_portal.ativo = 1;
    
    pthread_mutex_lock(&p_portal.mutex);
    do {
        srand((unsigned)time(NULL));  // Inicializa o gerador de números aleatórios
        pos_x_portal = (rand() % 640) + 1;  
        pos_y_portal = (rand() % 480) + 1; 

        //posiciona o portal longe das posicoes iniciais
        if (pos_x_portal > 400 && pos_y_portal > 300){
            pos_x_portal -= 200;
            pos_y_portal -= 180;
        } else if (pos_x_portal < 140 && pos_y_portal < 180){
            pos_x_portal += 240;
            pos_y_portal += 120;
        }

        colidiu = colide(pos_x_portal, pos_y_portal); 
        
        if (!colidiu) {
            converte_sprite_para_labirinto(pos_x_portal, pos_y_portal, &i, &j);
            labirinto[i][j] = 'v';
                
            pos_x_portal &= mascara_10bits;
            pos_y_portal &= mascara_10bits;
            p_portal.pos_xy_20b = (pos_x_portal << 10 | pos_y_portal);
        
            animacao_portal( p_portal.pos_xy_20b, 1);
            
            pos_ant_portal = p_portal.pos_xy_20b;
        }
    } while ( colidiu );
    pthread_mutex_unlock(&p_estrela.mutex);
}

int montaJogo(){
    srand(time(NULL)); // Semente para números aleatórios

    inicializaLabirinto();

    geraLabirinto(ESPESSURA, ESPESSURA);

    // Coloca as saidas
    def_saidas_labirinto();
    // Posiciona o p1 e p2
    def_posicao_jogadores();
    def_borda_labirinto();


    inicializa_fpga();

    apaga_sprite();

    for (int i = 0; i < 150; i++) {
        grava_sprite_ovni();
        grava_sprite_portal();
        grava_sprite_estrela();
    }

    posiciona_sprites(&p1_acelerometro.pos_xy_20b, &p2_mouse.pos_xy_20b);

    for (int i = 0; i < 50; i++) {
        apagaLabirinto();
    }

    // Inicializa mutexes
    pthread_mutex_init(&p1_acelerometro.mutex, NULL);
    pthread_mutex_init(&p2_mouse.mutex, NULL);
    pthread_mutex_init(&p_estrela.mutex, NULL);
    pthread_mutex_init(&p_portal.mutex, NULL);

    configurar_acelerometro();

    animacao_menu();

    exibe_sprite(1, p1_acelerometro.pos_xy_inicial, 1, 1);
    exibe_sprite(1, p2_mouse.pos_xy_inicial, 2, 2);
    
    for (int i = 0; i < 50; i++) {
        apagaLabirinto();
    }

    for (int i = 0; i < 1200; i++) {
        imprimeLabirintoVGA();
    }
    
    if (pthread_create(&thread_acel, NULL, move_acelerometro, NULL) != 0) {
        perror("Erro ao criar thread do acelerometro");
        return 1;
    }
    
    if (pthread_create(&thread_mouse, NULL, move_mouse, NULL) != 0) {
        perror("Erro ao criar thread do mouse");
        p1_acelerometro.ativo = 0;
        pthread_join(thread_acel, NULL);
        return 1;
    }

    if (pthread_create(&thread_passiva, NULL, elemento_passivo, NULL) != 0) {
        p1_acelerometro.ativo = 0;
        p2_mouse.ativo = 0;
        perror("Erro ao criar thread do elemento passivo");
        return 1;
    }

    if (pthread_create(&thread_portal, NULL, portal, NULL) != 0) {
        p1_acelerometro.ativo = 0;
        p2_mouse.ativo = 0;
        p_estrela.ativo = 0;
        perror("Erro ao criar thread do elemento passivo");
        return 1;
    }
}

void finalizaJogo(){
    p1_acelerometro.ativo = 0;
    p2_mouse.ativo = 0;
    p_estrela.ativo = 0;
    p_portal.ativo = 0;

    for (int i = 0; i < 50; i++) {
        apagaLabirinto();
    }

    // Espera as threads terminarem
    pthread_join(thread_acel, NULL);
    pthread_join(thread_mouse, NULL);
    pthread_join(thread_passiva, NULL);

    pthread_mutex_destroy(&p1_acelerometro.mutex);
    pthread_mutex_destroy(&p2_mouse.mutex);
    pthread_mutex_destroy(&p_estrela.mutex);

}

int main(){
    montaJogo();
    while (1) {
        if (button() == 0){ // Encerra o jogo
            apaga_sprite();
            finalizaJogo();
            desmapear_memoria();
            fecha_dev_mem();
            break;
        }
        /*
        if (button() == 1){ // Reinicia o jogo
            apaga_sprite();
            finalizaJogo();
            main();
        }*/
    }
    
    return 0;
}
