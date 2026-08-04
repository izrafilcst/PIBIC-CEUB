// MACs das duas placas. Não são segredo — podem ser versionados.
//
// GERADO por `make macs`. Para editar à mão, use o MAC que cada sketch imprime
// no banner de boot.
//
// Tanto no ESP32 clássico quanto no C3 o MAC base da efuse é igual ao MAC da
// interface STA, que é a que o ESP-NOW usa aqui — o valor que o esptool reporta
// entra direto.
#pragma once

#include <stdint.h>

// Placeholders: o emissor foi trocado por um DevKit clássico, então os dois
// MACs precisam ser lidos de novo. Rode `make macs`. Enquanto estiverem assim,
// os dois sketches avisam no boot.
static const uint8_t MAC_EMISSOR[6]  = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
static const uint8_t MAC_RECEPTOR[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

// Canal fixo do ESP-NOW. Deixar no "canal atual" (channel = 0) não garante que
// os dois rádios fiquem no mesmo canal quando nenhum está associado a um AP.
#define CANAL_ESPNOW 1
