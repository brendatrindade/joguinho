#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define HPS_PHYS_BASE 0xFF000000
#define HPS_SPAN 0x01000000   
#define SYSMGR_BASE 0xFFD08000
#define I2C0_BASE 0xFFC04000 

#define SYSMGR_I2C0USEFPGA (SYSMGR_BASE + 0x704)
#define SYSMGR_GENERALIO7 (SYSMGR_BASE + 0x49C)
#define SYSMGR_GENERALIO8 (SYSMGR_BASE + 0x4A0)

#define I2C0_CON (I2C0_BASE + 0x00) 
#define I2C0_TAR (I2C0_BASE + 0x04)
#define I2C0_DATA_CMD (I2C0_BASE + 0x10)
#define I2C0_FS_SCL_HCNT (I2C0_BASE + 0x1C)
#define I2C0_FS_SCL_LCNT (I2C0_BASE + 0x20)
#define I2C0_ENABLE (I2C0_BASE + 0x6C) 
#define I2C0_RXFLR (I2C0_BASE + 0x78)   
#define I2C0_TXFLR (I2C0_BASE + 0x74)
#define I2C0_CLR_INTR (I2C0_BASE + 0x40)

#define ADXL345_ADDR 0x53   
#define ADXL345_DEVID 0x00 
#define ADXL345_POWER_CTL 0x2D
#define ADXL345_DATA_FORMAT 0x31
#define ADXL345_DATAX0 0x32 
#define ADXL345_DATAX1 0x33 
#define ADXL345_DATAY0 0x34
#define ADXL345_DATAY1 0x35
#define ADXL345_DATAZ0 0x36
#define ADXL345_DATAZ1 0x37
#define ADXL345_INT_ENABLE 0x2E
#define ADXL345_INT_MAP 0x2F
#define ADXL345_INT_SOURCE 0x30
#define ADXL345_BW_RATE 0x2C

#define AMOSTRAS_CALIBRACAO 100
#define FILTRO_MOVIMENTO 0.20

volatile uint32_t *base_hps; //ponteiro para a memoria HPS mapeada
int mg_por_lsb = 4; //mg por bit menos significativo para conversão
int16_t offset_x, offset_y, offset_z = 0; //offset dos eixos X, Y e Z para calibracao
int fd;

void escrever_registrador(uint32_t endereco, uint32_t valor) {
   *(volatile uint32_t*)(base_hps + (endereco - HPS_PHYS_BASE) / 4) = valor; //endereco inicial da regiao mapeada na memoria virtual + ( endereco do registrador na memoria fisica - endereco inicial da regiao mapeada na memoria fisica )
}

uint32_t ler_registrador(uint32_t endereco) {
   return *(volatile uint32_t*)(base_hps + (endereco - HPS_PHYS_BASE) / 4);
}

void inicializar_i2c() {
   escrever_registrador(SYSMGR_I2C0USEFPGA, 0); //HPS
   escrever_registrador(SYSMGR_GENERALIO7, 1); //I2C
   escrever_registrador(SYSMGR_GENERALIO8, 1); //I2C

   escrever_registrador(I2C0_ENABLE, 0x0); // Desabilita
   escrever_registrador(I2C0_CON, 0x65); //Mestre, modo rapido 400kbit/s, enderecamento 7 bits
   escrever_registrador(I2C0_TAR, ADXL345_ADDR);//destino acelerometro

   escrever_registrador(I2C0_FS_SCL_HCNT, 60 + 30);
   escrever_registrador(I2C0_FS_SCL_LCNT, 130 + 30);
  
   escrever_registrador(I2C0_ENABLE, 0x1); //Habilita
   usleep(10000);
}

void verificar_status_i2c() {
   uint32_t status = ler_registrador(I2C0_ENABLE);
   printf("Status I2C: 0x%08X\n", status);
   if (status & 0x1) {
       printf("I2C habilitado\n");
   } else {
       printf("I2C nao habilitado\n");
   }
}

void ler_reg_acel(uint8_t endereco, uint8_t *valor) {
   escrever_registrador(I2C0_DATA_CMD, endereco + 0x400); //endereco e sinal Start
   escrever_registrador(I2C0_DATA_CMD, 0x100); //sinal de leitura
   while (ler_registrador(I2C0_RXFLR) == 0) {} //aguarda até que o buffer de recepcao nao esteja vazio
   *valor = ler_registrador(I2C0_DATA_CMD) & 0xFF; //0b11111111 - le os bits LSB do reg 7-0 escritos em dat 
}

void escrever_reg_acel(uint8_t endereco, uint8_t valor) {
   escrever_registrador(I2C0_DATA_CMD, endereco + 0x400);// endereco e sinal start
   escrever_registrador(I2C0_DATA_CMD, valor); //valor para ser escrito
}

void escrever_i2c(uint8_t endereco_reg, uint8_t valor) {
   escrever_reg_acel(endereco_reg, valor);
}

uint8_t ler_i2c(uint8_t endereco_reg) {
   uint8_t valor;
   ler_reg_acel(endereco_reg, &valor);
   return valor;
}

void inicializar_acelerometro() {
   escrever_i2c(ADXL345_POWER_CTL, 0x00); //standby
   usleep(10000);
   escrever_i2c(ADXL345_POWER_CTL, 0x08); //modo medicao
   escrever_i2c(ADXL345_DATA_FORMAT, 0x00); // ±2g
   escrever_i2c(ADXL345_BW_RATE, 0x0A); // 100 Hz
   escrever_i2c(ADXL345_INT_ENABLE, 0x80); //habilita interrupcao de dados prontos
   escrever_i2c(ADXL345_INT_MAP, 0x00); //mapeia interrupcao 
}

uint8_t ler_devid_acelerometro() {
   return ler_i2c(ADXL345_DEVID);
}

