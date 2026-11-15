# Alimentação e chaveamento

Registro das decisões de projeto sobre alimentação e chaveamento, com a lista de
materiais que resulta delas. Escrito em 25/08/2026, revisado em 28/08/2026.

**Configuração vigente: D10**, a migração para célula de lítio recarregável,
decidida depois que a carga real do brinquedo finalmente foi identificada.

Este documento existe para responder "por que está assim?" seis meses depois.
Cada decisão traz o número que a sustenta e o que foi descartado no caminho.
Os cálculos completos, com folhas de dados primárias citadas, estão na nota
técnica publicada como artefato.

---

## Parte I. Decisões

| # | Decisão | Situação |
|---|---|---|
| D1 | O brinquedo opera em 4,5 V, de três pilhas AA em série | **Superada** por D10: o trilho passa a 5,5 V e a carga recebe 4,5 V depois da queda do transistor |
| D2 | ESP32 alimentados por bateria A23 de 12 V com regulador L7805CV | **Encerrada** por D10, na receptora |
| D3 | As três pilhas AA dão lugar a uma célula de lítio 14500 ou 16500, com carga por USB-C | **Retomada e vigente** por D10, na forma 14500 |
| D4 | A receptora é alimentada pela mesma célula, 3,7 a 4,2 V direto no pino VIN | **Retomada em outra forma** por D10: a célula alimenta a receptora, mas através do elevador, não direto |
| D5 | O TIP122 sai e entra MOSFET de canal N nível lógico | **Adiada**, agora com número: 0,98 A garantidos cobrem a carga medida |
| D6 | A emissora ganha célula de lítio própria, com TP4056 e regulador linear | **Adiada**, fora do escopo de D10 |
| D7 | Nenhum conversor comutado em ponto algum do projeto | **Parcialmente revertida** por D10 |
| D8 | Trocar a placa da emissora para ESP32-C3 Super Mini | **Recomendada, não decidida** |
| D9 | Entregar já com as peças em mãos: TIP122, duas A23 em paralelo por unidade, L7805CV e chave geral | **Superada** por D10 |
| **D10** | **Célula de lítio 14500 com TP4056 e elevador MT3608 em 5,5 V, trilho único para brinquedo e receptora** | **Vigente** |

> **Leia D10 primeiro.** Ela é a configuração que vai ser montada.
> D9 continua registrada porque foi a configuração vigente entre 25/08 e 28/08,
> e porque o motivo pelo qual ela caiu é a parte instrutiva da história: não foi
> um erro de cálculo, foi um dado que faltava.

### D2. Por que a A23 foi reprovada como solução permanente

Escolhida em 24/08 e revertida em 25/08 depois de calcular com folhas de dados
primárias. Reprovada por três critérios independentes, cada um suficiente por si:

- **Capacidade.** A Energizer publica a queda de capacidade com o dreno: 52 mAh
  a 2 mA, 27 mAh a 15 mA, e o gráfico **termina em 15 mA**. Extrapolando a curva
  do próprio fabricante para os 90 mA do ponto de operação sobram cerca de
  **15 mAh**, ou dez minutos de autonomia.
- **Queda mínima do regulador.** Para o L7805 continuar regulando durante os
  picos de 335 mA, a resistência interna teria de ficar abaixo de 14,9 Ω com a
  bateria nova e de 2,99 Ω aos 8 V. A exigência aperta conforme a bateria
  descarrega e a resistência interna faz o contrário.
- **Exigência do fabricante do SoC.** A Espressif especifica fonte capaz de
  0,5 A. A A23 é caracterizada sobre carga de 20 kΩ, ou 0,48 mA. Mil vezes de
  distância.

Capacitor não resolve nenhum dos três: ele cobre o termo dinâmico e não toca no
integral.

**Isto continua verdadeiro, e D9 não o contradiz.** D9 mantém a A23 sabendo de
tudo isso, porque em uso comutado e intermitente a autonomia deixa de ser
medida em relógio e passa a ser medida em demonstrações. O que D2 estabelece é
que a A23 não serve como alimentação permanente, e essa conclusão segue de pé:
ela é a razão de D3 a D6 existirem como evolução planejada.

### D3. Célula de lítio no lugar das três pilhas AA

O ganho decisivo não é capacidade, é **resistência interna**. Três AA alcalinas
somam até 0,9 Ω; uma 14500 fica perto de 100 mΩ. Na partida do motor, com 1,5 A:

| Fonte | Queda no trilho |
|---|---|
| 3 × AA alcalina | **1,05 V** |
| 14500 | 0,15 V |
| 18650 | 0,06 V |

Aquele 1,05 V era o que ameaçava reinicializar o ESP32 no trilho compartilhado.
A troca de química resolve o problema na origem, e com ele saem o diodo Schottky
em série e o eletrolítico de 2200 µF que tinham sido especificados para
contorná-lo.

O segundo ganho é a forma da curva: a alcalina cai continuamente de 4,5 V até
acabar, a de lítio se mantém achatada em torno de 3,7 V. O brinquedo passa a
funcionar igual do começo ao fim da carga.

**A 18650 não cabe.** Tem 65 mm de comprimento contra cerca de 50 mm úteis do
compartimento. A 14500 é substituição direta de uma AA, e a 16500 é 1,5 mm mais
gorda com 50 % mais capacidade.

**O que se perde:** energia. Medida na corrente do motor, e não na de placa, a
perda é de cerca de **2,3 vezes**, porque a alcalina entrega apenas 1550 mAh a
500 mA contra os 2550 mAh de 25 mA. Em troca, recarrega.

### D4. Receptora alimentada pela célula, direto no VIN

Compartilhar a fonte com o brinquedo não cria restrição nova, porque o
chaveamento de lado baixo **já obriga** terra comum entre a fonte e o GND da
placa. Compartilhar o positivo apenas elimina um segundo pacote de dentro do
brinquedo.

Fica pendente uma verificação: o pino VIN entra no regulador da própria placa, e
é a queda dele que decide até onde a célula é aproveitada. Com um ME6211 ou
equivalente de baixa queda, o sistema opera até a célula chegar a cerca de
3,42 V. Com um AMS1117 desiste bem antes.

**Sem Schottky em série, por decisão consciente.** Ele custaria 0,25 V, e como a
célula precisa ficar acima de 3,42 V para o regulador entregar 3,3 V, esses
0,25 V descartariam perto de metade da carga útil. Em 4,5 V de alcalina o diodo
era barato; em 3,7 V de lítio é caro demais. A consequência é operacional:
**desligar a célula pela chave geral antes de plugar o USB para gravar**, porque
sem o diodo os 5 V do USB entram na célula.

### D5. MOSFET no lugar do TIP122

Adiada por D9, que mantém os TIP122 já disponíveis. Fica registrada porque é
**condição de D3**: no dia em que a célula de lítio entrar, esta troca deixa de
ser melhoria e vira requisito, já que a tensão nominal cai de 4,5 V para 3,7 V e
a queda de um Darlington é aproximadamente fixa:

| Montagem | Motor a 0,5 A |
|---|---|
| Hoje: 3 × AA nova + TIP122 | 3,15 V |
| Hoje: 3 × AA meia-vida + TIP122 | 2,10 V |
| Lítio + TIP122 | 3,15 V cheia, **2,65 V** meia |
| **Lítio + MOSFET** | **4,13 V** cheia, 3,63 V meia |

Com o Darlington, trocar a química deixaria o brinquedo mais fraco no meio da
carga do que ele é hoje. Com o MOSFET ele melhora em todos os pontos.

