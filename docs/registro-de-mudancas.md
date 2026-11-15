# Registro de mudanças do relatório: v1 → v2

Escrito em 28/08/2026.

Este documento existe para uma finalidade só: permitir comparar a versão do
relatório anterior à migração para lítio com a versão atual, sem precisar
recompor a história de memória.

## As duas versões

| | Arquivo | Estado |
|---|---|---|
| **v1** | `docs/relatorio-v1-a23.html` | **Arquivada e congelada.** Topologia A23 + L7805CV + 3 pilhas AA. Preserva dois erros técnicos, assinalados abaixo. Traz uma tarja de arquivamento na primeira página. |
| **v2** | `docs/relatorio.html` | **Vigente.** Topologia 14500 + TP4056 + MT3608 em 5,5 V. |

Gerar os `.docx` de cada uma:

```
make docs                              # gera docs/relatorio.docx (v2)
python3 ferramentas/html2docx.py docs/relatorio-v1-a23.html docs/relatorio-v1-a23.docx
```

O `Makefile` usa regra genérica `docs/%.docx: docs/%.html`, então
`make docs/relatorio-v1-a23.docx` também funciona.

Para ver a diferença bruta linha a linha:

```
diff -u docs/relatorio-v1-a23.html docs/relatorio.html | less
```

O diff bruto é grande e pouco legível, porque as duas seções centrais foram
reescritas por inteiro. A tabela abaixo é o caminho recomendado.

---

## Parte 1. Correções técnicas

São as duas mudanças em que **a v1 está errada**, e não apenas desatualizada.
Se algo desta lista for aproveitado da v1, será um defeito.

### C1. Corrente máxima de coletor do TIP122

| | |
|---|---|
| **Onde** | v1, seção "Dimensionamento do resistor de base" |
| **v1 dizia** | `I_C(máx) = h_FE(mín) × I_B = 1000 × 3,9 mA ≈ 3,9 A`, com "folga de três ordens de grandeza" |
| **v2 diz** | `I_C(sat) = β_forçado × I_B = 250 × 3,9 mA ≈ 0,98 A`, com folga de cerca de uma ordem de grandeza |
| **Por quê** | O ganho de 1000 é especificado em V_CE = 3 V, ou seja, na **região ativa**. Num trilho de 5,5 V, ter 3 V sobre o transistor significaria entregar 2,5 V à carga: é uma condição que este circuito nunca ocupa. A saturação é caracterizada por beta forçado 250 (V_CE(sat) 2,0 V a 3 A com 12 mA de base; 4,0 V a 5 A com 20 mA). |
| **Muda a decisão?** | **Não.** O resistor de 330 Ω continua adequado. O que estava errado era o número e a justificativa. |
| **Consequência prática** | A margem real é bem menor do que se supunha, e por isso a v2 acrescenta o **rotor bloqueado** como o caso a medir, que a v1 nem mencionava. |

Esta correção estava pendente desde 20/08 e foi mantida em aberto por depender
da medição da carga. Ela deixou de depender: o argumento é uma razão entre
condições de ensaio da folha de dados.

### C2. Diodo de roda-livre

| | |
|---|---|
| **Onde** | v1, seção "Escolha do TIP122" |
| **v1 dizia** | "Esse diodo integrado dispensa o diodo de roda-livre discreto que seria obrigatório em cargas indutivas." |
| **v2 diz** | O diodo integrado **não** dispensa nada. O diodo discreto 1N4007 é obrigatório, com anodo no coletor e catodo no +5,5 V. A v2 dedica uma subseção inteira ao assunto. |
| **Por quê** | O diodo integrado ao TIP122 tem anodo no emissor e catodo no coletor: conduz só quando o coletor vai **abaixo** do emissor. No chaveamento de lado baixo, o chute indutivo joga o coletor para **cima**, e nessa condição o diodo integrado está reversamente polarizado. O diodo de corpo de um MOSFET tem a mesma orientação inútil, então trocar de transistor não corrigiria a omissão. |
| **Muda a decisão?** | **Sim.** Entra um componente que a v1 não previa. |
| **Consequência prática** | Montar segundo a v1 deixaria o transistor sem proteção contra a sobretensão de desligamento do motor. Como a v1 também supunha carga "essencialmente resistiva", o erro era duplo. |

