// Contrato de dados entre Emissor e Receptor. Sem dependência de Arduino:
// este header é compilado tanto pelos sketches quanto pelos testes de host.
#pragma once

#include <stdint.h>

#define NUM_CANAIS 2

typedef struct __attribute__((packed)) {
  uint32_t bootId;  // sorteado a cada boot do emissor, invalida seq antiga
  uint8_t  botao;   // 0 ou 1
  uint8_t  estado;  // 0 = apagado, 1 = aceso
  uint16_t seq;     // sequência por canal, detecta repetição da rajada
} pacote_t;
