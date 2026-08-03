// Identificação do papel de cada placa. Puro, sem Arduino: o sketch só
// entrega o texto pronto para o Serial.
#pragma once

#include <stddef.h>
#include <stdint.h>

enum Papel {
  PAPEL_EMISSOR  = 0,
  PAPEL_RECEPTOR = 1
};

const char *nomeDoPapel(Papel p);

// true quando o MAC de destino claramente não foi configurado: o placeholder
// do repositório, tudo zero ou broadcast. Nenhum deles é um unicast válido.
bool macEhPlaceholder(const uint8_t *mac);

// Escreve o banner de boot em `saida`, sempre terminando em NUL.
// Devolve quantos bytes seriam necessários (como snprintf).
int montarBanner(char *saida, size_t tamanho, Papel papel, const char *mac,
                 uint32_t bootId);
