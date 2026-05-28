#include "interpreter.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>

// ------------------------------------------------------------------ //
// Umgebung                                                            //
// ------------------------------------------------------------------ //

Umgebung::Umgebung(std::shared_ptr<Umgebung> eltern) : eltern_(std::move(eltern)) {}

Wert Umgebung::holen(const std::string& name, int zeile, int spalte) const {
    auto it = variablen_.find(name);
    if (it != variablen_.end()) return it->second;
    if (eltern_) return eltern_->holen(name, zeile, spalte);
    throw LaufzeitFehler("Unbekannte Variable '" + name + "'", zeile, spalte);
}

void Umgebung::setzen(const std::string& name, const Wert& wert) {
    // Immer im aktuellen Scope setzen (Lesen kann via holen() nach oben gehen)
    variablen_[name] = wert;
}

bool Umgebung::hatVariable(const std::string& name) const {
    if (variablen_.count(name)) return true;
    if (eltern_) return eltern_->hatVariable(name);
    return false;
}

// ------------------------------------------------------------------ //
// Interpreter                                                         //
// ------------------------------------------------------------------ //

Interpreter::Interpreter() : global_(std::make_shared<Umgebung>()) {}

// Alle Funktionsdefinitionen vorab registrieren, danach Top-Level ausfuehren
void Interpreter::fuehre_programm_aus(const Programm& programm) {
    for (auto& anweisung : programm.anweisungen) {
        if (auto def = std::dynamic_pointer_cast<FunktionsDefinition>(anweisung)) {
            funktionen_[def->name] = def;
        }
    }
    for (auto& anweisung : programm.anweisungen) {
        if (!std::dynamic_pointer_cast<FunktionsDefinition>(anweisung)) {
            fuehre_aus(anweisung, global_);
        }
    }
}

// ------------------------------------------------------------------ //
// Anweisungen                                                         //
// ------------------------------------------------------------------ //

void Interpreter::fuehre_block_aus(const std::vector<KnotenPtr>& block,
                                    std::shared_ptr<Umgebung> scope) {
    for (auto& knoten : block)
        fuehre_aus(knoten, scope);
}

void Interpreter::fuehre_aus(const KnotenPtr& knoten, std::shared_ptr<Umgebung> scope) {

    // Zuweisung
    if (auto z = std::dynamic_pointer_cast<Zuweisung>(knoten)) {
        Wert val = auswerten(z->ausdruck, scope);
        scope->setzen(z->ziel, val);
        return;
    }

    // Indexzuweisung: liste[i] := wert
    if (auto iz = std::dynamic_pointer_cast<IndexZuweisung>(knoten)) {
        Wert obj = auswerten(iz->objekt, scope);
        Wert idx = auswerten(iz->index,  scope);
        Wert val = auswerten(iz->wert,   scope);
        auto lst = als_liste(obj, iz->zeile, iz->spalte);
        long long i = (long long)als_zahl(idx, iz->zeile, iz->spalte);
        if (i < 0) i += (long long)lst->elemente.size();
        if (i < 0 || (size_t)i >= lst->elemente.size())
            throw LaufzeitFehler("Index " + std::to_string(i) + " ausserhalb des Bereichs",
                                 iz->zeile, iz->spalte);
        lst->elemente[(size_t)i] = val;
        return;
    }

    // Ausgabe
    if (auto a = std::dynamic_pointer_cast<AusgabeAnweisung>(knoten)) {
        drucke_wert(auswerten(a->ausdruck, scope));
        return;
    }

    // Wenn / Sonstwenn / Sonst
    if (auto w = std::dynamic_pointer_cast<WennAnweisung>(knoten)) {
        if (ist_wahr(auswerten(w->bedingung, scope))) {
            fuehre_block_aus(w->dann_block, scope);
            return;
        }
        for (auto& zweig : w->sonstwenn_zweige) {
            if (ist_wahr(auswerten(zweig.bedingung, scope))) {
                fuehre_block_aus(zweig.koerper, scope);
                return;
            }
        }
        fuehre_block_aus(w->sonst_block, scope);
        return;
    }

    // Solange
    if (auto s = std::dynamic_pointer_cast<SolangeSchleife>(knoten)) {
        while (ist_wahr(auswerten(s->bedingung, scope)))
            fuehre_block_aus(s->koerper, scope);
        return;
    }

    // Fuer (C-Stil)
    if (auto f = std::dynamic_pointer_cast<FuerSchleife>(knoten)) {
        fuehre_aus(f->init, scope);
        while (ist_wahr(auswerten(f->bedingung, scope))) {
            fuehre_block_aus(f->koerper, scope);
            fuehre_aus(f->schritt, scope);
        }
        return;
    }

    // Bereichsschleife
    if (auto b = std::dynamic_pointer_cast<BereichsSchleife>(knoten)) {
        double start   = als_zahl(auswerten(b->start, scope), b->zeile, b->spalte);
        double ende    = als_zahl(auswerten(b->ende,  scope), b->zeile, b->spalte);
        double schritt = 1.0;
        if (b->schrittweite)
            schritt = als_zahl(auswerten(b->schrittweite, scope), b->zeile, b->spalte);
        if (schritt == 0)
            throw LaufzeitFehler("Schrittweite darf nicht 0 sein", b->zeile, b->spalte);
        for (double i = start; (schritt > 0 ? i <= ende : i >= ende); i += schritt) {
            scope->setzen(b->variable, i);
            fuehre_block_aus(b->koerper, scope);
        }
        return;
    }

    // Rueckgabe
    if (auto r = std::dynamic_pointer_cast<RueckgabeAnweisung>(knoten)) {
        Wert val = r->ausdruck ? auswerten(r->ausdruck, scope) : NullWert{};
        throw RueckgabeSignal{val};
    }

    // FunktionsDefinition (wird in fuehre_programm_aus bereits vorab registriert)
    if (std::dynamic_pointer_cast<FunktionsDefinition>(knoten)) return;

    // Ausdrucksanweisung (z.B. Funktionsaufruf ohne Rueckgabewert)
    auswerten(knoten, scope);
}

