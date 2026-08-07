# PIBIC-CEUB: Emissor / Receptor via ESP-NOW

Dois botões controlam dois LEDs em outra placa, por ESP-NOW.

| Papel | Placa | Serial | Pinos |
|---|---|---|---|
| Emissor | ESP32 DevKit clássico | `/dev/ttyUSB*` | botões em GPIO4/5 |
| Receptor | ESP32-C3 | `/dev/ttyACM*` | LEDs em GPIO6/7 |

As duas pontas são chips diferentes de propósito: o par de C3 original não
conseguia se comunicar (ver *Histórico* no fim). Os sketches escolhem os pinos
por `CONFIG_IDF_TARGET_*`, então o mesmo código compila nas duas famílias.

## Estrutura

```
lib/          lógica pura, sem Arduino, é aqui que mora o comportamento
test/         testes de host (g++), rodam sem hardware
emissor/      sketch do emissor (FreeRTOS) + links para lib/
receptor/     sketch do receptor (FreeRTOS) + links para lib/
ferramentas/  scripts de bancada (macs, chaves, smoke, teste de RF)
docs/         documentação do PIBIC e registros de ensaio
legado/       versão original dos sketches, antes da reestruturação
esquematico.html  diagrama de ligação (desatualizado: ver pendências)
```

`lib/` é a única fonte da verdade. As pastas de sketch contêm symlinks porque a
IDE do Arduino só compila arquivos que estejam dentro da pasta do sketch.

## Documentação

Documentos em padrão ABNT, um por camada. São HTML autocontidos com CSS de
impressão (margens 3/2/3/2 cm, Times 12, entrelinha 1,5): abra no navegador e
use *Imprimir → Salvar como PDF* para gerar a versão de entrega.

| Documento | Conteúdo |
|---|---|
| [`docs/hardware.html`](docs/hardware.html) | Esquemáticos em SVG vetorial (simbologia IEC), dimensionamento de `pull-up` e limitador, escolha de pinos, ensaios de RF e funcional |
| [`docs/software.html`](docs/software.html) | Arquitetura FreeRTOS, protocolo, criptografia, trechos comentados do código, verificação |
| [`docs/diagnostico-rf-esp32c3.md`](docs/diagnostico-rf-esp32c3.md) | Registro bruto da depuração do par de C3 |

A folha de rosto dos dois documentos tem lacunas `[preencher]` para autor e
orientador(a).

## Comandos

```sh
make test        # 42 testes de host, sem precisar de placa
make chaves      # gera lib/chaves.h (PMK/LMK aleatórias, não versionado)
make macs        # lê o MAC das duas placas via esptool -> lib/config_rede.h
make gravar      # compila e grava as duas
make smoke       # grava, reinicia e verifica a comunicação ponta a ponta
make teste-rf    # diagnóstico de RF puro (AP + scanner), sem ESP-NOW
make sketches    # recria os symlinks de lib/ em emissor/ e receptor/
make limpar
```

As portas padrão são `/dev/ttyUSB0` (emissor, DevKit clássico) e `/dev/ttyACM0`
(receptor, C3); sobrescreva com `make smoke PORTA_EMISSOR=... PORTA_RECEPTOR=...`.
FQBN, chip e opções de placa também são variáveis do Makefile, por papel.

### Ordem para subir do zero

```sh
make chaves                     # 1. material de chave
make macs                       # 2. descobre e grava os MACs
make gravar                     # 3. grava as duas placas
make smoke                      # 4. confirma que conversam
```

## Compilar à mão

```sh
arduino-cli compile --fqbn esp32:esp32:esp32 emissor
arduino-cli compile --fqbn esp32:esp32:esp32c3 --board-options CDCOnBoot=cdc receptor
```

Verificado no core `esp32:esp32` 3.0.7. Três detalhes que mordem:

