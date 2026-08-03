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

## Comandos

```sh
make test        # 42 testes de host, sem precisar de placa
make chaves      # gera lib/chaves.h (PMK/LMK aleatórias, não versionado)
make macs        # lê o MAC das duas placas via esptool -> lib/config_rede.h
make gravar      # compila e grava as duas
make smoke       # grava, reinicia e verifica a comunicação ponta a ponta
make sketches    # recria os symlinks de lib/ em emissor/ e receptor/
make limpar
```

As portas padrão são `/dev/ttyACM0` (emissor) e `/dev/ttyACM1` (receptor);
sobrescreva com `make macs PORTA_EMISSOR=... PORTA_RECEPTOR=...`.

### Ordem para subir do zero

```sh
make chaves                     # 1. material de chave
make macs                       # 2. descobre e grava os MACs
make gravar                     # 3. grava as duas placas
make smoke                      # 4. confirma que conversam
```

## Compilar à mão

```sh
arduino-cli compile --fqbn esp32:esp32:esp32c3 --board-options CDCOnBoot=cdc emissor
```

Verificado no core `esp32:esp32` 3.0.7. Dois detalhes que mordem:

- **`CDCOnBoot=cdc` é obrigatório.** O padrão do C3 é `cdc_on_boot=0`, que manda
  o `Serial` para a UART0 em vez da USB nativa — sem essa opção você não vê nada
  em `/dev/ttyACM*`.
- Os callbacks do ESP-NOW mudaram de assinatura nos cores 3.0.0 e 3.2.0; os
  sketches têm guardas de versão para compilar nas três gerações.

## Criptografia

Usa a criptografia **nativa do ESP-NOW** (AES-CCM), que não é ligada por padrão:
o código anterior tinha `encrypt = false` e trafegava em claro.

- **PMK** (16 bytes) cifra a LMK durante o handshake; **LMK** (16 bytes) cifra o
  tráfego do par. Ambas em `lib/chaves.h`, **fora do git** — o versionado é
  `lib/chaves.exemplo.h`, e o firmware detecta as chaves de exemplo e grita no
  boot.
- As **duas** placas registram a outra como peer cifrado. O receptor precisa
  disso para conseguir decifrar, e por isso agora ele também precisa saber o MAC
  do emissor.
- A criptografia do ESP-NOW só cobre peers registrados: um quadro vindo de um MAC
  desconhecido **ainda chega ao callback, em claro**. Por isso há um filtro de
  remetente por MAC, em tempo constante, antes de qualquer processamento.

Isso protege contra bisbilhotar e contra um vizinho acender seus LEDs. **Não**
protege contra alguém com acesso físico à placa: as chaves estão na flash sem
Flash Encryption. Fechar isso exigiria migrar o build para ESP-IDF.

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

- **`lib/config_rede.h` ainda tem os dois MACs como placeholder.** `make macs`
  resolve, mas exige acesso às portas seriais (veja abaixo). Sem isso nada
  funciona em hardware — os dois sketches avisam no boot.
- **Nada foi rodado em placa.** Toda a verificação até aqui é teste de host e
  compilação. `make smoke` existe mas nunca chegou a executar.
- Acesso serial: as placas ficam em `/dev/ttyACM*` com dono `root:dialout`, e o
  grupo `dialout` está vazio. Precisa de `sudo usermod -aG dialout $USER` e
  logout/login.
- Os sketches originais mencionavam um `CLAUDE.md` que nunca existiu neste
  repositório — a justificativa de usar o L7805CV em vez de deep sleep não está
  documentada em lugar nenhum. Vale registrar antes de esquecer.
- `esquematico.html` tem correções pendentes apontadas em revisão: falta
  `<meta name="viewport">`, o símbolo das chaves desenha os dois fios no mesmo
  ponto (lê como curto) e o GPIO11 não está na lista de pinos a evitar.
