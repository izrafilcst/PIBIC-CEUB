# Council Transcript — Análise de software.docx + hardware.docx

**Data:** 2026-08-07
**Metodologia:** LLM Council (5 conselheiros independentes → revisão por pares anônima → síntese do chairman)

---

## Pergunta original do usuário

> analise os documentos @software.docx e @hardware.docx de um ponto de vista tecnico, buscando avaliar se seu conteúdo está completo. Avalie em seguida de um ponto de vista academico, buscando falhas comunicativas, conteúdos que desqualificam argumentos e etc.

## Pergunta enquadrada para o conselho

Dois documentos acadêmicos em português (relatório de iniciação científica, programa PIBIC do CEUB — Centro Universitário de Brasília) descrevem um projeto de sistemas embarcados de duas unidades: acionamento remoto de dois modos de um brinquedo via ESP-NOW (unidade emissora ESP32 com dois botões; unidade receptora ESP32-C3 que aciona transistores de potência). O Documento 1 é "software.docx" (firmware: arquitetura de tarefas, protocolo de aplicação, segurança, verificação). O Documento 2 é "hardware.docx" (circuitos, dimensionamento de componentes, enlace de rádio, mecanismo de criptografia, ensaios de bancada).

Os dois documentos dividem responsabilidade explicitamente: software.docx afirma que circuitos/dimensionamento/ensaios elétricos são tratados no documento de hardware, e que o protocolo ESP-NOW + criptografia são descritos no documento de hardware "por serem indissociáveis da camada física" — software.docx trata apenas de como o firmware os configura. hardware.docx confirma essa divisão e cobre adicionalmente a estrutura do protocolo, criptografia AES-CCM com hierarquia de chaves PMK/LMK, e ensaios de bancada (varredura de RF, continuidade de pinos, ensaio funcional).

Ambos os documentos têm lacunas de preenchimento: campos de Autor e Orientador(a) estão como "[preencher]" nos dois documentos. hardware.docx tem "[VCC a preencher]" para a tensão/corrente real de alimentação do brinquedo — o que significa que os cálculos de dissipação de potência e tensão entregue são estimados, não verificados, e isso está listado explicitamente como limitação conhecida. As figuras referenciadas em ambos os documentos não puderam ser extraídas como conteúdo visual verificável (apenas as legendas "Fonte: elaborado pelo autor" foram extraídas do texto).

**Pergunta em duas partes:**
1. **Completude técnica** — o conteúdo está completo para o escopo PIBIC? O que falta, está subespecificado ou é uma alegação não verificada?
2. **Qualidade acadêmica/comunicação** — falhas de comunicação, alegações sem sustentação, fraquezas estruturais/retóricas que preocupariam um orientador ou avaliador?

**O que está em jogo:** documento formal de entrega PIBIC, sujeito a normas ABNT, citado nas referências, destinado a revisão do orientador e provável defesa.

---

## Rodada 1 — Respostas independentes dos conselheiros

### The Contrarian

Both documents have a hole neither one owns: nobody validated the toy's actual load. hardware.docx flags "[VCC a preencher]" as a known limitation, but that's not a rounding error — the base resistor sizing (IB≈3.9mA), the worst-case IC(max)≈3.9A check, and the saturation power dissipation calc all depend on the real supply voltage/current. The bench test used LEDs, not the toy. So the one component the entire project exists to control was never electrically characterized. A defense committee will ask "what happens when you plug in the actual toy" and the honest answer is "we don't know, we estimated." That's a fatal gap dressed up as a footnote.

Second: the "inseparable from the physical layer" justification for putting ESP-NOW/encryption entirely in hardware.docx is a rationalization, not an engineering argument. Protocol structure and AES-CCM/PMK-LMK key hierarchy are firmware-configured software concerns — the physical layer is the RF front-end, not the packet format or crypto scheme. This split looks like it was drawn to make word counts or authorship boundaries work, not because the material belongs there. A reviewer will notice the seam.

Third: "0 losses" over 84 frames is being treated as a reliability claim, but n=84 in a benign RF environment with no stated distance/interference conditions is not evidence of protocol robustness — it's a smoke test. If packet loss handling matters enough to design sequence-number deduplication with wraparound handling, it matters enough to actually stress it.

Fourth: the RF-test-diagnosed-hardware-not-software story is presented as validated, but if VCC/current to the transistor stage was never confirmed, that same "defective hardware" diagnosis is standing on an unverified assumption.

Fifth: unfilled Author/Advisor fields aren't just cosmetic — submitting with "[preencher]" signals the document wasn't proofread end-to-end, which invites a reviewer to assume other things weren't either.

### The First Principles Thinker