A razão física: a queda do Darlington é o piso estrutural
V<sub>BE</sub> + V<sub>CE(sat)</sub> do par, cerca de 0,9 V, e **não diminui com
mais corrente de base**. A do MOSFET é I × R e tende a zero junto com a carga.

**Componente escolhido: AO3400A**, em SOT-23. Especificado em V<sub>GS</sub> de
2,5 V com limiar de 650 mV, o que casa de verdade com um GPIO de 3,3 V, e mede
2,9 × 1,3 mm contra os 10 × 15 mm mais aba de um TO-220.

Sob D9 os TIP122 permanecem. Esta seção descreve a substituição planejada.

**O MOSFET traz duas obrigações que o Darlington dispensava:** resistor de 100 Ω em série
com a porta, para não exceder o limite de 40 mA do pino, e resistor de 100 kΩ da
porta para o source, porque o TIP122 tinha resistores internos que o mantinham
cortado durante o reset e o MOSFET não tem.

### D6. Emissora com célula de lítio própria

Ela não tem fonte por perto, então carrega a sua. E aqui vale registrar o erro
que quase aconteceu: **uma célula de lítio não pode ir direto em nenhum dos dois
pinos óbvios da placa**. No VIN, o AMS1117 da DevKit V1 precisa de cerca de
4,4 V, e uma célula cheia tem 4,2 V. No pino 3V3, os 4,2 V passam do máximo
absoluto de 3,6 V do ESP32 e queimam o chip.

Daí o regulador linear de baixa queda entre a célula e o pino 3V3.

### D7. Nenhum conversor comutado

Aparece duas vezes na conversa, e as duas foram descartadas por motivos
diferentes:

- **Buck-boost para o 3,3 V da emissora.** Desnecessário: de uma célula de lítio
  para 3,3 V a razão é de 1,27 para 1, então o rendimento do linear já é 79 %
  com a célula cheia subindo para 94 % perto do fim, contra os cerca de 90 %
  constantes de um comutado. A diferença é de poucos pontos, e o linear é um
  SOT-23-5 de 2,9 × 1,6 mm contra um módulo de 22 × 17 mm.
- **Elevador para devolver 4,5 V ao brinquedo.** Resolveria um problema que D3 e
  D5 já resolveram, já que o pior ponto da célula entrega mais que o ponto médio
  das alcalinas. Custaria uns 10 % de rendimento sobre a corrente do motor, um
  indutor grande para sustentar 2 A de entrada na partida, e reintroduziria como
  ruído comutado o mesmo problema de trilho que a troca de química eliminou.

### D8. Trocar a placa da emissora (recomendada, não decidida)

A DevKit V1 trabalha contra os dois objetivos declarados, espaço e bateria:

1. **Deep sleep é impossível com o pinout atual.** O despertar por GPIO no ESP32
   clássico exige pinos RTC, e o **GPIO5 não é um deles**. O botão 1 não
   acordaria a placa. No C3 os pinos de despertar são GPIO0 a GPIO5, então
   GPIO4 e GPIO5, que o firmware já usa, funcionam sem mudar nada.
2. **A ponte USB-UART continua consumindo miliampères com o SoC dormindo**, e é
   ela que determina a autonomia real. A Super Mini tem USB nativo, sem ponte.
3. **O firmware já compila para os dois alvos**, com GPIO4/5 em ambos
   (`emissor/emissor.ino:55`). A troca é `FQBN_EMISSOR` e a opção
   `CDCOnBoot=cdc` no Makefile, que o alvo do receptor já usa.

### D9. Configuração de entrega, com as peças em mãos

Decidida em 25/08 por custo e prazo: montar com o que já existe na gaveta e
entregar o brinquedo funcionando. Duas baterias A23 **em paralelo** por unidade
alimentam o L7805CV, com **chave geral**, e os TIP122 permanecem.

**A chave geral é o que torna isso viável, e não é um detalhe.** Duas A23 em
paralelo rendem cerca de 25 minutos, e 25 minutos de relógio seriam inúteis. Mas
com a chave, esses 25 minutos são de **uso acumulado**, o que dá aproximadamente
cinco demonstrações de cinco minutos por par de baterias. Para entregar,
apresentar e testar, funciona.

**O que o paralelo realmente compra.** Mais do que o dobro, e por um motivo
específico: cada célula passa a trabalhar em 45 mA em vez de 90 mA, e a
penalidade de dreno da alcalina é fortemente não linear.

| Arranjo | Por célula | Total | Autonomia |
|---|---|---|---|
| 1 × A23 | 15,1 mAh | 15,1 mAh | 10 min |
| **2 × A23 em paralelo** | 18,9 mAh | **37,8 mAh** | **25 min** |

E o paralelo ataca também o segundo modo de falha, o da queda mínima do
regulador, porque a resistência interna vista pelo circuito cai pela metade. O
teto admissível por célula dobra:

| Tensão da bateria | Resistência interna máxima por célula |
|---|---|
| 12 V, nova | 29,9 Ω |
| 10 V | 17,9 Ω |
| 8 V | 6,0 Ω |

**Cuidado ao ligar alcalinas em paralelo.** Célula primária não é feita para
isso: se uma estiver mais nova que a outra, a mais forte empurra corrente para
dentro da mais fraca, o que desperdiça energia e pode fazer a mais fraca vazar.
Duas regras simples resolvem: **use células do mesmo lote e troque sempre o par
junto**. Se preferir garantia em vez de disciplina, um Schottky em série com
cada célula impede a corrente cruzada e custa 0,4 V de um orçamento que tem 5 V
de folga até o limite de 7 V. A contrapartida é que elas passam a descarregar em
sequência em vez de dividir, e a autonomia cai de 25 para cerca de 20 minutos.

**O que se aceita ao manter o TIP122.** Em 4,5 V, a queda de 0,9 a 1,2 V entrega
ao motor entre 80 % e 73 % da tensão. O brinquedo funciona, apenas com menos
força do que teria. É uma escolha consciente de prazo, não um descuido, e está
registrada aqui para não virar dívida esquecida.

**O lado térmico continua confortável:** 0,63 W no L7805 a 90 mA, cerca de 32 °C
de elevação em TO-220 sem dissipador.

> **Uma coisa vale a pena mesmo na versão rápida: os diodos de roda-livre.**
> Custam centavos, provavelmente já estão na gaveta, e são obrigatórios em
> qualquer das topologias. O relatório afirma hoje que o diodo interno do TIP122
> dispensa esse componente, e essa afirmação está errada pelo motivo descrito em
> "Correções pendentes". Sem eles, o chute indutivo do motor bate direto no
> coletor a cada desligamento.

**Ponto a confirmar.** "As duas pilhas em paralelo alimentando o regulador"
admite duas leituras, e elas mudam a compra: **duas células por unidade, quatro
no total**, que é a hipótese adotada aqui porque cada unidade tem seu próprio
regulador e as duas só se acoplam por rádio, ou duas no total, o que deixaria
uma das unidades sem alimentação.

### D10. Célula de lítio, trilho único em 5,5 V (vigente)

Decidida em 28/08/2026. Substitui D9 inteira na unidade receptora.

**A topologia.** Uma única cadeia alimenta tudo o que está dentro do brinquedo:

```
célula 14500  →  TP4056 com proteção  →  chave geral  →  MT3608 em 5,5 V  →  trilho único
     (lítio, 3,0 a 4,2 V)   (carga por USB-C)      (em OUT+)      (elevador)          │
                                                                                      ├→ ESP32-C3 (pino 5V)
                                                                                      ├→ LEDs, via Q1
                                                                                      └→ motor N20, via Q2
```

