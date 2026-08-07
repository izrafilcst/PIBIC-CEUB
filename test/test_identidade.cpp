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

TESTE(mac_formatado_sai_em_maiusculas_com_dois_pontos) {
  const uint8_t mac[6] = {0x80, 0x7D, 0x3A, 0xF2, 0x83, 0x9C};
  char buf[TAM_MAC_TEXTO];

  formatarMac(buf, sizeof(buf), mac);

  VERIFICA(std::strcmp(buf, "80:7D:3A:F2:83:9C") == 0);
}

TESTE(mac_formatado_preenche_zero_a_esquerda) {
  const uint8_t mac[6] = {0x00, 0x0A, 0x0B, 0x00, 0x01, 0x0F};
  char buf[TAM_MAC_TEXTO];

  formatarMac(buf, sizeof(buf), mac);

  VERIFICA(std::strcmp(buf, "00:0A:0B:00:01:0F") == 0);
}

TESTE(tam_mac_texto_e_exatamente_o_necessario) {
  const uint8_t mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
  char buf[TAM_MAC_TEXTO];

  formatarMac(buf, sizeof(buf), mac);

  // 17 caracteres + NUL: se TAM_MAC_TEXTO encolher, o MAC sai truncado.
  VERIFICA_IGUAL(std::strlen(buf), (size_t)17);
}

TESTE(mac_formatado_nao_estoura_buffer_pequeno) {
  const uint8_t mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
  char buf[8];
  std::memset(buf, 'X', sizeof(buf));

  formatarMac(buf, sizeof(buf), mac);

  VERIFICA_IGUAL(buf[sizeof(buf) - 1], '\0');
}

TESTE(mac_nulo_nao_quebra_o_banner) {
  char buf[TAM_MAC_TEXTO];

  formatarMac(buf, sizeof(buf), nullptr);

  VERIFICA(!contem(buf, "0"));  // não finge um MAC zerado plausível
  VERIFICA(contem(buf, "?"));
}

TESTE(banner_aceita_o_mac_vindo_de_formatar_mac) {
  const uint8_t mac[6] = {0x14, 0x63, 0x93, 0xC3, 0x25, 0x04};
  char texto[TAM_MAC_TEXTO];
  char buf[256];

  formatarMac(texto, sizeof(texto), mac);
  montarBanner(buf, sizeof(buf), PAPEL_RECEPTOR, texto, 1);

  VERIFICA(contem(buf, "14:63:93:C3:25:04"));
  VERIFICA(!contem(buf, "00:00:00:00:00:00"));
}

TESTE(nome_do_papel_e_estavel) {
  VERIFICA(std::strcmp(nomeDoPapel(PAPEL_EMISSOR), "EMISSOR") == 0);
  VERIFICA(std::strcmp(nomeDoPapel(PAPEL_RECEPTOR), "RECEPTOR") == 0);
}