// ------------------------------------------------------------------ //
// Ausdruecke                                                          //
// ------------------------------------------------------------------ //

Wert Interpreter::auswerten(const KnotenPtr& knoten, std::shared_ptr<Umgebung> scope) {

    if (auto z = std::dynamic_pointer_cast<ZahlLiteral>(knoten))
        return z->wert;

    if (auto s = std::dynamic_pointer_cast<ZeichenkettenLiteral>(knoten))
        return s->wert;

    if (auto w = std::dynamic_pointer_cast<WahrheitsLiteral>(knoten))
        return w->wert;

    if (auto b = std::dynamic_pointer_cast<Bezeichner>(knoten))
        return scope->holen(b->name, b->zeile, b->spalte);

    // Listen-Literal
    if (auto ll = std::dynamic_pointer_cast<ListenLiteral>(knoten)) {
        auto lst = std::make_shared<ListenWert>();
        for (auto& elem : ll->elemente)
            lst->elemente.push_back(auswerten(elem, scope));
        return lst;
    }

    // Indexzugriff: liste[i]
    if (auto iz = std::dynamic_pointer_cast<IndexZugriff>(knoten)) {
        Wert obj = auswerten(iz->objekt, scope);
        Wert idx = auswerten(iz->index,  scope);
        auto lst = als_liste(obj, iz->zeile, iz->spalte);
        long long i = (long long)als_zahl(idx, iz->zeile, iz->spalte);
        if (i < 0) i += (long long)lst->elemente.size();
        if (i < 0 || (size_t)i >= lst->elemente.size())
            throw LaufzeitFehler("Index " + std::to_string(i) + " ausserhalb des Bereichs",
                                 iz->zeile, iz->spalte);
        return lst->elemente[(size_t)i];
    }

    // Methodenaufruf: liste.methode(args)
    if (auto ma = std::dynamic_pointer_cast<MethodenAufruf>(knoten)) {
        Wert obj = auswerten(ma->objekt, scope);
        std::vector<Wert> args;
        args.reserve(ma->argumente.size());
        for (auto& arg : ma->argumente)
            args.push_back(auswerten(arg, scope));
        auto lst = als_liste(obj, ma->zeile, ma->spalte);
        return rufe_methode_auf(lst, ma->methode, args, ma->zeile, ma->spalte);
    }

    // Unaer
    if (auto u = std::dynamic_pointer_cast<UnaerAusdruck>(knoten)) {
        Wert val = auswerten(u->operand, scope);
        if (u->op == "-") {
            if (auto* d = std::get_if<double>(&val)) return -(*d);
            throw LaufzeitFehler("Unary '-' erwartet eine Zahl", u->zeile, u->spalte);
        }
        if (u->op == "nicht") return !ist_wahr(val);
        throw LaufzeitFehler("Unbekannter unaerer Operator: " + u->op, u->zeile, u->spalte);
    }

    // Binaer
    if (auto b = std::dynamic_pointer_cast<BinaerAusdruck>(knoten)) {
        // Kurzschlussauswertung fuer logische Operatoren
        if (b->op == "und") {
            if (!ist_wahr(auswerten(b->links, scope))) return false;
            return ist_wahr(auswerten(b->rechts, scope));
        }
        if (b->op == "oder") {
            if (ist_wahr(auswerten(b->links, scope))) return true;
            return ist_wahr(auswerten(b->rechts, scope));
        }

        Wert links  = auswerten(b->links,  scope);
        Wert rechts = auswerten(b->rechts, scope);

        // Gleichheit / Ungleichheit
        if (b->op == "=")  return links == rechts;
        if (b->op == "!=") return links != rechts;

        // Zeichenketten-Verkettung
        if (b->op == "+") {
            if (std::holds_alternative<std::string>(links) ||
                std::holds_alternative<std::string>(rechts)) {
                std::ostringstream oss;
                std::visit([&oss](auto&& v) {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<T, double>) {
                        if (std::floor(v) == v && std::abs(v) < 1e15) oss << (long long)v;
                        else oss << v;
                    } else if constexpr (std::is_same_v<T, std::string>) {
                        oss << v;
                    } else if constexpr (std::is_same_v<T, bool>) {
                        oss << (v ? "wahr" : "falsch");
                    }
                }, links);
                std::visit([&oss](auto&& v) {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<T, double>) {
                        if (std::floor(v) == v && std::abs(v) < 1e15) oss << (long long)v;
                        else oss << v;
                    } else if constexpr (std::is_same_v<T, std::string>) {
                        oss << v;
                    } else if constexpr (std::is_same_v<T, bool>) {
                        oss << (v ? "wahr" : "falsch");
                    }
                }, rechts);
                return oss.str();
            }
        }

        // Arithmetik: beide Seiten muessen Zahlen sein
        if (!std::holds_alternative<double>(links) || !std::holds_alternative<double>(rechts))
            throw LaufzeitFehler(
                "Operator '" + b->op + "' erwartet Zahlen",
                b->zeile, b->spalte);

        double l = std::get<double>(links);
        double r = std::get<double>(rechts);

        if (b->op == "+")  return l + r;
        if (b->op == "-")  return l - r;
        if (b->op == "*")  return l * r;
        if (b->op == "/") {
            if (r == 0.0) throw LaufzeitFehler("Division durch Null", b->zeile, b->spalte);
            return l / r;
        }
        if (b->op == "%")  return std::fmod(l, r);
        if (b->op == "<")  return l <  r;
        if (b->op == ">")  return l >  r;
        if (b->op == "<=") return l <= r;
        if (b->op == ">=") return l >= r;

        throw LaufzeitFehler("Unbekannter Operator: " + b->op, b->zeile, b->spalte);
    }

    // Funktionsaufruf
    if (auto f = std::dynamic_pointer_cast<FunktionsAufruf>(knoten)) {
        std::vector<Wert> args;
        args.reserve(f->argumente.size());
        for (auto& arg : f->argumente)
            args.push_back(auswerten(arg, scope));
        return rufe_funktion_auf(f->name, args, f->zeile, f->spalte);
    }

    throw LaufzeitFehler("Unbekannter AST-Knoten", knoten->zeile, knoten->spalte);
}

