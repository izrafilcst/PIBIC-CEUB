// RECEPTOR — 2 modos independentes do brinquedo via ESP-NOW, FreeRTOS.
//
// Cada saída comanda a base de um TIP122 em chave de lado baixo, que alimenta
// um dos dois modos do brinquedo. O pino não alimenta a carga: só injeta a
// corrente de base. Ver docs/hardware.html para o dimensionamento.
//
// O callback do ESP-NOW roda no contexto da task de WiFi: ele apenas copia o
// quadro para uma fila e retorna. Toda a decisão (deduplicação por sequência,
// detecção de reboot do emissor) e o acionamento das saídas acontecem na
// tarefaModos, fora do caminho crítico do rádio.
//
// A lógica de decisão vive em receptor_logic.cpp, sem dependência de Arduino,
// e é coberta por testes de host (`make test` na raiz do repositório).
//
// Anote o MAC impresso no boot e coloque no receptorMAC do Emissor.

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_mac.h>
#include <esp_random.h>
#include <esp_wifi.h>
#include <inttypes.h>

#include <atomic>

#include "chaves.h"
#include "config_rede.h"
#include "identidade.h"
#include "protocolo.h"
#include "receptor_logic.h"
#include "seguranca.h"

// Pinos por família de chip, para o mesmo sketch servir nas duas placas.
// No ESP32 clássico a faixa GPIO6..GPIO11 é reservada à flash SPI, daí os
// pinos serem outros lá — a numeração não é intercambiável entre famílias.
#if CONFIG_IDF_TARGET_ESP32
  #define MODO0_PIN 25
  #define MODO1_PIN 26
#elif CONFIG_IDF_TARGET_ESP32C3
  #define MODO0_PIN 6
  #define MODO1_PIN 7
#else
  #error "Alvo não previsto: escolha pinos de saída válidos para este chip"
#endif

static const int      PROFUNDIDADE_FILA = 16;
static const uint32_t RELATO_MS         = 10000;

static const uint8_t pinosModo[NUM_CANAIS] = {MODO0_PIN, MODO1_PIN};

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

static void tarefaModos(void *arg) {
  (void)arg;
  QuadroRecebido q;

  for (;;) {
    if (xQueueReceive(filaQuadros, &q, portMAX_DELAY) != pdTRUE) continue;

    const Decisao d = receptorProcessar(&estado, q.dados, q.tamanho);
    if (!d.aceitar) continue;

    digitalWrite(pinosModo[d.canal], d.nivelModo ? HIGH : LOW);
    Serial.printf("modo%u -> %s\n", d.canal,
                  d.nivelModo ? "ligado" : "desligado");
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

  // Nível baixo antes de tudo: base sem corrente, TIP122 cortado, brinquedo
  // desligado. Até aqui os pinos estavam em alta impedância e os resistores
  // internos de base do Darlington é que seguravam o transistor no corte.
  for (uint8_t i = 0; i < NUM_CANAIS; i++) {
    pinMode(pinosModo[i], OUTPUT);
    digitalWrite(pinosModo[i], LOW);
  }
  receptorIniciar(&estado);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  esp_wifi_set_channel(CANAL_ESPNOW, WIFI_SECOND_CHAN_NONE);

  // Ver comentário equivalente no emissor: WiFi.macAddress() ainda não vale
  // neste ponto do setup(). A efuse vale. Lê uma vez, usa no banner e na
  // checagem de identidade.
  uint8_t meuMac[6];
  esp_read_mac(meuMac, ESP_MAC_WIFI_STA);
  char meuMacTexto[TAM_MAC_TEXTO];
  formatarMac(meuMacTexto, sizeof(meuMacTexto), meuMac);

  char banner[256];
  montarBanner(banner, sizeof(banner), PAPEL_RECEPTOR, meuMacTexto,
               esp_random());
  Serial.print(banner);

  // Só pede para copiar quando config_rede.h ainda não tem o MAC desta placa.
  // Incondicional, a dica contradizia o próprio banner depois de `make macs`.
  if (!remetenteAutorizado(meuMac, MAC_RECEPTOR)) {
    Serial.println("  copie o MAC acima para MAC_RECEPTOR em lib/config_rede.h");
  }

  uint8_t canalAtual = 0;
  wifi_second_chan_t canalSecundario = WIFI_SECOND_CHAN_NONE;
  esp_wifi_get_channel(&canalAtual, &canalSecundario);
  wifi_ps_type_t modoPs = WIFI_PS_NONE;
  esp_wifi_get_ps(&modoPs);
  Serial.printf("  radio ..: canal=%u ps=%d  saidas=GPIO%d/GPIO%d\n", canalAtual,
                (int)modoPs, MODO0_PIN, MODO1_PIN);

  Serial.printf("  emissor aceito: %02X:%02X:%02X:%02X:%02X:%02X\n",
                MAC_EMISSOR[0], MAC_EMISSOR[1], MAC_EMISSOR[2], MAC_EMISSOR[3],
                MAC_EMISSOR[4], MAC_EMISSOR[5]);

  if (macEhPlaceholder(MAC_EMISSOR)) {
    Serial.println("  *** ATENCAO: MAC_EMISSOR ainda e o placeholder.     ***");
    Serial.println("  *** O filtro de remetente vai descartar tudo.       ***");
  }

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
  peerEmissor.channel = CANAL_ESPNOW;
  peerEmissor.encrypt = true;

  if (esp_now_add_peer(&peerEmissor) != ESP_OK) {
    Serial.println("falha ao adicionar peer emissor");
    return;
  }
  Serial.println("  criptografia ESP-NOW: ativa (AES-CCM)");

  xTaskCreate(tarefaModos,       "modos", 4096, NULL, 3, NULL);
  xTaskCreate(tarefaDiagnostico, "diag", 3072, NULL, 1, NULL);
}

void loop() {
  // Todo o trabalho está nas tasks. A loopTask fica dormindo.
  vTaskDelay(pdMS_TO_TICKS(1000));
}
