<div align="center">   
  <img align="center" width=100% src="https://github.com/brendatrindade/Lost-in-Space/blob/main/assets/imagens/Lost%20In%20Space%20-%20SD%2003.gif" alt="LostInSpace">
</div>

<h4 align="center">Projeto da disciplina TEC 499 - Sistemas Digitais - TP04 | Grupo 02</h4>


## Sumário
- [Visão Geral do Projeto](#Visão-Geral-do-Projeto)
- [Sobre o Lost in Space](#Sobre-o-Lost-in-Space)
- [Uso da GraphLib](#Uso-da-GraphLib)
- [Colisão](#Colisão)
- [Mouse](#Mouse)
- [Labirinto randômico](#Labirinto-randômico)
- [Tecnologias e Ferramentas utilizadas](#Tecnologias-e-Ferramentas-utilizadas)
- [Configurações de Ambiente e Execução](#Configurações-de-Ambiente-e-Execução)
- [Desenvolvedoras](#Desenvolvedoras)
- [Referências](#Referências)


# Visão Geral do Projeto
O objetivo central do Problema 3 foi desenvolver um jogo multiplayer em C, utilizando a biblioteca desenvolvida no [Problema 2](https://github.com/sarinhasf/Biblioteca-GPU). A descrição completa do problema juntamente com os requisitos pode ser acessada em 
[Descrição do problema.pdf](<assets/Descrição do problema.pdf>).

A ideia do jogo foi inspirada em um mangá conhecido como: Astra lost in Space. Dessa forma, temos um labirinto e dois jogadores representados por naves alienígenas  O objetivo é que os jogadores andem pelo labirinto a procura desse buraco negro, ganha quem encontrar o buraco negro primeiro em menos tempo, porém no meio do caminho podem aparecer algumas surpresas… 

# Sobre o Lost in Space

## Sinopse
Vocês são dois aliens perdidos pelo espaço que tem o objetivo de voltar para casa através de um buraco negro, porém o buraco negro suporta somente uma nave que levará ao planeta natal… Dessa forma, vocês estão perdidos no espaço a proucura do buraco negro que os levará para casa, porém somente um irá voltar!

## Características

<div align="center">  
  <img align="center" width=60% src= assets/imagens/sobre.gif alt="Sobre">
</div>

| **Elemento** | **Funcionalidade**                                                                                        |
|--------------|-----------------------------------------------------------------------------------------------------------|
| Nave 1       | Nave a ser movimentada com o acelerômetro.                                                                |
| Nave 2       | Nave a ser movimentada com o mouse.                                                                       |
| Buraco negro | Ponto final do jogo.                                                                                      |
| Estrela      | Os elementos passivos que vão aparecer pelo labirinto, caso toque em um jogador, ele volta para o início. |
| Labirinto    | Labirinto que se autogera, assim, toda partida temos um labirinto diferente.                              |

<div align="center">  
  <img align="center" width=60% src= assets/imagens/controle-do-jogo.png alt="Controle do jogo">
  <p><em>Controle do jogo</em></p>
</div>

# Uso da GraphLib

# Colisão

# Mouse 

# Labirinto randômico

# Tecnologias e Ferramentas utilizadas
- **Hardwares:**   
  - Kit de Desenvolvimento DE1-SoC
  - Monitor
  - Mouse
- **Linguagens:** Assembly e C
- **Ambiente de Desenvolvimento:** Visual Studio Code   
- **Compilador:** GCC  
- **Controle de Versão:** Git     
- **Ferramenta de Sistema:** Terminal Linux (Ubuntu)

# Configurações de Ambiente e Execução
Para ter acesso ao projeto, clone o repositório disponível na plataforma GitHub utilizando o seguinte comando no terminal Linux:
```bash
git clone https:
```
Após clonar o repositório, conecte-se à placa via SSH utilizando o seu respectivo IP. Por exemplo, se o IP for `10.0.0.120`, use o seguinte comando:
```bash
ssh aluno@10.0.0.120
```
Em seguida, transfira a pasta clonada do seu computador para o sistema de arquivos da placa:
```bash
mv Lost-in-Space/[caminho do destino]
```
Para compilar e executar o projeto desenvolvido, navegue até o diretório onde está o repositório e execute o comando:
```bash
make
```
O comando `make` gerará o arquivo de compilação e o executará. Se a operação for bem-sucedida, a tela inicial do Joguinho deverá aparecer no monitor ao qual a placa está conectada.
<br>
⚠️ **Observação:** para seguir esse passo a passo será necessário saber a senha do usuário `aluno`.


# Desenvolvedoras
<table>
  <tr>
    <td align="center"><img style="" src="https://avatars.githubusercontent.com/u/142849685?v=4" width="100px;" alt=""/><br /><sub><b> Brenda Araújo </b></sub></a><br />👨‍💻</a></td>
    <td align="center"><img style="" src="https://avatars.githubusercontent.com/u/89545660?v=4" width="100px;" alt=""/><br /><sub><b> Naylane Ribeiro </b></sub></a><br />👨‍💻</a></td>
    <td align="center"><img style="" src="https://avatars.githubusercontent.com/u/143294885?v=4" width="100px;" alt=""/><br /><sub><b> Sara Souza </b></sub></a><br />👨‍💻</a></td>    
  </tr>
</table>


# Referências
- [1] FPGAcademy. (2024) https://fpgacademy.org/
- [2] Trabalho de Conclusão de Curso de Gabriel Sá Barreto Alves. (2024) https://drive.google.com/file/d/1MlIlpB9TSnoPGEMkocr36EH9-CFz8psO/view
- [3] Artigo de Rômulo C. Menezes Jr http://www.de.ufpb.br/~labteve/publi/2012_cmac2.pdf
  
<img width=100% src="https://capsule-render.vercel.app/api?type=waving&color=6959CD&height=120&section=footer"/>
