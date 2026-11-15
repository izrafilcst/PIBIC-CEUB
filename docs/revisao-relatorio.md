# Base para a revisão do relatório

> **SUPERADO EM 28/08/2026. Documento histórico.**
>
> A revisão que este arquivo planejava **foi aplicada**, porém em escopo maior do
> que o previsto aqui: a decisão D10 substituiu a topologia inteira da unidade
> receptora, e as seções 5 e 6 do relatório foram reescritas em vez de
> emendadas. As duas correções técnicas que este documento rastreava, o cálculo
> do beta forçado e o diodo de roda-livre, estão fechadas.
>
> Os números de linha citados abaixo referem-se à versão anterior do relatório,
> hoje congelada em `docs/relatorio-v1-a23.html`, e **não valem** para
> `docs/relatorio.html`.
>
> O que consultar em vez deste arquivo:
> `docs/registro-de-mudancas.md` (o que mudou, seção por seção) e
> `docs/alimentacao-e-chaveamento.md` (por que mudou, decisão D10).
>
> Mantido no repositório porque registra o estado do raciocínio antes da
> inspeção do brinquedo, que é o que explica por que D9 existiu.

Documento de trabalho, escrito em 25/08/2026. Reúne as decisões de alimentação e
chaveamento tomadas em 24 e 25/08 e traduz cada uma no que precisa mudar em
`docs/relatorio.html`, com o número da linha e o texto proposto.

Não é um documento final. Ele existe para você corrigir, riscar e completar
antes que qualquer edição seja aplicada ao relatório.

**Como ler as marcações:**

| Marca | Significado |
|---|---|
| `>> ` | campo em branco, para você preencher |
| `[ ]` | item ainda não aplicado ao relatório |
| `[x]` | item já aplicado |
| **A conferir** | eu assumi alguma coisa e posso ter assumido errado |

O registro completo das decisões, com os cálculos, está em
`docs/alimentacao-e-chaveamento.md`. Aqui fica só o que toca o relatório.

---

## Sumário: a revisão encolheu

Vale começar por isto, porque muda o tamanho do trabalho. A decisão D9, montar
com as peças em mãos, **preservou a arquitetura que o relatório já descreve**.
As duas A23 continuam, o L7805CV continua, os TIP122 continuam, as três pilhas
AA do brinquedo continuam. Os dois domínios de alimentação e o ponto único de
terra seguem exatamente como estão escritos.

Sob a topologia de lítio que chegou a ser considerada, a seção 6 inteira, a
Figura 4 e a lista de materiais seriam reescritas do zero. Sob D9, não são.

O que sobra é uma revisão de quatro tipos:

1. **Dois erros técnicos** na seção do TIP122, encontrados durante a análise.
   Independem de D9 e teriam de ser corrigidos de qualquer maneira.
2. **Três acréscimos de D9**: o paralelo das A23, a chave geral e os diodos de
   roda-livre.
3. **Números do orçamento energético** que estavam otimistas, agora substituídos
   por valores calculados a partir da curva do fabricante.
4. **Uma frase que passou a se contradizer** com a decisão de não usar conversor
   comutado.

>> Sua avaliação do escopo:

---

## Parte A. Decisões, para conferir

| # | Decisão | Situação | Confere? |
|---|---|---|---|
| D1 | O brinquedo opera em 4,5 V, de três pilhas AA em série | Vigente | `[ ]` |
| D2 | A A23 não serve como alimentação permanente | Estabelecida por cálculo | `[ ]` |
| D7 | Nenhum conversor comutado em ponto algum do projeto | Firme | `[ ]` |
| D8 | Trocar a placa da emissora para ESP32-C3 Super Mini | Recomendada, não decidida | `[ ]` |
| **D9** | **Entrega com as peças em mãos: TIP122, duas A23 em paralelo por unidade, L7805CV e chave geral** | **Vigente** | `[ ]` |

