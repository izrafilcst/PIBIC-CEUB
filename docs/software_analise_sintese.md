# Síntese da análise de `software.docx`

Documento gerado a partir de cinco leituras independentes (contrária, primeiros
princípios, leitor externo, execução prática e expansão), consolidando os
pontos em que houve convergência e descartando redundância. Marcações de
consenso (`[5/5]`, `[4/5]` etc.) indicam quantos dos cinco leitores
levantaram o mesmo ponto — quanto maior o número, mais confiável o achado.

---

## Veredito central

Os cinco leitores, por caminhos diferentes, chegaram à mesma conclusão de
fundo: **o documento é uma documentação de engenharia de firmware sólida,
mas não se decidiu a ser um relatório de iniciação científica.** Ele descreve
bem *o que foi construído*, mas não estabelece *por que isso é pesquisa*.
Essa é a falha estrutural que precede — e explica — quase todas as falhas
pontuais listadas abaixo. Não é um problema de um parágrafo faltando; é a
ausência do aparato que faria a banca reconhecer o texto como PIBIC e não
como manual de produto.

---

## 1. Completude técnica

### 1.1 Lacunas estruturais (afetam a confiabilidade do sistema, não só o texto)

- **`[5/5]` Nenhum dado quantitativo.** Todas as afirmações de robustez são
  qualitativas: "folga permanente na fila", "nenhum evento perdido", "três
  retransmissões" — sem número de amostras, duração do ensaio, taxa de perda
  medida, profundidade máxima de fila observada, ou latência botão→acionamento
  (nem mesmo estimada por cálculo). Para um texto que se orgulha de rigor
  (comparação em tempo constante, aritmética de wraparound), a ausência total
  de medição no ensaio ponta a ponta é a lacuna mais citada pelos cinco
  leitores.
- **`[4/5]` Comportamento em falha não é tratado.** O que acontece se o
  receptor perder energia e reiniciar de forma independente do emissor? O
  documento vende o protocolo como "autocurativo" (estado absoluto), mas isso
  só vale do ponto de vista do emissor — não há heartbeat nem consulta de
  estado, então a correção depende do próximo toque de botão, que pode nunca
  vir. Também não há definição de estado *default* seguro dos pinos de
  potência (desligado por segurança vs. indefinido até o primeiro pacote) —
  pergunta de segurança básica para um dispositivo que aciona transistores em
  um brinquedo.
- **`[4/5]` Gestão de chaves como processo, não como fato isolado.** Diz-se
  que as chaves ficam fora do controle de versão e que há detecção de chave
  trivial, mas nada sobre geração (entropia), distribuição entre unidades ou
  rotação. Fica implícito um esquema 1:1 que não escala, sem isso ser
  discutido como decisão consciente.
- **`[3/5]` Sem caracterização de RF/alcance.** A ferramenta de diagnóstico
  de rádio é citada, mas nenhum resultado (alcance, RSSI, perda por
  distância) é reportado no corpo do texto.
- **`[2/5]` Consumo de energia ausente do firmware** — relevante para um
  brinquedo presumivelmente a bateria (sleep modes, duty cycling nunca
  discutidos).
- **`[2/5]` Ausência de campo de versão de protocolo** em `pacote_t` — nada
  impede incompatibilidade silenciosa entre firmwares diferentes.
- **`[2/5]` Sem discussão de watchdog/brown-out** nem footprint de
  memória/pilha das tasks do FreeRTOS.

### 1.2 Lacunas de autocontenção

- **`[4/5]` Dependência estrutural do "documento de hardware".** Toda vez
  que o assunto fica tecnicamente denso (protocolo ESP-NOW, AES-CCM,
  dimensionamento elétrico), o texto delega para o outro documento — que não
  é referenciado formalmente (sem título, autor ou forma de obtê-lo). Isso
  significa que este documento, isoladamente, não pode ter sua alegação de
  segurança avaliada.
- **`[2/5]` Sem apêndice de build/flash completo nem lista dos 48 testes** —
  a cobertura é declarada por categoria, não demonstrada.

### 1.3 O que já está bem resolvido (não mexer)

Vale registrar, porque nenhum dos cinco leitores contestou: a separação
lógica/plataforma, a arquitetura de tarefas por prioridade, a máquina de
debounce testável por injeção de dependência, a aritmética de sequência com
tratamento de wraparound, e a comparação de remetente em tempo constante são
decisões de projeto bem concebidas e bem explicadas. O problema não é a
engenharia — é a lacuna de medição e de tratamento de falha em torno dela.

---

## 2. Falhas acadêmicas e comunicativas

### 2.1 Desqualificantes (uma banca provavelmente barra o texto por isso)

- **`[5/5]` Ausência de pergunta de pesquisa / objetivos / justificativa /
  metodologia.** O texto vai direto da capa para a Introdução como se fosse
  um manual técnico — porque, na estrutura atual, é isso que ele é. Para
  PIBIC, isso é o achado mais grave: sem essa seção, a banca não tem como
  julgar "o que foi investigado", só "o que foi construído".
- **`[5/5]` Referencial teórico praticamente inexistente.** As sete
  referências são: três normas ABNT, um livro técnico de 1999 e três manuais
  de fabricante (Espressif, FreeRTOS). Nenhum artigo revisado por pares,
  nenhuma comparação com trabalhos correlatos (protocolos de baixa potência,
  segurança em IoT, RTOS em sistemas embarcados). Um dos leitores nota
  inclusive que BARR (1999) aparece na lista de referências mas nunca é
  citado no corpo — quebra de NBR 6023.
