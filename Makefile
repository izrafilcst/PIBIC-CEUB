CXX      ?= g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Werror -O1 -g -Ilib -Itest

FONTES := lib/debounce.cpp \
          lib/receptor_logic.cpp \
          lib/emissor_logic.cpp \
          lib/identidade.cpp \
          test/main.cpp \
          test/test_debounce.cpp \
          test/test_receptor.cpp \
          test/test_integracao.cpp \
          test/test_identidade.cpp

CABECALHOS := lib/identidade.h lib/debounce.h lib/receptor_logic.h lib/emissor_logic.h lib/protocolo.h test/framework.h

.PHONY: test sketches limpar

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