// ------------------------------------------------------------------ //
// Eingebaute Funktionen & Hilfsmethoden                              //
// ------------------------------------------------------------------ //

Wert Interpreter::rufe_funktion_auf(const std::string& name,
                                     const std::vector<Wert>& args,
                                     int zeile, int spalte) {
    // Eingebaute Funktionen
    if (name == "abs") {
        if (args.size() != 1) throw LaufzeitFehler("'abs' erwartet 1 Argument", zeile, spalte);
        return std::abs(als_zahl(args[0], zeile, spalte));
    }
    if (name == "runden") {
        if (args.size() != 1) throw LaufzeitFehler("'runden' erwartet 1 Argument", zeile, spalte);
        return std::round(als_zahl(args[0], zeile, spalte));
    }
    if (name == "boden") {
        if (args.size() != 1) throw LaufzeitFehler("'boden' erwartet 1 Argument", zeile, spalte);
        return std::floor(als_zahl(args[0], zeile, spalte));
    }
    if (name == "decke") {
        if (args.size() != 1) throw LaufzeitFehler("'decke' erwartet 1 Argument", zeile, spalte);
        return std::ceil(als_zahl(args[0], zeile, spalte));
    }
    if (name == "wurzel") {
        if (args.size() != 1) throw LaufzeitFehler("'wurzel' erwartet 1 Argument", zeile, spalte);
        double v = als_zahl(args[0], zeile, spalte);
        if (v < 0) throw LaufzeitFehler("'wurzel' erwartet eine nicht-negative Zahl", zeile, spalte);
        return std::sqrt(v);
    }
    if (name == "laenge") {
        if (args.size() != 1) throw LaufzeitFehler("'laenge' erwartet 1 Argument", zeile, spalte);
        if (auto* s = std::get_if<std::string>(&args[0])) return (double)s->size();
        if (auto* l = std::get_if<ListenPtr>(&args[0]))   return (double)(*l)->elemente.size();
        throw LaufzeitFehler("'laenge' erwartet eine Zeichenkette oder Liste", zeile, spalte);
    }
    if (name == "text") {
        if (args.size() != 1) throw LaufzeitFehler("'text' erwartet 1 Argument", zeile, spalte);
        return wert_zu_text(args[0]);
    }
    if (name == "eingabe") {
        if (args.size() > 1) throw LaufzeitFehler("'eingabe' erwartet 0 oder 1 Argument", zeile, spalte);
        if (args.size() == 1) {
            std::cout << wert_zu_text(args[0]) << std::flush;
        }
        std::string eingabe_text;
        std::getline(std::cin, eingabe_text);
        return eingabe_text;
    }
    if (name == "zahl") {
        if (args.size() != 1) throw LaufzeitFehler("'zahl' erwartet 1 Argument", zeile, spalte);
        if (auto* s = std::get_if<std::string>(&args[0])) {
            try { return std::stod(*s); }
            catch (...) { throw LaufzeitFehler("'" + *s + "' ist keine gueltige Zahl", zeile, spalte); }
        }
        return als_zahl(args[0], zeile, spalte);
    }
    if (name == "datentyp") {
        if (args.size() != 1) throw LaufzeitFehler("'datentyp' erwartet 1 Argument", zeile, spalte);
        return std::visit([](auto&& v) -> std::string {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, double>)      return "zahl";
            if constexpr (std::is_same_v<T, std::string>) return "zeichenkette";
            if constexpr (std::is_same_v<T, bool>)        return "wahrheitswert";
            if constexpr (std::is_same_v<T, ListenPtr>)   return "liste";
            return "nichts";
        }, args[0]);
    }

    // Benutzerdefinierte Funktion
    auto it = funktionen_.find(name);
    if (it == funktionen_.end())
        throw LaufzeitFehler("Unbekannte Funktion '" + name + "'", zeile, spalte);

    auto& def = *it->second;
    if (args.size() != def.parameter.size())
        throw LaufzeitFehler(
            "Funktion '" + name + "' erwartet " + std::to_string(def.parameter.size()) +
            " Argument(e), aber " + std::to_string(args.size()) + " wurden gegeben",
            zeile, spalte);

    auto scope = std::make_shared<Umgebung>(global_);
    for (size_t i = 0; i < def.parameter.size(); i++)
        scope->setzen(def.parameter[i], args[i]);

    try {
        fuehre_block_aus(def.koerper, scope);
    } catch (const RueckgabeSignal& r) {
        return r.wert;
    }
    return NullWert{};
}