D3 a D6, a migração para célula de lítio com carga por USB-C, estão adiadas por
D9 e **ficam fora desta revisão**. A análise continua registrada no outro
documento, para quando houver orçamento.

### A1. D9, o que foi decidido

Duas baterias A23 em paralelo por unidade alimentam o L7805CV, com chave geral,
e os TIP122 permanecem. Decidida por custo e prazo.

**O que o paralelo compra.** Mais do que o dobro, porque cada célula passa a
trabalhar em 45 mA em vez de 90 mA e a penalidade de dreno da alcalina é
fortemente não linear:

| Arranjo | Por célula | Total | Autonomia |
|---|---|---|---|
| 1 × A23 | 15,1 mAh | 15,1 mAh | 10 min |
| 2 × A23 em paralelo | 18,9 mAh | 37,8 mAh | 25 min |

**A chave geral é o que torna isso viável.** Vinte e cinco minutos de relógio
seriam inúteis. Com a chave, são 25 minutos de uso acumulado, ou cerca de cinco
demonstrações de cinco minutos por par de baterias.

**A conferir, e muda a compra.** "As duas pilhas em paralelo alimentando o
regulador" admite duas leituras: duas células **por unidade**, quatro no total,
que é a hipótese adotada aqui porque cada unidade tem seu próprio regulador e as
duas só se acoplam por rádio; ou duas no total, o que deixaria uma das unidades
sem alimentação.

>> É quatro no total?

**A conferir.** Assumi **uma chave por unidade**, duas no total, cada uma
cortando o positivo da sua própria bateria.

>> São duas chaves?

### A2. Cuidado que precisa aparecer no relatório

Célula primária não é feita para trabalhar em paralelo. Se uma estiver mais nova
que a outra, a mais forte empurra corrente para dentro da mais fraca, o que
desperdiça energia e pode fazer a mais fraca vazar. Duas regras resolvem: usar
células do mesmo lote e trocar sempre o par junto.

Isso não é só nota de manutenção, é requisito de segurança de um brinquedo, e a
ABNT NBR NM 300-1 trata de acessibilidade e contenção de fontes de energia em
brinquedos. Vale uma frase na seção de alimentação.

>> Quer que isso entre como parágrafo próprio ou como nota de rodapé?

---

## Parte B. O que muda no relatório, linha a linha

### B1. Erro técnico: o diodo interno do TIP122

`docs/relatorio.html`, linhas **674 a 679**, seção "Escolha do TIP122".

**Texto atual:**

> O componente já traz, internamente, dois resistores de polarização entre base
> e emissor e um diodo de proteção em antiparalelo entre coletor e emissor. Esse
> diodo integrado dispensa o diodo de roda-livre discreto que seria obrigatório
> em cargas indutivas. A carga aqui prevista é a eletrônica do brinquedo, de
> natureza essencialmente resistiva; ainda assim, caso a inspeção posterior
> revele algum motor ou relé embutido, a proteção já está presente por
> construção.

**Por que está errado.** O diodo integrado tem anodo no emissor e catodo no
coletor, então conduz apenas quando o coletor vai **abaixo** do emissor. No
chaveamento de lado baixo, quando o transistor corta, a indutância do motor
mantém a corrente e joga o coletor para **cima**, muito acima de V<sub>CC</sub>.
Nessa polaridade o diodo interno está reversamente polarizado e não faz nada.

Ele protege contra outra coisa: tensão reversa aplicada ao conjunto, útil quando
a carga é ligada por engano com polaridade trocada ou em pontes com carga
indutiva flutuante. Não é a proteção que este circuito precisa.

O diodo de corpo de um MOSFET tem exatamente a mesma orientação, então trocar o
transistor não corrigiria a omissão.

**A frase final é a mais perigosa das três**, porque promete cobertura futura:
"caso a inspeção posterior revele algum motor ou relé embutido, a proteção já
está presente por construção". É justamente no caso do motor ou do relé que a
proteção não está presente. Um relatório que registra uma proteção inexistente
para uma condição que ainda não foi verificada é pior do que um que não menciona
o assunto.

