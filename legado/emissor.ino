// EMISSOR — teste de 2 botões via ESP-NOW
// Sem sleep nesta fase: ver CLAUDE.md sobre o L7805CV anular o ganho do deep sleep.
// Preencher receptorMAC com o endereço real do Receptor (usar WiFi.macAddress() nele para descobrir).

#include <esp_now.h>
#include <WiFi.h>

uint8_t receptorMAC[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}; // TODO: ajustar

#define BOTAO0_PIN 4
#define BOTAO1_PIN 5

#define DEBOUNCE_MS   40
#define RAJADA_QTD    3     // reenvios por evento, cobre perda de pacote
#define RAJADA_GAP_MS 15

typedef struct {
  uint8_t  botao;   // 0 ou 1
  uint8_t  estado;  // 0 = apagado, 1 = aceso
  uint16_t seq;
} pacote_t;

struct CanalBotao {
  uint8_t  pino;
  uint8_t  estadoAtual;
  uint16_t seq;
  int      leituraAnterior;
  unsigned long ultimaMudanca;
};

CanalBotao canais[2] = {
  {BOTAO0_PIN, 0, 0, HIGH, 0},
  {BOTAO1_PIN, 0, 0, HIGH, 0}
};

void onEnvioConcluido(const uint8_t *mac, esp_now_send_status_t status) {
  Serial.printf("envio: %s\n", status == ESP_NOW_SEND_SUCCESS ? "ok" : "falhou");
}

void enviarEstado(uint8_t idBotao) {
  pacote_t pacote;
  pacote.botao  = idBotao;
  pacote.estado = canais[idBotao].estadoAtual;
  pacote.seq    = canais[idBotao].seq;

  for (int i = 0; i < RAJADA_QTD; i++) {
    esp_now_send(receptorMAC, (uint8_t *)&pacote, sizeof(pacote));
    delay(RAJADA_GAP_MS);
  }

  Serial.printf("botao=%d estado=%d seq=%d\n", idBotao, pacote.estado, pacote.seq);
}

void lerBotoes() {
  unsigned long agora = millis();

  for (int i = 0; i < 2; i++) {
    int leitura = digitalRead(canais[i].pino);

    if (leitura != canais[i].leituraAnterior) {
      canais[i].ultimaMudanca = agora;
    }

    if ((agora - canais[i].ultimaMudanca) > DEBOUNCE_MS) {
      // borda de descida: botão em INPUT_PULLUP, pressionado = LOW
      if (leitura == LOW && canais[i].leituraAnterior == HIGH) {
        canais[i].estadoAtual = !canais[i].estadoAtual;
        canais[i].seq++;
        enviarEstado(i);
      }
    }

    canais[i].leituraAnterior = leitura;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(BOTAO0_PIN, INPUT_PULLUP);
  pinMode(BOTAO1_PIN, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);
  Serial.print("MAC deste emissor: ");
  Serial.println(WiFi.macAddress());

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
  }
}

void loop() {
  lerBotoes();
}