- **`[4/5]` Inconsistência entre normas citadas e normas seguidas.** O
  documento cita a NBR 14724 (apresentação de trabalhos acadêmicos) mas não
  tem resumo, palavras-chave nem sumário — os próprios elementos que essa
  norma exige. Citar a norma e não segui-la é pior do que não citá-la.
- **`[3/5]` Placeholders não preenchidos**: `Autor: [preencher]` e
  `Orientador(a): [preencher]`. Isso por si só sinaliza documento não
  finalizado, antes de qualquer avaliação de conteúdo — no limite, motivo de
  devolução administrativa.

### 2.2 Enfraquecem o argumento (não desqualificam sozinhas, mas se acumulam)

- **`[4/5]` Afirmações de valor não sustentadas por métrica comparativa.**
  "Foi a decisão de projeto de maior retorno", "mostrou-se decisiva" — maior
  retorno comparado a quê? Lê como nota de release, não como inferência
  apoiada em dado.
  Ex.: seção 10.
- **`[4/5]` Validação anedótica.** "Ensaios com acionamento manual não
  registraram evento perdido" sem N de tentativas, duração ou condições —
  mesmo padrão de ausência de número do item técnico 1.1.
- **`[3/5]` "Considerações finais" não é uma conclusão científica.** Resume
  pontos positivos, mas não retoma objetivos (que nunca existiram), não
  discute limitações de forma consolidada (a única exceção real — cifragem
  de flash, §7.3 — fica isolada em vez de reunida numa seção própria), e não
  aponta trabalhos futuros.
- **`[2/5]` Tom de log de desenvolvimento.** Comentários de código
  coloquiais ("// Zera tudo.") e blocos de código extensos citados como
  "Quadros" ocupam espaço que poderia ser argumentação — código extenso
  deveria migrar para apêndice, com o algoritmo resumido em texto/pseudocódigo
  no corpo.
- **`[2/5]` Detalhe de formatação revelador**: "Brasília2026" sem espaço na
  folha de rosto — pequeno isoladamente, mas soma ao efeito de "rascunho não
  revisado" quando combinado com os placeholders em aberto.
- **`[1/5]` Ausência de controle de validade interna**: os testes foram
  projetados, executados e interpretados pelo mesmo autor do sistema, sem
  menção a revisão por terceiros.

---

## 3. Plano de ação recomendado

Ordenado por relação custo/impacto — os itens 1–3 evitam os riscos mais
caros na banca (devolução administrativa e "isso não é ciência"); os itens
4–6 fecham as lacunas técnicas mais baratas de resolver sem hardware novo.

| # | Ação | Resolve | Custo estimado |
|---|------|---------|-----------------|
| 1 | Preencher autor/orientador; corrigir "Brasília2026" e demais detalhes de folha de rosto | §2.1, §2.2 | 5 min |
| 2 | Escrever resumo, palavras-chave e seção de objetivo geral/específico/justificativa antes da atual seção 1 | §2.1 (maior risco) | ~meio dia — reescrita estrutural, não polimento |
| 3 | Levantar 6–10 referências acadêmicas reais (ex.: "ESP-NOW security", "low-power embedded remote actuation", "RTOS scheduling embedded systems", "debounce formal verification") e escrever um parágrafo de revisão de literatura que posicione o trabalho frente a elas | §2.1 | ~1 dia, depende de acesso a bases (IEEE/Scopus/SciELO) |
| 4 | Quantificar o que já foi observado, sem novo ensaio: nº de execuções do teste ponta a ponta, profundidade máxima de fila registrada, latência estimada por cálculo (5 ms de amostragem + tempo de rajada + processamento) | §1.1 | ~meio dia |
| 5 | Consolidar as limitações espalhadas (flash não cifrada, pareamento 1:1, ausência de gestão de energia, dependência do documento de hardware) em uma seção "Limitações e Trabalhos Futuros" | §1.2, §2.2 | 1–2h |
| 6 | Referenciar formalmente o documento de hardware (autor, título, como obtê-lo) ou anexá-lo como apêndice | §1.2 | ~1h |
| 7 *(opcional, mais caro)* | Medir taxa de perda real de pacote em função da distância, para justificar quantitativamente as três retransmissões | §1.1 | ~1 dia de bancada |

### O que quebra se isso for ignorado até a entrega

- Primeira pergunta previsível da banca: **"qual trabalho relacionado vocês
  compararam?"** — sem resposta pronta, a impressão vira "faltou embasamento
  científico", mesmo com o firmware correto. É o risco mais caro e o mais
  barato de mitigar (ação 3).
- Se os `[preencher]` sobreviverem até a submissão, o risco é devolução
  administrativa antes mesmo da leitura de conteúdo — perde-se um ciclo de
  prazo inteiro por um erro de cinco minutos.
- Se a limitação da cifragem de flash não for consolidada, o examinador a
  "descobre" lendo a seção 7.3 no meio do texto e questiona por que não foi
  discutida como risco — dá a impressão de que a equipe não percebeu, quando
  na verdade percebeu e só documentou no lugar errado.

---

## 4. Ponto de tensão entre os leitores (não resolvido, decisão do autor)

O advisor EXPANSIONIST assinalou que sua instrução original ("proibido
listar riscos") conflitava com o pedido real da tarefa e optou por ignorá-la
para cumprir a análise crítica — o que, na prática, faz seus achados
convergirem com os dos outros quatro. Não há divergência de conteúdo entre
os cinco leitores neste caso: a única tensão é estrutural (papel vs. tarefa),
já resolvida a favor da tarefa. Ou seja, os achados acima não têm
contraponto relevante a considerar — a convergência de 4 a 5 leitores
independentes nos mesmos pontos é o sinal mais forte deste relatório.
