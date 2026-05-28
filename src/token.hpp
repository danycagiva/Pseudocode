#pragma once
#include <string>

enum class TokenTyp {
    // Literale
    ZAHL,
    ZEICHENKETTE,
    WAHR,
    FALSCH,

    // Bezeichner
    BEZEICHNER,

    // Schluesselwoerter
    SEI,
    WENN,
    DANN,
    SONSTWENN,
    SONST,
    ENDE,
    SOLANGE,
    TUE,
    FUER,
    VON,
    BIS,
    SCHRITTWEITE,
    FUNKTION,
    RUECKGABE,
    AUSGABE,
    UND,
    ODER,
    NICHT,

    // Operatoren
    ZUWEISUNG,        // :=
    GLEICH,           // =
    UNGLEICH,         // !=
    KLEINER,          // <
    GROESSER,         // >
    KLEINER_GLEICH,   // <=
    GROESSER_GLEICH,  // >=
    PLUS,             // +
    MINUS,            // -
    STERN,            // *
    SLASH,            // /
    MODULO,           // %
    PLUS_PLUS,        // ++
    MINUS_MINUS,      // --

    // Satzzeichen
    LPAREN,           // (
    RPAREN,           // )
    KOMMA,            // ,
    STRICHPUNKT,      // ;
    LBRACKET,         // [
    RBRACKET,         // ]
    PUNKT,            // .
    LISTE,            // Schluesselwort 'liste'

    DATEIENDE
};

struct Token {
    TokenTyp typ;
    std::string wert;
    int zeile = 1;
    int spalte = 1;
};
