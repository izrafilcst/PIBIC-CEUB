#include "debounce.h"

void canalIniciar(CanalBotao *c) {
  c->estadoAtual     = 0;
  c->seq             = 0;
  c->leituraAnterior = NIVEL_ALTO;
  c->estadoEstavel   = NIVEL_ALTO;
  c->ultimaMudanca   = 0;
}

bool canalAtualizar(CanalBotao *c, int leitura, uint32_t agora,
                    uint32_t debounceMs) {
  // Toda transição crua reinicia o cronômetro do debounce.
  if (leitura != c->leituraAnterior) {
    c->leituraAnterior = leitura;
    c->ultimaMudanca   = agora;
    return false;
  }

  // Leitura estável tempo suficiente e diferente do que já estava valendo:
  // só aqui existe uma borda confirmada.
  if ((agora - c->ultimaMudanca) <= debounceMs) return false;
  if (leitura == c->estadoEstavel) return false;

  c->estadoEstavel = leitura;

  // Borda de descida: com INPUT_PULLUP, pressionado = NIVEL_BAIXO.
  if (leitura != NIVEL_BAIXO) return false;

  c->estadoAtual = !c->estadoAtual;
  c->seq++;
  return true;
}
