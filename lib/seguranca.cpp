#include "seguranca.h"

bool remetenteAutorizado(const uint8_t *remetente, const uint8_t *esperado) {
  if (remetente == NULL || esperado == NULL) return false;

  // Comparação de tempo constante: não vaza em qual byte divergiu.
  uint8_t diferenca = 0;
  for (int i = 0; i < TAM_MAC; i++) {
    diferenca |= (uint8_t)(remetente[i] ^ esperado[i]);
  }
  return diferenca == 0;
}

bool chaveEhInsegura(const uint8_t *chave) {
  if (chave == NULL) return true;

  static const uint8_t exemplo[TAM_CHAVE] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                                             0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
                                             0x0C, 0x0D, 0x0E, 0x0F};
  bool tudoZero  = true;
  bool tudoUm    = true;
  bool ehExemplo = true;

  for (int i = 0; i < TAM_CHAVE; i++) {
    if (chave[i] != 0x00) tudoZero = false;
    if (chave[i] != 0xFF) tudoUm = false;
    if (chave[i] != exemplo[i]) ehExemplo = false;
  }

  return tudoZero || tudoUm || ehExemplo;
}
