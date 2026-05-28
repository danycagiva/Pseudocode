#pragma once
#include "ast.hpp"
#include "fehler.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

// ------------------------------------------------------------------ //
// Laufzeitwert                                                        //
// ------------------------------------------------------------------ //

struct NullWert {
    bool operator==(const NullWert&) const { return true; }
    bool operator!=(const NullWert&) const { return false; }
};

// Vorwaertsdeklaration fuer rekursiven Listentyp
struct ListenWert;
using ListenPtr = std::shared_ptr<ListenWert>;

using Wert = std::variant<double, std::string, bool, NullWert, ListenPtr>;

struct ListenWert {
    std::vector<Wert> elemente;
};

// Internes Signal fuer 'rueckgabe'
struct RueckgabeSignal {
    Wert wert;
};

// ------------------------------------------------------------------ //
// Geltungsbereich (Umgebung)                                         //
// ------------------------------------------------------------------ //

class Umgebung {
public:
    explicit Umgebung(std::shared_ptr<Umgebung> eltern = nullptr);

    Wert holen(const std::string& name, int zeile, int spalte) const;
    void setzen(const std::string& name, const Wert& wert);
    bool hatVariable(const std::string& name) const;

private:
    std::unordered_map<std::string, Wert> variablen_;
    std::shared_ptr<Umgebung> eltern_;
};

// ------------------------------------------------------------------ //
// Interpreter                                                         //
// ------------------------------------------------------------------ //

class Interpreter {
public:
    Interpreter();
    void fuehre_programm_aus(const Programm& programm);

private:
    std::shared_ptr<Umgebung> global_;
    std::unordered_map<std::string, std::shared_ptr<FunktionsDefinition>> funktionen_;

    // Anweisungen ausfuehren
    void fuehre_aus(const KnotenPtr& knoten, std::shared_ptr<Umgebung> scope);
    void fuehre_block_aus(const std::vector<KnotenPtr>& block, std::shared_ptr<Umgebung> scope);

    // Ausdruecke auswerten
    Wert auswerten(const KnotenPtr& knoten, std::shared_ptr<Umgebung> scope);

    // Eingebaute Hilfsmethoden
    bool          ist_wahr(const Wert& w) const;
    double        als_zahl(const Wert& w, int zeile, int spalte) const;
    ListenPtr     als_liste(const Wert& w, int zeile, int spalte) const;
    std::string   wert_zu_text(const Wert& w) const;
    void          drucke_wert(const Wert& w) const;

    Wert rufe_funktion_auf(const std::string& name,
                           const std::vector<Wert>& args,
                           int zeile, int spalte);
    Wert rufe_methode_auf(const ListenPtr& liste,
                          const std::string& methode,
                          const std::vector<Wert>& args,
                          int zeile, int spalte);
};