---

## Parte 2. Mudanças de topologia

Decorrem da decisão **D10**, registrada em `docs/alimentacao-e-chaveamento.md`.
A v1 não está errada aqui: está descrevendo uma configuração que foi
substituída.

| Assunto | v1 | v2 |
|---|---|---|
| Fonte do brinquedo | 3 pilhas AA em série, 4,5 V | Célula de lítio 14500 recarregável, 3,0 a 4,2 V |
| Fonte das placas | 1 bateria A23 de 12 V + L7805CV **por unidade** | **Receptora:** a mesma célula, via MT3608. **Emissora:** inalterada, segue com A23 + L7805CV |
| Tensão na carga | 4,5 V menos a queda do Darlington, ou seja, 2,5 a 3,5 V | 5,5 V menos 1,0 V de queda, ou seja, **4,5 V**, igual ao projeto original |
| Domínios elétricos | Dois domínios separados, unidos só no terra | **Trilho único**, um só terra |
| Recarga | Não havia | USB-C, pelo TP4056 |
| Chave geral | Componente a comprar | Chave de 4 estágios original do brinquedo, reaproveitada |
| Autonomia | ~10 min (A23), ~25 min com duas em paralelo | ~5,8 h estimadas |
| Capacitores | 330 nF e 100 nF no regulador | C_in 100 µF, C_out 220 µF, 3 × 100 nF no motor |
| Conversores comutados | Proibidos por D7 | D7 **parcialmente revertida**: elevador admitido, buck-boost segue descartado |

### Por que a carga mudou tudo

A v1 dimensionava sobre uma carga hipotética de **0,5 A em regime e 1,5 A na
partida**, valor de espaço reservado que nunca foi medido. Em 28/08 o brinquedo
foi aberto: a carga é **um motor N20 e uma linha de LEDs**, cerca de uma ordem de
grandeza abaixo.

Isso derrubou de uma vez a objeção ao conversor elevador (o pior caso passou de
2,6 A estimados para ~1,9 A, contra 4 A de limite do MT3608), destravou a
correção C1, e permitiu escolher a tensão de trilho em vez de herdá-la.

### Por que o elevador, e não a célula direto no pino 5V

A primeira análise recomendou a célula direto, alegando 39 % de perda de
autonomia com o elevador. **Estava errada**, e o usuário apontou o problema. A
conta supunha que o regulador da placa regula até o fundo da descarga; ele não
regula, porque a queda mínima de um LDO cresce com a corrente (no ME6211, de
~120 mV a 100 mA para ~420 mV a 350 mA, e 350 mA é o pico de rádio). Refeita:

| Arranjo | Aproveitável | Autonomia |
|---|---|---|
| Célula direto no pino 5V | ~545 mAh de 800 | ~6,5 h |
| Célula → MT3608 em 5,5 V | 800 mAh | ~5,8 h |

O custo real é ~11 %, e em troca destrava o fundo da célula. A v2 registra o
episódio no texto, em vez de apresentar só a conclusão.

### O que a inspeção do brinquedo revelou

A v1 supunha que a chave do brinquedo unia os modos ao terra, e que bastaria pôr
o transistor no lugar dela. A chave é de **4 estágios e de lado alto**: o comum
distribui o positivo. Consequência: o transistor vai no **retorno** de cada
carga, não no lugar da chave, e o chaveamento de lado baixo que o projeto já
usava continua valendo sem alteração.

---

## Parte 3. Mudanças editoriais

Pedidas para que o documento seja legível por quem não é da área, já que a
fundamentação do projeto é de medicina.