**E a premissa também está em aberto.** "Carga essencialmente resistiva" é
suposição, não constatação: o brinquedo não foi aberto para conferir. Se qualquer
um dos dois modos aciona motor, solenoide ou relé, o diodo deixa de ser prudência
e passa a ser obrigatório. Isso entrou na Parte D como item de inspeção.

**O que passa a valer.** O diodo de roda-livre discreto é obrigatório, com
**anodo no coletor e catodo no positivo de 4,5 V**, em paralelo com cada carga.

`[ ]` aplicado &nbsp;&nbsp; >> observação:

### B2. Erro técnico: o ganho usado no dimensionamento da base

`docs/relatorio.html`, linha **703**, e a consequência na linha **717**.

**Texto atual:**

> I<sub>C(máx)</sub> = h<sub>FE(mín)</sub> × I<sub>B</sub> = 1000 × 3,9 mA ≈ 3,9 A

e, mais adiante:

> a saturação já está garantida com folga de três ordens de grandeza sobre a
> corrente de carga esperada

**Por que está errado.** O ganho de 1000 é especificado em
V<sub>CE</sub> = 3 V, ou seja, na região ativa. Num trilho de 4,5 V, ter
V<sub>CE</sub> de 3 V significaria entregar 1,5 V à carga: a condição sob a qual
esse ganho vale é uma condição que este circuito nunca ocupa. Usar h<sub>FE</sub>
de região ativa para calcular corrente de saturação mistura dois pontos de
operação diferentes da mesma folha de dados.

A saturação é caracterizada com **beta forçado de 250**
(V<sub>CE(sat)</sub> de 2,0 V com I<sub>C</sub> de 3 A e I<sub>B</sub> de 12 mA).

**O que passa a valer:**

> I<sub>C(sat)</sub> = β<sub>forçado</sub> × I<sub>B</sub> = 250 × 3,9 mA ≈ 0,98 A

A folga cai de três ordens de grandeza para algo entre três e dez vezes,
dependendo da corrente que a medição apontar. **A conclusão sobrevive**: 330 Ω
continua adequado e a margem continua confortável. O que cai é o número e a
justificativa.

**Esta correção já não depende da medição da corrente do brinquedo**, porque o
argumento é uma razão entre condições de ensaio. A medição serve para confirmar
que 0,98 A cobre a carga real, não para fechar o raciocínio.

Há um terceiro sítio a corrigir junto, a nota de rodapé do `esquematico.html`,
se ela ainda repetir os 3,9 A.

`[ ]` aplicado &nbsp;&nbsp; >> observação:

### B3. Número pessimista demais: a queda de saturação

`docs/relatorio.html`, seção "Queda de saturação e dissipação", linhas **723 em
diante**.

**Texto atual:** V<sub>CE(sat)</sub> entre 1,0 e 2,0 V, o que dá ao brinquedo
entre 3,5 V e 2,5 V, ou de 78 % a 56 % da tensão nominal.

**O problema.** Os 2,0 V são o máximo garantido **em I<sub>C</sub> de 3 A**, uma
corrente muito acima de qualquer carga de brinquedo. Aplicar esse número a uma
carga de centenas de miliampères é usar o pior caso da folha de dados fora da
faixa em que ele foi medido, e o relatório fica mais pessimista do que a
realidade.

**O que passa a valer.** O Darlington tem um piso estrutural, e é ele que
governa em corrente baixa:

V<sub>CE(sat)</sub> = V<sub>BE</sub>(Q<sub>saída</sub>) + V<sub>CE(sat)</sub>(Q<sub>entrada</sub>) ≈ 0,7 V + 0,2 V ≈ 0,9 V

Em corrente de brinquedo, portanto, entre 0,9 e 1,2 V, o que entrega ao motor
entre 3,6 V e 3,3 V, ou de 80 % a 73 %.

