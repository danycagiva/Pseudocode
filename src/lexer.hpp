#pragma once
#include "token.hpp"
#include <string>
#include <vector>

class Lexer {
public:
    explicit Lexer(const std::string& quelltext, const std::string& dateiname = "<eingabe>");
    std::vector<Token> tokenisiere();

private:
    std::string quelltext_;
    std::string dateiname_;
    size_t pos_ = 0;
    int zeile_ = 1;
    int spalte_ = 1;

    char aktuell() const;
    char vorschau() const;
    char verbrauche();

    Token lese_zahl();
    Token lese_zeichenkette();
    Token lese_bezeichner_oder_schluessel();
    void  ueberspringe_zeilenkommentar();
};
