// MACs das duas placas. Não são segredo — podem ser versionados.
//
// GERADO por `make macs`. Para editar à mão, use o MAC que cada sketch imprime
// no banner de boot.
//
// Tanto no ESP32 clássico quanto no C3 o MAC base da efuse é igual ao MAC da
// interface STA, que é a que o ESP-NOW usa aqui.
#pragma once

#include <stdint.h>

static const uint8_t MAC_EMISSOR[6]  = {0x80, 0x7D, 0x3A, 0xF2, 0x83, 0x9C};
static const uint8_t MAC_RECEPTOR[6] = {0x14, 0x63, 0x93, 0xC3, 0x25, 0x04};

// Canal fixo do ESP-NOW.
#define CANAL_ESPNOW 1