**Ressalva que precisa ficar escrita:** 0,9 V é o piso típico, não um valor
garantido pela folha de dados, que só especifica em 3 A e em 5 A. O número real
sai da medição.

>> Quer manter o pior caso de 2,0 V como limite superior declarado, ou trocar
>> pela faixa típica com a ressalva?

`[ ]` aplicado &nbsp;&nbsp; >> observação:

### B4. Orçamento energético, seção 6.2

`docs/relatorio.html`, linhas **945 a 993**.

Esta é a parte que mais muda, e muda para melhor: os números saem de estimativa
e passam a vir da curva publicada pelo fabricante.

| O que | Está escrito | Passa a valer | Origem |
|---|---|---|---|
| Capacidade da A23 | cerca de 55 mAh | 55 mAh de placa, **15,1 mAh no ponto de operação** | curva capacidade × dreno da Energizer |
| Arranjo | uma bateria por unidade | **duas em paralelo por unidade** | D9 |
| Capacidade útil por unidade | não declarada | **37,8 mAh** | 2 × 18,9 mAh a 45 mA por célula |
| Autonomia | cerca de 40 minutos | **cerca de 25 minutos de uso acumulado** | 37,8 mAh ÷ 90 mA |
| Dissipação no L7805 | 0,56 W a 80 mA | **0,63 W a 90 mA** | (12 − 5) × 0,090 |
| Elevação de temperatura | cerca de 35 °C | **cerca de 39 °C** | 0,63 W × 62 °C/W |
| Chave geral | não existe | **converte relógio em uso acumulado** | D9 |

**Como o número de 15,1 mAh foi obtido**, e isto precisa aparecer no relatório
porque é uma extrapolação:

A Energizer publica 52 mAh a 2 mA e 27 mAh a 15 mA, e **o gráfico termina em
15 mA**. Ajustando uma lei de potência a esses dois pontos,

k = ln(27/52) ÷ ln(15/2) = −0,325

C(I) = 52 × (I/2)<sup>−0,325</sup>

o que dá 15,1 mAh a 90 mA e 18,9 mAh a 45 mA. **Os 90 mA estão seis vezes além
do fim da curva caracterizada.** A direção do erro é conhecida e é otimista: a
capacidade real tende a ser menor que a extrapolada, porque a queda se acentua
fora da faixa. O relatório deve declarar isso, não escondê-lo.

>> Você quer a dedução completa no corpo do texto, ou o resultado no corpo e a
>> dedução em nota?

**O segundo modo de falha, o da queda mínima do regulador**, continua no texto e
ganha número. Para o L7805 seguir regulando durante os picos de 335 mA, a
resistência interna vista pelo circuito precisa ficar abaixo de:

| Tensão da bateria | Com uma célula | Com duas em paralelo, por célula |
|---|---|---|
| 12 V, nova | 14,9 Ω | 29,9 Ω |
| 10 V | 8,96 Ω | 17,9 Ω |
| 8 V | 2,99 Ω | 6,0 Ω |

O paralelo dobra o teto admissível porque a resistência vista cai pela metade.
Esse é o segundo ganho de D9, e é tão importante quanto a capacidade: era este o
mecanismo que reinicializava o SoC por subtensão antes de a carga acabar.

`[ ]` aplicado &nbsp;&nbsp; >> observação:

### B5. Frase que passou a se contradizer

`docs/relatorio.html`, fim da seção 6.2, linhas **988 a 993**.

**Texto atual:**

> Uma bateria com capacidade e corrente de descarga compatíveis com o consumo do
> rádio, associada a um **conversor comutado** no lugar do regulador linear, é o
> caminho natural para converter a montagem em um protótipo autônomo.

