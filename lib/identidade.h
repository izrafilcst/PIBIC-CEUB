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

// Buffer suficiente para "AA:BB:CC:DD:EE:FF" mais o NUL.
static const size_t TAM_MAC_TEXTO = 18;

// Escreve o MAC como "AA:BB:CC:DD:EE:FF" em `saida`, sempre terminando em NUL.
// O sketch lê os bytes da efuse (esp_read_mac) em vez de WiFi.macAddress(),
// que no ESP32 clássico ainda devolve zeros no começo do setup().
void formatarMac(char *saida, size_t tamanho, const uint8_t *mac);

// Escreve o banner de boot em `saida`, sempre terminando em NUL.
// Devolve quantos bytes seriam necessários (como snprintf).
int montarBanner(char *saida, size_t tamanho, Papel papel, const char *mac,
                 uint32_t bootId);