void ler_aceleracao_x(int16_t *x) {
   while(!dados_prontos()); //aguarda novos dados
   *x = (ler_i2c(ADXL345_DATAX1) << 8) | ler_i2c(ADXL345_DATAX0); //le e combina em 16bits os dados MSB e LSB do eixo x
   ler_i2c(ADXL345_INT_SOURCE); //limpa interrupcoes detectadas
}

void ler_aceleracao_y(int16_t *y) {
   while(!dados_prontos()); //aguarda novos dados
   *y = (ler_i2c(ADXL345_DATAY1) << 8) | ler_i2c(ADXL345_DATAY0); //le e combina em 16bits os dados MSB e LSB do eixo y
   ler_i2c(ADXL345_INT_SOURCE); //limpa interrupcoes detectadas
}

void ler_aceleracao_z(int16_t *z) {
   while(!dados_prontos()); //aguarda novos dados
   *z = (ler_i2c(ADXL345_DATAZ1) << 8) | ler_i2c(ADXL345_DATAZ0); //le e combina em 16bits os dados MSB e LSB do eixo z
   ler_i2c(ADXL345_INT_SOURCE); //limpa interrupcoes detectadas
}

int dados_prontos() {
   return (ler_i2c(ADXL345_INT_SOURCE) & 0x80) != 0; //0b10000000 - verifica o bit 7 data_ready
}

void calibrar_acelerometro(int16_t *offset_x, int16_t *offset_y, int16_t *offset_z) {
    int32_t soma_x, soma_y, soma_z = 0;
    int16_t x, y, z;
    int i;
 
    for (i = 0; i < AMOSTRAS_CALIBRACAO; i++) {
        ler_aceleracao_x(&x);
        ler_aceleracao_y(&y);
        ler_aceleracao_z(&z);
        soma_x += x;
        soma_y += y;
        soma_z += z;
    }

    *offset_x = soma_x / AMOSTRAS_CALIBRACAO; //calcula a media de leituras obtidas por x
    *offset_y = soma_y / AMOSTRAS_CALIBRACAO; //calcula a media de leituras obtidas por y
    *offset_z = soma_z / AMOSTRAS_CALIBRACAO; //calcula a media de leituras obtidas por z

    printf("Calibracao completa \n Offset: X=%d \n Offset: Y=%d \n Offset: Z=%d \n", *offset_x, *offset_y, *offset_z);
}


int configurar_acelerometro(){
   uint8_t devid;

   fd = open("/dev/mem", O_RDWR | O_SYNC); // Abre o /dev/mem para o mapeamento de memoria
   if (fd < 0) {
       printf("Erro ao abrir /dev/mem\n");
       return -1;
   }

   base_hps = mmap(NULL, HPS_SPAN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, HPS_PHYS_BASE); // Mapeia a memoria HPS
   if (base_hps == MAP_FAILED) {
       printf("Erro ao mapear memoria\n");
       close(fd);
       return -1;
   }

   inicializar_i2c();
   verificar_status_i2c();

   int i;
   for (i = 0; i < 5; i++) {
       devid = ler_devid_acelerometro();
       printf("Tentativa %d - ADXL345 DEVID: 0x%02X\n", i+1, devid);
       if (devid == 0xE5) {
           break;
       }
       usleep(100000);
   }

   inicializar_acelerometro();
   printf("Acelerometro inicializado\n");

   printf("Calibrando acelerometro...\n");
   calibrar_acelerometro(&offset_x, &offset_y, &offset_z);

   printf("Calibrado! Iniciando leituras de aceleracao...\n");
   return 0;
}

int desmapear_memoria(){
    munmap((void*)base_hps, HPS_SPAN);
    close(fd);
    return 0;
}

int get_direcao_movimento_x(){
    int16_t x_bruto;
    float x_g;

    ler_aceleracao_x(&x_bruto);

    x_g = (x_bruto - offset_x) * (mg_por_lsb / 1000.0);

    if (x_g > FILTRO_MOVIMENTO) {
        return 1;  // direita
    } else if (x_g < -FILTRO_MOVIMENTO) {
        return -1; // esquerda
    } 
    return 0; // sem movimento   
}

//Movimento para cima e para baixo
int get_direcao_movimento_y(int *velocidade){
   int16_t y_bruto;
   float y_g;

   ler_aceleracao_y(&y_bruto);

   y_g = (y_bruto - offset_y) * (mg_por_lsb / 1000.0);

   if (y_g > FILTRO_MOVIMENTO) {
      if (y_g < (5*FILTRO_MOVIMENTO)) {
         *velocidade = 1; // velocidade baixa
      } else if (y_g < (10*FILTRO_MOVIMENTO)) {
         *velocidade = 2; // velocidade media
      } else {
         *velocidade = 3; // velocidade alta
      }
      return 1; // direcao para cima
   } else if (y_g < -FILTRO_MOVIMENTO) {
      if (y_g < (5*-FILTRO_MOVIMENTO)) {
         *velocidade = 1; // velocidade baixa
      } else if (y_g < (10*-FILTRO_MOVIMENTO)) {
         *velocidade = 2; // velocidade media
      } else {
         *velocidade = 3; // velocidade alta
      }
      return -1; // direcao para baixo
   } 
   return 0; // sem movimento   
}

int get_direcao_movimento_z(){
    int16_t z_bruto;
    float z_g;

    ler_aceleracao_z(&z_bruto);

    z_g = (z_bruto - offset_z) * (mg_por_lsb / 1000.0);

    if (z_g > FILTRO_MOVIMENTO) {
        return 1;  // frente
    } else if (z_g < -FILTRO_MOVIMENTO) {
        return -1; // tras
    } 
    return 0; // sem movimento   
}