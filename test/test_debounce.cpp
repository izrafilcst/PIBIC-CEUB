#include "debounce.h"
#include "framework.h"

static const uint32_t DEBOUNCE_MS = 40;

// Alimenta a máquina com um nível fixo, 1 amostra por ms, e conta os eventos.
static int alimentar(CanalBotao *c, int nivel, uint32_t inicio,
                     uint32_t duracaoMs) {
  int eventos = 0;
  for (uint32_t i = 0; i < duracaoMs; i++) {
    if (canalAtualizar(c, nivel, inicio + i, DEBOUNCE_MS)) eventos++;
  }
  return eventos;
}

TESTE(press_estabilizado_gera_um_evento) {
  CanalBotao c;
  canalIniciar(&c);

  alimentar(&c, NIVEL_ALTO, 0, 100);            // botão solto
  int eventos = alimentar(&c, NIVEL_BAIXO, 100, 100);  // pressionado e segurado

  VERIFICA_IGUAL(eventos, 1);
}

TESTE(press_alterna_o_estado_do_canal) {
  CanalBotao c;
  canalIniciar(&c);
  VERIFICA_IGUAL(c.estadoAtual, 0);

  alimentar(&c, NIVEL_ALTO, 0, 100);
  alimentar(&c, NIVEL_BAIXO, 100, 100);
  VERIFICA_IGUAL(c.estadoAtual, 1);

  alimentar(&c, NIVEL_ALTO, 200, 100);
  alimentar(&c, NIVEL_BAIXO, 300, 100);
  VERIFICA_IGUAL(c.estadoAtual, 0);
}

TESTE(cada_press_incrementa_a_sequencia) {
  CanalBotao c;
  canalIniciar(&c);

  alimentar(&c, NIVEL_ALTO, 0, 100);
  alimentar(&c, NIVEL_BAIXO, 100, 100);
  VERIFICA_IGUAL(c.seq, 1);

  alimentar(&c, NIVEL_ALTO, 200, 100);
  alimentar(&c, NIVEL_BAIXO, 300, 100);
  VERIFICA_IGUAL(c.seq, 2);
}

TESTE(segurar_pressionado_gera_apenas_um_evento) {
  CanalBotao c;
  canalIniciar(&c);

  alimentar(&c, NIVEL_ALTO, 0, 100);
  int eventos = alimentar(&c, NIVEL_BAIXO, 100, 5000);  // segura 5 segundos

  VERIFICA_IGUAL(eventos, 1);
}

TESTE(soltar_o_botao_nao_gera_evento) {
  CanalBotao c;
  canalIniciar(&c);

  alimentar(&c, NIVEL_ALTO, 0, 100);
  alimentar(&c, NIVEL_BAIXO, 100, 100);
  int eventosAoSoltar = alimentar(&c, NIVEL_ALTO, 200, 100);

  VERIFICA_IGUAL(eventosAoSoltar, 0);
}

TESTE(ruido_mais_curto_que_o_debounce_nao_gera_evento) {
  CanalBotao c;
  canalIniciar(&c);

  alimentar(&c, NIVEL_ALTO, 0, 100);
  int eventos = alimentar(&c, NIVEL_BAIXO, 100, 10);  // 10 ms < 40 ms
  eventos += alimentar(&c, NIVEL_ALTO, 110, 100);

  VERIFICA_IGUAL(eventos, 0);
}

TESTE(bounce_no_aperto_gera_exatamente_um_evento) {
  CanalBotao c;
  canalIniciar(&c);
  uint32_t t = 0;

  alimentar(&c, NIVEL_ALTO, t, 100);
  t += 100;

  int eventos = 0;
  for (int i = 0; i < 6; i++) {  // 6 quiques de 5 ms cada
    eventos += alimentar(&c, NIVEL_BAIXO, t, 5);
    t += 5;
    eventos += alimentar(&c, NIVEL_ALTO, t, 5);
    t += 5;
  }
  eventos += alimentar(&c, NIVEL_BAIXO, t, 200);  // estabiliza pressionado

  VERIFICA_IGUAL(eventos, 1);
}

TESTE(wraparound_de_millis_nao_perde_o_evento) {
  CanalBotao c;
  canalIniciar(&c);

  const uint32_t antesDoEstouro = 0xFFFFFF00u;
  alimentar(&c, NIVEL_ALTO, antesDoEstouro, 100);

  // pressiona 100 ms antes do estouro e segura atravessando o zero
  int eventos = alimentar(&c, NIVEL_BAIXO, antesDoEstouro + 100, 300);

  VERIFICA_IGUAL(eventos, 1);
}
