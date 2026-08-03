// Emissor e receptor conversando através da camada pura: um press vira pacote,
// o pacote é repetido pela rajada e o receptor decide o nível do LED.
#include "debounce.h"
#include "emissor_logic.h"
#include "framework.h"
#include "receptor_logic.h"

static const uint32_t DEBOUNCE_MS = 40;
static const int      RAJADA_QTD  = 3;

namespace {

// Emissor + receptor ligados por um "rádio" que entrega tudo, em ordem.
struct Bancada {
  CanalBotao     canais[NUM_CANAIS];
  EstadoReceptor receptor;
  uint32_t       bootId;
  uint32_t       relogio;
  int            nivelLed[NUM_CANAIS];
  int            acionamentosDeLed;

  void ligar(uint32_t boot) {
    for (int i = 0; i < NUM_CANAIS; i++) {
      canalIniciar(&canais[i]);
      nivelLed[i] = 0;
    }
    receptorIniciar(&receptor);
    bootId            = boot;
    relogio           = 0;
    acionamentosDeLed = 0;
  }

  // Reinicia só o emissor; o receptor mantém o histórico de sequências.
  void reiniciarEmissor(uint32_t novoBoot) {
    for (int i = 0; i < NUM_CANAIS; i++) canalIniciar(&canais[i]);
    bootId = novoBoot;
  }

  void transmitir(const pacote_t &p) {
    for (int i = 0; i < RAJADA_QTD; i++) {  // rajada: o mesmo pacote N vezes
      Decisao d = receptorProcessar(&receptor, (const uint8_t *)&p,
                                    (int)sizeof(p));
      if (!d.aceitar) continue;
      nivelLed[d.canal] = d.nivelLed;
      acionamentosDeLed++;
    }
  }

  void alimentar(uint8_t canal, int nivel, uint32_t duracaoMs) {
    for (uint32_t i = 0; i < duracaoMs; i++) {
      if (canalAtualizar(&canais[canal], nivel, relogio + i, DEBOUNCE_MS)) {
        transmitir(montarPacote(bootId, canal, &canais[canal]));
      }
    }
    relogio += duracaoMs;
  }

  void clicar(uint8_t canal) {
    alimentar(canal, NIVEL_ALTO, 100);
    alimentar(canal, NIVEL_BAIXO, 100);
    alimentar(canal, NIVEL_ALTO, 100);
  }
};

}  // namespace

TESTE(um_clique_acende_o_led_correspondente) {
  Bancada b;
  b.ligar(0x11111111u);

  b.clicar(0);

  VERIFICA_IGUAL(b.nivelLed[0], 1);
  VERIFICA_IGUAL(b.nivelLed[1], 0);
}

TESTE(a_rajada_aciona_o_led_uma_unica_vez) {
  Bancada b;
  b.ligar(0x11111111u);

  b.clicar(0);

  VERIFICA_IGUAL(b.acionamentosDeLed, 1);  // 3 pacotes enviados, 1 aceito
}

TESTE(dois_cliques_acendem_e_apagam) {
  Bancada b;
  b.ligar(0x11111111u);

  b.clicar(0);
  b.clicar(0);

  VERIFICA_IGUAL(b.nivelLed[0], 0);
  VERIFICA_IGUAL(b.acionamentosDeLed, 2);
}

TESTE(os_dois_canais_nao_interferem) {
  Bancada b;
  b.ligar(0x11111111u);

  b.clicar(0);
  b.clicar(1);
  b.clicar(1);

  VERIFICA_IGUAL(b.nivelLed[0], 1);
  VERIFICA_IGUAL(b.nivelLed[1], 0);
}

TESTE(emissor_reiniciado_continua_controlando_o_led) {
  Bancada b;
  b.ligar(0x11111111u);

  for (int i = 0; i < 50; i++) b.clicar(0);  // sequência sobe até 50
  b.reiniciarEmissor(0x22222222u);           // seq volta pro começo

  const int antes = b.acionamentosDeLed;
  b.clicar(0);

  VERIFICA_IGUAL(b.acionamentosDeLed, antes + 1);
}