Saem do projeto, na receptora: as duas A23, o L7805CV e as três pilhas AA.
Permanecem: os dois TIP122 com resistor de base de 330 Ω, e o chaveamento de
lado baixo. A unidade emissora não é tocada por D10.

#### O que destravou a decisão: o dado que faltava

Esta é a parte que vale registrar, porque a decisão não mudou por um cálculo
novo, mudou por uma medida que nunca tinha sido feita.

Desde 20/08 os documentos vinham assumindo **0,5 A em regime e 1,5 A na
partida** para a carga do brinquedo. Esses números nunca foram medidos: eram um
motor genérico de brinquedo, adotado como espaço reservado enquanto a medição não
acontecia. Toda a análise de alimentação foi construída sobre eles, inclusive a
rejeição do elevador em D7.

Em 28/08 o brinquedo foi aberto e a carga real apareceu: **um micromotor N20 e
uma linha de LEDs**. Um N20 fica uma ordem de grandeza abaixo da estimativa que
estava no lugar dele. Três conclusões caíram junto:

| O que se afirmava | Sobre que número | O que acontece com o número real |
|---|---|---|
| O MT3608 entraria em limitação de corrente e derrubaria a receptora | 1,5 A de partida, refletidos em ~2,6 A na entrada do elevador | O pior caso fica em ~2,2 A contra os 4 A do limite interno. **Cai** |
| A queda do Darlington é severa demais em 4,5 V | 4,5 V de trilho fixo | O trilho passa a ser escolhido, não herdado. **Cai** |
| A correção do beta forçado depende da medição | 0,98 A garantidos contra carga desconhecida | Um N20 mais LEDs cabe com folga. **Cai** |

**A lição de processo:** um valor colocado como espaço reservado se comportou,
por oito dias, como se fosse um dado. Ele foi propagado para três seções do
relatório e sustentou uma decisão de arquitetura, D7, que precisou ser revertida.
O adiamento da medição custou mais do que a medição teria custado.

#### Por que o elevador, e não a célula direto no pino 5V

Esta foi a única discordância técnica real da conversa, e a primeira resposta
estava errada. Vale registrar por que.

O argumento inicial era que o elevador desperdiça autonomia e que a célula
deveria alimentar a placa diretamente, já que 3,7 V bastariam. A conta que
sustentava isso comparava 9,5 h no direto contra 5,8 h no elevador, e concluía
por uma perda de 39 %.

**Essa conta estava viciada**, e o furo é o seguinte: ela supôs que o regulador
da própria placa continua regulando até o fundo da descarga da célula. Ele não
continua. A queda mínima de um regulador linear de baixa queda não é constante,
ela cresce com a corrente. No ME6211, típico dessas placas, a queda vai de cerca
de 120 mV em 100 mA para cerca de 420 mV em 350 mA, e 350 mA é exatamente o pico
que o rádio puxa ao transmitir. Nesse instante o regulador exige aproximadamente
3,72 V na entrada, que é o **patamar nominal da célula**, não o fundo dela.

Ou seja: sem elevador, o sistema só usa a parte de cima da descarga, e o resto da
célula fica inacessível porque a receptora reinicia por subtensão antes.
Refazendo com esse efeito incluído:

| Arranjo | Capacidade aproveitável | Autonomia | Observação |
|---|---|---|---|
| Célula direto no pino 5V | ~545 mAh de 800 | ~6,5 h | Perde o fundo da descarga; reinicia nos picos de rádio |
| **Célula → MT3608 em 5,5 V** | **800 mAh** | **~5,8 h** | Usa a célula inteira, até 3,0 V |

O elevador custa cerca de **11 % de autonomia**, não 39 %, e em troca destrava o
fundo da célula e elimina a reinicialização por subtensão durante a transmissão.
**O usuário estava certo e a recomendação inicial foi retratada.** O trilho único
ganha ainda pelo lado da montagem: um só terra, um só ponto de distribuição.

#### Por que 5,5 V, e não 5,0 V

O número é escolhido de trás para frente, a partir do que o motor precisa
receber. O TIP122 é um Darlington, e Darlington não satura em 0,2 V como um
transistor comum: nesta faixa de corrente a queda fica em torno de **1,0 V**.
Então:

```
5,5 V (trilho)  −  1,0 V (queda do Darlington)  =  4,5 V na carga
```

que é exatamente a tensão de projeto original do brinquedo, as três pilhas AA em
série. **O elevador não está compensando uma deficiência da célula, está
compensando a queda do transistor.** Com 5,0 V no trilho o motor receberia 4,0 V
e giraria mais devagar que no acionamento original por chave mecânica, o que
seria percebido como uma piora causada pela automação.

#### D7 fica parcialmente revertida

D7 dizia "nenhum conversor comutado em ponto algum do projeto". Ela cai na parte
do elevador e continua de pé na parte do buck-boost da emissora.

O argumento original contra o elevador tinha três pernas, e duas quebraram:

- ~~"Um indutor grande para sustentar 2 A de entrada na partida"~~: com um N20 o
  pior caso é 2,2 A instantâneos, dentro do módulo pronto de 4 A.
- ~~"Resolveria um problema que D3 e D5 já resolveram"~~: não resolve o mesmo
  problema. D3 e D5 tratavam da tensão entregue ao motor; o elevador aqui trata
  da queda mínima do regulador da placa nos picos de rádio, que é outro
  fenômeno.
- **"Reintroduziria ruído comutado no trilho"**: esta continua verdadeira, e é
  por isso que D10 traz capacitores que D9 não tinha. Ver abaixo.

#### Acréscimos que não estavam em nenhum documento anterior

Três componentes entram por causa da topologia nova, e nenhum deles é opcional:

1. **C_out de 220 µF na saída do MT3608.** Quando o motor desliga, o elevador
   leva alguns ciclos para perceber que a carga sumiu, e nesse intervalo a saída
   sobe. O regulador da placa C3 tem **6,0 V de máximo absoluto**, e o trilho já
   está em 5,5 V: a margem é de meio volt. O capacitor é o que absorve esse
   excesso. Sem ele, o componente que morre é a placa, não o motor.
2. **C_in de 100 µF na entrada do MT3608.** Segura a corrente pulsada que o
   elevador puxa da célula, para que ela não apareça como ondulação de tensão.
3. **Três cerâmicos de 100 nF no motor N20**, um entre os terminais e um de cada
   terminal para a carcaça. Motor de escovas gera faísca, e faísca é ruído de
   rádio de banda larga. Em D9 isso era tolerável porque o motor tinha
   alimentação separada. Em D10 ele divide o trilho com um rádio de 2,4 GHz.

E o **diodo de roda-livre**, que já era obrigatório em D9 e continua sendo, agora
com modelo definido: **1N4007**. A escolha de um retificador de silício comum, em
vez de um Schottky, se justifica porque `receptor/receptor.ino` aciona as saídas
com `digitalWrite` puro, sem PWM. Sem chaveamento repetitivo, o tempo de
recuperação reversa do diodo não pesa, e o 1N4007 é mais barato e mais robusto.

#### A chave original do brinquedo: o que a inspeção mostrou

A suposição inicial era que a chave do brinquedo unia os modos ao terra, e que
bastaria colocar o transistor no lugar dela. **A inspeção mostrou o contrário.**

