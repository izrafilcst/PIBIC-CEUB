// EMISSOR — 2 botões via ESP-NOW, arquitetura FreeRTOS.
//
// Três tasks:
//   tarefaBotoes (prio 3) — amostra os pinos em período fixo, roda o debounce
//                           e publica o evento numa fila. Nunca bloqueia.
//   tarefaEnvio  (prio 2) — consome a fila e faz a rajada de reenvio. O
//                           vTaskDelay entre pacotes cede a CPU, então a
//                           leitura dos botões continua rodando durante ela.
//   tarefaDiag   (prio 1) — consome o status dos callbacks e relata.
//
// A lógica de debounce vive em debounce.cpp, sem dependência de Arduino, e é
// coberta por testes de host (`make test` na raiz do repositório).
//
// Preencher MAC_RECEPTOR com o endereço real do Receptor (ele imprime o próprio
// MAC no boot).

#include <Arduino.h>
#include <WiFi.h>
#include <esp_mac.h>
#include <esp_now.h>
#include <esp_random.h>
#include <esp_wifi.h>
#include <inttypes.h>

#include <atomic>

#include "chaves.h"
#include "config_rede.h"
#include "debounce.h"
#include "emissor_logic.h"
#include "identidade.h"
#include "protocolo.h"
#include "seguranca.h"

// Pinos por família de chip: o mesmo sketch compila no DevKit clássico e no
// C3, e trocar de placa não exige editar código.
// GPIO4/5 nos dois alvos: é como a bancada está montada, verificado com uma
// varredura de nível pino a pino.
//
// No ESP32 clássico o GPIO4 é um GPIO comum, sem função especial. O GPIO5 é
// strapping — amostrado no reset para escolher o timing de SDIO slave — mas
// esse strap não interfere no boot a partir da flash, e o pull-up (interno
// mais o externo de 10k da bancada) o mantém alto justamente no instante em
// que ele é lido. Segurar o botão durante o reset é inofensivo aqui.
//
// O JTAG do clássico fica em GPIO12..15, não em GPIO4: uma versão anterior
// deste comentário dizia o contrário e motivou a escolha de GPIO32/33.
#if CONFIG_IDF_TARGET_ESP32
  #define BOTAO0_PIN 4
  #define BOTAO1_PIN 5
#elif CONFIG_IDF_TARGET_ESP32C3
  #define BOTAO0_PIN 4
  #define BOTAO1_PIN 5
#else
  #error "Alvo não previsto: escolha pinos de botão válidos para este chip"
#endif

static const uint32_t DEBOUNCE_MS    = 40;
static const int      RAJADA_QTD     = 3;   // reenvios por evento
static const uint32_t RAJADA_GAP_MS  = 15;
static const uint32_t AMOSTRAGEM_MS  = 5;   // período da tarefaBotoes
static const uint32_t RELATO_MS      = 10000;
static const int      PROFUNDIDADE_FILA = 16;

static const uint8_t pinos[NUM_CANAIS] = {BOTAO0_PIN, BOTAO1_PIN};

static CanalBotao canais[NUM_CANAIS];
static uint32_t   bootId;

static QueueHandle_t filaEventos;      // pacote_t: botões -> envio
static QueueHandle_t filaStatusEnvio;  // status do callback -> diagnóstico

// Contadores cruzam fronteira de task: atômicos, não voláteis. Só contam
// ocorrências, então memory_order_relaxed basta.
static std::atomic<uint32_t> eventosPerdidos{0};  // escrito por tarefaBotoes
static std::atomic<uint32_t> errosDeEnvio{0};     // escrito por tarefaEnvio

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