**Por que sai.** D7 descarta conversor comutado em qualquer ponto do projeto. O
motivo é específico e vale escrever: um conversor comutado ao lado de uma antena
de 2,4 GHz injeta ruído na faixa em que o rádio opera, e o ganho de eficiência
não paga o risco de degradar o alcance, ainda mais tendo um diagnóstico de RF
já registrado no projeto.

**O que entra no lugar.** O caminho de evolução deixa de ser "conversor
comutado" e passa a ser "célula de lítio recarregável com regulador linear de
baixa queda", que é o conteúdo de D3 a D6.

`[ ]` aplicado &nbsp;&nbsp; >> observação:

### B6. Lista de materiais, seção 3

`docs/relatorio.html`, linhas **344 a 366**.

| Ref. | Ação | Especificação |
|---|---|---|
| BT2 | **alterar quantidade** | A23 (23A), 12 V: de 2 para **4**, duas em paralelo por unidade |
| S3, S4 | **acrescentar** | Chave SPST, 500 mA, corte do positivo da bateria, 2 unidades |
| D1, D2 | **acrescentar** | Diodo de roda-livre 1N5819 (Schottky) ou 1N4007, 2 unidades, receptora |
| Q1, Q2 | **corrigir texto** | tirar "h<sub>FE</sub> ≥ 1000" da coluna de especificação, ou qualificar como ganho de região ativa |

A numeração de S1 e S2 já está usada pelos botões da emissora, por isso as
chaves entram como S3 e S4. **A conferir**, caso você prefira outra convenção.

`[ ]` aplicado &nbsp;&nbsp; >> observação:

### B7. Figuras

| Figura | Linha | O que muda |
|---|---|---|
| Diagrama de blocos | 285 | Nada de estrutural. Só rever a legenda, que diz "bateria de 12 V" no singular |
| Esquemático do chaveamento | 573 | **Acrescentar os dois diodos de roda-livre**, anodo no coletor, catodo no trilho de 4,5 V |
| Distribuição de alimentação | 840 | **Duas A23 em paralelo por unidade, a chave em série com o positivo, e os diodos** |

A Figura do chaveamento é a mais importante das três, porque hoje ela desenha um
circuito que o texto declara completo e que não está.

>> Prefere que eu edite os SVG existentes ou que redesenhe?

`[ ]` aplicado &nbsp;&nbsp; >> observação:

---

## Parte C. Texto proposto, para você corrigir

Rascunhos em prosa, no registro do relatório, prontos para adaptar. Nenhum deles
foi inserido no arquivo.

### C1. Substituto do trecho do diodo interno (seção 5, escolha do TIP122)

> O componente já traz, internamente, dois resistores de polarização entre base e
> emissor e um diodo entre coletor e emissor. Esse diodo tem anodo no emissor e
> catodo no coletor, de modo que conduz apenas quando o coletor é levado a
> potencial inferior ao do emissor. No chaveamento de lado baixo aqui adotado, o
> transitório que importa tem sentido oposto: ao corte do transistor, a
> indutância da carga sustenta a corrente e eleva o coletor acima da tensão de
> alimentação, condição em que o diodo integrado permanece reversamente
> polarizado. Ele não substitui, portanto, o diodo de roda-livre, que é
> acrescentado em paralelo com cada carga, com anodo no coletor e catodo no
> positivo de 4,5 V. O diodo de corpo de um transistor de efeito de campo
> apresenta a mesma orientação, razão pela qual a substituição do dispositivo de
> chaveamento não dispensaria esse componente.

### C2. Substituto do cálculo de corrente de coletor (seção 5)