A chave é de quatro estágios e é de **lado alto**: o comum distribui o positivo
para as linhas de carga. As posições são: nada, só o fio vermelho de VCC, VCC
unido ao estágio 1 (LEDs), e VCC unido aos estágios 1 e 2 (LEDs e motor).

Duas consequências:

- **O transistor não vai no lugar da chave.** Como LEDs e motor são cargas de
  dois terminais, cada transistor entra no **fio de retorno** da sua carga. O
  chaveamento de lado baixo que o projeto já usa continua valendo sem alteração.
- **A chave vira a chave geral.** A posição "nada" desliga a célula, e as
  posições 2 a 4 são unidas entre si. Isso resolve de graça uma exigência que D4
  tinha deixado em aberto: isolar a célula antes de gravar firmware pela USB.

**Ponto a verificar na bancada:** a posição 2, "só o fio vermelho de VCC", não
teve carga identificada. Confirmar o que ela alimenta antes de unir as posições
2 a 4.

#### O que D10 não resolve

- **D5 continua adiada.** O MOSFET eliminaria a queda de 1,0 V e dispensaria o
  elevador de subir a 5,5 V. Não entra agora porque o TIP122 já está comprado e
  porque 0,98 A garantidos cobrem a carga real com folga.
- **D6 continua adiada.** A emissora segue com A23 e L7805CV, com todas as
  limitações que D2 documentou.
- **Segurança de célula de lítio.** O TP4056 escolhido é a variante **com
  proteção** (TP4056 mais DW01A mais FS8205, seis pinos, USB-C). A proteção atua
  nos terminais OUT, não nos B: a carga deve ser tirada de OUT+ e OUT−, nunca de
  B+ e B−. Ligar em B contorna a proteção contra descarga profunda e
  curto-circuito.
- **O resistor R_PROG do TP4056 vem errado de fábrica** para esta célula. Vem
  1,2 kΩ, marcado `122`, que programa 1 A de carga. Para uma 14500 de 800 mAh o
  alvo é 0,5 C, ou seja, cerca de 0,37 A: trocar por **3 kΩ**, marcado `302`,
  pela relação R = 1100 / I_carga.

#### Guia de montagem

O passo a passo visual, em blocos, com os diagramas de ligação, está em
`docs/montagem-litio.html`. Ele é material de bancada, não documento acadêmico:
a versão normalizada, com símbolos conforme NBR IEC 60617, está nas Figuras 3 e 4
do relatório.

---

## Estado do firmware, verificado em 25/08

**Não há deep sleep em nenhuma das pontas.** Mais do que isso, o rádio está
explicitamente impedido de dormir em `emissor/emissor.ino:204`
(`WiFi.setSleep(false)`) e a `tarefaBotoes` varre os pinos a cada 5 ms sem parar.
O rádio fica ligado 100 % do tempo, nas duas unidades.

Implementar não é chamar uma função:

- **Deep sleep no ESP32 é reinicialização.** As três tasks, as filas e o
  `bootId` recomeçam do zero. O `bootId` é sorteado por boot para invalidar a
  sequência antiga (`lib/protocolo.h:10`); com sono profundo ele passa a mudar a
  cada aperto, o que neutraliza a proteção contra repetição entre eventos.
  Dentro de uma mesma rajada ela continua valendo.
- **Só o aperto gera pacote.** Verificado em `lib/debounce.cpp:29`: a borda de
  subida atualiza o estado estável e retorna `false`, então a soltura não
  transmite nada. Cada aperto inverte o estado e envia o estado absoluto
  resultante. Isso favorece o sono profundo: o ciclo é "acorda, envia a rajada,
  dorme", sem precisar ficar acordado enquanto o botão está pressionado.
- **Latência.** Do despertar até o ESP-NOW pronto são algumas centenas de
  milissegundos, e isso aparece como atraso entre apertar e o brinquedo reagir.

É reestruturação da arquitetura de tasks, não ajuste. E o tamanho da célula
depende disso: sem sono, o rádio a 84 mA obriga a carregar célula grande; com
sono, quase qualquer célula serve e a escolha passa a ser pelo formato da caixa.

---

## Correções do relatório: aplicadas em 28/08

> **Situação: FECHADAS.** As duas correções abaixo foram aplicadas ao
> `docs/relatorio.html` em 28/08/2026, junto com a reescrita das seções 5 e 6
> exigida por D10. O texto anterior, com os dois erros preservados, está
> congelado em `docs/relatorio-v1-a23.html` para comparação, e o que mudou está
> listado em `docs/registro-de-mudancas.md`.

Duas são erros técnicos encontrados durante esta análise, e as duas estão na
seção sobre a escolha do TIP122:

1. **O cálculo `1000 × 3,9 mA = 3,9 A`.** Os 1000 são o ganho especificado em
   **V<sub>CE</sub> = 3 V**, ou seja, na região ativa. Num trilho de 4,5 V, ter
   V<sub>CE</sub> de 3 V significaria entregar 1,5 V à carga: a condição sob a
   qual esse ganho vale é uma condição que o circuito nunca ocupa. A saturação é
   caracterizada com beta forçado de 250, o que dá **0,98 A**. A afirmação de
   folga de três ordens de grandeza também cai. **Esta correção já não depende de
   medição**, porque o argumento é uma razão entre condições de ensaio.
2. **A afirmação de que o diodo interno do TIP122 dispensa o diodo de
   roda-livre.** É falsa. O diodo integrado tem anodo no emissor e catodo no
   coletor, então conduz apenas quando o coletor vai abaixo do emissor. No
   chaveamento de lado baixo o chute indutivo joga o coletor para **cima**, e
   esse diodo fica reversamente polarizado. O diodo de corpo de um MOSFET tem a
   mesma orientação inútil, então a troca de transistor não corrige a omissão.

Além disso, D3 a D7 invalidam a seção 6 inteira, a Figura 4 e a lista de
materiais do relatório, que ainda descrevem a topologia da A23. **D10 fechou
também esse ponto:** a seção 6 foi reescrita para a cadeia de lítio, a Figura 4
foi redesenhada e o Quadro de componentes foi atualizado.

**A nota técnica publicada também está parcialmente vencida:** a seção 3 e a
Figura 1 mostram três pilhas AA com Schottky e buck-boost, topologia substituída
por D3 e D4. O resto da nota, sobre a A23 e sobre MOSFET contra TIP122, segue
válido.

---

## O que ainda depende de medição

Atualizado em 28/08. O item 3, que travava o projeto desde 20/08, foi **resolvido
por inspeção** e não por medição: abrir o brinquedo identificou a carga como
motor N20 mais LEDs, o que bastou para destravar D10.

| # | O que medir | O que destrava | Situação |
|---|---|---|---|
| 1 | Marcação do regulador da placa C3, chip de 5 pinos perto do pino 5V | Confirma a margem até os 6,0 V de máximo absoluto, que é o que C_out protege | **Aberto**, e é o mais urgente de D10 |
| 2 | Código dos MOSFETs que já estão na gaveta | Diz se é preciso comprar os AO3400A | Aberto, mas D5 está adiada |
| 3 | Corrente do brinquedo por modo, multímetro em série na escala de 10 A | Travava o dimensionamento desde 20/08 | **Fechado por inspeção** em 28/08: N20 mais LEDs |
| 4 | Dimensões internas reais do compartimento de pilhas | Decide entre 14500 e 16500 | **Fechado** por D10: 14500 |
| 5 | Resistência interna das três fontes, com resistor de carga | Vira evidência para o relatório | Aberto, hoje é ilustrativo e não bloqueia nada |
| 6 | **Se os retornos dos LEDs e do motor chegam separados** | Único ponto que invalidaria o chaveamento de lado baixo por modo | **Aberto**, e é o mais crítico de D10 |
| 7 | O que a posição 2 da chave de quatro estágios alimenta | Confirma que as posições 2 a 4 podem ser unidas para virar chave geral | **Aberto** |

