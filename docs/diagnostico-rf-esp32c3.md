# Diagnóstico de falha unidirecional de rádio em módulos ESP32-C3

**Projeto:** PIBIC-CEUB — Emissor/Receptor via ESP-NOW
**Data dos ensaios:** 4 de agosto de 2026
**Bancada:** Linux 7.0.0-28-generic, arduino-cli 1.5.1, core `esp32:esp32` 3.0.7

---

## Resumo

Dois módulos ESP32-C3 apresentavam falha de comunicação por ESP-NOW sem erro
reportado em software. Este documento registra o procedimento que isolou a
causa: **a recepção (RX) dos dois módulos está íntegra, enquanto a transmissão
(TX) de ambos é indetectável.** A conclusão foi obtida por dois métodos de
medição independentes — varredura de RF sem ESP-NOW e contabilidade de ACK da
camada 2 — e sobreviveu à troca de cabo USB e à inversão de papéis entre as
placas. Ao final, documenta-se a mitigação em software que permitiu ao sistema
operar apesar do defeito.

---

## 1. Contexto e motivação

O sistema consiste em duas placas: um **emissor** com dois botões e um
**receptor** com dois LEDs, comunicando-se por ESP-NOW com criptografia nativa
(AES-CCM). O par original era composto por dois ESP32-C3.

O sintoma inicial era desconcertante do ponto de vista de software: o emissor
reportava transmissão bem-sucedida e o receptor não registrava um único quadro,
nem no nível bruto do callback. Antes destes ensaios já haviam sido descartadas,
com evidência, as seguintes causas de software:

- criptografia (falha idêntica com `encrypt = false`);
- *power save* (falha idêntica com `WiFi.setSleep(false)` nos dois lados);
- divergência de canal (falha idêntica com canal fixado explicitamente);
- assimetria entre as placas (falha idêntica ao trocar os papéis).

A ausência de erro em software com falha total de entrega é a assinatura
clássica de um problema abaixo da camada de software. Isso motivou construir um
instrumento que medisse o rádio **sem** o ESP-NOW no caminho.

## 2. Objetivo e hipóteses

**Objetivo.** Determinar se a falha de comunicação tem origem em software
(configuração, canal, criptografia, filtro de remetente) ou em hardware de
rádio, e, no segundo caso, localizar qual sentido do enlace (TX ou RX) e qual
placa estão comprometidos.

**Hipóteses testadas:**

| # | Hipótese | Predição se verdadeira |
|---|---|---|
| H1 | Falha de software/configuração no ESP-NOW | Enlace funciona sem ESP-NOW (varredura Wi-Fi) |
| H2 | RX do C3 comprometido | C3 não enxerga nenhuma rede, nem as do ambiente |
| H3 | TX do C3 comprometido | Beacon emitido pelo C3 é invisível a outro rádio saudável |
| H4 | Queda de tensão no pico de TX (alimentação/cabo USB) | Trocar o cabo restaura o TX |
| H5 | Defeito isolado de uma unidade | A segunda unidade transmite normalmente |

## 3. Materiais

| Papel na bancada | Placa | Identificação (MAC STA) | Interface serial |
|---|---|---|---|
| Referência saudável | ESP32 DevKit clássico (ESP32-D0WDQ6 rev v1.0) | `80:7D:3A:F2:83:9C` | `/dev/ttyUSB0` (CP2102) |
| Unidade sob teste A | ESP32-C3 (rev v0.4) | `14:63:93:C3:25:04` | `/dev/ttyACM0` (USB nativa) |
| Unidade sob teste B | ESP32-C3 (rev v0.4) | `14:63:93:C2:F1:9C` | `/dev/ttyACM0` (USB nativa) |

O DevKit clássico cumpre aqui um papel metodológico essencial: é o **padrão de
comparação de rádio conhecidamente bom**, sem o qual não é possível atribuir
uma falha de enlace a uma das duas pontas.

