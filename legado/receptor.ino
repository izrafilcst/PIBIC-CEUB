// RECEPTOR — teste de 2 LEDs independentes via ESP-NOW
// Sem sleep nesta fase: ver CLAUDE.md sobre o L7805CV anular o ganho do deep sleep.
// Rode uma vez só com Serial.println(WiFi.macAddress()) para descobrir este MAC
// e colocar no receptorMAC do Emissor.

#include <esp_now.h>
#include <WiFi.h>

#define LED0_PIN 6
#define LED1_PIN 7

typedef struct {
  uint8_t  botao;
  uint8_t  estado;
  uint16_t seq;
} pacote_t;

uint8_t  ledPinos[2]   = {LED0_PIN, LED1_PIN};
uint16_t ultimaSeq[2]  = {0, 0};
bool     primeiraVez[2] = {true, true};

bool sequenciaMaisNova(uint16_t nova, uint16_t ultima) {
  // trata wraparound do uint16_t
  return (int16_t)(nova - ultima) > 0;
}

void onRecebimento(const esp_now_recv_info_t *info, const uint8_t *dados, int tamanho) {
  if (tamanho != sizeof(pacote_t)) return;

  pacote_t pacote;
  memcpy(&pacote, dados, sizeof(pacote));

  if (pacote.botao > 1) return; // canal inválido, ignora

  if (!primeiraVez[pacote.botao] && !sequenciaMaisNova(pacote.seq, ultimaSeq[pacote.botao])) {
    return; // pacote repetido ou atrasado da rajada, ignora
  }

  primeiraVez[pacote.botao] = false;
  ultimaSeq[pacote.botao] = pacote.seq;
  digitalWrite(ledPinos[pacote.botao], pacote.estado ? HIGH : LOW);

  Serial.printf("recebido: botao=%d estado=%d seq=%d\n", pacote.botao, pacote.estado, pacote.seq);
}

void setup() {
  Serial.begin(115200);

  pinMode(LED0_PIN, OUTPUT);
  pinMode(LED1_PIN, OUTPUT);
  digitalWrite(LED0_PIN, LOW);
  digitalWrite(LED1_PIN, LOW);

  WiFi.mode(WIFI_STA);
  Serial.print("MAC deste receptor: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("falha ao iniciar esp-now");
    return;
  }
  esp_now_register_recv_cb(onRecebimento);
}

void loop() {
  // nada aqui nesta fase, tudo reage no callback
}
