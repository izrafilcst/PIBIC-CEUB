// MACs das duas placas. Não são segredo — podem ser versionados.
//
// Preencher com `make macs` (lê as duas placas via esptool) ou à mão, a partir
// do MAC que cada sketch imprime no banner de boot.
//
// No ESP32-C3 o MAC base da efuse é igual ao MAC da interface STA, que é a que
// o ESP-NOW usa aqui — o valor que o esptool reporta entra direto.
#pragma once

#include <stdint.h>

static const uint8_t MAC_EMISSOR[6]  = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
static const uint8_t MAC_RECEPTOR[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
