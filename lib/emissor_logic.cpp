#include "emissor_logic.h"

pacote_t montarPacote(uint32_t bootId, uint8_t canal, const CanalBotao *c) {
  pacote_t p;
  p.bootId = bootId;
  p.botao  = canal;
  p.estado = c->estadoAtual;
  p.seq    = c->seq;
  return p;
}
