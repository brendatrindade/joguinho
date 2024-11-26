@ GRAPH LIB
@ Biblioteca com funcoes graficas para o GPU do Gabriel Sá Barreto

.section .data
dev: .asciz "/dev/mem"       
ponteFPGA: .word 0xff200
mem_vga_max: .word 4799
cor_apaga_bloco: .word 510
mascara_9b: .word 0b111111111
mascara_14b: .word 0b11111111111111
fd: .word 0
end_base: .word 0

@ mem_vga_min, 0

.equ dataB, 0x70
.equ dataA, 0x80
.equ wrreg, 0xC0
.equ exit, 0x01
.equ write, 0x04
.equ open, 0x05
.equ close, 0x06
.equ t_pag, 0x1000
.equ mmap2, 0xC0
.equ munmap, 0x5B
.equ stdout, 0x01

@ Mensagens de erros e acertos
.section .rodata
erro: .asciz "Vix deu ruim 1\n"
sucesso: .asciz "Aoba deu bom\n"
erro2: .asciz "Vix deu ruim 2\n"
sp_dataB: .asciz "Data B ok\n" 
sp_dataA: .asciz "Data A ok\n" 
sp_sucesso: .asciz "Wrreg em 0\n" 

@ Definindo funções
.section .text
.global inicializa_fpga
.type inicializa_fpga, %function

.global escreve_bloco
.type escreve_bloco, %function

.global apaga_bloco
.type apaga_bloco, %function

.global fecha_dev_mem
.type fecha_dev_mem, %function

.global exibe_sprite
.type exibe_sprite, %function

.global altera_pixel_sprite
.type altera_pixel_sprite, %function

.global desenha_poligono
.type desenha_poligono, %function

.global altera_cor_bg
.type altera_cor_bg, %function


@ inicializa_fpga: responsável por abrir a devman e configurar o mmap
@ sem parâmetros

inicializa_fpga:
  push {r0-r11, lr}
  @ Abre /dev/mem
  ldr r0, =dev @ caminho do arquivo devman
  mov r1, #2   @ para leitura e escrita
  mov r2, #0   @ sem flags
  mov r7, #open   @ syscall open, o 5 é para abrir arquivos
  svc 0        @ chama o sistema para executar
  mov r4, r0   @ guardando o resultado em r4 - fd

  @ Salva o fd na memória
  ldr r1, =fd  @ Carrega o endereço da variável fd
  str r4, [r1] @ Salva o fd na memória

  @ verifica se o arquivo foi aberto corretamente, pois o retorno indica se deu erro ou não
  cmp r0, #0  @ compara o valor com 0
  blt _erro   @ se r0 < 0 deu erro e vai para mensagem de erro
  bl _sucesso @ se não vai para mensagem de sucesso

  @ Configura o mmap
  ldr r5, =ponteFPGA   @carrega o endereço base da FPGA, mas não carrega necessariamente o valor real associado a esse endereço
  mov r0, #0           @ para deixar o kernel decidir o enderço virtual
  mov r1, #t_pag        @ tamanho da pagina
  mov r2, #3           @ leitura/escrita
  mov r3, #1           @ compartilhado com outros processos
  mov r7, #mmap2         @ syscall do mmap2
  ldr r5, [r5]         @ acessa o valor do endereco e ler o valor real do enderço da FPGA
  svc 0                @ kernel é chamado para executar a syscall

  @conferindo se deu tudo bom
  cmp r0, #-1   @ verifica se igual
  beq _erro2

  mov r11, r0   @ endereço virtual retornado salvando em r11

  @ Passa o endereço mapeado para a memória
  ldr r1, =end_base @ Carrega o endereço da variável end_base
  str r11, [r1]     @ Salva o endereço mapeado
  pop {r0-r11, pc}
  bx lr

@_________________________________________________________________________________________________________________________________________________________________________

@ escreve_bloco (WBM): essa função é responsável por mostrar um bloco na tela usando o a intrução WBM da GPU
@ parâmetros:
@ r3 -> posição
@ r4 -> cor

