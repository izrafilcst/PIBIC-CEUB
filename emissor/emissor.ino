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
// Preencher receptorMAC com o endereço real do Receptor (ele imprime o próprio
// MAC no boot).

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_random.h>
#include <inttypes.h>

#include <atomic>

#include "debounce.h"
#include "emissor_logic.h"
#include "identidade.h"
#include "protocolo.h"

uint8_t receptorMAC[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};  // TODO: ajustar

#define BOTAO0_PIN 4
#define BOTAO1_PIN 5

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
          esp_now_send(receptorMAC, (const uint8_t *)&p, sizeof(p));
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

void setup() {
  Serial.begin(115200);

  for (uint8_t i = 0; i < NUM_CANAIS; i++) {
    pinMode(pinos[i], INPUT_PULLUP);
    canalIniciar(&canais[i]);
  }

  WiFi.mode(WIFI_STA);

  // Depois do WiFi ligado o RNG por hardware tem entropia de verdade.
  bootId = esp_random();

  char banner[256];
  montarBanner(banner, sizeof(banner), PAPEL_EMISSOR,
               WiFi.macAddress().c_str(), bootId);
  Serial.print(banner);

  Serial.printf("  receptor alvo: %02X:%02X:%02X:%02X:%02X:%02X\n",
                receptorMAC[0], receptorMAC[1], receptorMAC[2], receptorMAC[3],
                receptorMAC[4], receptorMAC[5]);

  if (macEhPlaceholder(receptorMAC)) {
    Serial.println("  *** ATENCAO: receptorMAC ainda e o placeholder. ***");
    Serial.println("  *** Nenhum pacote vai chegar ao receptor.       ***");
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

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receptorMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("falha ao adicionar peer");
    return;
  }

  xTaskCreate(tarefaBotoes,      "botoes", 3072, NULL, 3, NULL);
  xTaskCreate(tarefaEnvio,       "envio",  4096, NULL, 2, NULL);
  xTaskCreate(tarefaDiagnostico, "diag",   3072, NULL, 1, NULL);
}

void loop() {
  // Todo o trabalho está nas tasks. A loopTask fica dormindo.
  vTaskDelay(pdMS_TO_TICKS(1000));
}
