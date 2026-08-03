// Framework de teste mínimo, sem dependências externas.
// Testes se auto-registram; main.cpp só chama executarTestes().
#pragma once

#include <cstdio>
#include <vector>

struct CasoDeTeste {
  const char *nome;
  void (*fn)();
};

inline std::vector<CasoDeTeste> &registroDeTestes() {
  static std::vector<CasoDeTeste> r;
  return r;
}

inline int &falhasDoCasoAtual() {
  static int f = 0;
  return f;
}

struct RegistradorDeTeste {
  RegistradorDeTeste(const char *nome, void (*fn)()) {
    registroDeTestes().push_back({nome, fn});
  }
};

#define TESTE(nome)                                                            \
  static void nome();                                                          \
  static RegistradorDeTeste registrador_##nome(#nome, nome);                   \
  static void nome()

#define VERIFICA(cond)                                                         \
  do {                                                                         \
    if (!(cond)) {                                                             \
      falhasDoCasoAtual()++;                                                   \
      std::printf("      linha %d: VERIFICA(%s) falhou\n", __LINE__, #cond);    \
    }                                                                          \
  } while (0)

#define VERIFICA_IGUAL(obtido, esperado)                                       \
  do {                                                                         \
    long long o_ = (long long)(obtido);                                        \
    long long e_ = (long long)(esperado);                                      \
    if (o_ != e_) {                                                            \
      falhasDoCasoAtual()++;                                                   \
      std::printf("      linha %d: %s -> obtido %lld, esperado %lld\n",         \
                  __LINE__, #obtido, o_, e_);                                  \
    }                                                                          \
  } while (0)

inline int executarTestes() {
  int casosComFalha = 0;

  for (const CasoDeTeste &t : registroDeTestes()) {
    falhasDoCasoAtual() = 0;
    t.fn();
    if (falhasDoCasoAtual() == 0) {
      std::printf("  ok    %s\n", t.nome);
    } else {
      std::printf("  FALHA %s\n", t.nome);
      casosComFalha++;
    }
  }

  std::printf("\n%d teste(s), %d com falha\n", (int)registroDeTestes().size(),
              casosComFalha);
  return casosComFalha == 0 ? 0 : 1;
}