- **`CDCOnBoot=cdc` é obrigatório no C3, e inválido no clássico.** O padrão do C3
  é `cdc_on_boot=0`, que manda o `Serial` para a UART0 em vez da USB nativa,
  então sem a opção você não vê nada em `/dev/ttyACM*`. Já o board clássico
  nem tem esse menu: passar a opção para ele é erro. Por isso o Makefile tem
  `OPCOES_EMISSOR` e `OPCOES_RECEPTOR` separados.
- **Canal fixo.** Sem associação a um AP, `channel = 0` ("canal atual") não
  garante que os dois rádios fiquem no mesmo canal; os sketches chamam
  `esp_wifi_set_channel(CANAL_ESPNOW, ...)` e usam esse canal no peer.
- Os callbacks do ESP-NOW mudaram de assinatura nos cores 3.0.0 e 3.2.0; os
  sketches têm guardas de versão para compilar nas três gerações.

## Criptografia

Usa a criptografia **nativa do ESP-NOW** (AES-CCM), que não é ligada por padrão:
o código anterior tinha `encrypt = false` e trafegava em claro.

- **PMK** (16 bytes) cifra a LMK durante o handshake; **LMK** (16 bytes) cifra o
  tráfego do par. Ambas em `lib/chaves.h`, **fora do git**. O versionado é
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

**Emissor.** `tarefaBotoes` (prio 3) amostra os pinos a cada 5 ms com
`vTaskDelayUntil`, roda o debounce e publica o evento em `filaEventos`.
`tarefaEnvio` (prio 2) consome a fila e faz a rajada de 3 reenvios; o
`vTaskDelay` entre pacotes cede a CPU, então a leitura dos botões continua
rodando durante a rajada. `tarefaDiagnostico` (prio 1) consome o status dos
callbacks e relata a cada 10 s.

**Receptor.** O callback do ESP-NOW roda no contexto da task de WiFi e apenas
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

O emissor manda o **estado absoluto**, não um "toggle", de modo que perder um
pacote não dessincroniza os LEDs. A rajada de 3 cobre perda; o receptor
deduplica por `seq`. O `bootId` existe porque, sem ele, um reboot do emissor
faz a sequência voltar ao começo e o receptor descartaria os N cliques
seguintes.

## Pendências conhecidas

- **O TX do ESP32-C3 receptor está inoperante.** Confirmado por teste de RF
  invertido: o AP subido pelo C3 fica invisível ao clássico a centímetros de
  distância, enquanto este lista 11 redes, uma delas a `-94 dBm`. Os quadros
  chegam e acendem os LEDs, mas o ACK nunca volta, então `make smoke` só passa
  limpo com `FLAGS_SMOKE=--receptor-sem-tx`. Sem correção em software: exige
  trocar a placa.
- A justificativa de usar o L7805CV em vez de deep sleep não está documentada
  em lugar nenhum. Vale registrar antes de esquecer. (Os sketches originais
  também mencionavam um `CLAUDE.md` que nunca existiu neste repositório.)
- A folha de rosto dos dois documentos ABNT tem lacunas `[preencher]` para
  autor e orientador(a).

## Histórico: por que as placas são diferentes

O projeto começou com dois ESP32-C3. Eles nunca conseguiram se comunicar. A
depuração eliminou, com evidência, todas as causas de software:

- criptografia desligada: mesma falha;
- power save desligado nos dois lados: mesma falha;
- canal fixado explicitamente nos dois lados: mesma falha;
- papéis trocados entre as placas: falha simétrica, então não era uma placa só.

O que fechou a questão foi um teste de RF puro, sem ESP-NOW (`make teste-rf`):
uma placa subiu um AP e a outra escaneou. O AP a centímetros de distância ficou
**invisível**, enquanto o roteador de um vizinho aparecia a `-34 dBm` e uma rede
distante a `-87 dBm`. RX excelente nas duas, TX que não sai em nenhuma. Com o
histórico de problemas conhecidos daquele par, o emissor foi trocado por um
DevKit clássico.

`make teste-rf` continua no repositório: é o primeiro comando a rodar se a
comunicação parar, porque separa problema de rádio de problema de software em
um minuto.