> Do lado do coletor, a corrente que essa base sustenta em saturação não decorre
> do ganho de 1000, que a folha de dados especifica com V<sub>CE</sub> de 3 V,
> ou seja, na região ativa. Com alimentação de 4,5 V, operar com V<sub>CE</sub>
> de 3 V significaria entregar apenas 1,5 V à carga, condição que o circuito não
> ocupa. A saturação é caracterizada por beta forçado de 250, valor que decorre
> das condições de ensaio publicadas, e é ele que se aplica aqui:
>
> I<sub>C(sat)</sub> = β<sub>forçado</sub> × I<sub>B</sub> = 250 × 3,9 mA ≈ 0,98 A
>
> A margem permanece adequada para uma carga de brinquedo alimentada em baixa
> tensão, ainda que da ordem de poucas vezes a corrente esperada, e não de
> ordens de grandeza. O resistor de 330 Ω é mantido.

### C3. Substituto do orçamento energético (seção 6.2)

> A bateria A23 é um empilhamento de oito células alcalinas miniatura, com
> capacidade nominal da ordem de 55 mAh e corrente de descarga recomendada na
> faixa de poucos miliampères, compatível com a aplicação para a qual foi
> concebida, o controle remoto de uso intermitente. A capacidade nominal, no
> entanto, não é a capacidade disponível no ponto de operação deste projeto. O
> fabricante publica a queda da capacidade com o aumento do dreno, 52 mAh a 2 mA
> e 27 mAh a 15 mA, e a caracterização encerra em 15 mA. Ajustada uma lei de
> potência a esses pontos e extrapolada até os 90 mA médios exigidos, a
> capacidade disponível por célula cai a aproximadamente 15 mAh. Registre-se que
> se trata de extrapolação seis vezes além da faixa caracterizada, e que a
> direção do erro é conhecida: a capacidade real tende a ser inferior à
> extrapolada.
>
> Por essa razão, cada unidade é alimentada por duas baterias A23 associadas em
> paralelo. O ganho não é o dobro, mas superior a ele: com a corrente por célula
> reduzida a 45 mA, a capacidade disponível por célula sobe a aproximadamente
> 18,9 mAh, e o conjunto passa a oferecer cerca de 37,8 mAh, correspondentes a
> aproximadamente 25 minutos de operação contínua.
>
> O paralelo atende também a um segundo requisito, de natureza dinâmica. A
> resistência interna da A23 é elevada e cresce ao longo da descarga, de modo
> que os picos de transmissão produzem queda momentânea nos terminais. O
> regulador mantém a saída regulada enquanto a entrada permanecer acima de 7 V,
> soma dos 5 V de saída com os 2 V de queda mínima do L7805CV; abaixo desse
> limiar o SoC é reinicializado por subtensão. Com uma única célula, a
> resistência interna admissível seria de 14,9 Ω com a bateria nova e de 2,99 Ω
> aos 8 V; com duas em paralelo, a resistência vista pelo circuito é a metade, e
> o limite por célula dobra. É este, e não a capacidade, o mecanismo que
> encerrava a operação antes de a carga se esgotar.
>
> A associação de células primárias em paralelo exige cuidado próprio: células em
> estados de carga distintos trocam corrente entre si, com desperdício de energia
> e risco de vazamento na célula mais descarregada. Adota-se, por isso, a regra
> de empregar células do mesmo lote e substituir sempre o par simultaneamente.
>
> Uma chave geral interrompe o positivo de cada conjunto. Sua função não é de
> conveniência: ela converte a autonomia de tempo de relógio em tempo de uso
> acumulado, o que corresponde a aproximadamente cinco demonstrações de cinco
> minutos por par de baterias.

>> Suas correções em C3:

### C4. Nota sobre a evolução, em substituição à frase do conversor comutado

> A topologia descrita é adequada a ensaios de bancada e a demonstrações, e não
> constitui solução de alimentação para uso prolongado. A evolução prevista
> substitui as células primárias por uma célula de lítio recarregável, com
> circuito de carga por conector USB-C, mantido o regulador linear. Descarta-se
> deliberadamente o emprego de conversor comutado: a comutação em frequências
> elevadas, nas proximidades imediatas de uma antena de 2,4 GHz, introduz risco
> de degradação do enlace que o ganho de eficiência não compensa nesta escala de
> consumo.

---