bool Interpreter::ist_wahr(const Wert& w) const {
    return std::visit([](auto&& v) -> bool {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, bool>)        return v;
        if constexpr (std::is_same_v<T, double>)      return v != 0.0;
        if constexpr (std::is_same_v<T, std::string>) return !v.empty();
        if constexpr (std::is_same_v<T, ListenPtr>)   return v && !v->elemente.empty();
        return false;
    }, w);
}

double Interpreter::als_zahl(const Wert& w, int zeile, int spalte) const {
    if (auto* d = std::get_if<double>(&w)) return *d;
    if (auto* b = std::get_if<bool>(&w))   return *b ? 1.0 : 0.0;
    throw LaufzeitFehler("Wert ist keine Zahl", zeile, spalte);
}

ListenPtr Interpreter::als_liste(const Wert& w, int zeile, int spalte) const {
    if (auto* l = std::get_if<ListenPtr>(&w)) return *l;
    throw LaufzeitFehler("Wert ist keine Liste", zeile, spalte);
}

std::string Interpreter::wert_zu_text(const Wert& w) const {
    return std::visit([this](auto&& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, double>) {
            std::ostringstream oss;
            if (std::floor(v) == v && std::abs(v) < 1e15) oss << (long long)v;
            else oss << v;
            return oss.str();
        } else if constexpr (std::is_same_v<T, std::string>) {
            return v;
        } else if constexpr (std::is_same_v<T, bool>) {
            return v ? "wahr" : "falsch";
        } else if constexpr (std::is_same_v<T, ListenPtr>) {
            std::string r = "[";
            for (size_t i = 0; i < v->elemente.size(); i++) {
                if (i > 0) r += ", ";
                if (std::holds_alternative<std::string>(v->elemente[i]))
                    r += "\"" + std::get<std::string>(v->elemente[i]) + "\"";
                else
                    r += wert_zu_text(v->elemente[i]);
            }
            return r + "]";
        } else {
            return "nichts";
        }
    }, w);
}