escreve_bloco:
  push {r3-r11, lr}
  ldr r11, =end_base @ Endereço base
  ldr r11, [r11]
  mov r3, r0         @ Posição
  mov r4, r1         @ Cor

  @ colocando 0 no wrreg
  mov r5, #0
  mov r8, #wrreg
  str r5, [r11, r8] @ Calcula o endereço (0xc0 hexadecimal) ao endereço base armazenado em r11, e coloca o valor logico baixo ao start (werreg)

  @ Configurando o dataA
  @ O barramento dataA é utilizado para opcodes e endereçamento do Banco de Registrador de Memórias
  mov r1, #dataA
  mov r2, #0b0010      @opcode do dataA (WBM)
  lsl r3, r3, #4       @ arrasta 4 bits para a esquerda, esse deslocamento é necessario para que o endereco de r3 ocupe a posicao correta dos bits ao final, pensando na estrutura dos bits do wbm
  add r3, r3, r2       @ soma o opcode com o r3
  mov r12, #dataA
  str r3, [r11, r12]   @ basicamente aqui pega o r3 e armezana no endereço r11 (endereço virtual) + 0x80 (endereço do dataA)

  @ Configurando o dataB
  @ O barramento dataB é utilizado para envio de dados a serem armazenados e/ou atualizados
  mov r9, #dataB
  str r4, [r11, r9] @ carrega a cor no endereco do dataB

  mov r5, #1
  mov r8, #wrreg
  str r5, [r11, r8] @ coloca o start (wrreg) em positivo pra executar os barramentos

  bl _sucesso @ se não vai para mensagem de sucesso

  @ colocando 0 no wrreg
  mov r5, #0
  mov r8, #wrreg
  str r5, [r11, r8] @ Calcula o endereço (0xc0 hexadecimal) ao endereço base armazenado em r11, e coloca o valor logico baixo ao start (werreg)

  pop {r3-r11, pc}
  bx lr

@_________________________________________________________________________________________________________________________________________________________________________

@ apaga_bloco (WBM): é a função que é responsável por apagar um bloco na tela usando a instrução WBM da GPU
@ parâmetros:
@

apaga_bloco:
  push {r3-r11, lr}

  ldr r11, =end_base @ Endereço base
  ldr r11, [r11]
  mov r3, r0 @ Posição

  @ colocando 0 no wrreg
  mov r5, #0
  mov r8, #wrreg
  str r5, [r11, r8] @ Calcula o endereço (0xc0 hexadecimal) ao endereço base armazenado em r11, e coloca o valor logico baixo ao start (werreg)

  @ Configurando o dataA
  @ O barramento dataA é utilizado para opcodes e endereçamento do Banco de Registrador de Memórias
  mov r1, #dataA
  mov r2, #0b0010      @opcode do dataA (WBM)

  lsl r3, r3, #4       @ arrasta 4 bits para a esquerda, esse deslocamento é necessario para que o endereco de r3 ocupe a posicao correta dos bits ao final, pensando na estrutura dos bits do wbm
  add r3, r3, r2       @ soma o opcode com o r3
  mov r12, #dataA
  str r3, [r11, r12]   @ basicamente aqui pega o r3 e armezana no endereço r11 (endereço virtual) + 0x80 (endereço do dataA)

  @ Configurando o dataB
  @ O barramento dataB é utilizado para envio de dados a serem armazenados e/ou atualizados
  ldr r4, =cor_apaga_bloco @ carrega cor da memória 
  ldr r4, [r4]
  mov r9, #dataB
  str r4, [r11, r9] @ carrega a cor no endereco do dataB

  mov r5, #1
  mov r8, #wrreg
  str r5, [r11, r8] @ coloca o start (wrreg) em positivo pra executar os barramentos

  bl _sucesso @mensagem de sucesso

  @ colocando 0 no wrreg
  mov r5, #0
  mov r8, #wrreg
  str r5, [r11, r8] @ Calcula o endereço (0xc0 hexadecimal) ao endereço base armazenado em r11, e coloca o valor logico baixo ao start (werreg)


  pop {r3-r11, pc}
  bx lr

@_________________________________________________________________________________________________________________________________________________________________________

