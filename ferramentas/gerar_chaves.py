#!/usr/bin/env python3
"""Gera lib/chaves.h com PMK e LMK aleatórias para a criptografia do ESP-NOW.

As duas placas precisam dos mesmos 32 bytes. Se regerar, regrave as duas.
O arquivo não é versionado (ver .gitignore).
"""
import os
import pathlib
import sys

RAIZ = pathlib.Path(__file__).resolve().parent.parent
DESTINO = RAIZ / "lib" / "chaves.h"

MODELO = """// GERADO POR `make chaves` -- NAO VERSIONADO (ver .gitignore)
// Chaves da criptografia nativa do ESP-NOW (AES-CCM). As DUAS placas precisam
// dos mesmos 32 bytes. Se regerar, regrave as duas.
#pragma once

#include <stdint.h>

// PMK: chave mestra, cifra a LMK durante o handshake.
static const uint8_t PMK[16] = {{{pmk}}};

// LMK: chave de sessao do par emissor<->receptor.
static const uint8_t LMK[16] = {{{lmk}}};
"""


def como_array(dados: bytes) -> str:
    return ", ".join(f"0x{b:02X}" for b in dados)


def main() -> int:
    if DESTINO.exists() and "--forcar" not in sys.argv:
        print(f"{DESTINO} já existe. Use --forcar para regerar "
              f"(exige regravar as DUAS placas).")
        return 0

    DESTINO.write_text(MODELO.format(pmk=como_array(os.urandom(16)),
                                     lmk=como_array(os.urandom(16))))
    print(f"chaves geradas em {DESTINO}")
    print("regrave as duas placas para que fiquem com o mesmo material.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