void Interpreter::drucke_wert(const Wert& w) const {
    std::cout << wert_zu_text(w) << "\n";
}

// ------------------------------------------------------------------ //
// Listenmethoden                                                      //
// ------------------------------------------------------------------ //

Wert Interpreter::rufe_methode_auf(const ListenPtr& liste,
                                    const std::string& methode,
                                    const std::vector<Wert>& args,
                                    int zeile, int spalte) {
    if (methode == "hinzufuegen") {
        if (args.size() != 1)
            throw LaufzeitFehler("'hinzufuegen' erwartet 1 Argument", zeile, spalte);
        liste->elemente.push_back(args[0]);
        return NullWert{};
    }
    if (methode == "entfernen") {
        if (args.size() != 1)
            throw LaufzeitFehler("'entfernen' erwartet 1 Argument (Index)", zeile, spalte);
        long long i = (long long)als_zahl(args[0], zeile, spalte);
        if (i < 0) i += (long long)liste->elemente.size();
        if (i < 0 || (size_t)i >= liste->elemente.size())
            throw LaufzeitFehler("Index " + std::to_string(i) + " ausserhalb des Bereichs", zeile, spalte);
        liste->elemente.erase(liste->elemente.begin() + i);
        return NullWert{};
    }
    if (methode == "einfuegen") {
        if (args.size() != 2)
            throw LaufzeitFehler("'einfuegen' erwartet 2 Argumente (Index, Wert)", zeile, spalte);
        long long i = (long long)als_zahl(args[0], zeile, spalte);
        if (i < 0) i += (long long)liste->elemente.size();
        if (i < 0 || (size_t)i > liste->elemente.size())
            throw LaufzeitFehler("Index " + std::to_string(i) + " ausserhalb des Bereichs", zeile, spalte);
        liste->elemente.insert(liste->elemente.begin() + i, args[1]);
        return NullWert{};
    }
    if (methode == "pop") {
        if (!args.empty())
            throw LaufzeitFehler("'pop' erwartet keine Argumente", zeile, spalte);
        if (liste->elemente.empty())
            throw LaufzeitFehler("'pop' auf leerer Liste", zeile, spalte);
        Wert letzter = liste->elemente.back();
        liste->elemente.pop_back();
        return letzter;
    }
    if (methode == "umkehren") {
        if (!args.empty())
            throw LaufzeitFehler("'umkehren' erwartet keine Argumente", zeile, spalte);
        std::reverse(liste->elemente.begin(), liste->elemente.end());
        return NullWert{};
    }
    if (methode == "sortieren") {
        if (!args.empty())
            throw LaufzeitFehler("'sortieren' erwartet keine Argumente", zeile, spalte);
        if (liste->elemente.empty()) return NullWert{};
        bool alle_zahlen = true, alle_texte = true;
        for (auto& e : liste->elemente) {
            if (!std::holds_alternative<double>(e))      alle_zahlen = false;
            if (!std::holds_alternative<std::string>(e)) alle_texte  = false;
        }
        if (alle_zahlen) {
            std::sort(liste->elemente.begin(), liste->elemente.end(),
                [](const Wert& a, const Wert& b){ return std::get<double>(a) < std::get<double>(b); });
        } else if (alle_texte) {
            std::sort(liste->elemente.begin(), liste->elemente.end(),
                [](const Wert& a, const Wert& b){ return std::get<std::string>(a) < std::get<std::string>(b); });
        } else {
            throw LaufzeitFehler("'sortieren': Liste muss nur Zahlen oder nur Zeichenketten enthalten", zeile, spalte);
        }
        return NullWert{};
    }
    throw LaufzeitFehler("Unbekannte Listenmethode '" + methode + "'", zeile, spalte);
}
