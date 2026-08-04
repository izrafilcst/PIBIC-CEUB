#!/usr/bin/env python3
"""Smoke test de comunicação entre emissor e receptor.

Grava as duas placas, reinicia, e verifica pelo serial que:
  - cada placa se identifica com o papel certo;
  - a criptografia do ESP-NOW subiu nas duas;
  - nenhum aviso de configuração pendente apareceu;
  - um evento sai do emissor e chega ao receptor, decifrado.

O emissor é gravado com -DDINO_SMOKETEST, que injeta eventos sintéticos: assim
o teste não depende de alguém apertar o botão. No fim, o emissor é regravado
com o firmware normal (a menos que --manter-smoke seja passado).

O ESP32-C3 precisa de CDCOnBoot=cdc, senão o Serial sai pela UART0 e não pela
USB que aparece como /dev/ttyACM*.
"""
import argparse
import pathlib
import re
import subprocess
import sys
import time

try:
    import serial
except ImportError:
    raise SystemExit("pyserial não instalado: pip install pyserial")

RAIZ = pathlib.Path(__file__).resolve().parent.parent

VERDE, VERMELHO, NEUTRO = "\033[32m", "\033[31m", "\033[0m"
falhas = []


def verifica(condicao: bool, descricao: str) -> None:
    marca = f"{VERDE}ok   {NEUTRO}" if condicao else f"{VERMELHO}FALHA{NEUTRO}"
    print(f"  {marca} {descricao}")
    if not condicao:
        falhas.append(descricao)


def gravar(sketch: str, porta: str, fqbn: str, opcoes: list,
           smoke: bool = False) -> None:
    cmd = ["arduino-cli", "compile", "--fqbn", fqbn, *opcoes,
           "--upload", "--port", porta]
    if smoke:
        cmd += ["--build-property", "build.defines=-DDINO_SMOKETEST"]
    cmd.append(sketch)

    print(f"  gravando {sketch}{' (smoke)' if smoke else ''} em {porta}...")
    r = subprocess.run(cmd, cwd=RAIZ, capture_output=True, text=True,
                       timeout=900)
    if r.returncode != 0:
        print(r.stdout[-3000:], file=sys.stderr)
        print(r.stderr[-3000:], file=sys.stderr)
        raise SystemExit(f"falha ao gravar {sketch} em {porta}")


def reiniciar(ser: serial.Serial) -> None:
    """Pulso de reset pelas linhas de controle, como o esptool faz."""
    ser.setDTR(False)
    ser.setRTS(True)
    time.sleep(0.15)
    ser.setRTS(False)
    time.sleep(0.05)
    ser.reset_input_buffer()


def coletar(portas: dict, segundos: float) -> dict:
    seriais = {nome: serial.Serial(p, 115200, timeout=0.1)
               for nome, p in portas.items()}
    saidas = {nome: "" for nome in portas}
    try:
        for ser in seriais.values():
            reiniciar(ser)

        limite = time.time() + segundos
        while time.time() < limite:
            for nome, ser in seriais.items():
                dados = ser.read(4096)
                if dados:
                    saidas[nome] += dados.decode("utf-8", errors="replace")
            time.sleep(0.05)
    finally:
        for ser in seriais.values():
            ser.close()
    return saidas


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--emissor", default="/dev/ttyUSB0")
    p.add_argument("--fqbn-emissor", default="esp32:esp32:esp32")
    p.add_argument("--receptor", default="/dev/ttyACM0")
    p.add_argument("--fqbn-receptor", default="esp32:esp32:esp32c3")
    p.add_argument("--opcoes-receptor", default="--board-options CDCOnBoot=cdc")
    p.add_argument("--segundos", type=float, default=20.0)
    p.add_argument("--manter-smoke", action="store_true",
                   help="não regravar o firmware normal no fim")
    p.add_argument("--receptor-sem-tx", action="store_true",
                   help="receptor não consegue transmitir (TX quebrado): "
                        "não exige ACK, só a entrega em si")
    args = p.parse_args()
    opcoes_receptor = args.opcoes_receptor.split()

    print("== gravando ==")
    gravar("receptor", args.receptor, args.fqbn_receptor, opcoes_receptor)
    gravar("emissor", args.emissor, args.fqbn_emissor, [], smoke=True)

    print(f"\n== coletando serial por {args.segundos:.0f}s ==")
    saidas = coletar({"emissor": args.emissor, "receptor": args.receptor},
                     args.segundos)
    emi, rec = saidas["emissor"], saidas["receptor"]

    print("\n== identidade ==")
    verifica("esta placa e o EMISSOR" in emi, "emissor se identifica")
    verifica("esta placa e o RECEPTOR" in rec, "receptor se identifica")
    verifica("esta placa e o RECEPTOR" not in emi, "emissor não se diz receptor")

    print("\n== criptografia ==")
    verifica("criptografia ESP-NOW: ativa" in emi, "emissor cifrando")
    verifica("criptografia ESP-NOW: ativa" in rec, "receptor decifrando")

    print("\n== configuração ==")
    verifica("*** ATENCAO" not in emi, "emissor sem avisos de configuração")
    verifica("*** ATENCAO" not in rec.replace(
        "*** BUILD DE SMOKE TEST", ""), "receptor sem avisos de configuração")

    print("\n== comunicação ponta a ponta ==")
    verifica("[smoke] press sintetico" in emi, "emissor gerou eventos")
    verifica("botao=" in emi, "emissor transmitiu")
    verifica("led0 -> aceso" in rec, "receptor acendeu o LED0")
    verifica("led1 -> aceso" in rec, "receptor acendeu o LED1")
    # 4 eventos sintéticos x 3 reenvios da rajada = 12 quadros unicast. Em
    # hardware saudável, todos voltam com ACK. Conferir o último relatório em
    # vez de só procurar texto.
    relatos = re.findall(r"confirmados=(\d+) sem_confirmacao=(\d+)", emi)
    confirmados, sem_ack = (int(relatos[-1][0]), int(relatos[-1][1])) if relatos \
        else (0, -1)
    if args.receptor_sem_tx:
        # O ACK sai do receptor: sem TX nele, nunca chega. A entrega em si
        # (led0/led1 acesos, checado acima) é o que importa aqui.
        print(f"  (info) confirmados={confirmados} sem_confirmacao={sem_ack} "
              f"-- ACK ignorado, receptor sem TX")
    else:
        verifica(confirmados >= 12, f"12 quadros confirmados por ACK (obtido {confirmados})")
        verifica(sem_ack == 0, f"nenhum quadro sem ACK (obtido {sem_ack})")

    if falhas:
        print(f"\n{VERMELHO}--- serial do emissor ---{NEUTRO}\n{emi[-2000:]}")
        print(f"\n{VERMELHO}--- serial do receptor ---{NEUTRO}\n{rec[-2000:]}")

    if not args.manter_smoke:
        print("\n== restaurando firmware normal do emissor ==")
        gravar("emissor", args.emissor, args.fqbn_emissor, [], smoke=False)

    print(f"\n{len(falhas)} falha(s)")
    return 1 if falhas else 0


if __name__ == "__main__":
    raise SystemExit(main())