Ambiente eletromagnético: laboratório com múltiplas redes Wi-Fi institucionais
(`CEUB`, `UniCEUB-visitante`) e dispositivos móveis, entre −29 dBm e −90 dBm.
Essas redes não são ruído a ser eliminado — são o **controle positivo** do
receptor, como se argumenta na §7.

## 4. Instrumentação

### 4.1 O medidor: RF puro, sem ESP-NOW

Para testar H1 é preciso remover o ESP-NOW do caminho. O instrumento é um
sketch mínimo em que uma placa sobe um ponto de acesso e a outra executa
varredura passiva, reportando o RSSI de tudo que enxerga. Se o enlace funciona
aqui e falha no ESP-NOW, o problema é software; se falha aqui, é rádio.

`ferramentas/teste_rf/teste_rf.ino`:

```cpp
// DIAGNOSTICO de RF puro, sem ESP-NOW: uma placa vira AP, a outra escaneia.
// Mede se as duas se enxergam e com que RSSI.
#include <Arduino.h>
#include <WiFi.h>

#define SSID_TESTE "DINO-TESTE-RF"

void setup() {
  Serial.begin(115200);
  delay(1500);

#ifdef DINO_AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(SSID_TESTE, "dino12345", 1);
  Serial.println("[rf] papel=AP ssid=" SSID_TESTE " canal=1");
  Serial.print("[rf] mac_ap="); Serial.println(WiFi.softAPmacAddress());
#else
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.println("[rf] papel=SCANNER");
#endif
}

void loop() {
#ifdef DINO_AP
  Serial.printf("[rf] AP no ar, clientes=%d\n", WiFi.softAPgetStationNum());
  delay(3000);
#else
  const int n = WiFi.scanNetworks();
  Serial.printf("[rf] varredura: %d redes\n", n);
  bool achou = false;
  for (int i = 0; i < n; i++) {
    const bool alvo = (WiFi.SSID(i) == SSID_TESTE);
    if (alvo) achou = true;
    Serial.printf("[rf]   %-24s rssi=%4d canal=%2d%s\n",
                  WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.channel(i),
                  alvo ? "   <== ALVO" : "");
  }
  Serial.println(achou ? "[rf] ALVO ENCONTRADO" : "[rf] ALVO AUSENTE");
  WiFi.scanDelete();
  delay(2000);
#endif
}
```

Duas decisões de projeto do instrumento merecem registro:

1. **O papel é escolhido em tempo de compilação** (`#ifdef DINO_AP`), não em
   tempo de execução. Isso elimina a possibilidade de a placa entrar no papel
   errado por estado residual, e torna a inversão de papéis (§6.3) uma operação
   de uma linha, não uma reescrita.
2. **A varredura lista *todas* as redes**, não apenas o alvo. É essa lista que
   fornece o controle positivo: uma varredura que retorna 12 redes do ambiente
   mas não o alvo prova que o receptor está funcionando e que a ausência do
   alvo é real, não uma falha da medição.

A inversão de papéis é feita apenas trocando a placa que recebe `-DDINO_AP`
(`Makefile:58-64`):

```make
teste-rf:
	arduino-cli compile --fqbn $(FQBN_EMISSOR) $(OPCOES_EMISSOR) \
	  --build-property build.defines=-DDINO_AP \
	  --upload --port $(PORTA_EMISSOR) ferramentas/teste_rf
	arduino-cli compile --fqbn $(FQBN_RECEPTOR) $(OPCOES_RECEPTOR) \
	  --upload --port $(PORTA_RECEPTOR) ferramentas/teste_rf
```

### 4.2 O segundo medidor: contabilidade de ACK

O ESP-NOW unicast usa o ACK da camada 2 do 802.11: o receptor confirma cada
quadro por hardware. Isso fornece uma medida de TX do receptor **independente**
da varredura de RF, e obtida sem qualquer firmware de teste — pelo firmware de
produção.

