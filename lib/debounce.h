// Máquina de estados de botão, pura: não chama digitalRead nem millis.
// A leitura e o tempo entram como parâmetro, então dá para testar no host.
#pragma once

#include <stdint.h>

// Níveis lógicos iguais aos do Arduino. Com INPUT_PULLUP:
// botão solto = NIVEL_ALTO, pressionado = NIVEL_BAIXO.
enum {
  NIVEL_BAIXO = 0,
  NIVEL_ALTO  = 1
};

struct CanalBotao {
  uint8_t  estadoAtual;      // estado lógico alternado a cada press
  uint16_t seq;              // incrementa a cada press aceito
  int      leituraAnterior;  // última leitura crua, alimenta o cronômetro
  int      estadoEstavel;    // leitura já filtrada, é dela que sai a borda
  uint32_t ultimaMudanca;    // millis() da última transição crua
};

void canalIniciar(CanalBotao *c);

// Alimenta a máquina com uma leitura. Devolve true quando um press válido
// (borda de descida já estabilizada) foi confirmado.
bool canalAtualizar(CanalBotao *c, int leitura, uint32_t agora,
                    uint32_t debounceMs);
