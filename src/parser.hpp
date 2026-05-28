#pragma once
#include "ast.hpp"
#include "token.hpp"
#include <memory>
#include <vector>

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    std::shared_ptr<Programm> parse();

private:
    std::vector<Token> tokens_;
    size_t pos_ = 0;

    // Token-Hilfsmethoden
    const Token& aktuell() const;
    const Token& voraus(int offset = 1) const;
    bool   stimmt_ueberein(TokenTyp typ) const;
    Token  verbrauche(TokenTyp erwartet, const std::string& fehlermeldung);
    Token  verbrauche_beliebig();
    bool   ist_ende() const;

    // Anweisungen
    KnotenPtr parse_anweisung();
    KnotenPtr parse_wenn();
    KnotenPtr parse_solange();
    KnotenPtr parse_fuer();
    KnotenPtr parse_funktion();
    KnotenPtr parse_rueckgabe();
    KnotenPtr parse_ausgabe();
    KnotenPtr parse_zuweisung(const std::string& name, int zeile, int spalte);

    // Block: liest bis eines der Block-Endtokens erscheint (ohne es zu verbrauchen)
    std::vector<KnotenPtr> parse_block(std::initializer_list<TokenTyp> stop_tokens);

    // Ausdruecke (Praezedenzleiter)
    KnotenPtr parse_ausdruck();
    KnotenPtr parse_oder();
    KnotenPtr parse_und();
    KnotenPtr parse_nicht();
    KnotenPtr parse_vergleich();
    KnotenPtr parse_addition();
    KnotenPtr parse_multiplikation();
    KnotenPtr parse_unaer();
    KnotenPtr parse_primaer();
};
