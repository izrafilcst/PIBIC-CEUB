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

#include "chaves.h"
#include "config_rede.h"
#include "identidade.h"
#include "protocolo.h"
#include "receptor_logic.h"
#include "seguranca.h"

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
  const uint8_t *remetente = (info != NULL) ? info->src_addr : NULL;
#else
static void onRecebimento(const uint8_t *mac, const uint8_t *dados,
                          int tamanho) {
  const uint8_t *remetente = mac;
#endif
  // Contexto da task de WiFi: copiar e sair. Nada de digitalWrite nem Serial.

  // A criptografia do ESP-NOW só cobre peers registrados: um quadro de um MAC
  // desconhecido ainda chega aqui, em claro. Este filtro fecha essa porta.
  if (!remetenteAutorizado(remetente, MAC_EMISSOR)) {
    quadrosDescartados.fetch_add(1, std::memory_order_relaxed);
    return;
  }

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
  Serial.println("  copie o MAC acima para MAC_RECEPTOR em lib/config_rede.h");

  Serial.printf("  emissor aceito: %02X:%02X:%02X:%02X:%02X:%02X\n",
                MAC_EMISSOR[0], MAC_EMISSOR[1], MAC_EMISSOR[2], MAC_EMISSOR[3],
                MAC_EMISSOR[4], MAC_EMISSOR[5]);

  if (macEhPlaceholder(MAC_EMISSOR)) {
    Serial.println("  *** ATENCAO: MAC_EMISSOR ainda e o placeholder.     ***");
    Serial.println("  *** O filtro de remetente vai descartar tudo.       ***");
  }

  uint8_t meuMac[6];
  WiFi.macAddress(meuMac);
  if (!macEhPlaceholder(MAC_RECEPTOR) &&
      !remetenteAutorizado(meuMac, MAC_RECEPTOR)) {
    Serial.println("  *** ATENCAO: esta placa nao e a listada como RECEPTOR ***");
    Serial.println("  *** em config_rede.h. Firmware trocado de placa?      ***");
  }

  if (chaveEhInsegura(PMK) || chaveEhInsegura(LMK)) {
    Serial.println("  *** ATENCAO: chaves de exemplo em uso.        ***");
    Serial.println("  *** Rode `make chaves` e regrave as 2 placas. ***");
  }

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

  // Para DECIFRAR os quadros do emissor, o receptor também precisa registrá-lo
  // como peer cifrado com a mesma LMK. Sem isso os pacotes chegam ilegíveis.
  if (esp_now_set_pmk(PMK) != ESP_OK) {
    Serial.println("falha ao definir PMK");
    return;
  }

  esp_now_peer_info_t peerEmissor = {};
  memcpy(peerEmissor.peer_addr, MAC_EMISSOR, 6);
  memcpy(peerEmissor.lmk, LMK, TAM_CHAVE);
  peerEmissor.channel = 0;
  peerEmissor.encrypt = true;

  if (esp_now_add_peer(&peerEmissor) != ESP_OK) {
    Serial.println("falha ao adicionar peer emissor");
    return;
  }
  Serial.println("  criptografia ESP-NOW: ativa (AES-CCM)");

  xTaskCreate(tarefaLeds,        "leds", 4096, NULL, 3, NULL);
  xTaskCreate(tarefaDiagnostico, "diag", 3072, NULL, 1, NULL);
}

void loop() {
  // Todo o trabalho está nas tasks. A loopTask fica dormindo.
  vTaskDelay(pdMS_TO_TICKS(1000));
}
