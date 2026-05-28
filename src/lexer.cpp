#include "lexer.hpp"
#include "fehler.hpp"
#include <cctype>
#include <unordered_map>

// Schluesselwort-Tabelle (nur Kleinbuchstaben)
static const std::unordered_map<std::string, TokenTyp> SCHLUESSELWOERTER = {
    {"sei",         TokenTyp::SEI},
    {"wenn",        TokenTyp::WENN},
    {"dann",        TokenTyp::DANN},
    {"sonstwenn",   TokenTyp::SONSTWENN},
    {"sonst",       TokenTyp::SONST},
    {"ende",        TokenTyp::ENDE},
    {"solange",     TokenTyp::SOLANGE},
    {"tue",         TokenTyp::TUE},
    {"fuer",        TokenTyp::FUER},
    {"von",         TokenTyp::VON},
    {"bis",         TokenTyp::BIS},
    {"schrittweite",TokenTyp::SCHRITTWEITE},
    {"funktion",    TokenTyp::FUNKTION},
    {"rueckgabe",   TokenTyp::RUECKGABE},
    {"ausgabe",     TokenTyp::AUSGABE},
    {"und",         TokenTyp::UND},
    {"oder",        TokenTyp::ODER},
    {"nicht",       TokenTyp::NICHT},
    {"wahr",        TokenTyp::WAHR},
    {"falsch",      TokenTyp::FALSCH},
    {"liste",       TokenTyp::LISTE},    // ende-Varianten – alle auf ENDE normalisieren
    {"endewenn",     TokenTyp::ENDE},
    {"endesolange",  TokenTyp::ENDE},
    {"endefuer",     TokenTyp::ENDE},
    {"endefunktion", TokenTyp::ENDE},
};

Lexer::Lexer(const std::string& quelltext, const std::string& dateiname)
    : quelltext_(quelltext), dateiname_(dateiname) {}

char Lexer::aktuell() const {
    if (pos_ >= quelltext_.size()) return '\0';
    return quelltext_[pos_];
}

char Lexer::vorschau() const {
    if (pos_ + 1 >= quelltext_.size()) return '\0';
    return quelltext_[pos_ + 1];
}

char Lexer::verbrauche() {
    char c = quelltext_[pos_++];
    if (c == '\n') { zeile_++; spalte_ = 1; }
    else           { spalte_++; }
    return c;
}

void Lexer::ueberspringe_zeilenkommentar() {
    while (pos_ < quelltext_.size() && quelltext_[pos_] != '\n')
        pos_++;
}

Token Lexer::lese_zahl() {
    int tok_zeile = zeile_, tok_spalte = spalte_;
    std::string puffer;
    while (pos_ < quelltext_.size() && std::isdigit((unsigned char)quelltext_[pos_]))
        puffer += verbrauche();
    if (pos_ < quelltext_.size() && quelltext_[pos_] == '.' && std::isdigit((unsigned char)vorschau())) {
        puffer += verbrauche(); // '.'
        while (pos_ < quelltext_.size() && std::isdigit((unsigned char)quelltext_[pos_]))
            puffer += verbrauche();
    }
    return {TokenTyp::ZAHL, puffer, tok_zeile, tok_spalte};
}

Token Lexer::lese_zeichenkette() {
    int tok_zeile = zeile_, tok_spalte = spalte_;
    verbrauche(); // oeffnendes "
    std::string puffer;
    while (pos_ < quelltext_.size() && quelltext_[pos_] != '"') {
        if (quelltext_[pos_] == '\\') {
            verbrauche(); // Backslash
            if (pos_ >= quelltext_.size())
                throw LexerFehler("Unerwartetes Dateiende in Zeichenkette", zeile_, spalte_);
            char esc = verbrauche();
            switch (esc) {
                case 'n':  puffer += '\n'; break;
                case 't':  puffer += '\t'; break;
                case '"':  puffer += '"';  break;
                case '\\': puffer += '\\'; break;
                default:
                    puffer += '\\';
                    puffer += esc;
                    break;
            }
        } else {
            puffer += verbrauche();
        }
    }
    if (pos_ >= quelltext_.size())
        throw LexerFehler("Nicht abgeschlossene Zeichenkette", tok_zeile, tok_spalte);
    verbrauche(); // schliessendes "
    return {TokenTyp::ZEICHENKETTE, puffer, tok_zeile, tok_spalte};
}

Token Lexer::lese_bezeichner_oder_schluessel() {
    int tok_zeile = zeile_, tok_spalte = spalte_;
    std::string puffer;
    // Erstes Zeichen: Buchstabe, Unterstrich oder UTF-8-Hochbyte
    while (pos_ < quelltext_.size()) {
        unsigned char c = (unsigned char)quelltext_[pos_];
        if (std::isalpha(c) || c == '_' || c >= 0x80 || (std::isdigit(c) && !puffer.empty()))
            puffer += verbrauche();
        else
            break;
    }
    auto it = SCHLUESSELWOERTER.find(puffer);
    if (it != SCHLUESSELWOERTER.end())
        return {it->second, puffer, tok_zeile, tok_spalte};
    return {TokenTyp::BEZEICHNER, puffer, tok_zeile, tok_spalte};
}

