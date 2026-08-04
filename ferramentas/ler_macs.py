#!/usr/bin/env python3
"""Lê o MAC das duas placas via esptool e escreve lib/config_rede.h.

No ESP32-C3 o MAC base da efuse é igual ao MAC da interface STA, que é a que o
ESP-NOW usa aqui — o valor lido entra direto na configuração.
"""
import argparse
import pathlib
import re
import subprocess
import sys

RAIZ = pathlib.Path(__file__).resolve().parent.parent
DESTINO = RAIZ / "lib" / "config_rede.h"
ESPTOOL = pathlib.Path.home() / (
    ".arduino15/packages/esp32/tools/esptool_py/4.6/esptool.py")

MODELO = """// MACs das duas placas. Não são segredo — podem ser versionados.
//
// GERADO por `make macs`. Para editar à mão, use o MAC que cada sketch imprime
// no banner de boot.
//
// Tanto no ESP32 clássico quanto no C3 o MAC base da efuse é igual ao MAC da
// interface STA, que é a que o ESP-NOW usa aqui.
#pragma once

#include <stdint.h>

static const uint8_t MAC_EMISSOR[6]  = {{{emissor}}};
static const uint8_t MAC_RECEPTOR[6] = {{{receptor}}};

// Canal fixo do ESP-NOW.
#define CANAL_ESPNOW 1
"""


def ler_mac(porta: str, chip: str) -> str:
    if not ESPTOOL.exists():
        raise SystemExit(f"esptool não encontrado em {ESPTOOL}")

    saida = subprocess.run(
        [sys.executable, str(ESPTOOL), "--chip", chip, "--port", porta,
         "read_mac"],
        capture_output=True, text=True, timeout=120)

    achados = re.findall(r"MAC:\s*((?:[0-9a-fA-F]{2}:){5}[0-9a-fA-F]{2})",
                         saida.stdout)
    if not achados:
        print(saida.stdout, file=sys.stderr)
        print(saida.stderr, file=sys.stderr)
        raise SystemExit(
            f"não consegui ler o MAC de {porta}. Se a mensagem for sobre a "
            f"porta não existir, quase sempre é permissão: veja se você está "
            f"no grupo dialout.")
    return achados[0]


def como_array(mac: str) -> str:
    return ", ".join(f"0x{parte.upper()}" for parte in mac.split(":"))


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--emissor", default="/dev/ttyUSB0")
    p.add_argument("--chip-emissor", default="esp32")
    p.add_argument("--receptor", default="/dev/ttyACM0")
    p.add_argument("--chip-receptor", default="esp32c3")
    args = p.parse_args()

    if args.emissor == args.receptor:
        raise SystemExit("as duas portas não podem ser a mesma")

    mac_e = ler_mac(args.emissor, args.chip_emissor)
    mac_r = ler_mac(args.receptor, args.chip_receptor)

    if mac_e == mac_r:
        raise SystemExit("as duas portas devolveram o mesmo MAC — "
                         "provavelmente é a mesma placa")

    print(f"EMISSOR  ({args.emissor}): {mac_e}")
    print(f"RECEPTOR ({args.receptor}): {mac_r}")

    DESTINO.write_text(MODELO.format(emissor=como_array(mac_e),
                                     receptor=como_array(mac_r)))
    print(f"\nescrito em {DESTINO}")
    print("A associação porta->papel é a que você passou; o banner de boot "
          "confirma depois de gravar.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
