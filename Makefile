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

PORTA_EMISSOR  ?= /dev/ttyACM0
PORTA_RECEPTOR ?= /dev/ttyACM1
FQBN           := esp32:esp32:esp32c3
# O C3 precisa de CDCOnBoot=cdc, senão o Serial sai pela UART0 e não pela USB.
OPCOES_PLACA   := --board-options CDCOnBoot=cdc

.PHONY: test sketches chaves macs gravar smoke limpar

chaves:
	@python3 ferramentas/gerar_chaves.py

macs:
	@python3 ferramentas/ler_macs.py --emissor $(PORTA_EMISSOR) --receptor $(PORTA_RECEPTOR)

gravar: sketches
	arduino-cli compile --fqbn $(FQBN) $(OPCOES_PLACA) --upload --port $(PORTA_RECEPTOR) receptor
	arduino-cli compile --fqbn $(FQBN) $(OPCOES_PLACA) --upload --port $(PORTA_EMISSOR) emissor

smoke: sketches
	@python3 ferramentas/smoketest.py --emissor $(PORTA_EMISSOR) --receptor $(PORTA_RECEPTOR)

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
