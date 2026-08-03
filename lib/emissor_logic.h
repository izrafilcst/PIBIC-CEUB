// Montagem do pacote a partir do estado de um canal. Puro, sem Arduino.
#pragma once

#include <stdint.h>

#include "debounce.h"
#include "protocolo.h"

pacote_t montarPacote(uint32_t bootId, uint8_t canal, const CanalBotao *c);