static void tarefaBotoes(void *arg) {
  (void)arg;
  TickType_t ultimoDespertar = xTaskGetTickCount();

  for (;;) {
    const uint32_t agora = millis();

    for (uint8_t i = 0; i < NUM_CANAIS; i++) {
      const int leitura =
          (digitalRead(pinos[i]) == LOW) ? NIVEL_BAIXO : NIVEL_ALTO;

      if (!canalAtualizar(&canais[i], leitura, agora, DEBOUNCE_MS)) continue;

      const pacote_t p = montarPacote(bootId, i, &canais[i]);

      if (xQueueSend(filaEventos, &p, 0) != pdTRUE) {
        // fila cheia; aparece no relatório de diagnóstico
        eventosPerdidos.fetch_add(1, std::memory_order_relaxed);
      }
    }

    vTaskDelayUntil(&ultimoDespertar, pdMS_TO_TICKS(AMOSTRAGEM_MS));
  }
}

static void tarefaEnvio(void *arg) {
  (void)arg;
  pacote_t p;

  for (;;) {
    if (xQueueReceive(filaEventos, &p, portMAX_DELAY) != pdTRUE) continue;

    for (int i = 0; i < RAJADA_QTD; i++) {
      const esp_err_t r =
          esp_now_send(MAC_RECEPTOR, (const uint8_t *)&p, sizeof(p));
      if (r != ESP_OK) errosDeEnvio.fetch_add(1, std::memory_order_relaxed);

      // Cede a CPU: a tarefaBotoes tem prioridade maior e volta a rodar aqui.
      vTaskDelay(pdMS_TO_TICKS(RAJADA_GAP_MS));
    }

    Serial.printf("botao=%u estado=%u seq=%u\n", p.botao, p.estado, p.seq);
  }
}

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

#ifdef DINO_SMOKETEST
// Build de smoke test: injeta eventos sintéticos para exercitar o rádio sem
// depender de alguém apertando os botões. Não entra no firmware normal.
static void tarefaSmoke(void *arg) {
  (void)arg;
  vTaskDelay(pdMS_TO_TICKS(3000));  // deixa o receptor terminar o boot

  for (int ciclo = 0; ciclo < 2; ciclo++) {
    for (uint8_t canal = 0; canal < NUM_CANAIS; canal++) {
      canais[canal].estadoAtual = !canais[canal].estadoAtual;
      canais[canal].seq++;

      const pacote_t p = montarPacote(bootId, canal, &canais[canal]);
      xQueueSend(filaEventos, &p, 0);

      Serial.printf("[smoke] press sintetico canal=%u estado=%u\n", canal,
                    canais[canal].estadoAtual);
      vTaskDelay(pdMS_TO_TICKS(800));
    }
  }

  Serial.println("[smoke] fim");
  vTaskDelete(NULL);
}
#endif

