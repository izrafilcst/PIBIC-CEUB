// Verificações de segurança puras, sem Arduino: filtro de remetente e
// sanidade do material de chave.
#pragma once

#include <stddef.h>
#include <stdint.h>

#define TAM_MAC   6
#define TAM_CHAVE 16  // PMK e LMK do ESP-NOW têm 16 bytes cada

// A criptografia do ESP-NOW só protege quadros de peers registrados; quadros
// de um MAC desconhecido ainda chegam ao callback em claro. Este filtro fecha
// essa porta.
bool remetenteAutorizado(const uint8_t *remetente, const uint8_t *esperado);

// true quando a chave claramente não foi trocada: tudo zero, tudo 0xFF ou a
// chave de exemplo versionada no repositório.
bool chaveEhInsegura(const uint8_t *chave);