Para o pico de partida, que um multímetro não captura, o atalho é medir a
resistência ôhmica do motor e calcular I = V / R, que dá a corrente de rotor
bloqueado, o pior caso.

**Se o item 6 der "retorno único"**, os dois transistores deixam de poder comandar
os modos independentemente, e a saída passa a ser reproduzir a lógica da chave
original: um transistor no retorno comum e o segundo chaveando só o motor. É uma
mudança de Figura 3, não de topologia de alimentação, e D10 permanece válida.

---

## Parte II. Lista de materiais

### Configuração vigente (D10): comprar agora

Esta é a lista que corresponde a D10, e cobre **apenas a unidade receptora**,
que é a que fica dentro do brinquedo. A emissora não muda.

| Ref | Item | Especificação | Qtd | Observação |
|---|---|---|---|---|
| BT1 | Célula de lítio 14500 | 3,7 V nominal, ~800 mAh, com **topo saliente** | 1 | Topo plano não faz contato em porta-pilhas de AA. |
| U1 | Módulo TP4056 **com proteção** | Variante USB-C, seis pinos, com DW01A e FS8205 | 1 | A versão sem proteção é fisicamente parecida. Conferir os dois CIs pequenos ao lado do TP4056. |
| R_PROG | Resistor SMD 0805 | **3 kΩ**, marcado `302` | 2 | Substitui o de fábrica (1,2 kΩ, `122`). Baixa a carga de 1 A para ~0,37 A. Comprar dois: é minúsculo e some. |
| U2 | Módulo elevador MT3608 | Ajustável, chave interna de 4 A | 1 | Ajustar para **5,5 V com multímetro antes de ligar qualquer carga**. |
| S1 | Chave geral | A própria chave de quatro estágios do brinquedo | 0 | Posição 1 vira desligado, posições 2 a 4 unidas. Ver D10. |
| D1 | Diodo de roda-livre | **1N4007** | 4 | Em antiparalelo com o motor, **anodo no coletor e catodo no +5,5 V**. Dois de reserva. |
| C_in | Capacitor eletrolítico | 100 µF, 16 V | 1 | Entrada do MT3608. |
| C_out | Capacitor eletrolítico | **220 µF, 16 V** | 1 | Saída do MT3608. É o que protege o regulador da placa C3. Não omitir. |
| C1 a C3 | Capacitor cerâmico | 100 nF, 50 V | 10 | Três no motor N20 contra ruído de escovas, um na alimentação da placa. |
| | Porta-pilhas AA simples | Para acomodar a 14500 no compartimento | 1 | A 14500 tem o mesmo formato de uma AA. |

**Já em mãos, não comprar:** os dois TIP122, os resistores de base de 330 Ω, a
placa ESP32-C3 Super Mini.

**Sai do projeto na receptora:** as duas A23, o L7805CV, os capacitores de
330 nF e as três pilhas AA.

> **Ordem de energização, na primeira montagem.** Ajustar o MT3608 para 5,5 V com
> o multímetro **antes** de ligar a placa C3 nele. O módulo sai de fábrica em
> tensão arbitrária, e o regulador da placa tem 6,0 V de máximo absoluto.

**O que D10 não resolve, e fica registrado:** a emissora continua com A23 e
L7805CV, com a autonomia curta que D2 documentou. E a queda de 1,0 V do
Darlington continua existindo: ela deixou de importar porque o trilho subiu para
5,5 V, não porque desapareceu.

---

Reflete as decisões de 25/08/2026: as três pilhas AA do brinquedo dão lugar a
uma célula de lítio recarregável com carga por USB-C, a receptora passa a ser
alimentada por ela, a emissora ganha célula própria, e o chaveamento migra de
Darlington para MOSFET. As duas últimas mudanças deixaram de ser independentes:
ver seção 1.

Os cálculos que fundamentam essas escolhas estão na nota de alimentação
(`docs/`, artefato publicado). Os valores de capacitância partem de 50 ms de
partida do motor e 90 mA de consumo, ambos ainda não medidos: ver
"Antes de comprar" no fim.

---

### Antes de comprar

Três verificações mudam o que entra na lista. Vale fazer as três primeiro.

| # | O que verificar | Por que decide |
|---|---|---|
| 1 | **Marcação do regulador da placa C3.** Chip de 5 pinos, SOT-23-5, perto do pino 5V. | Decide se 4,5 V direto no VIN funciona. `6211`, `662K`, `SGM2036` são de baixa queda e servem. `AMS1117` não serve, ver seção 1. |
| 2 | **Código dos MOSFETs que você já tem.** | Se forem nível lógico, você não compra os IRLB8721. Critério na seção 3. Com a troca para lítio, o MOSFET deixou de ser melhoria e virou requisito. |
| 3 | **Corrente do brinquedo por modo.** Multímetro em série, escala de 10 A. | Confirma a capacitância de C1 e o dimensionamento do chaveador. Trava o projeto desde 20/08. |

---

### 1. Unidade receptora, dentro do brinquedo

Decisão de 25/08/2026: o compartimento das três pilhas AA vira uma célula de
lítio recarregável com carga por USB-C. Isso muda a lista inteira desta seção, e
para melhor. Ver "Por que a troca melhora o circuito" abaixo.

| Ref | Item | Especificação | Qtd | Para quê |
|---|---|---|---|---|
| BT1 | Célula de lítio | **14500** (14 × 50 mm, 800 mAh) ou **16500** (16 × 50 mm, 1200 mAh). **18650 não cabe**, ver dimensões. | 1 | Substitui as 3 AA. |
|  | Suporte de célula | Do diâmetro escolhido, com fios, ou solda em abas | 1 | Nunca solde direto no corpo da célula sem abas: o calor danifica a separadora. |
|  | Módulo de carga **com proteção** | **TP4056 + DW01A + FS8205**, 6 pinos, USB-C | 1 | A proteção não é opcional aqui, ver o alerta de segurança. **Trocar o R<sub>PROG</sub>**, ver abaixo. |
| R<sub>PROG</sub> | Resistor SMD 1206 | **3 kΩ** para 14500, **1,8 kΩ** para 16500 | 4 | Substitui o resistor de fábrica do TP4056. |
| S2 | Chave geral | SPST, 2 A, entre a célula e o resto | 1 | Corta o consumo em repouso **e** isola a célula quando você plugar o USB da placa para gravar. |
| C1 | Capacitor eletrolítico | **220 µF / 10 V** | 2 | Desacoplamento geral. Não precisa mais dos 2200 µF, ver abaixo. |
| C2 | Capacitor cerâmico | 100 nF, 50 V | 10 | Um na entrada da placa, um em cada motor. |
| D1, D2 | Diodo de roda-livre | **1N5819** (Schottky) | 4 | Em antiparalelo com cada modo, anodo no coletor e catodo no positivo. Continua obrigatório. |
| Fio | Cabo flexível | 22 AWG, dois cores | 1 m | Retorno de terra da placa direto ao negativo da célula. |

**Saiu da lista:** o diodo Schottky em série (D3) e o eletrolítico de 2200 µF.
Os dois existiam para proteger o ESP32 do afundamento das pilhas alcalinas, e a
célula de lítio resolve o problema na origem.

#### Por que a troca melhora o circuito

