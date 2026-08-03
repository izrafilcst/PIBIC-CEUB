#include "framework.h"
#include "seguranca.h"

static const uint8_t MAC_EMISSOR[6]  = {0x34, 0x85, 0x18, 0x01, 0x02, 0x03};
static const uint8_t MAC_INTRUSO[6]  = {0x34, 0x85, 0x18, 0x01, 0x02, 0x04};

TESTE(remetente_esperado_e_aceito) {
  VERIFICA(remetenteAutorizado(MAC_EMISSOR, MAC_EMISSOR));
}

TESTE(remetente_diferente_e_rejeitado) {
  VERIFICA(!remetenteAutorizado(MAC_INTRUSO, MAC_EMISSOR));
}

TESTE(remetente_que_difere_so_no_ultimo_byte_e_rejeitado) {
  const uint8_t quase[6] = {0x34, 0x85, 0x18, 0x01, 0x02, 0x02};
  VERIFICA(!remetenteAutorizado(quase, MAC_EMISSOR));
}

TESTE(remetente_nulo_e_rejeitado) {
  VERIFICA(!remetenteAutorizado(NULL, MAC_EMISSOR));
  VERIFICA(!remetenteAutorizado(MAC_EMISSOR, NULL));
}

TESTE(chave_toda_zero_e_insegura) {
  const uint8_t zerada[16] = {0};
  VERIFICA(chaveEhInsegura(zerada));
}

TESTE(chave_toda_ff_e_insegura) {
  uint8_t ff[16];
  for (int i = 0; i < 16; i++) ff[i] = 0xFF;
  VERIFICA(chaveEhInsegura(ff));
}

TESTE(chave_de_exemplo_do_repositorio_e_insegura) {
  // a mesma que vai em chaves.exemplo.h: se alguém esquecer de trocar,
  // o firmware precisa gritar no boot
  const uint8_t exemplo[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                               0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
  VERIFICA(chaveEhInsegura(exemplo));
}

TESTE(chave_sorteada_de_verdade_e_aceita) {
  const uint8_t boa[16] = {0x9C, 0x41, 0xE7, 0x2A, 0x0D, 0xB8, 0x53, 0xF6,
                           0x17, 0xCA, 0x38, 0x90, 0x6E, 0x2B, 0xD4, 0x75};
  VERIFICA(!chaveEhInsegura(boa));
}