@ exibe_sprite (WBR): aqui exibimos um sprit usando a instrução WBR da GPU
@ parâmetros:
@ r0 -> sp (0-1)
@ r1 -> x,y (0-1023)
@ r2 -> offset (0-511)
@ r3 -> registrador (0-31)

exibe_sprite:
  push {r0-r10, lr}      

  ldr r9, =end_base
  ldr r9, [r9]

  and r7, r0, #1  @ ...|sp|                (01bit )
  lsl r7, r7, #20 @ ...|sp|20b|            (21bits)
  orr r7, r7, r1
  lsl r7, r7, #9  @ ...|sp| x | y |  09b |
  orr r7, r7, r2  @ ...|sp| x | y |offset| (30bits)

  str r7, [r9, #dataB] @ Escreve em dataB

  @bl _sp_dataB

  mov r10, #0b0000 @ Opcode de WBR

  and r7, r3, #0b11111 @ ...|registrador|        (05bits)
  lsl r7, r7, #4       @ ...|registrador|  04b |
  orr r7, r7, r10      @ ...|registrador|opcode| (09bits)

  str r7, [r9, #dataA] @ Escreve em dataA

  @bl _sp_dataA

  mov r5, #1
  mov r8, #wrreg
  str r5, [r9, r8] @ start (wrreg)

  mov r5, #0
  mov r8, #wrreg
  str r5, [r9, r8] @ espera (wrreg)

  @bl _sp_sucesso

  pop {r0-r10, pc}
  bx lr

@_________________________________________________________________________________________________________________________________________________________________________

@ altera_pixel_sprite: essa função armazena ou modifica o conteúdo presente na Memória de Sprites usando a intrução WSM da GPU
@ parâmetros:
@ r0 - Cor - 9b
@ r1 - Endereço - 14b

altera_pixel_sprite:
  push {r0-r7, lr}

  ldr r3, =end_base
  ldr r3, [r3]

  ldr r7, =mascara_9b
  ldr r7, [r7]

  and r2, r0, r7 @ ...|B|G|R| (09bits)
  str r2, [r3, #dataB] @ Escreve em dataB

  @bl _sp_dataB

  mov r4, #0b0001 @ Opcode WSM

  ldr r7, =mascara_14b
  ldr r7, [r7]

  and r2, r1, r7   @ ...|endereco|        (14bits)
  lsl r2, r2, #4   @ ...|endereco|  04b |
  orr r2, r2, r4   @ ...|endereco|opcode| (18bits)

  str r2, [r3, #dataA] @ Escreve em dataA

  @bl _sp_dataA

  mov r5, #1
  mov r6, #wrreg
  str r5, [r3, r6] @ start (wrreg)

  mov r5, #0
  mov r6, #wrreg
  str r5, [r3, r6] @ espera (wrreg)

  @bl _sp_sucesso

  pop {r0-r7, pc}
  bx lr

@_________________________________________________________________________________________________________________________________________________________________________

@ desenha_poligono: é responsável por desenhar poligonos (quadrado ou triângulo) usando a função DP da GPU
@ parâmetros:
@ r0 -> passa cor
@ r1 -> pega tamanho (vai de 1-15)
@ r2 -> pega forma (1 é triangulo e 0 quadrado)
@ r3 -> x e y

desenha_poligono:
  push {r0-r11, lr}
  ldr r11, =end_base @ Endereço base
  ldr r11, [r11]

  @ colocando 0 no wrreg
  mov r5, #0
  mov r8, #wrreg
  str r5, [r11, r8] @ Calcula o endereço (0xc0 hexadecimal) ao endereço base armazenado em r11, e coloca o valor logico baixo ao start (werreg)

  @dataB
  mov r4, r2 @add a forma que esta no r2 (se não der com coloca #1 ou #0) em r4
  lsl r4, r4, #9 @ faz deslocamento de 9 bits para colocar a cor
  add r0, r4, r0 @ adicionando cor + forma em r0
  lsl r0, r0, #4 @ desloca 4 bits a esquerda para colocar o tamanho
  add r0, r0, r1 @ pegando o tamanho em r1
  lsl r0, r0, #18 @ desloca 18 bits que se refere ao tamanho (9 do x e 9 do y)
  add r0, r0, r3 @ pegando o x e y que esta em r3
  str r0, [r11, #dataB] @passando parametros para o data B

  @dataA
  lsl r6, r6, #4 @onde ta o endereco, então aqui recebe o endereço e desloca 4 bits a esquerda
  mov r6, #0b0011 @opcode de DP adicionado em 4 bits
  str r6, [r11, #dataA] @passando parametros para o data A

  mov r5, #1
  mov r8, #wrreg
  str r5, [r11, r8] @ coloca o start (wrreg) em positivo pra executar os barramentos

  bl _sucesso @vai para mensagem de sucesso

  @ colocando 0 no wrreg
  mov r5, #0
  mov r8, #wrreg
  str r5, [r11, r8] @ Calcula o endereço (0xc0 hexadecimal) ao endereço base armazenado em r11, e coloca o valor logico baixo ao start (werreg)

  pop {r0-r11, pc}
  bx lr

@_________________________________________________________________________________________________________________________________________________________________________

@ altera_cor_bg: usa a instrução WBR da GPU para alterar a cor do backgraund
@ parâmetros:
@

altera_cor_bg:
  @ Salvando contexto
  push {r0-r11, lr}

  ldr r11, =end_base @ r11 = endereço base
  ldr r11, [r11]

  @ colocando 0 no wrreg
  mov r5, #0
  mov r8, #wrreg
  str r5, [r11, r8] @ Calcula o endereço (0xc0 hexadecimal) ao endereço base armazenado em r11, e coloca o valor logico baixo ao start (werreg)

  @ Escreve no barramento dataA
  lsl r1, r1, #4
  mov r3, #0b0000 @ r3 = opcode de WBR
  add r1, r1, r3
  str r1, [r11, #dataA] 

  @ Escreve o valor de cor no barramento dataB
  str r0, [r11, #dataB] 

  @ colocando 1 no wrreg
  mov r5, #1
  mov r8, #wrreg
  str r5, [r11, r8] @ coloca o start (wrreg) em positivo pra executar os barramentos

  bl _sucesso

  @ colocando 0 no wrreg
  mov r5, #0
  mov r8, #wrreg
  str r5, [r11, r8] @ Calcula o endereço (0xc0 hexadecimal) ao endereço base armazenado em r11, e coloca o valor logico baixo ao start (werreg)

  @ Restaurando contexto
  pop {r0-r11, pc}
  bx lr

@_________________________________________________________________________________________________________________________________________________________________________

@fecha_dev_mem: essa função é responsável por encerrar o arquivo devman 
fecha_dev_mem:
  push {r7, lr}
  mov r7, #munmap
  mov r1, #t_pag
  ldr r0, =end_base
  ldr r0, [r0]
  svc 0  

  mov r7, #close
  ldr r0, =fd
  ldr r0, [r0]
  svc 0
  pop {r7, pc}

  bl _fim
@_________________________________________________________________________________________________________________________________________________________________________

@ Aqui o codigo é finalizado:

_fim: @ funcao para finalizar
  mov r0, #0
  mov r7, #exit
  svc 0

@_________________________________________________________________________________________________________________________________________________________________________

@Funções que exibem mensagem de erro:

_erro:
  mov r0, #stdout
  ldr r1, =erro
  mov r2, #15
  mov r7, #write
  svc 0
  mov r0, #-1
  pop {r4-r11, pc}
  bx lr

_erro2:
  mov r0, #stdout
  ldr r1, =erro2
  mov r2, #15
  mov r7, #write
  svc 0
  bx lr

_sucesso:
  mov r0, #stdout
  ldr r1, =sucesso
  mov r2, #13
  mov r7, #write
  svc 0
  bx lr

_sp_dataB:
  mov r0, #stdout
  ldr r1, =sp_dataB
  mov r2, #10
  mov r7, #write
  svc 0
  bx lr

_sp_dataA:
  mov r0, #stdout
  ldr r1, =sp_dataA
  mov r2, #10
  mov r7, #write
  svc 0
  bx lr

_sp_sucesso:
  mov r0, #stdout
  ldr r1, =sp_sucesso
  mov r2, #11
  mov r7, #write
  svc 0
  bx lr

@_________________________________________________________________________________________________________________________________________________________________________