A razão não é capacidade, é **resistência interna**. Uma pilha alcalina AA tem
de 150 a 300 mΩ, e três em série chegam a 0,9 Ω. Uma 14500 fica perto de
100 mΩ, uma 18650 perto de 40 mΩ. Na partida do motor, com 1,5 A:

| Fonte | Queda no trilho a 1,5 A |
|---|---|
| 3 × AA alcalina | **1,05 V** |
| 14500 | 0,15 V |
| 18650 | 0,06 V |

É esse 1,05 V que ameaçava reinicializar o ESP32 e que motivava o Schottky, os
2200 µF e o ensaio 4. Com lítio o problema some sozinho, e sobra apenas o bom
hábito do terra em estrela.

O segundo ganho é a curva. A alcalina cai continuamente de 4,5 V até acabar; a
de lítio se mantém achatada em torno de 3,7 V pela maior parte da descarga. O
brinquedo passa a funcionar igual do começo ao fim da carga, em vez de ir
enfraquecendo.

#### A troca torna o MOSFET obrigatório

Aqui está a armadilha. A tensão nominal cai de 4,5 V para 3,7 V, e um Darlington
que come 1 V de queda fixa passa de caro a inviável:

| Montagem | Motor em regime (0,5 A) | Motor na partida (1,5 A) |
|---|---|---|
| Hoje: 3 × AA nova + TIP122 | 3,15 V | 2,25 V |
| Hoje: 3 × AA meia-vida + TIP122 | 2,30 V | não medido |
| **Lítio + TIP122** (a armadilha) | 3,15 V cheia, **2,65 V** meia | não vale calcular |
| **Lítio + MOSFET** | **4,13 V** cheia, 3,63 V meia | **3,99 V** |

Com o TIP122, trocar para lítio deixa o brinquedo mais fraco no meio da carga do
que ele é hoje. Com o MOSFET, ele fica melhor em todos os pontos, e na partida a
diferença é enorme: 2,25 V hoje contra 3,99 V. As duas mudanças precisam andar
juntas.

#### Dimensões: a 18650 não cabe

| Célula | Diâmetro × comprimento | Volume | Capacidade |
|---|---|---|---|
| AA (LR6), referência | 14,5 × 50,5 mm | 8,3 cm³ | 2550 mAh alcalina |
| **14500** | 14 × 50 mm | 7,7 cm³ | ~800 mAh |
| **16500** | 16 × 50 mm | 10,1 cm³ | ~1200 mAh |
| 18650 | 18 × **65 mm** | 16,5 cm³ | ~2500 mAh |
| Bolsa LiPo 103450 | 10 × 34 × 50 mm | 17,0 cm³ | ~2000 mAh |

O compartimento de 3 AA tem cerca de 50 mm de comprimento útil, então os 65 mm
da 18650 não entram sem cortar o corpo do brinquedo. A **14500 é substituição
direta**, mesmo formato de uma AA. A 16500 é 1,5 mm mais gorda e rende 50 % a
mais.

O compartimento inteiro tem por volta de 32 cm³, e uma 14500 usa 7,7 cm³. Sobra
espaço de sobra para o módulo de carga e para a placa, que era o seu objetivo
original.

Se quiser capacidade máxima no mesmo envelope, a **bolsa LiPo é o melhor
aproveitamento de volume**, porque preenche os cantos que uma célula cilíndrica
desperdiça. A contrapartida é mecânica: a bolsa é um laminado de alumínio mole e
fura com facilidade, enquanto a cilíndrica tem lata de aço. **Num brinquedo que
vai cair no chão, a lata de aço é a escolha mais segura.**

#### O TP4056 de fábrica carrega rápido demais

O módulo vem com R<sub>PROG</sub> de 1,2 kΩ, o que programa cerca de **900 mA**.
Numa 14500 de 800 mAh isso é 1,1 C, no limite ou acima do que a célula aceita, e
encurta a vida dela. A relação da folha de dados é
R<sub>PROG</sub> = 1100 / I<sub>carga</sub>, com a corrente em ampères:

| Célula | Carga a 0,5 C | R<sub>PROG</sub> |
|---|---|---|
| 14500, 800 mAh | 400 mA | **3 kΩ** |
| 16500, 1200 mAh | 600 mA | **1,8 kΩ** |

Tem um segundo motivo, e ele é térmico. Carregando a 900 mA o TP4056 dissipa
(5 − 3,7) × 0,9 = **1,17 W** num encapsulamento SOP-8, dentro de um
compartimento fechado sem circulação de ar. A 400 mA cai para 0,52 W. O chip tem
limitação térmica e se protege reduzindo a corrente, mas projetar contando com
isso é errado.

O resistor é SMD e fica marcado como `122` na placa. Dessoldar e pôr um de 3 kΩ
(`302`) é trabalho de dois minutos.

#### Espaço: o que cabe nos dois slots livres

O compartimento inteiro tem por volta de 43,5 × 50,5 × 14,5 mm, cerca de 32 cm³.
Uma 14500 ocupa 7,7 cm³ e sobram uns 24 cm³.

| O que entra | Medida aproximada | Volume |
|---|---|---|
| ESP32-C3 Super Mini | 22,5 × 18 × 8 mm | 3,2 cm³ |
| Módulo TP4056 USB-C | 26 × 17 × 5 mm | 2,2 cm³ |
| Plaquinha com MOSFETs, resistores e diodos | 25 × 20 × 6 mm | 3,0 cm³ |
| Capacitor de 220 µF | 8 × 12 mm | 0,6 cm³ |
| Chave geral e fiação | | ~3 cm³ |
| **Total** | | **~12 cm³** |

Cabe com folga. Dois pontos práticos, porém:

- **As divisórias entre os slots têm que sair.** Cada slot tem só 14,5 mm de
  largura, e a placa C3 Super Mini tem 22,5 mm. O espaço útil é um bolso único de
  cerca de 43 × 50 mm, não três canaletas.
- **O conector USB-C precisa alcançar o lado de fora.** Planeje o recorte antes
  de fixar qualquer coisa, porque a posição do módulo de carga passa a ser
  determinada por ele, não pelo espaço que sobra.

#### Não coloque um buck aí

A ideia natural é usar o espaço para um conversor que devolva os 4,5 V originais
ao brinquedo. **Não vale a pena, porque o problema que ele resolveria já foi
resolvido pela própria troca.** Compare o que o motor recebe:

| | Motor a 0,5 A |
|---|---|
| Hoje: 3 × AA nova + TIP122 | 3,15 V |
| Hoje: 3 × AA meia-vida + TIP122 | 2,10 V |
| 14500 + MOSFET, célula cheia | **4,13 V** |
| 14500 + MOSFET, célula na reserva | **3,23 V** |

O pior ponto da célula de lítio entrega mais que o ponto médio das alcalinas. Um
elevador para 4,5 V custaria uns 10 % de rendimento em cima da corrente do motor,
justamente na parte apertada do orçamento, exigiria um indutor grande para
sustentar 2 A de entrada na partida, e reintroduziria como ruído comutado o
mesmo problema de trilho que você acabou de eliminar trocando a química.

O rendimento certo para esse espaço é **uma célula maior**: a 16500 rende 50 % a
mais no mesmo comprimento.

#### Autonomia: aqui é onde você perde

Essa é a única perda real da troca, e vale medir a régua certa. A capacidade de
placa da alcalina não é a que ela entrega na corrente do motor: a folha da
Energizer dá **1550 mAh a 500 mA**, não os 2550 mAh de 25 mA. A célula de lítio
não sofre essa degradação.