Wrong question, and I'll say why bluntly: you asked "is it complete and does it read well" for a document that is currently architecturally incomplete in a way no amount of prose-polishing fixes. Two "[preencher]" author/advisor fields aren't a typo — they're evidence this was never run through a full read-print-review cycle. If that's still true at submission, nothing else about quality matters, because the document will bounce at intake before anyone reads the content.

Strip the framing further: this isn't "two documents," it's one system description artificially cut along a seam that the authors themselves admit is porous — they had to write a justification ("inseparable from the physical layer") for why ESP-NOW/encryption lives in hardware.docx and not software.docx. When you need a footnote to defend your own document boundary, the boundary is wrong, or at minimum it's a decision that should be surfaced to the advisor explicitly, not buried in scope-disclaimer sentences in each doc. Ask: does CEUB's PIBIC format even permit two-document submissions for one project? If that's not settled, everything downstream — completeness, ABNT compliance, citation structure — is being evaluated against the wrong unit of assessment.

The real first-order task isn't "polish these two docs." It's: (1) confirm the two-document structure is institutionally valid, (2) fill placeholders — non-negotiable, zero-tolerance, this is a defense-blocking issue not a technical one, (3) then ask whether content is complete. Doing completeness review before resolving structural validity is optimizing the wrong layer.

### The Expansionist

The upside case here isn't "polish two documents" — it's that the author is sitting on a genuinely uncommon asset and is about to bury it in a compliance checkbox.

Look at what's actually described: constant-time MAC comparison for sender authentication, sequence-number deduplication with signed-integer wraparound handling, a host-testable debounce state machine with 48 unit tests running without hardware, esp_read_mac() vs WiFi.macAddress() as an early-boot timing bug catch, AES-CCM with PMK/LMK key hierarchy, and RF-vs-firmware fault isolation via AP+scanner. That is not typical undergraduate toy-project rigor — most PIBIC embedded projects stop at "it blinks the LED, done." This one has security-conscious firmware engineering practices that would look at home in a professional embedded security writeup. The council is being asked "is it complete for PIBIC scope" — the more interesting question is whether PIBIC scope is underselling what's here.

Concretely: the constant-time comparison detail and the key-triviality-at-boot check are the kind of content that, expanded with threat-model framing (replay attack resistance via seq+MAC, why toggle-state was rejected in favor of absolute-state transmission), turns this from "a class project writeup" into a conference-submittable short paper (SBrT, WCGA, or an embedded systems workshop) or at minimum a strong portfolio/GitHub writeup for grad school applications or an embedded/IoT security job. The 8-byte packed protocol struct and dedup logic alone is publishable-quality protocol design documentation.

Don't just fix the "[preencher]" placeholders and ship for the PIBIC bar. Before submission, ask: does the advisor see conference-paper potential in the security-hardening angle? Is there a lab or venue where "we caught a wraparound bug and an early-boot MAC race condition" is worth a poster? The floor here is passing PIBIC review. The ceiling, with maybe two extra weeks of reframing around the security-engineering narrative, is a citable published result. Don't let ABNT formatting anxiety be the whole conversation.

### The Outsider

I have zero context on ESP-NOW, PIBIC norms, or ABNT formatting, so I'm reacting purely to the structure of what's presented — and structurally, this thing is riddled with tells.

Start with the division-of-labor story. Software.docx says "encryption is inseparable from the physical layer, so it belongs in hardware." That's a category error dressed up as a methodology decision. Encryption (AES-CCM, PMK/LMK) is a protocol-layer concern, not a physical-layer one — it has nothing to do with voltage or RF. If I'm a reviewer who doesn't already know the project, this sentence reads as a justification invented after the fact to explain why content ended up in the wrong document, not a real architectural principle. That's the single biggest red flag here: a claim that sounds technical but doesn't hold up to a first-principles read.

Second: "[preencher]" placeholders for Author and Advisor in a document allegedly ready to submit. A reviewer opens the file, sees blank author/advisor fields, and immediately loses confidence that anyone proofread the final pass — it signals the whole document might be a draft mistaken for a final.

Third: "[VCC a preencher]" feeding into power dissipation math that's then presented as a bench-verified calculation elsewhere. If I can't tell, as an outside reader, which numbers are measured versus guessed, I don't trust any of the numbers.

Fourth: figures with only "Fonte: elaborado pelo autor" captions extracted, no verifiable image content. If I were grading this blind, I cannot confirm the diagrams exist or say anything — that's not a minor gap, that's unverifiable claims wearing citation clothing.

None of this requires embedded-systems expertise to flag. It requires reading what's actually on the page.

### The Executor

Monday morning: nobody submits this yet. Here's what actually blocks submission and what doesn't.