| Onde | O que mudou |
|---|---|
| Após o Resumo | **Nova "Nota ao leitor"**, pré-textual, declarando o público, as convenções e o versionamento |
| Seção 1 | **Nova subseção "O sistema em termos não técnicos"**, que descreve o que o aparelho faz em linguagem corrente antes de qualquer detalhe |
| Seções 4, 7, 8, 9, 10, 11, 12, 13, 14 | **Abertura "Em termos simples"** em cada seção técnica, em caixa de nota, antes do texto formal. As seções 5 e 6 não a receberam por terem sido reescritas por inteiro já nesse registro |
| Seção 5 | Nova subseção "As duas cargas do brinquedo": o que é um N20, o que é a linha de LEDs |
| Seção 5 | Nova subseção "Diodo de roda-livre", com a explicação física da sobretensão indutiva |
| Seção 5 | Nova subseção "Supressão de ruído de escovas" |
| Seção 5 | Explicação de por que lado baixo e não lado alto, em termos de tensão base-emissor |
| Seção 5 | Novo quadro de ligação ponta a ponta entre a placa e os transistores, com o alerta de que a aba do TO-220 é o coletor |
| Seção 6 | Subseção "Da topologia anterior à atual", que registra o que havia antes e por que caiu |
| Seção 6 | Nova subseção de **advertências de segurança da célula de lítio** |
| Considerações finais | Nova subseção "Sobre a revisão da topologia de alimentação", que registra a lição de método |
| Pós-textual | **Novo Glossário** com 30 verbetes, após as Referências, conforme NBR 14724 |
| Figuras 3 e 4 | Redesenhadas com símbolos **ABNT NBR IEC 60617**, com `aria-label` descritivo e nota de convenções gráficas no rodapé do desenho |
| Todo o texto | Convenção explícita distinguindo **valor calculado**, **valor estimado** e **valor medido** |

### Convenção adotada sobre números

Nasceu do episódio descrito em "Por que a carga mudou tudo". A partir da v2:

- **calculado**: sai de fórmula com entradas conhecidas;
- **estimado**: sai de fórmula com pelo menos uma entrada suposta, e o texto diz
  qual e por quê;
- **medido**: veio de bancada, com o instrumento identificado.

Onde a v1 dizia apenas "valores plausíveis", a v2 diz de qual das três
categorias o número saiu.

---

## Parte 4. O que ficou pendente

Nada disto bloqueia a leitura do relatório, mas fecha lacunas conhecidas:

| # | Pendência | O que destrava | Urgência |
|---|---|---|---|
| 1 | Marcação do regulador da placa C3 | Confirma a margem até os 6,0 V de máximo absoluto, que é o que C_out protege | **Alta** |
| 2 | Se os retornos dos LEDs e do motor chegam separados | Único ponto que invalidaria o chaveamento de lado baixo por modo, e portanto a Figura 3 | **Alta** |
| 3 | O que a posição 2 da chave de 4 estágios alimenta | Confirma que as posições 2 a 4 podem ser unidas para virar chave geral | Média |
| 4 | Corrente de rotor bloqueado do N20 | Fecha a verificação contra os 0,98 A garantidos | Média |
| 5 | Levantamento quantitativo de alcance do enlace | Já era pendência na v1, sem relação com D10 | Baixa |

Se a pendência 2 der "retorno único", os dois transistores deixam de poder
comandar os modos independentemente, e a saída passa a reproduzir a lógica da
chave original: um transistor no retorno comum e o segundo chaveando só o motor.
É mudança de Figura 3, não de topologia de alimentação.

---

## Onde está cada coisa

| Arquivo | Papel |
|---|---|
| `docs/relatorio.html` | Documento acadêmico vigente, padrão ABNT |
| `docs/relatorio-v1-a23.html` | Versão anterior, congelada, com tarja de arquivamento |
| `docs/registro-de-mudancas.md` | Este arquivo |
| `docs/alimentacao-e-chaveamento.md` | Registro de decisões D1 a D10, com a justificativa completa e a BOM |
| `docs/montagem-litio.html` | Guia visual de bancada, em blocos, para montar. Não é documento acadêmico |
| `docs/revisao-relatorio.md` | Base de revisão linha a linha, anterior a D10 |