| Fonte | Carga a 500 mA | Energia útil |
|---|---|---|
| 3 × AA alcalina | 1550 mAh a 4,5 V | 7,0 Wh |
| 14500 | 800 mAh a 3,7 V | 3,0 Wh |
| 16500 | 1200 mAh a 3,7 V | 4,4 Wh |

Ou seja, a perda é de cerca de **2,3 vezes** contra a 14500, não as quase quatro
vezes que a comparação de placa sugeriria. Boa parte da vantagem aparente da
alcalina evapora justamente na corrente em que o brinquedo trabalha.

O que decide o intervalo entre cargas, porém, não é o motor, é o rádio:

| Consumo | Por dia |
|---|---|
| Rádio escutando, 84 mA, 4 h ligado | 336 mAh |
| Motor, 500 mA, 30 min de uso | 250 mAh |
| **Total** | **~590 mAh** |

Uma 14500 dá pouco mais de um dia por carga; uma 16500 dá dois. Com o rádio
desligado pela chave S2 quando o brinquedo não está em uso, bem mais. E se o
intervalo incomodar, o remédio é o mesmo da emissora e é de firmware: sono leve
com janela de escuta derruba a média do rádio para poucos mA e tira o motor do
segundo lugar para o primeiro.

#### Sobre o USB e o Schottky

Sem o Schottky em série, plugar o USB da placa com a célula conectada empurra
5 V para dentro do lítio. A proteção do módulo corta em torno de 4,3 V, mas
depender dela é má prática. Por isso a chave S2 está na lista: **desligue a
célula antes de gravar firmware**.

Recolocar o Schottky resolveria isso, mas custa 0,25 V, e como a célula precisa
ficar acima de cerca de 3,42 V para o regulador da placa entregar 3,3 V, esses
0,25 V descartariam perto de metade da carga útil. Em 4,5 V de alcalina o diodo
era barato; em 3,7 V de lítio ele é caro demais.

> **Segurança, uma vez e sem rodeios.** Célula de lítio dentro de brinquedo
> infantil exige três cuidados que não são opcionais: **módulo com proteção**
> contra sobrecarga, descarga excessiva e curto; **fixação mecânica** da célula
> com tampa aparafusada, não a portinhola deslizante que o compartimento de
> pilhas costuma ter; e **não carregar sem supervisão**. Uma célula perfurada ou
> esmagada entra em fuga térmica, e a proteção eletrônica não faz nada contra
> dano físico. Se o relatório precisar citar a norma aplicável, é a ABNT NBR NM
> 300 (segurança de brinquedos) e a IEC 62115 (brinquedos elétricos).

---

### 2. Unidade emissora, caixa do botão

| Ref | Item | Especificação | Qtd | Para quê |
|---|---|---|---|---|
| BT3 | Célula de lítio | **18650** de 2000 a 3500 mAh, **ou** bolsa LiPo 503035 de 500 mAh se o espaço apertar | 1 | Ver "Qual célula" abaixo: a escolha depende do deep sleep, não do consumo. |
|  | Suporte para 18650 | 1 célula, com fios | 1 | Dispensável se optar pela bolsa LiPo, que já vem com fios. |
|  | Módulo de carga **com proteção** | **TP4056 + DW01A + FS8205**, versão de 6 pinos, USB-C | 1 | A versão de 4 pinos **não tem proteção**. Confira: a placa protegida tem dois CIs pequenos além do TP4056, e saídas B+ B- OUT+ OUT-. |
| U1 | Regulador 3,3 V de baixa queda | **ME6211C33M5G** (SOT-23-5, 500 mA) ou **AP2112K-3.3** (SOT-25, 600 mA) | 2 | Ver "Não é buck, é LDO". Só necessário se a placa não resolver sozinha. |
| C3 | Capacitor eletrolítico | 100 µF / 10 V | 2 | Na saída do regulador, junto à placa. |
| C4 | Capacitor cerâmico | 100 nF | ver §1 | Sai do lote da seção 1. |

#### O 18650 não pode ir direto no VIN

Este é o ponto que precisa de atenção antes de montar. A célula percorre 4,2 V
cheia até 3,0 V vazia, e nenhum desses valores serve para as duas entradas
óbvias da placa:

- **No pino VIN**, o AMS1117 da DevKit V1 precisa de cerca de 4,4 V para
  entregar 3,3 V. Uma 18650 **cheia**, a 4,2 V, já está abaixo disso. Não
  funciona nem no melhor momento da célula.
- **No pino 3V3**, os 4,2 V ultrapassam o máximo absoluto de 3,6 V do ESP32.
  **Queima o chip.**

Por isso a lista traz o regulador de baixa queda: célula → TP4056 → regulador
3,3 V → pino 3V3.

#### Não é buck, é LDO

**Nenhum ponto deste projeto precisa de conversor comutado.** O buck-boost
apareceu antes como nota de rodapé opcional, não como requisito, e a lista nunca
dependeu dele.

O componente é um regulador linear de três terminais em encapsulamento
SOT-23-5, de **2,9 mm × 1,6 mm**. Um módulo de buck pronto mede em torno de
22 mm × 17 mm. É uma diferença de quase cem vezes em área.

| | Entrada | Saída | Queda | Repouso | Tamanho |
|---|---|---|---|---|---|
| **ME6211C33M5G** | 2,0 a 6,0 V | 3,3 V, 500 mA | 120 mV a 100 mA | 60 µA | SOT-23-5 |
| **AP2112K-3.3** | até 6,0 V | 3,3 V, 600 mA | 250 mV | 55 µA | SOT-25 |

Repare na faixa de entrada do ME6211: 2,0 a 6,0 V cobre tanto a 18650 quanto os
4,5 V do pacote de pilhas. **É o mesmo componente para as duas unidades**, se
você precisar dele nas duas.

E o argumento de eficiência que normalmente justifica um buck não se aplica
aqui. De uma célula de lítio para 3,3 V a razão é de apenas 1,27 para 1, então
o rendimento do linear é V<sub>saída</sub> / V<sub>entrada</sub>: **79 % com a
célula cheia, subindo para 94 % perto do fim**. Um buck entregaria cerca de
90 % constantes. A diferença é de poucos pontos percentuais, e some diante da
economia de espaço.

O que o linear realmente custa é o fundo da curva: ele para quando a célula
chega a cerca de 3,45 V. Numa Li-ion sob carga leve isso acontece já bem perto
do fim da capacidade útil.

**Pode ser que você não precise nem disso.** Se a placa da emissora tiver um
ME6211 ou equivalente de bordo, é só ligar a célula no pino 5V e pronto, zero
componentes extras. Mesma verificação de marcação da seção "Antes de comprar".

#### Qual célula: é a mesma pergunta do deep sleep

Como o firmware está hoje, o rádio fica ligado 100 % do tempo. São 84 mA
contínuos numa C3; numa DevKit V1 é mais, somando o rádio do clássico com a
ponte USB-UART. Com deep sleep, o consumo médio cai para a faixa de poucos mAh por
dia. Isso muda completamente o tamanho de célula necessário:

| Célula | Volume | Firmware atual | Com deep sleep |
|---|---|---|---|
| 18650, 2500 mAh | 18 × 65 mm | cerca de 30 h | mais de um ano |
| Bolsa LiPo 503035, 500 mAh | 5 × 30 × 35 mm | cerca de 6 h | cerca de 3 meses |

