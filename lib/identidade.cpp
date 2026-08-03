#include "identidade.h"

#include <inttypes.h>
#include <stdio.h>

const char *nomeDoPapel(Papel p) {
  return (p == PAPEL_EMISSOR) ? "EMISSOR" : "RECEPTOR";
}

bool macEhPlaceholder(const uint8_t *mac) {
  static const uint8_t placeholder[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

  bool ehPlaceholder = true;
  bool tudoZero      = true;
  bool tudoUm        = true;

  for (int i = 0; i < 6; i++) {
    if (mac[i] != placeholder[i]) ehPlaceholder = false;
    if (mac[i] != 0x00) tudoZero = false;
    if (mac[i] != 0xFF) tudoUm = false;
  }

  return ehPlaceholder || tudoZero || tudoUm;
}

int montarBanner(char *saida, size_t tamanho, Papel papel, const char *mac,
                 uint32_t bootId) {
  if (saida == NULL || tamanho == 0) return 0;

  const int n = snprintf(saida, tamanho,
                         "\n"
                         "========================================\n"
                         "  DINO -- esta placa e o %s\n"
                         "  MAC ....: %s\n"
                         "  bootId .: %08" PRIX32 "\n"
                         "========================================\n",
                         nomeDoPapel(papel), mac, bootId);

  saida[tamanho - 1] = '\0';  // snprintf já garante, mas deixa explícito
  return n;
}