O emissor registra um callback de conclusão de envio que apenas repassa o
status (`emissor/emissor.ino:71-81`):

```cpp
// A assinatura do send callback mudou no core 3.2.0 (IDF 5.4). Sem esta guarda
// o sketch só compila numa faixa estreita de versões.
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 2, 0)
static void onEnvioConcluido(const wifi_tx_info_t *info,
                             esp_now_send_status_t status) {
  (void)info;
#else
static void onEnvioConcluido(const uint8_t *mac, esp_now_send_status_t status) {
  (void)mac;
#endif
  // Roda no contexto da task de WiFi: só repassa, sem Serial e sem bloquear.
  xQueueSend(filaStatusEnvio, &status, 0);
}
```

A contabilidade separa três grandezas que costumam ser confundidas
(`emissor/emissor.ino:128-153`):

```cpp
static void tarefaDiagnostico(void *arg) {
  (void)arg;
  uint32_t confirmados = 0;
  uint32_t semConfirmacao = 0;
  TickType_t ultimoRelato = xTaskGetTickCount();
  esp_now_send_status_t status;

  for (;;) {
    if (xQueueReceive(filaStatusEnvio, &status, pdMS_TO_TICKS(500)) == pdTRUE) {
      if (status == ESP_NOW_SEND_SUCCESS) {
        confirmados++;
      } else {
        semConfirmacao++;
      }
    }

    if ((xTaskGetTickCount() - ultimoRelato) >= pdMS_TO_TICKS(RELATO_MS)) {
      ultimoRelato = xTaskGetTickCount();
      Serial.printf("[diag] confirmados=%" PRIu32 " sem_confirmacao=%" PRIu32
                    " erro_no_envio=%" PRIu32 " eventos_perdidos=%" PRIu32 "\n",
                    confirmados, semConfirmacao,
                    errosDeEnvio.load(std::memory_order_relaxed),
                    eventosPerdidos.load(std::memory_order_relaxed));
    }
  }
}
```

A distinção é diagnóstica:

- `erro_no_envio` — `esp_now_send()` retornou erro: falha **local**, antes do ar;
- `sem_confirmacao` — o quadro saiu, mas nenhum ACK voltou: falha **do enlace**;
- `confirmados` — ACK recebido: o quadro chegou *e* o receptor conseguiu responder.

Sem essa separação, um único contador de "falhas" confundiria um peer mal
configurado com um rádio quebrado.

## 5. Controles experimentais

Três variáveis capazes de produzir o mesmo sintoma foram fixadas antes dos
ensaios, para que a falha observada não pudesse ser atribuída a elas.

