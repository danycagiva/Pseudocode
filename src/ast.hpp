#pragma once
#include <memory>
#include <optional>
#include <string>
#include <vector>

// Basisklasse aller AST-Knoten
struct Knoten {
    int zeile = 0;
    int spalte = 0;
    virtual ~Knoten() = default;
};

using KnotenPtr = std::shared_ptr<Knoten>;

// ------------------------------------------------------------------ //
// Literale                                                            //
// ------------------------------------------------------------------ //

struct ZahlLiteral : Knoten {
    double wert;
};

struct ZeichenkettenLiteral : Knoten {
    std::string wert;
};

struct WahrheitsLiteral : Knoten {
    bool wert;
};

// ------------------------------------------------------------------ //
// Ausdruecke                                                          //
// ------------------------------------------------------------------ //

struct Bezeichner : Knoten {
    std::string name;
};

struct BinaerAusdruck : Knoten {
    std::string op;
    KnotenPtr links;
    KnotenPtr rechts;
};

struct UnaerAusdruck : Knoten {
    std::string op;
    KnotenPtr operand;
};

struct FunktionsAufruf : Knoten {
    std::string name;
    std::vector<KnotenPtr> argumente;
};

// ------------------------------------------------------------------ //
// Anweisungen                                                         //
// ------------------------------------------------------------------ //

struct Zuweisung : Knoten {
    std::string ziel;
    KnotenPtr ausdruck;
};

struct AusgabeAnweisung : Knoten {
    KnotenPtr ausdruck;
};

struct SonstwennZweig {
    KnotenPtr bedingung;
    std::vector<KnotenPtr> koerper;
};

struct WennAnweisung : Knoten {
    KnotenPtr bedingung;
    std::vector<KnotenPtr> dann_block;
    std::vector<SonstwennZweig> sonstwenn_zweige;
    std::vector<KnotenPtr> sonst_block;
};

struct SolangeSchleife : Knoten {
    KnotenPtr bedingung;
    std::vector<KnotenPtr> koerper;
};

// C-Stil: fuer i := 0; i < 10; i++ tue ... ende
struct FuerSchleife : Knoten {
    KnotenPtr init;        // Zuweisung
    KnotenPtr bedingung;
    KnotenPtr schritt;     // Zuweisung (i++ wird zu i := i + 1 umgeschrieben)
    std::vector<KnotenPtr> koerper;
};

// Bereichsstil: fuer i von 1 bis 10 [schrittweite 2] tue ... ende
struct BereichsSchleife : Knoten {
    std::string variable;
    KnotenPtr start;
    KnotenPtr ende;
    KnotenPtr schrittweite;  // nullptr = Standardschritt 1
    std::vector<KnotenPtr> koerper;
};

struct RueckgabeAnweisung : Knoten {
    KnotenPtr ausdruck;  // nullptr = leere Rueckgabe
};

struct FunktionsDefinition : Knoten {
    std::string name;
    std::vector<std::string> parameter;
    std::vector<KnotenPtr> koerper;
};

struct Programm : Knoten {
    std::vector<KnotenPtr> anweisungen;
};

// ------------------------------------------------------------------ //
// Listen                                                              //
// ------------------------------------------------------------------ //

struct ListenLiteral : Knoten {
    std::vector<KnotenPtr> elemente;
};

struct IndexZugriff : Knoten {
    KnotenPtr objekt;
    KnotenPtr index;
};

// liste[i] := wert
struct IndexZuweisung : Knoten {
    KnotenPtr objekt;
    KnotenPtr index;
    KnotenPtr wert;
};

// liste.methode(args)
struct MethodenAufruf : Knoten {
    KnotenPtr objekt;
    std::string methode;
    std::vector<KnotenPtr> argumente;
};