void setup() {
  Serial.begin(115200);

  for (uint8_t i = 0; i < NUM_CANAIS; i++) {
    pinMode(pinos[i], INPUT_PULLUP);
    canalIniciar(&canais[i]);
  }

  WiFi.mode(WIFI_STA);
  // Sem associação a um AP, o modem sleep não tem beacon para sincronizar.
  WiFi.setSleep(false);
  // "Canal atual" (channel = 0) não garante que os dois rádios fiquem no mesmo
  // canal quando nenhum dos dois está associado. Fixa explicitamente.
  esp_wifi_set_channel(CANAL_ESPNOW, WIFI_SECOND_CHAN_NONE);

  // Depois do WiFi ligado o RNG por hardware tem entropia de verdade.
  bootId = esp_random();

  // Lê o próprio MAC uma vez e reusa: no banner e na checagem de identidade.
  // NÃO usar WiFi.macAddress() aqui: no ESP32 clássico ela devolve zeros até o
  // netif da STA subir, o que só acontece alguns milissegundos depois de
  // WiFi.mode(). esp_read_mac() lê da efuse e vale desde o primeiro instante,
  // nos dois alvos — e é o mesmo valor que o ESP-NOW usa como remetente.
  uint8_t meuMac[6];
  esp_read_mac(meuMac, ESP_MAC_WIFI_STA);
  char meuMacTexto[TAM_MAC_TEXTO];
  formatarMac(meuMacTexto, sizeof(meuMacTexto), meuMac);

  char banner[256];
  montarBanner(banner, sizeof(banner), PAPEL_EMISSOR, meuMacTexto, bootId);
  Serial.print(banner);

  uint8_t canalAtual = 0;
  wifi_second_chan_t canalSecundario = WIFI_SECOND_CHAN_NONE;
  esp_wifi_get_channel(&canalAtual, &canalSecundario);
  wifi_ps_type_t modoPs = WIFI_PS_NONE;
  esp_wifi_get_ps(&modoPs);
  Serial.printf("  radio ..: canal=%u ps=%d  botoes=GPIO%d/GPIO%d\n", canalAtual,
                (int)modoPs, BOTAO0_PIN, BOTAO1_PIN);

  Serial.printf("  receptor alvo: %02X:%02X:%02X:%02X:%02X:%02X\n",
                MAC_RECEPTOR[0], MAC_RECEPTOR[1], MAC_RECEPTOR[2], MAC_RECEPTOR[3],
                MAC_RECEPTOR[4], MAC_RECEPTOR[5]);

  if (macEhPlaceholder(MAC_RECEPTOR)) {
    Serial.println("  *** ATENCAO: MAC_RECEPTOR ainda e o placeholder. ***");
    Serial.println("  *** Nenhum pacote vai chegar ao receptor.        ***");
  }

  // Firmware trocado de placa: o MAC gravado em config_rede.h não bate com o
  // MAC real desta placa.
  if (!macEhPlaceholder(MAC_EMISSOR) && !remetenteAutorizado(meuMac, MAC_EMISSOR)) {
    Serial.println("  *** ATENCAO: esta placa nao e a listada como EMISSOR ***");
    Serial.println("  *** em config_rede.h. Firmware trocado de placa?     ***");
  }

  if (chaveEhInsegura(PMK) || chaveEhInsegura(LMK)) {
    Serial.println("  *** ATENCAO: chaves de exemplo em uso.        ***");
    Serial.println("  *** Rode `make chaves` e regrave as 2 placas. ***");
  }

  // As filas precisam existir antes de qualquer callback poder ser chamado.
  filaEventos     = xQueueCreate(PROFUNDIDADE_FILA, sizeof(pacote_t));
  filaStatusEnvio = xQueueCreate(PROFUNDIDADE_FILA,
                                 sizeof(esp_now_send_status_t));
  if (filaEventos == NULL || filaStatusEnvio == NULL) {
    Serial.println("falha ao criar filas");
    return;
  }

  if (esp_now_init() != ESP_OK) {
    Serial.println("falha ao iniciar esp-now");
    return;
  }
  esp_now_register_send_cb(onEnvioConcluido);

  // Criptografia nativa do ESP-NOW (AES-CCM). A PMK cifra a LMK no handshake;
  // a LMK cifra o tráfego deste par. As duas placas usam os mesmos 32 bytes.
  if (esp_now_set_pmk(PMK) != ESP_OK) {
    Serial.println("falha ao definir PMK");
    return;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, MAC_RECEPTOR, 6);
  memcpy(peerInfo.lmk, LMK, TAM_CHAVE);
  peerInfo.channel = CANAL_ESPNOW;
  peerInfo.encrypt = true;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("falha ao adicionar peer");
    return;
  }
  Serial.println("  criptografia ESP-NOW: ativa (AES-CCM)");

  xTaskCreate(tarefaBotoes,      "botoes", 3072, NULL, 3, NULL);
  xTaskCreate(tarefaEnvio,       "envio",  4096, NULL, 2, NULL);
  xTaskCreate(tarefaDiagnostico, "diag",   3072, NULL, 1, NULL);

#ifdef DINO_SMOKETEST
  Serial.println("  *** BUILD DE SMOKE TEST -- nao use como firmware final ***");
  xTaskCreate(tarefaSmoke, "smoke", 3072, NULL, 2, NULL);
#endif
}

void loop() {
  // Todo o trabalho está nas tasks. A loopTask fica dormindo.
  vTaskDelay(pdMS_TO_TICKS(1000));
}
