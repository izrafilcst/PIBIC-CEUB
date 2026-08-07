#include "framework.h"
#include "receptor_logic.h"

static const uint32_t BOOT_PADRAO = 0xA1B2C3D4u;

static Decisao entregarDeBoot(EstadoReceptor *e, uint32_t bootId, uint8_t botao,
                              uint8_t estado, uint16_t seq) {
  pacote_t p;
  p.bootId = bootId;
  p.botao  = botao;
  p.estado = estado;
  p.seq    = seq;
  return receptorProcessar(e, (const uint8_t *)&p, (int)sizeof(p));
}

static Decisao entregar(EstadoReceptor *e, uint8_t botao, uint8_t estado,
                        uint16_t seq) {
  return entregarDeBoot(e, BOOT_PADRAO, botao, estado, seq);
}

TESTE(primeiro_pacote_do_canal_e_sempre_aceito) {
  EstadoReceptor e;
  receptorIniciar(&e);

  Decisao d = entregar(&e, 0, 1, 7);  // seq alta, mas é o primeiro

  VERIFICA(d.aceitar);
  VERIFICA_IGUAL(d.canal, 0);
  VERIFICA_IGUAL(d.nivelModo, 1);
}

TESTE(pacote_repetido_da_rajada_e_ignorado) {
  EstadoReceptor e;
  receptorIniciar(&e);

  entregar(&e, 0, 1, 5);
  Decisao repetido = entregar(&e, 0, 1, 5);

  VERIFICA(!repetido.aceitar);
}

TESTE(pacote_atrasado_e_ignorado) {
  EstadoReceptor e;
  receptorIniciar(&e);

  entregar(&e, 0, 1, 10);
  Decisao atrasado = entregar(&e, 0, 0, 9);

  VERIFICA(!atrasado.aceitar);
}

TESTE(sequencia_avancando_e_aceita) {
  EstadoReceptor e;
  receptorIniciar(&e);

  entregar(&e, 0, 1, 10);
  Decisao nova = entregar(&e, 0, 0, 11);

  VERIFICA(nova.aceitar);
  VERIFICA_IGUAL(nova.nivelModo, 0);
}

TESTE(wraparound_da_sequencia_e_aceito) {
  EstadoReceptor e;
  receptorIniciar(&e);

  entregar(&e, 0, 1, 65535);
  Decisao aposEstouro = entregar(&e, 0, 0, 0);

  VERIFICA(aposEstouro.aceitar);
}

TESTE(canal_invalido_e_rejeitado) {
  EstadoReceptor e;
  receptorIniciar(&e);

  Decisao d = entregar(&e, 2, 1, 1);

  VERIFICA(!d.aceitar);
}

TESTE(pacote_de_tamanho_errado_e_rejeitado) {
  EstadoReceptor e;
  receptorIniciar(&e);

  uint8_t lixo[2] = {0, 1};
  Decisao d = receptorProcessar(&e, lixo, (int)sizeof(lixo));

  VERIFICA(!d.aceitar);
}

TESTE(estado_zero_desliga_o_modo) {
  EstadoReceptor e;
  receptorIniciar(&e);

  entregar(&e, 1, 1, 1);
  Decisao apagar = entregar(&e, 1, 0, 2);

  VERIFICA(apagar.aceitar);
  VERIFICA_IGUAL(apagar.canal, 1);
  VERIFICA_IGUAL(apagar.nivelModo, 0);
}

TESTE(reboot_do_emissor_volta_a_ser_aceito) {
  EstadoReceptor e;
  receptorIniciar(&e);

  // emissor rodou até a sequência 100 e então reiniciou: seq volta pro começo
  entregar(&e, 0, 1, 100);
  Decisao aposReboot = entregarDeBoot(&e, 0x0BADCAFEu, 0, 1, 1);

  VERIFICA(aposReboot.aceitar);
}

TESTE(seq_atrasada_do_mesmo_boot_continua_sendo_ignorada) {
  EstadoReceptor e;
  receptorIniciar(&e);

  entregar(&e, 0, 1, 100);
  Decisao atrasado = entregar(&e, 0, 1, 99);

  VERIFICA(!atrasado.aceitar);
}

TESTE(reboot_reinicia_a_sequencia_dos_dois_canais) {
  EstadoReceptor e;
  receptorIniciar(&e);

  entregar(&e, 0, 1, 100);
  entregar(&e, 1, 1, 100);

  entregarDeBoot(&e, 0x0BADCAFEu, 0, 1, 1);
  Decisao canal1 = entregarDeBoot(&e, 0x0BADCAFEu, 1, 1, 1);

  VERIFICA(canal1.aceitar);
}

TESTE(canais_tem_sequencias_independentes) {
  EstadoReceptor e;
  receptorIniciar(&e);

  entregar(&e, 0, 1, 50);
  Decisao outroCanal = entregar(&e, 1, 1, 1);  // seq baixa, mas canal 1

  VERIFICA(outroCanal.aceitar);
  VERIFICA_IGUAL(outroCanal.canal, 1);
}