**Blocking, fix first (both are 10-minute fixes):**
1. "[preencher]" for Author and Advisor in both docs. This isn't a technical gap, it's a "did you even proofread before sending it to your advisor" gap. If this goes to the advisor with their own name unfilled, that's the first impression. Fix it before anything else gets touched.
2. "[VCC a preencher]" — go measure the toy's actual supply voltage and current draw with a multimeter. This is not a research question, it's a five-minute bench task. You already have the toy in hand (you did a functional test with it). There is no excuse for this still being a placeholder at write-up time. Do this before you do literally anything else on this list, because it's the single item that turns an "estimated, unverified" limitation into a closed one.

**Not blocking, don't touch before submission:**
The Figures-not-extractable-as-text issue is a documentation-tooling artifact, not a project defect — someone needs to physically open both .docx files and eyeball whether the diagrams render, but that's a 5-minute visual check, not a task, and shouldn't be treated as a "finding."

**What I will not let this turn into:** a redesign discussion, a "should we re-run more bench tests" discussion, or a "is AES-CCM the right crypto choice" discussion. The functional test already passed (28 events, 0 losses) — that's your evidence of working system. Don't chase more rigor than the deliverable requires.

**Monday morning sequence:** (1) measure VCC/current on the toy, plug numbers into the dissipation calc, remove the limitation flag or narrow it; (2) fill in both name fields; (3) open both .docx files and confirm figures render; (4) submit. Everything else raised by the other advisors is post-submission or next-project scope — don't let academic-quality nitpicking delay a document that is otherwise functionally complete and already has passing bench-test evidence.

---

## Rodada 2 — Revisão por pares (anonimizada)

**Mapeamento (revelado):** A = Expansionist · B = Contrarian · C = Executor · D = First Principles Thinker · E = Outsider

### Revisor 1
1. **Strongest: B.** It's the only response that follows the causal chain instead of listing findings in parallel — it connects the unverified VCC directly to the base-resistor sizing, the saturation dissipation calc, *and* the "RF-not-firmware" fault diagnosis, showing that one placeholder undermines three separate claims.
2. **Biggest blind spot: A.** It reframes an unfinished deliverable with unverified core specs (VCC) and admittedly-rationalized document boundaries as a near-publishable result. That's actively risky advice.
3. **What all five missed:** none addresses whether a defense committee/grader can actually cross-reference claims between the two separate .docx files without figures rendering — the two-document format itself creates unverifiable cross-claims for a single grader.

### Revisor 2
1. **Strongest: B.** Connects technical and academic quality into one causal chain: the VCC gap doesn't just violate "verify your inputs" — it undermines the transistor sizing calcs, the "0 losses" reliability claim, and the RF-vs-hardware fault diagnosis simultaneously.
2. **Biggest blind spot: A.** Reframes "is this complete for PIBIC" into "should this be a conference paper," essentially opting out of the question asked.
3. **What all five missed:** None engaged with ABNT compliance itself (citation formatting, figure/table numbering, sectioning norms) despite it being named explicitly in the stakes. None checked cross-document consistency between software.docx and hardware.docx.

