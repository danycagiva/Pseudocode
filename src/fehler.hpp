#pragma once
#include <stdexcept>
#include <string>

struct PseudoFehler : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct LexerFehler : PseudoFehler {
    int zeile, spalte;
    LexerFehler(const std::string& nachricht, int z, int s)
        : PseudoFehler(nachricht), zeile(z), spalte(s) {}
};

struct ParseFehler : PseudoFehler {
    int zeile, spalte;
    ParseFehler(const std::string& nachricht, int z, int s)
        : PseudoFehler(nachricht), zeile(z), spalte(s) {}
};

struct LaufzeitFehler : PseudoFehler {
    int zeile, spalte;
    LaufzeitFehler(const std::string& nachricht, int z = 0, int s = 0)
        : PseudoFehler(nachricht), zeile(z), spalte(s) {}
};