A 18650 é o maior objeto da caixa, maior que o módulo de carga e que a placa.
Se o espaço é o problema, **a bolsa LiPo é bem mais eficiente em volume para uma
caixa achatada**, carrega pelo mesmo TP4056, e três meses de autonomia num botão
é bastante. Mas ela só faz sentido depois do deep sleep implementado.

Ou seja: a decisão de tamanho não se resolve comprando, se resolve no firmware.

> **Alerta de compra.** Célula 18650 anunciada com 6000, 9800 ou 12000 mAh é
> falsificada, sem exceção. O limite real da tecnologia hoje está em torno de
> 3500 mAh. Compre Samsung, LG, Sony ou Molicel, e desconfie de preço baixo
> demais: uma célula ruim com carregador é um risco de incêndio dentro de um
> brinquedo infantil.

---

### 3. Chaveamento com MOSFET (evolução, não vigente)

| Ref | Item | Especificação | Qtd | Para quê |
|---|---|---|---|---|
| Q1, Q2 | MOSFET canal N nível lógico | **AO3400A** (SOT-23) para montagem apertada, ou **IRLB8721PbF** (TO-220) na bancada | 6 | Substituem os TIP122. |
| Rg | Resistor | 100 Ω, 1/4 W | 10 | Em série com a porta. Sem ele o pico inicial no GPIO passa de 115 mA, contra o limite de 40 mA por pino. |
| Rpd | Resistor | 100 kΩ, 1/4 W | 10 | Da porta para o source. **Obrigatório.** O TIP122 tinha resistores internos que seguravam o transistor cortado no reset; o MOSFET não tem, e a porta flutuando pode acionar o brinquedo sozinho no boot. |

**Critério para aproveitar o que você já tem.** Abra a folha de dados e procure
uma linha só: o R<sub>DS(on)</sub> precisa estar **especificado em V<sub>GS</sub>
de 4,5 V ou menos**. Se a única especificação for em 10 V, o componente está
errado para um GPIO de 3,3 V, e o modo de falha é ruim: conduz parcialmente e
dissipa watts em vez de miliwatts.

**O AO3400A é a melhor escolha aqui**, e por dois motivos. Ele é especificado em
V<sub>GS</sub> = 2,5 V (48 mΩ) e em 4,5 V (32 mΩ), com limiar de apenas 650 mV,
então 3,3 V de GPIO o acionam com folga real, coisa que o IRLB8721 não garante,
já que ele só é caracterizado a partir de 4,5 V. E ele é SOT-23, de
**2,9 × 1,3 mm**, contra os 10 × 15 mm mais aba do TO-220. A 0,5 A ele cai 24 mV
e dissipa 12 mW, então não precisa de dissipador nem de espaço.

Servem: AO3400A, IRLB8721, IRLZ44N, IRL540N, SI2302, IRLML2502.
Não servem, apesar de comuns na gaveta: IRF540N, IRFZ44N, IRF3205, IRF630,
2N7000. A letra **L** em IRLxxx marca a família certa.

---

### 4. Bancada, para os ensaios

| Item | Especificação | Qtd | Para quê |
|---|---|---|---|
| Resistor de potência | 47 Ω, **2 W** ou mais | 1 | Ensaio de resistência interna, a cerca de 96 mA. Vale rodar nas três fontes: pacote de AA, A23 e célula de lítio. |
| Resistor de potência | 120 Ω, **2 W** ou mais | 1 | Mesmo ensaio na A23 em 12 V. Opcional, serve para a medição entrar no relatório. |

Um resistor de 1/4 W queima em segundos nesses ensaios.

Com a mudança para lítio, o ensaio de resistência interna deixa de ser um teste
de aprovação e vira **evidência para o relatório**: é ele que demonstra, com
número medido, por que a troca de química resolveu o afundamento. Os três
resultados lado a lado valem uma tabela no texto.

O ensaio 4, de subtensão no trilho compartilhado, continua valendo a pena, mas
agora como confirmação de que o problema realmente sumiu, e não como decisor da
topologia.

---

### 5. O que sai do projeto (na evolução, não agora)

Sob D9 nada disso sai ainda. Quando a evolução acontecer, não comprar mais, e
tirar da lista de materiais do relatório:

- **Bateria A23 (23A), 12 V.** Reprovada por três critérios independentes.
  Entrega cerca de 15 mAh no ponto de operação real contra 55 mAh de placa, e a
  Espressif exige fonte capaz de 0,5 A.
- **Regulador L7805CV e os capacitores de 330 nF.** Existiam só para descer os
  12 V da A23. Sem a A23, não têm função.
- **Resistores de base de 330 Ω**, junto com os TIP122.
- **Pilhas AA**, se a célula de lítio for adiante.

---

### 6. Recomendação: trocar a placa da emissora

Você disse que a ideia é economizar espaço e bateria. A DevKit V1 trabalha
contra os dois objetivos, e a troca por uma **ESP32-C3 Super Mini** custa quase
nada:

1. **Deep sleep é impossível com o pinout atual na DevKit V1.** O despertar por
   GPIO no ESP32 clássico exige pinos RTC, e o **GPIO5 não é um deles**. O
   botão 1 simplesmente não acordaria a placa. No C3, os pinos de despertar são
   GPIO0 a GPIO5, então **GPIO4 e GPIO5, que o firmware já usa, funcionam sem
   mudar nada**.
2. **A ponte USB-UART da DevKit V1 continua consumindo miliampères com o SoC
   dormindo.** É ela que determina a autonomia real, não o chip. A Super Mini
   tem USB nativo, sem ponte, e dorme em dezenas de µA.
3. **O firmware já compila para os dois alvos**, com GPIO4/5 em ambos
   (`emissor/emissor.ino:55`). A troca é uma linha no Makefile: `FQBN_EMISSOR`
   e a opção `CDCOnBoot=cdc`, que o alvo do receptor já usa.
4. Cabe em uma fração do espaço.

| Item | Qtd |
|---|---|
| ESP32-C3 Super Mini | 2 (uma reserva) |

---

### Sobre o deep sleep

Verificado no código: **a emissora não tem deep sleep hoje.** Mais do que isso,
o rádio está explicitamente impedido de dormir em
`emissor/emissor.ino:204` (`WiFi.setSleep(false)`), e a `tarefaBotoes` varre os
pinos a cada 5 ms sem parar. O rádio fica ligado 100 % do tempo.

Implementar significa mais do que chamar uma função, e vale saber disso antes de
contar com a economia:

- **Deep sleep no ESP32 é reinicialização.** As três tasks, as filas e o
  `bootId` recomeçam do zero a cada despertar. O `bootId` é sorteado por boot e
  serve para invalidar a sequência antiga (`lib/protocolo.h:10`); com sono
  profundo ele passa a mudar a cada aperto, o que neutraliza a proteção contra
  repetição entre eventos. Dentro de uma mesma rajada ela continua valendo.
- **Só o aperto gera pacote.** Verificado em `lib/debounce.cpp:29`: a borda de
  subida atualiza o estado estável e retorna `false`, então a soltura não
  transmite nada. Cada aperto inverte o estado e envia o estado absoluto
  resultante. O ciclo compatível com sono profundo é "acorda, envia a rajada de
  três, dorme".
- **Latência.** Do despertar até o ESP-NOW pronto são algumas centenas de
  milissegundos, e isso aparece como atraso entre apertar o botão e o brinquedo
  reagir.

Nada disso inviabiliza, mas é reestruturação da arquitetura de tasks, não um
ajuste. Faz sentido decidir a placa primeiro, porque na DevKit V1 o trabalho não
tem como dar certo.
