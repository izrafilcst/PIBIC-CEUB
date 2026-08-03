// RECEPTOR — 2 LEDs independentes via ESP-NOW, arquitetura FreeRTOS.
//
// O callback do ESP-NOW roda no contexto da task de WiFi: ele apenas copia o
// quadro para uma fila e retorna. Toda a decisão (deduplicação por sequência,
// detecção de reboot do emissor) e o acionamento dos LEDs acontecem na
// tarefaLeds, fora do caminho crítico do rádio.
//
// A lógica de decisão vive em receptor_logic.cpp, sem dependência de Arduino,
// e é coberta por testes de host (`make test` na raiz do repositório).
//
// Anote o MAC impresso no boot e coloque no receptorMAC do Emissor.

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_random.h>
#include <inttypes.h>

#include <atomic>

#include "identidade.h"
#include "protocolo.h"
#include "receptor_logic.h"

#define LED0_PIN 6
#define LED1_PIN 7

static const int      PROFUNDIDADE_FILA = 16;
static const uint32_t RELATO_MS         = 10000;

static const uint8_t ledPinos[NUM_CANAIS] = {LED0_PIN, LED1_PIN};

static EstadoReceptor estado;
static QueueHandle_t  filaQuadros;

// Escrito pelo callback (task de WiFi), lido pela task de diagnóstico.
static std::atomic<uint32_t> quadrosDescartados{0};

struct QuadroRecebido {
  uint8_t dados[sizeof(pacote_t)];
  int     tamanho;
};

// A assinatura do recv callback mudou no core 3.0.0. Sem esta guarda o sketch
// não compila nas duas gerações do arduino-esp32.
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
static void onRecebimento(const esp_now_recv_info_t *info, const uint8_t *dados,
                          int tamanho) {
  (void)info;
#else
static void onRecebimento(const uint8_t *mac, const uint8_t *dados,
                          int tamanho) {
  (void)mac;
#endif
  // Contexto da task de WiFi: copiar e sair. Nada de digitalWrite nem Serial.
  if (tamanho <= 0 || tamanho > (int)sizeof(pacote_t)) {
    quadrosDescartados.fetch_add(1, std::memory_order_relaxed);
    return;
  }

  QuadroRecebido q;
  memcpy(q.dados, dados, (size_t)tamanho);
  q.tamanho = tamanho;

  if (xQueueSend(filaQuadros, &q, 0) != pdTRUE) {
    quadrosDescartados.fetch_add(1, std::memory_order_relaxed);
  }
}

static void tarefaLeds(void *arg) {
  (void)arg;
  QuadroRecebido q;

  for (;;) {
    if (xQueueReceive(filaQuadros, &q, portMAX_DELAY) != pdTRUE) continue;

    const Decisao d = receptorProcessar(&estado, q.dados, q.tamanho);
    if (!d.aceitar) continue;

    digitalWrite(ledPinos[d.canal], d.nivelLed ? HIGH : LOW);
    Serial.printf("led%u -> %s\n", d.canal, d.nivelLed ? "aceso" : "apagado");
  }
}

static void tarefaDiagnostico(void *arg) {
  (void)arg;

  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(RELATO_MS));
    Serial.printf("[diag] descartados=%" PRIu32 " fila_livre=%u\n",
                  quadrosDescartados.load(std::memory_order_relaxed),
                  (unsigned)uxQueueSpacesAvailable(filaQuadros));
  }
}

void setup() {
  Serial.begin(115200);

  for (uint8_t i = 0; i < NUM_CANAIS; i++) {
    pinMode(ledPinos[i], OUTPUT);
    digitalWrite(ledPinos[i], LOW);
  }
  receptorIniciar(&estado);

  WiFi.mode(WIFI_STA);

  char banner[256];
  montarBanner(banner, sizeof(banner), PAPEL_RECEPTOR,
               WiFi.macAddress().c_str(), esp_random());
  Serial.print(banner);
  Serial.println("  copie o MAC acima para o receptorMAC do emissor");

  filaQuadros = xQueueCreate(PROFUNDIDADE_FILA, sizeof(QuadroRecebido));
  if (filaQuadros == NULL) {
    Serial.println("falha ao criar fila");
    return;
  }

  // A fila precisa existir antes do callback poder ser chamado.
  if (esp_now_init() != ESP_OK) {
    Serial.println("falha ao iniciar esp-now");
    return;
  }
  esp_now_register_recv_cb(onRecebimento);

  xTaskCreate(tarefaLeds,        "leds", 4096, NULL, 3, NULL);
  xTaskCreate(tarefaDiagnostico, "diag", 3072, NULL, 1, NULL);
}

void loop() {
  // Todo o trabalho está nas tasks. A loopTask fica dormindo.
  vTaskDelay(pdMS_TO_TICKS(1000));
}