**Canal fixo.** Sem associação a um ponto de acesso, `channel = 0` ("canal
atual") não garante que os dois rádios permaneçam no mesmo canal
(`lib/config_rede.h`, `emissor/emissor.ino:189-194`):

```cpp
WiFi.mode(WIFI_STA);
// Sem associação a um AP, o modem sleep não tem beacon para sincronizar.
WiFi.setSleep(false);
// "Canal atual" (channel = 0) não garante que os dois rádios fiquem no mesmo
// canal quando nenhum dos dois está associado. Fixa explicitamente.
esp_wifi_set_channel(CANAL_ESPNOW, WIFI_SECOND_CHAN_NONE);
```

**Power save desligado.** O modem sleep sincroniza janelas de recepção pelo
beacon do AP; sem associação, não há beacon, e o rádio pode perder quadros de
forma imprevisível. Desligá-lo remove essa fonte de perda intermitente.

**Verificação do estado real do rádio.** Não basta configurar: os dois sketches
leem de volta o canal e o modo de *power save* efetivos e os imprimem no boot
(`emissor/emissor.ino:204-210`), de modo que cada ensaio registra as condições
sob as quais foi realizado:

```cpp
uint8_t canalAtual = 0;
wifi_second_chan_t canalSecundario = WIFI_SECOND_CHAN_NONE;
esp_wifi_get_channel(&canalAtual, &canalSecundario);
wifi_ps_type_t modoPs = WIFI_PS_NONE;
esp_wifi_get_ps(&modoPs);
Serial.printf("  radio ..: canal=%u ps=%d  botoes=GPIO%d/GPIO%d\n", canalAtual,
              (int)modoPs, BOTAO0_PIN, BOTAO1_PIN);
```

Em todos os ensaios os banners confirmaram `canal=1 ps=0`.

## 6. Procedimento e resultados

Cada ensaio consistiu em gravar as duas placas e capturar 15 s de saída serial
do lado escaneador, a 115200 bps. O intervalo entre varreduras é de 2 s, de
modo que cada captura contém de 3 a 4 varreduras completas.

### 6.1 E1 — RX do C3-A (DevKit = AP)

| Varredura | Redes detectadas | Alvo `DINO-TESTE-RF` | Veredito |
|---|---|---|---|
| 1 | 9 | −50 dBm | ENCONTRADO |
| 2 | 6 | −51 dBm | ENCONTRADO |
| 3 | 10 | −52 dBm | ENCONTRADO |

Redes do ambiente na mesma captura: −77 a −87 dBm.

### 6.2 E2 — RX do C3-B (DevKit = AP)

| Varredura | Redes detectadas | Alvo | Veredito |
|---|---|---|---|
| 1 | 12 | −52 dBm | ENCONTRADO |
| 2 | 13 | −52 dBm | ENCONTRADO |
| 3 | 12 | −50 dBm | ENCONTRADO |

**H2 refutada.** O RX dos dois módulos é excelente e mutuamente
indistinguível: o alvo aparece 25 a 35 dB acima das redes institucionais, com
variação de ±1 dB entre varreduras — dentro do ruído normal de RSSI. Nenhum dos
dois se destaca como melhor receptor.

### 6.3 E3 — TX do C3 (papéis invertidos: C3 = AP, DevKit = scanner)

Este é o ensaio decisivo. A inversão é o que transforma "as duas não se falam"
em uma afirmação sobre um sentido específico do enlace.

| Varredura | Redes detectadas | Alvo | Veredito |
|---|---|---|---|
| 1 | 10 | — | **AUSENTE** |
| 2 | 10 | — | **AUSENTE** |
| 3 | 11 | — | **AUSENTE** |

O DevKit enxergou normalmente as redes do ambiente (−36 a −89 dBm) e um celular
a −37 dBm, mas **não** o AP do C3 a poucos centímetros de distância.

**H1 refutada, H3 sustentada.** A falha ocorre sem ESP-NOW, sem criptografia,
sem filtro de remetente e sem peers — portanto não é software. E é
unidirecional.

### 6.4 E4 — Confirmação por segundo método (ACK)

Executado o teste de integração de produção (`make smoke`), com DevKit como
emissor e C3-A como receptor. O emissor injeta 4 eventos sintéticos, cada um
retransmitido 3 vezes (12 quadros unicast).

```
== comunicação ponta a ponta ==
  ok    emissor gerou eventos
  ok    emissor transmitiu
  ok    receptor acendeu o LED0
  ok    receptor acendeu o LED1
  FALHA 12 quadros confirmados por ACK (obtido 0)
  FALHA nenhum quadro sem ACK (obtido 12)

[diag] confirmados=0 sem_confirmacao=12 erro_no_envio=0 eventos_perdidos=0   (emissor)
[diag] descartados=0 fila_livre=16                                            (receptor)
```

Este resultado é o mais informativo de todo o estudo, porque as duas metades
são contraditórias apenas na aparência:

- **os dados chegaram** — os LEDs acenderam, o receptor decifrou corretamente
  (AES-CCM ativo) e `descartados=0` mostra que nenhum quadro foi rejeitado pelo
  filtro de remetente;
- **nenhum ACK voltou** — `sem_confirmacao=12` de 12;
- **não houve erro local** — `erro_no_envio=0`: a API aceitou todos os envios.

O ACK do 802.11 é gerado pelo *receptor* e transmitido de volta. Um receptor que
decifra corretamente a carga útil mas não consegue emitir um ACK de 14 bytes tem,
necessariamente, RX íntegro e TX comprometido. **Um método totalmente
independente da varredura de RF chega à mesma conclusão.**

### 6.5 E5 — Teste de H4 (alimentação/cabo USB)

O pico de corrente durante a transmissão Wi-Fi (tipicamente 300–500 mA) pode
provocar queda de tensão suficiente para abortar o TX em cabos ou portas de
baixa qualidade — um mecanismo que produziria exatamente o sintoma observado.
Trocou-se o cabo USB e repetiu-se E3 sem qualquer alteração de firmware.

| Varredura | Redes detectadas | Alvo | Veredito |
|---|---|---|---|
| 1 | 13 | — | AUSENTE |
| 2 | 13 | — | AUSENTE |
| 3 | 12 | — | AUSENTE |

**H4 refutada** para a via testada (cabo).

### 6.6 E6 — Teste de H5 (segunda unidade)

Repetiu-se E3 com a outra unidade C3 como AP.

| Varredura | Redes detectadas | Alvo | Veredito |
|---|---|---|---|
| 1 | 12 | — | AUSENTE |
| 2 | 14 | — | AUSENTE |
| 3 | 13 | — | AUSENTE |

**H5 refutada.** As duas unidades falham de modo idêntico.

### 6.7 Controle negativo (não planejado)

Uma execução do `make smoke` com o MAC errado em `lib/config_rede.h` produziu,
involuntariamente, um controle negativo valioso:

```
  *** ATENCAO: esta placa nao e a listada como RECEPTOR ***
  FALHA receptor acendeu o LED0
  FALHA receptor acendeu o LED1
```

Com o MAC do emissor divergente, o filtro de remetente do receptor descartou
todos os quadros e os LEDs permaneceram apagados. Isso demonstra que o
acendimento dos LEDs em E4 **não** é um artefato — é entrega genuína,
autenticada pelo MAC de origem. O filtro responsável (`receptor/receptor.ino:69-74`):

```cpp
// A criptografia do ESP-NOW só cobre peers registrados: um quadro de um MAC
// desconhecido ainda chega aqui, em claro. Este filtro fecha essa porta.
if (!remetenteAutorizado(remetente, MAC_EMISSOR)) {
  quadrosDescartados.fetch_add(1, std::memory_order_relaxed);
  return;
}
```

### 6.8 Síntese

| Ensaio | Unidade | Sentido medido | Método | Resultado |
|---|---|---|---|---|
| E1 | C3-A | RX | Varredura RF | ✅ −50…−52 dBm |
| E2 | C3-B | RX | Varredura RF | ✅ −50…−52 dBm |
| E3 | C3 | **TX** | Varredura RF | ❌ ausente (3/3) |
| E4 | C3-A | **TX** | ACK 802.11 | ❌ 0/12 confirmados |
| E5 | C3-A | **TX** | Varredura RF (cabo novo) | ❌ ausente (3/3) |
| E6 | C3-B | **TX** | Varredura RF | ❌ ausente (3/3) |

**16 varreduras e 12 quadros unicast, dois métodos independentes, duas
unidades: nenhuma transmissão detectada em nenhum ensaio.**

## 7. Análise

O resultado central é uma **assimetria extrema entre os sentidos do enlace** na
mesma placa e no mesmo instante. Isso é o que dá força ao diagnóstico: RX e TX
compartilham antena, alimentação, cristal, canal e configuração de software.
Qualquer explicação que envolva essas partes comuns preveria degradação nos dois
sentidos. Apenas um defeito na cadeia exclusiva de transmissão explica RX a
−50 dBm com TX indetectável.

O controle positivo é o que sustenta cada resultado negativo. Uma varredura que
retorna "0 redes" seria inconclusiva — poderia ser o scanner com defeito. As
varreduras de E3, E5 e E6 retornaram consistentemente 10 a 14 redes, incluindo
sinais fracos a −89 dBm. O instrumento estava demonstravelmente funcionando
quando não encontrou o alvo a poucos centímetros.

Como os dois módulos falham identicamente, a causa mais provável não é dano
acidental a uma peça, e sim uma característica comum: mesmo lote de fabricação,
mesmo defeito de montagem no *front-end* de RF, ou uma limitação de projeto da
variante de placa adquirida. **Esta atribuição permanece uma hipótese não
testada** — distinguir entre elas exigiria instrumentação indisponível na
bancada (§8).

Registra-se ainda a consequência prática: no ESP-NOW unicast, um nó que apenas
"recebe" ainda precisa transmitir o ACK. Não existe, portanto, papel puramente
passivo que contorne o defeito — qualquer lógica que dependa de confirmação de
entrega interpretará permanentemente a entrega como falha, ainda que os dados
tenham chegado.

## 8. Ameaças à validade

Registram-se as limitações que o leitor deve considerar antes de generalizar
estes resultados.

**"TX ausente" significa "abaixo do piso de detecção", não "zero".** A varredura
detectou redes até −90 dBm. Um transmissor apenas atenuado em ~40 dB colocaria o
AP, esperado em ~−50 dBm a curta distância, próximo ou abaixo desse piso. A
afirmação rigorosa é: *a potência irradiada está pelo menos 40 dB abaixo do
esperado*. Isso é suficiente para inviabilizar o enlace, mas não distingue
"cadeia de TX morta" de "severamente atenuada".

**Identidade das unidades não foi registrada em todos os ensaios.** Os MACs
foram confirmados por esptool e pelos banners de boot em E1, E4 e nos ensaios
finais. Em E2, E3 e E6 a identidade da placa apoia-se no relato de troca física,
não em evidência instrumentada. Isso não afeta a conclusão principal — as duas
unidades foram testadas e ambas falharam — mas impede atribuir com certeza cada
varredura individual a uma unidade específica. **Recomendação para trabalhos
futuros:** imprimir e capturar o MAC em toda captura serial, como o `teste_rf`
já faz no modo AP (`Serial.print(WiFi.softAPmacAddress())`).

**H4 foi testada apenas parcialmente.** Trocou-se o cabo USB, o que refuta a via
mais provável de queda de tensão. Não se testou porta USB alternativa,
alimentação externa regulada, nem redução de potência de transmissão via
`WiFi.setTxPower()` — esta última foi considerada e **não executada**. Uma
queda de tensão de origem interna à placa permanece, portanto, não descartada.

**Ausência de medição física.** Não houve acesso a analisador de espectro,
medidor de potência de RF ou osciloscópio no trilho de alimentação. Todas as
medições são feitas pelo próprio rádio de uma segunda placa. A conclusão é
sólida quanto ao *efeito* (o enlace não se estabelece em um sentido), porém a
*causa raiz* física permanece inferida.

**Amostra de duas unidades.** A generalização para o lote ou para a variante de
placa é uma inferência plausível, não um resultado estabelecido.

## 9. Mitigação implementada

Estabelecido que o defeito é de hardware e que não há unidades substitutas
disponíveis, o objetivo passou a ser operar o sistema apesar dele. A entrega
funciona; apenas a confirmação não. A mitigação, portanto, não altera o
firmware — altera o **critério de aceitação** do teste, tornando explícito que
o ACK é inexigível neste hardware.

Em `ferramentas/smoketest.py`, as duas asserções de ACK passam a ser
condicionais, preservando intacta a verificação que de fato importa (a entrega
ponta a ponta, medida pelo acendimento dos LEDs):

```python
p.add_argument("--receptor-sem-tx", action="store_true",
               help="receptor não consegue transmitir (TX quebrado): "
                    "não exige ACK, só a entrega em si")
```

```python
# 4 eventos sintéticos x 3 reenvios da rajada = 12 quadros unicast. Em
# hardware saudável, todos voltam com ACK. Conferir o último relatório em
# vez de só procurar texto.
relatos = re.findall(r"confirmados=(\d+) sem_confirmacao=(\d+)", emi)
confirmados, sem_ack = (int(relatos[-1][0]), int(relatos[-1][1])) if relatos \
    else (0, -1)
if args.receptor_sem_tx:
    # O ACK sai do receptor: sem TX nele, nunca chega. A entrega em si
    # (led0/led1 acesos, checado acima) é o que importa aqui.
    print(f"  (info) confirmados={confirmados} sem_confirmacao={sem_ack} "
          f"-- ACK ignorado, receptor sem TX")
else:
    verifica(confirmados >= 12, f"12 quadros confirmados por ACK (obtido {confirmados})")
    verifica(sem_ack == 0, f"nenhum quadro sem ACK (obtido {sem_ack})")
```

Três propriedades desta mitigação são deliberadas:

1. **O comportamento padrão não muda.** Sem a flag, a exigência de 12 ACKs
   continua valendo. Hardware saudável não passa a ser avaliado com critério
   frouxo.
2. **Os números continuam sendo impressos.** A degradação fica visível no
   relatório (`confirmados=0 sem_confirmacao=12`), apenas não reprova a
   execução. Um teste que esconde a anomalia seria pior que um teste que falha.
3. **A entrega continua sendo exigida.** Os LEDs são verificados
   incondicionalmente; se a comunicação parar de fato, o teste reprova.

Exposta no `Makefile` por uma variável, sem alterar o alvo padrão:

```make
FLAGS_SMOKE ?=

smoke: sketches
	@python3 ferramentas/smoketest.py \
	  --emissor $(PORTA_EMISSOR)   --fqbn-emissor $(FQBN_EMISSOR) \
	  --receptor $(PORTA_RECEPTOR) --fqbn-receptor $(FQBN_RECEPTOR) \
	  --opcoes-receptor "$(OPCOES_RECEPTOR)" $(FLAGS_SMOKE)
```

**Resultado da validação final** — DevKit (`80:7D:3A:F2:83:9C`) como emissor,
C3-B (`14:63:93:C2:F1:9C`) como receptor:

```
== identidade ==
  ok    emissor se identifica
  ok    receptor se identifica
  ok    emissor não se diz receptor

== criptografia ==
  ok    emissor cifrando
  ok    receptor decifrando

== configuração ==
  ok    emissor sem avisos de configuração
  ok    receptor sem avisos de configuração

== comunicação ponta a ponta ==
  ok    emissor gerou eventos
  ok    emissor transmitiu
  ok    receptor acendeu o LED0
  ok    receptor acendeu o LED1
  (info) confirmados=0 sem_confirmacao=12 -- ACK ignorado, receptor sem TX

0 falha(s)
```

O sistema cumpre sua função — dois botões acionam dois LEDs remotamente, com
tráfego cifrado e remetente autenticado — operando sobre hardware com um dos
sentidos do enlace comprometido.

## 10. Conclusões

1. A falha de comunicação do par ESP32-C3 **não tem origem em software**:
   reproduz-se sem ESP-NOW, sem criptografia e sem filtro de remetente.
2. A falha é **unidirecional**: a recepção dos dois módulos opera normalmente
   (−50 a −52 dBm em curta distância, sensibilidade até −90 dBm), enquanto a
   transmissão é indetectável em todos os ensaios.
3. O diagnóstico é sustentado por **dois métodos independentes** — varredura de
   RF sem ESP-NOW e contabilidade de ACK da camada 2 — que convergem.
4. **As duas unidades falham identicamente**, o que aponta para causa comum
   (lote/variante) e não para dano isolado; a causa física precisa permanece
   inferida e não medida.
5. A troca de cabo USB **não** restaura a transmissão, refutando a via mais
   provável de queda de tensão por alimentação.
6. No ESP-NOW unicast **não existe papel puramente receptor**: o ACK exige TX.
   Sistemas que dependem de confirmação de entrega são inviáveis neste hardware.
7. É possível **operar o sistema apesar do defeito**, desde que o critério de
   sucesso seja a entrega verificada no receptor, e não a confirmação no
   emissor.

### Lição metodológica

O ganho decisivo veio de **inverter os papéis**. Enquanto o experimento pergunta
"as duas placas se comunicam?", a resposta é binária e não localiza o defeito —
e, no caso, levou a meses de investigação em software. Ao decompor o enlace em
dois testes unidirecionais, com uma referência sabidamente saudável em uma das
pontas, um sintoma opaco tornou-se uma medida com endereço. O mesmo princípio
aparece no controle positivo: um resultado negativo só tem valor quando se
demonstra, na mesma medição, que o instrumento estava funcionando.

## 11. Reprodutibilidade

**Ambiente:** Linux 7.0.0-28-generic · arduino-cli 1.5.1 · core `esp32:esp32`
3.0.7 · esptool 4.6 · Python 3.12.3 · pyserial 3.5.

```sh
# 1. Preparação
make chaves                     # gera lib/chaves.h (PMK/LMK aleatórias)
make macs                       # lê os MACs reais -> lib/config_rede.h
make test                       # 42 testes de host, sem hardware

# 2. Diagnóstico de RF — emissor como AP, receptor como scanner
make teste-rf

# 3. Inversão de papéis — o ensaio decisivo (E3)
arduino-cli compile --fqbn esp32:esp32:esp32c3 --board-options CDCOnBoot=cdc \
  --build-property build.defines=-DDINO_AP \
  --upload --port /dev/ttyACM0 ferramentas/teste_rf
arduino-cli compile --fqbn esp32:esp32:esp32 \
  --upload --port /dev/ttyUSB0 ferramentas/teste_rf

# 4. Captura serial (15 s, 115200 bps)
python3 - <<'EOF'
import serial, time
ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)
fim = time.time() + 15
while time.time() < fim:
    linha = ser.readline()
    if linha:
        print(linha.decode(errors='replace').rstrip())
ser.close()
EOF

# 5. Restaurar firmware de produção e validar
make gravar
make smoke FLAGS_SMOKE=--receptor-sem-tx
```

> **Atenção.** `make teste-rf` e a inversão de papéis **sobrescrevem** o firmware
> de produção das duas placas. Executar `make gravar` ao final para restaurá-lo.

**Notas de bancada.** O ESP32-C3 exige `CDCOnBoot=cdc`; sem essa opção o
`Serial` sai pela UART0 e nada aparece em `/dev/ttyACM*`. O DevKit clássico não
possui esse menu e rejeita a opção — por isso o `Makefile` mantém
`OPCOES_EMISSOR` e `OPCOES_RECEPTOR` separados. Durante os ensaios observou-se
também que o bloqueio de tela do computador derrubou a enumeração USB da ponte
CP2102, interrompendo uma gravação; recomenda-se desativar a suspensão de portas
USB durante capturas longas.

## 12. Trabalhos futuros

- Medir a potência irradiada com analisador de espectro, para quantificar a
  atenuação e distinguir cadeia de TX inoperante de severamente atenuada.
- Monitorar a tensão de alimentação no instante do TX com osciloscópio,
  fechando definitivamente H4.
- Repetir o protocolo com um C3 de lote/fornecedor distinto, testando a
  hipótese de defeito de lote.
- Avaliar `WiFi.setTxPower()` reduzido, verificando se um pico de corrente menor
  restabelece a transmissão.
- Ampliar a amostra além de duas unidades antes de qualquer generalização.
