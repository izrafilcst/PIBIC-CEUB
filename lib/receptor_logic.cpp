#include "receptor_logic.h"

#include <string.h>

void receptorIniciar(EstadoReceptor *e) {
  for (int i = 0; i < NUM_CANAIS; i++) {
    e->ultimaSeq[i]   = 0;
    e->primeiraVez[i] = true;
  }
  e->bootIdConhecido = 0;
  e->temBootId       = false;
}

static bool sequenciaMaisNova(uint16_t nova, uint16_t ultima) {
  // trata wraparound do uint16_t
  return (int16_t)(nova - ultima) > 0;
}

Decisao receptorProcessar(EstadoReceptor *e, const uint8_t *dados,
                          int tamanho) {
  Decisao d = {false, 0, 0};

  if (tamanho != (int)sizeof(pacote_t)) return d;

  pacote_t pacote;
  memcpy(&pacote, dados, sizeof(pacote));

  if (pacote.botao > 1) return d;  // canal inválido, ignora

  // Emissor reiniciou: a sequência dele voltou pro começo, então o histórico
  // antigo faria descartar os próximos N cliques. Zera tudo.
  if (!e->temBootId || pacote.bootId != e->bootIdConhecido) {
    e->bootIdConhecido = pacote.bootId;
    e->temBootId       = true;
    for (int i = 0; i < NUM_CANAIS; i++) {
      e->primeiraVez[i] = true;
    }
  }

  if (!e->primeiraVez[pacote.botao] &&
      !sequenciaMaisNova(pacote.seq, e->ultimaSeq[pacote.botao])) {
    return d;  // pacote repetido ou atrasado da rajada, ignora
  }

  e->primeiraVez[pacote.botao] = false;
  e->ultimaSeq[pacote.botao]   = pacote.seq;

  d.aceitar  = true;
  d.canal    = pacote.botao;
  d.nivelLed = pacote.estado ? 1 : 0;
  return d;
}
