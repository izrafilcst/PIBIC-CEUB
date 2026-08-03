// Decisão de aceitar/descartar um pacote recebido, pura: não chama
// digitalWrite nem depende do ESP-NOW.
#pragma once

#include <stdint.h>

#include "protocolo.h"

struct EstadoReceptor {
  uint16_t ultimaSeq[NUM_CANAIS];
  bool     primeiraVez[NUM_CANAIS];
  uint32_t bootIdConhecido;  // do emissor; se mudar, a sequência recomeça
  bool     temBootId;
};

struct Decisao {
  bool    aceitar;   // false = pacote descartado, não mexer no LED
  uint8_t canal;     // índice do LED, válido só quando aceitar
  uint8_t nivelLed;  // 1 = aceso, 0 = apagado
};

void receptorIniciar(EstadoReceptor *e);

Decisao receptorProcessar(EstadoReceptor *e, const uint8_t *dados, int tamanho);
