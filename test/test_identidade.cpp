#include <cstring>

#include "framework.h"
#include "identidade.h"

static bool contem(const char *texto, const char *trecho) {
  return std::strstr(texto, trecho) != nullptr;
}

TESTE(banner_do_emissor_diz_que_e_o_emissor) {
  char buf[256];
  montarBanner(buf, sizeof(buf), PAPEL_EMISSOR, "A1:B2:C3:D4:E5:F6", 0x1A2B3C4Du);

  VERIFICA(contem(buf, "EMISSOR"));
  VERIFICA(!contem(buf, "RECEPTOR"));
}

TESTE(banner_do_receptor_diz_que_e_o_receptor) {
  char buf[256];
  montarBanner(buf, sizeof(buf), PAPEL_RECEPTOR, "A1:B2:C3:D4:E5:F6", 1);

  VERIFICA(contem(buf, "RECEPTOR"));
}

TESTE(banner_mostra_o_mac_e_o_bootid) {
  char buf[256];
  montarBanner(buf, sizeof(buf), PAPEL_EMISSOR, "A1:B2:C3:D4:E5:F6", 0x1A2B3C4Du);

  VERIFICA(contem(buf, "A1:B2:C3:D4:E5:F6"));
  VERIFICA(contem(buf, "1A2B3C4D"));
}

TESTE(banner_nao_estoura_buffer_pequeno) {
  char buf[16];
  std::memset(buf, 'X', sizeof(buf));

  montarBanner(buf, sizeof(buf), PAPEL_EMISSOR, "A1:B2:C3:D4:E5:F6", 1);

  VERIFICA_IGUAL(buf[sizeof(buf) - 1], '\0');  // sempre termina em NUL
}

TESTE(mac_placeholder_e_detectado) {
  const uint8_t placeholder[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
  VERIFICA(macEhPlaceholder(placeholder));
}

TESTE(mac_real_nao_e_confundido_com_placeholder) {
  const uint8_t real[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFE};
  VERIFICA(!macEhPlaceholder(real));
}

TESTE(mac_zerado_tambem_e_invalido) {
  const uint8_t zerado[6] = {0, 0, 0, 0, 0, 0};
  VERIFICA(macEhPlaceholder(zerado));
}

TESTE(mac_broadcast_tambem_e_invalido) {
  const uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  VERIFICA(macEhPlaceholder(broadcast));
}

TESTE(nome_do_papel_e_estavel) {
  VERIFICA(std::strcmp(nomeDoPapel(PAPEL_EMISSOR), "EMISSOR") == 0);
  VERIFICA(std::strcmp(nomeDoPapel(PAPEL_RECEPTOR), "RECEPTOR") == 0);
}
