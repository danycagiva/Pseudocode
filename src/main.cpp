#include "fehler.hpp"
#include "interpreter.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

static std::string lese_datei(const std::string& pfad) {
    std::ifstream datei(pfad);
    if (!datei.is_open())
        throw std::runtime_error("Datei nicht gefunden: " + pfad);
    std::ostringstream oss;
    oss << datei.rdbuf();
    return oss.str();
}

static int interpretiere(const std::string& quelltext, const std::string& dateiname) {
    try {
        Lexer lexer(quelltext, dateiname);
        auto tokens = lexer.tokenisiere();

        Parser parser(std::move(tokens));
        auto programm = parser.parse();

        Interpreter interpreter;
        interpreter.fuehre_programm_aus(*programm);
        return 0;

    } catch (const LexerFehler& e) {
        std::cerr << dateiname << ":" << e.zeile << ":" << e.spalte
                  << ": Fehler: " << e.what() << "\n";
        return 1;
    } catch (const ParseFehler& e) {
        std::cerr << dateiname << ":" << e.zeile << ":" << e.spalte
                  << ": Fehler: " << e.what() << "\n";
        return 1;
    } catch (const LaufzeitFehler& e) {
        if (e.zeile > 0)
            std::cerr << dateiname << ":" << e.zeile << ":" << e.spalte
                      << ": Laufzeitfehler: " << e.what() << "\n";
        else
            std::cerr << dateiname << ": Laufzeitfehler: " << e.what() << "\n";
        return 1;
    }
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    // UTF-8 Konsolenausgabe unter Windows
    SetConsoleOutputCP(CP_UTF8);
#endif

    if (argc < 2) {
        std::cerr << "Verwendung: pseudocode <datei.pseudo>\n";
        std::cerr << "Beispiel:   pseudocode hallo_welt.pseudo\n";
        return 1;
    }

    std::string pfad = argv[1];
    std::string quelltext;
    try {
        quelltext = lese_datei(pfad);
    } catch (const std::exception& e) {
        std::cerr << "Fehler: " << e.what() << "\n";
        return 1;
    }

    return interpretiere(quelltext, pfad);
}
