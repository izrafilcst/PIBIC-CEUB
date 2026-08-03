# Dino — Emissor / Receptor ESP32-C3 via ESP-NOW

Dois botões num ESP32-C3 controlam dois LEDs em outro ESP32-C3, por ESP-NOW.

## Estrutura

```
lib/          lógica pura, sem Arduino — é aqui que mora o comportamento
test/         testes de host (g++), rodam sem hardware
emissor/      sketch do emissor (FreeRTOS) + links para lib/
receptor/     sketch do receptor (FreeRTOS) + links para lib/
legado/       versão original dos sketches, antes da reestruturação
esquematico.html  diagrama de ligação
```

`lib/` é a única fonte da verdade. As pastas de sketch contêm symlinks porque a
IDE do Arduino só compila arquivos que estejam dentro da pasta do sketch.

## Rodar os testes

```sh
make test        # 25 testes, sem precisar de placa
make sketches    # recria os symlinks em emissor/ e receptor/
make limpar
```

## Compilar

```sh
arduino-cli compile --fqbn esp32:esp32:esp32c3 emissor
arduino-cli compile --fqbn esp32:esp32:esp32c3 receptor
```

Verificado no core `esp32:esp32` 3.0.7. Os callbacks do ESP-NOW mudaram de
assinatura nos cores 3.0.0 e 3.2.0; os sketches têm guardas de versão para
compilar nas três gerações.

## Arquitetura FreeRTOS

**Emissor** — `tarefaBotoes` (prio 3) amostra os pinos a cada 5 ms com
`vTaskDelayUntil`, roda o debounce e publica o evento em `filaEventos`.
`tarefaEnvio` (prio 2) consome a fila e faz a rajada de 3 reenvios; o
`vTaskDelay` entre pacotes cede a CPU, então a leitura dos botões continua
rodando durante a rajada. `tarefaDiagnostico` (prio 1) consome o status dos
callbacks e relata a cada 10 s.

**Receptor** — o callback do ESP-NOW roda no contexto da task de WiFi e apenas
copia o quadro para `filaQuadros`. `tarefaLeds` (prio 3) consome a fila, decide
e aciona os LEDs fora do caminho crítico do rádio.

## Protocolo

```c
typedef struct __attribute__((packed)) {
  uint32_t bootId;  // sorteado a cada boot do emissor
  uint8_t  botao;
  uint8_t  estado;  // estado absoluto, não "alternar"
  uint16_t seq;     // por canal
} pacote_t;
```

O emissor manda o **estado absoluto**, não um "toggle" — perder um pacote não
dessincroniza os LEDs. A rajada de 3 cobre perda; o receptor deduplica por
`seq`. O `bootId` existe porque, sem ele, um reboot do emissor faz a sequência
voltar ao começo e o receptor descartaria os N cliques seguintes.

## Pendências conhecidas

- `receptorMAC` em `emissor/emissor.ino` ainda é o placeholder
  `AA:BB:CC:DD:EE:FF`. Suba o receptor, anote o MAC que ele imprime no boot e
  preencha ali.
- O receptor aceita pacotes de qualquer remetente (`encrypt = false`, sem filtro
  de MAC). Suficiente para bancada.
- Os sketches originais mencionavam um `CLAUDE.md` que nunca existiu neste
  repositório — a justificativa de usar o L7805CV em vez de deep sleep não está
  documentada em lugar nenhum. Vale registrar antes de esquecer.
- `esquematico.html` tem correções pendentes apontadas em revisão: falta
  `<meta name="viewport">`, o símbolo das chaves desenha os dois fios no mesmo
  ponto (lê como curto) e o GPIO11 não está na lista de pinos a evitar.