### Revisor 3
1. **Strongest: B.** Contradicts other advisors' claims with evidence — shows the VCC gap isn't cosmetic and directly rebuts C's "5-minute fix" framing.
2. **Biggest blind spot: A.** Recommending "two extra weeks of reframing" for a document whose author/advisor fields aren't filled in and whose core component was never electrically measured is misprioritized.
3. **All five missed:** flagged an apparent inconsistency between "28 events" (C) and "84 frames" (B) — note: this is a false alarm, since 28 events × 3 retransmissions = 84 frames (consistent with the source material's deliberate triple-repeat scheme); also nobody asked whether the missing figure content might hide missing/broken images rather than just being a tooling artifact.

### Revisor 4
1. **Strongest: B.** Traces the unverified-VCC gap forward into specific downstream consequences and gives a concrete, falsifiable critique of the "0 losses/84 frames" claim.
2. **Biggest blind spot: A.** Reframes a compliance-blocking submission as an opportunity for a conference paper — dodges the actual question.
3. **What all five missed:** None questioned whether presenting estimated figures as calculations without explicit "valor estimado"/error-bound labeling is itself an ABNT/PIBIC compliance problem. Nobody addressed how the two-document split affects citation/reference consistency.

### Revisor 5
1. **Strongest: B.** Connects the unverified VCC to both the sizing math AND the "hardware not software" fault diagnosis; correctly frames n=84 as a smoke test, not evidence.
2. **Biggest blind spot: C.** Waves off the ESP-NOW/hardware-doc boundary as a nitpick, but D and E independently identify that boundary as a structural red flag. Understates that fixing VCC means redoing dependent calculations and possibly revisiting the "hardware defect" conclusion.
3. **What all five missed:** none flagged that the figures issue (captions-only, no visual content extracted) is a limitation on the review itself, not just a project defect — nobody proposed actually opening the files to check.

---

## Rodada 3 — Síntese do Chairman

## Onde o Conselho Concorda

Há convergência forte, independente, em quatro pontos:

**1. O gap de VCC não é um detalhe — é uma falha que se propaga.** Contrarian, First Principles, Outsider e Executor identificaram — cada um por caminho próprio — que "[VCC a preencher]" não é uma lacuna isolada: ela contamina o dimensionamento do resistor de base, o cálculo de dissipação de potência em saturação, e (ponto mais fino, só o Contrarian traçou até o fim) a própria conclusão do teste RF de que a falha era "hardware defeituoso" — conclusão que também repousa sobre a tensão/corrente real nunca confirmadas.

**2. Os campos "[preencher]" de Autor/Orientador bloqueiam a submissão, independente de mérito técnico.** First Principles, Outsider e Executor concordam que isso é um sinal binário — ou o documento passou por revisão final completa, ou não passou.

**3. A justificativa "inseparável da camada física" para colocar ESP-NOW/criptografia inteiramente no hardware.docx é fraca.** Contrarian, First Principles e Outsider chegaram à mesma conclusão por ângulos diferentes: estrutura de protocolo e AES-CCM/hierarquia PMK-LMK são decisões de configuração de firmware, não de RF/física.

**4. "0 perdas em 84 quadros" é um teste de fumaça, não uma prova de robustez do protocolo.** n=84 em ambiente RF benigno, sem distância ou interferência declaradas, demonstra que o sistema funciona nas condições testadas — não que o protocolo é confiável de forma geral.

## Onde o Conselho Diverge

A divergência real é sobre prioridade, não sobre fatos. O Executor argumenta que VCC e os campos de preenchimento são "correções de 10 minutos" e que tudo mais é escopo de pós-submissão. First Principles e o Outsider discordam explicitamente: para eles, a fronteira entre os dois documentos é uma questão estrutural que pode ser rejeitada pela banca ou pelo próprio programa PIBIC. Essa divergência é genuína porque depende de uma pergunta que nenhum advisor pode responder sozinho: o formato PIBIC do CEUB permite submissão em dois documentos para um projeto?

Segunda divergência: o Expansionist propõe reenquadrar o projeto como submissão para congresso ou peça de portfólio. Os outros quatro advisors — e todos os cinco revisores pares — tratam isso como inversão de prioridade.

## Pontos Cegos que a Revisão por Pares Capturou

- Nenhum advisor, sozinho, levantou se um avaliador consegue de fato verificar afirmações cruzadas entre os dois arquivos .docx separados quando as figuras não renderizam como texto extraível.
- Nenhum advisor checou conformidade ABNT especificamente (numeração de figuras/tabelas, formatação de citação, seccionamento), apesar de estar citado explicitamente nas stakes.
- Ninguém checou se os cálculos "estimados" (dependentes do VCC não medido) estão rotulados como estimativa dentro do texto, versus apresentados com a mesma autoridade que os cálculos verificados por bancada.
- A alegação de "84 quadros" vs "28 eventos" foi sinalizada por um revisor como possível inconsistência — é falso alarme (28 × 3 retransmissões = 84), mas revela que a mecânica de retransmissão tripla não está reconciliada explicitamente no texto-fonte.
- A impossibilidade de extrair o conteúdo das figuras é também uma limitação da própria revisão — ninguém confirmou visualmente se os diagramas aparecem nos arquivos finais.

## A Recomendação

O documento não está pronto para submissão, mas está muito perto. Primeiro confirme se a divisão em dois documentos é institucionalmente válida para o formato PIBIC do CEUB. Se for válida, os problemas remanescentes — VCC, campos de autor/orientador, verificação visual das figuras, rotulagem explícita de valores estimados vs. medidos — são todos corrigíveis em uma sessão de trabalho. O caminho de reenquadrar para publicação (Expansionist) é uma ideia legítima e vale a pena levar ao orientador — depois, não em vez da submissão PIBIC.

## A Primeira Coisa a Fazer

Perguntar ao orientador, hoje, se o formato PIBIC do CEUB aceita entrega em dois documentos para um único projeto — porque essa resposta determina se as próximas correções são "encher lacunas" ou "refazer a arquitetura do relatório".

---

*Transcrição gerada automaticamente pelo LLM Council. Consulte `council-report-2026-08-07-1644.html` para a versão visual.*
