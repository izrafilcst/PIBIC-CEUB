CXX      ?= g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Werror -O1 -g -Ilib -Itest

FONTES := lib/debounce.cpp \
          lib/receptor_logic.cpp \
          lib/emissor_logic.cpp \
          lib/identidade.cpp \
          lib/seguranca.cpp \
          test/main.cpp \
          test/test_debounce.cpp \
          test/test_receptor.cpp \
          test/test_integracao.cpp \
          test/test_identidade.cpp \
          test/test_seguranca.cpp

CABECALHOS := lib/seguranca.h lib/identidade.h lib/debounce.h lib/receptor_logic.h lib/emissor_logic.h lib/protocolo.h test/framework.h

# As duas pontas são placas diferentes, então tudo aqui é por papel.
#
# EMISSOR  = ESP32 DevKit clássico. Serial pela ponte USB-UART (CP2102/CH340),
#            aparece como /dev/ttyUSB*. Não tem menu CDCOnBoot: passar essa
#            opção para ele dá erro.
# RECEPTOR = ESP32-C3. Serial pela USB nativa, aparece como /dev/ttyACM*, e
#            exige CDCOnBoot=cdc — sem isso o Serial sai pela UART0 e não
#            aparece nada na USB.
PORTA_EMISSOR   ?= /dev/ttyUSB0
PORTA_RECEPTOR  ?= /dev/ttyACM0
FQBN_EMISSOR    ?= esp32:esp32:esp32
FQBN_RECEPTOR   ?= esp32:esp32:esp32c3
CHIP_EMISSOR    ?= esp32
CHIP_RECEPTOR   ?= esp32c3
OPCOES_EMISSOR  ?=
OPCOES_RECEPTOR ?= --board-options CDCOnBoot=cdc
FLAGS_SMOKE     ?=

.PHONY: test sketches chaves macs gravar smoke limpar

chaves:
	@python3 ferramentas/gerar_chaves.py

macs:
	@python3 ferramentas/ler_macs.py \
	  --emissor $(PORTA_EMISSOR)   --chip-emissor $(CHIP_EMISSOR) \
	  --receptor $(PORTA_RECEPTOR) --chip-receptor $(CHIP_RECEPTOR)

gravar: sketches
	arduino-cli compile --fqbn $(FQBN_RECEPTOR) $(OPCOES_RECEPTOR) --upload --port $(PORTA_RECEPTOR) receptor
	arduino-cli compile --fqbn $(FQBN_EMISSOR)  $(OPCOES_EMISSOR)  --upload --port $(PORTA_EMISSOR)  emissor

smoke: sketches
	@python3 ferramentas/smoketest.py \
	  --emissor $(PORTA_EMISSOR)   --fqbn-emissor $(FQBN_EMISSOR) \
	  --receptor $(PORTA_RECEPTOR) --fqbn-receptor $(FQBN_RECEPTOR) \
	  --opcoes-receptor "$(OPCOES_RECEPTOR)" $(FLAGS_SMOKE)

# Diagnóstico de RF puro, sem ESP-NOW: uma placa vira AP, a outra escaneia.
# Foi o que provou que o par de C3 anterior não se enxergava.
teste-rf:
	arduino-cli compile --fqbn $(FQBN_EMISSOR) $(OPCOES_EMISSOR) \
	  --build-property build.defines=-DDINO_AP \
	  --upload --port $(PORTA_EMISSOR) ferramentas/teste_rf
	arduino-cli compile --fqbn $(FQBN_RECEPTOR) $(OPCOES_RECEPTOR) \
	  --upload --port $(PORTA_RECEPTOR) ferramentas/teste_rf
	@echo "abra os dois seriais: o scanner deve listar DINO-TESTE-RF com RSSI"

test: build/testes
	@./build/testes

# A IDE do Arduino só compila arquivos que estejam dentro da pasta do sketch.
# Liga TUDO de lib/ nas duas pastas: manter uma lista por arquivo aqui já
# quebrou uma vez ao adicionar um módulo novo. lib/ segue sendo a fonte única.
sketches:
	@for pasta in emissor receptor; do \
	  find $$pasta -maxdepth 1 -type l -delete; \
	  for f in lib/*.h lib/*.cpp; do \
	    ln -sf ../$$f $$pasta/$$(basename $$f); \
	  done; \
	done
	@echo "links de lib/ criados em emissor/ e receptor/"

build/testes: $(FONTES) $(CABECALHOS)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(FONTES) -o $@

limpar:
	rm -rf build