## Parte D. Números que ainda faltam

Sem eles, três afirmações do relatório continuam como estimativa declarada.

| # | O que medir | Como | O que destrava | Valor |
|---|---|---|---|---|
| 1 | **Corrente do brinquedo por modo** | multímetro em série, escala de 10 A | Confirma que 0,98 A cobre a carga, fecha B2 e B3 | >> |
| 2 | Corrente de rotor bloqueado | medir a resistência ôhmica do motor e calcular I = V ÷ R | Dimensiona o diodo de roda-livre e o pior caso | >> |
| 3 | V<sub>CE(sat)</sub> real, com a carga ligada | voltímetro entre coletor e emissor, modo acionado | Substitui a faixa estimada de B3 por medição | >> |
| 4 | Resistência interna das A23 | resistor de carga conhecido, medir a queda | Vira evidência da tabela de B4 | >> |
| 5 | Autonomia real, com a chave em uso | cronometrar até a primeira reinicialização | Confirma ou corrige os 25 minutos | >> |
| 6 | **Natureza da carga de cada modo**: motor, solenoide, relé ou apenas eletrônica | abrir o brinquedo e inspecionar | Decide se o diodo de roda-livre é obrigatório ou prudente, e corrige a premissa de B1 | >> |

A medição 1 trava o dimensionamento desde 20/08.

**Observação sobre a medição 5:** o modo de falha esperado não é o brinquedo
parar, é o ESP32 reinicializar por subtensão. Vale anotar qual dos dois
acontece primeiro, porque isso diz se quem governa é a capacidade ou a
resistência interna.

>> Notas de bancada:

---

## Parte E. Divergências entre documentos, a resolver antes de escrever

Três números aparecem com valores diferentes em lugares diferentes. Nenhum é
grave, mas é preciso escolher um de cada e usar o mesmo em toda parte.

| Grandeza | Relatório | Nota técnica e registro de decisões | Sugestão |
|---|---|---|---|
| Corrente média do domínio lógico | 85 mA (80 do SoC + 5 do L7805) | 90 mA | Adotar **90 mA**, que é conservador, e declarar a composição |
| Resistência térmica do TO-220 sem dissipador | 62 °C/W | 50 °C/W, implícito no "cerca de 32 °C" | Adotar **62 °C/W**, que é o valor já citado com fonte |
| Elevação de temperatura resultante | 35 °C a 80 mA | 32 °C | **39 °C a 90 mA**, consequência das duas escolhas acima |

Com 90 mA e 62 °C/W, o regulador dissipa 0,63 W e sobe cerca de 39 °C sobre o
ambiente. Continua dentro da faixa segura, sem folga confortável, que é
exatamente o que o relatório já diz em palavras.

>> Sua escolha:

---

## Parte F. Ordem sugerida de aplicação

Uma sequência que evita retrabalho, porque cada passo depende do anterior.

1. `[ ]` Você responde as três perguntas **A conferir** da Parte A: quantidade de
   baterias, quantidade de chaves, convenção de referências.
2. `[ ]` Você corrige e completa a Parte C, que é o texto que de fato vai entrar.
3. `[ ]` Aplicar B1 e B2, os dois erros técnicos. Independem de medição e de D9.
4. `[ ]` Aplicar B5 e B6, a frase do conversor comutado e a lista de materiais.
5. `[ ]` Aplicar B4, o orçamento energético, com o texto já revisto por você.
6. `[ ]` Redesenhar as figuras de B7.
7. `[ ]` Depois da bancada, fechar B3 e as demais estimativas com valores medidos.
8. `[ ]` Regerar o `.docx` pelo `make docs`.

Os passos 3 e 4 podem ser feitos hoje. O passo 7 é o único que depende da
medição.

---

## Parte G. Espaço livre

Para o que não coube nas seções acima: coisas que você quer acrescentar ao
relatório, dúvidas, decisões que mudaram depois que este documento foi escrito.

>>