std::vector<Token> Lexer::tokenisiere() {
    std::vector<Token> tokens;

    while (true) {
        // Leerzeichen und Zeilenumbrueche ueberspringen
        while (pos_ < quelltext_.size()) {
            unsigned char c = (unsigned char)quelltext_[pos_];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                if (c == '\n') { zeile_++; spalte_ = 1; pos_++; }
                else           { spalte_++; pos_++; }
            } else {
                break;
            }
        }

        if (pos_ >= quelltext_.size()) break;

        int tok_zeile = zeile_;
        int tok_spalte = spalte_;
        char c = quelltext_[pos_];

        // Kommentar
        if (c == '/' && vorschau() == '/') {
            ueberspringe_zeilenkommentar();
            continue;
        }

        // Zahl
        if (std::isdigit((unsigned char)c)) {
            tokens.push_back(lese_zahl());
            continue;
        }

        // Zeichenkette
        if (c == '"') {
            tokens.push_back(lese_zeichenkette());
            continue;
        }

        // Bezeichner / Schluesselwort (inkl. UTF-8-Umlaute)
        if (std::isalpha((unsigned char)c) || c == '_' || (unsigned char)c >= 0x80) {
            tokens.push_back(lese_bezeichner_oder_schluessel());
            continue;
        }

        // Operatoren und Satzzeichen
        verbrauche(); // c verbrauchen

        switch (c) {
            case ':':
                if (aktuell() == '=') { verbrauche(); tokens.push_back({TokenTyp::ZUWEISUNG,       ":=", tok_zeile, tok_spalte}); }
                else throw LexerFehler("Unbekanntes Zeichen ':'  (meintest du ':='?)", tok_zeile, tok_spalte);
                break;
            case '=': tokens.push_back({TokenTyp::GLEICH,           "=",  tok_zeile, tok_spalte}); break;
            case '!':
                if (aktuell() == '=') { verbrauche(); tokens.push_back({TokenTyp::UNGLEICH,         "!=", tok_zeile, tok_spalte}); }
                else throw LexerFehler("Unbekanntes Zeichen '!'  (meintest du '!='?)", tok_zeile, tok_spalte);
                break;
            case '<':
                if (aktuell() == '=') { verbrauche(); tokens.push_back({TokenTyp::KLEINER_GLEICH,   "<=", tok_zeile, tok_spalte}); }
                else                               tokens.push_back({TokenTyp::KLEINER,           "<",  tok_zeile, tok_spalte});
                break;
            case '>':
                if (aktuell() == '=') { verbrauche(); tokens.push_back({TokenTyp::GROESSER_GLEICH,  ">=", tok_zeile, tok_spalte}); }
                else                               tokens.push_back({TokenTyp::GROESSER,          ">",  tok_zeile, tok_spalte});
                break;
            case '+':
                if (aktuell() == '+') { verbrauche(); tokens.push_back({TokenTyp::PLUS_PLUS,        "++", tok_zeile, tok_spalte}); }
                else                               tokens.push_back({TokenTyp::PLUS,              "+",  tok_zeile, tok_spalte});
                break;
            case '-':
                if (aktuell() == '-') { verbrauche(); tokens.push_back({TokenTyp::MINUS_MINUS,      "--", tok_zeile, tok_spalte}); }
                else                               tokens.push_back({TokenTyp::MINUS,             "-",  tok_zeile, tok_spalte});
                break;
            case '*': tokens.push_back({TokenTyp::STERN,            "*",  tok_zeile, tok_spalte}); break;
            case '/': tokens.push_back({TokenTyp::SLASH,            "/",  tok_zeile, tok_spalte}); break;
            case '%': tokens.push_back({TokenTyp::MODULO,           "%",  tok_zeile, tok_spalte}); break;
            case '(': tokens.push_back({TokenTyp::LPAREN,           "(",  tok_zeile, tok_spalte}); break;
            case ')': tokens.push_back({TokenTyp::RPAREN,           ")",  tok_zeile, tok_spalte}); break;
            case ',': tokens.push_back({TokenTyp::KOMMA,            ",",  tok_zeile, tok_spalte}); break;
            case ';': tokens.push_back({TokenTyp::STRICHPUNKT,      ";",  tok_zeile, tok_spalte}); break;
            case '[': tokens.push_back({TokenTyp::LBRACKET,          "[",  tok_zeile, tok_spalte}); break;
            case ']': tokens.push_back({TokenTyp::RBRACKET,          "]",  tok_zeile, tok_spalte}); break;
            case '.': tokens.push_back({TokenTyp::PUNKT,             ".",  tok_zeile, tok_spalte}); break;
            default:
                throw LexerFehler(
                    std::string("Unbekanntes Zeichen '") + c + "'",
                    tok_zeile, tok_spalte);
        }
    }

    tokens.push_back({TokenTyp::DATEIENDE, "", zeile_, spalte_});
    return tokens;
}
