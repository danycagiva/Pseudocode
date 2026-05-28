#include "parser.hpp"
#include "fehler.hpp"
#include <cassert>
#include <stdexcept>

// ------------------------------------------------------------------ //
// Hilfsmethoden                                                       //
// ------------------------------------------------------------------ //

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

const Token& Parser::aktuell() const {
    return tokens_[pos_];
}

const Token& Parser::voraus(int offset) const {
    size_t idx = pos_ + offset;
    if (idx >= tokens_.size()) return tokens_.back(); // DATEIENDE
    return tokens_[idx];
}

bool Parser::stimmt_ueberein(TokenTyp typ) const {
    return aktuell().typ == typ;
}

bool Parser::ist_ende() const {
    return aktuell().typ == TokenTyp::DATEIENDE;
}

Token Parser::verbrauche(TokenTyp erwartet, const std::string& fehlermeldung) {
    if (!stimmt_ueberein(erwartet))
        throw ParseFehler(fehlermeldung, aktuell().zeile, aktuell().spalte);
    return tokens_[pos_++];
}

Token Parser::verbrauche_beliebig() {
    return tokens_[pos_++];
}

// ------------------------------------------------------------------ //
// Einstiegspunkt                                                      //
// ------------------------------------------------------------------ //

std::shared_ptr<Programm> Parser::parse() {
    auto prog = std::make_shared<Programm>();
    prog->zeile = 1; prog->spalte = 1;
    while (!ist_ende())
        prog->anweisungen.push_back(parse_anweisung());
    return prog;
}

// ------------------------------------------------------------------ //
// Block-Parsing                                                       //
// ------------------------------------------------------------------ //

std::vector<KnotenPtr> Parser::parse_block(std::initializer_list<TokenTyp> stop_tokens) {
    std::vector<KnotenPtr> block;
    while (!ist_ende()) {
        for (auto s : stop_tokens)
            if (stimmt_ueberein(s)) return block;
        block.push_back(parse_anweisung());
    }
    return block;
}

// ------------------------------------------------------------------ //
// Anweisungen                                                         //
// ------------------------------------------------------------------ //

KnotenPtr Parser::parse_anweisung() {
    // Optionales 'sei' ignorieren
    if (stimmt_ueberein(TokenTyp::SEI))
        verbrauche_beliebig();

    const Token& tok = aktuell();

    if (tok.typ == TokenTyp::WENN)      return parse_wenn();
    if (tok.typ == TokenTyp::SOLANGE)   return parse_solange();
    if (tok.typ == TokenTyp::FUER)      return parse_fuer();
    if (tok.typ == TokenTyp::FUNKTION)  return parse_funktion();
    if (tok.typ == TokenTyp::RUECKGABE) return parse_rueckgabe();
    if (tok.typ == TokenTyp::AUSGABE)   return parse_ausgabe();

    // Ausdruck parsen – danach prüfen ob := folgt (Zuweisung)
    auto expr = parse_ausdruck();

    if (stimmt_ueberein(TokenTyp::ZUWEISUNG)) {
        verbrauche_beliebig(); // :=
        auto wert_expr = parse_ausdruck();

        if (auto bez = std::dynamic_pointer_cast<Bezeichner>(expr)) {
            auto knoten = std::make_shared<Zuweisung>();
            knoten->zeile = bez->zeile; knoten->spalte = bez->spalte;
            knoten->ziel = bez->name;
            knoten->ausdruck = wert_expr;
            return knoten;
        }
        if (auto iz = std::dynamic_pointer_cast<IndexZugriff>(expr)) {
            auto knoten = std::make_shared<IndexZuweisung>();
            knoten->zeile = iz->zeile; knoten->spalte = iz->spalte;
            knoten->objekt = iz->objekt;
            knoten->index  = iz->index;
            knoten->wert   = wert_expr;
            return knoten;
        }
        throw ParseFehler(
            "Ungueltige Zuweisung: linke Seite muss eine Variable oder ein Listenelement sein",
            aktuell().zeile, aktuell().spalte);
    }

    return expr;
}

KnotenPtr Parser::parse_wenn() {
    auto knoten = std::make_shared<WennAnweisung>();
    knoten->zeile = aktuell().zeile; knoten->spalte = aktuell().spalte;

    verbrauche(TokenTyp::WENN, "Erwartet 'wenn'");
    knoten->bedingung = parse_ausdruck();
    verbrauche(TokenTyp::DANN, "Erwartet 'dann' nach Bedingung");

    knoten->dann_block = parse_block({
        TokenTyp::SONSTWENN, TokenTyp::SONST, TokenTyp::ENDE
    });

    while (stimmt_ueberein(TokenTyp::SONSTWENN)) {
        verbrauche_beliebig(); // sonstwenn
        SonstwennZweig zweig;
        zweig.bedingung = parse_ausdruck();
        verbrauche(TokenTyp::DANN, "Erwartet 'dann' nach 'sonstwenn'-Bedingung");
        zweig.koerper = parse_block({
            TokenTyp::SONSTWENN, TokenTyp::SONST, TokenTyp::ENDE
        });
        knoten->sonstwenn_zweige.push_back(std::move(zweig));
    }

    if (stimmt_ueberein(TokenTyp::SONST)) {
        verbrauche_beliebig(); // sonst
        knoten->sonst_block = parse_block({TokenTyp::ENDE});
    }

    verbrauche(TokenTyp::ENDE, "Erwartet 'ende' zum Abschluss von 'wenn'");
    return knoten;
}

KnotenPtr Parser::parse_solange() {
    auto knoten = std::make_shared<SolangeSchleife>();
    knoten->zeile = aktuell().zeile; knoten->spalte = aktuell().spalte;

    verbrauche(TokenTyp::SOLANGE, "Erwartet 'solange'");
    knoten->bedingung = parse_ausdruck();
    verbrauche(TokenTyp::TUE, "Erwartet 'tue' nach Schleifenbedingung");
    knoten->koerper = parse_block({TokenTyp::ENDE});
    verbrauche(TokenTyp::ENDE, "Erwartet 'ende' zum Abschluss von 'solange'");
    return knoten;
}

KnotenPtr Parser::parse_fuer() {
    int fz = aktuell().zeile, fs = aktuell().spalte;
    verbrauche(TokenTyp::FUER, "Erwartet 'fuer'");

    Token var_tok = verbrauche(TokenTyp::BEZEICHNER, "Erwartet Laufvariable nach 'fuer'");

    // C-Stil: fuer i := start; bedingung; schritt tue ... ende
    if (stimmt_ueberein(TokenTyp::ZUWEISUNG)) {
        verbrauche_beliebig(); // :=
        auto init_wert = parse_ausdruck();

        auto init = std::make_shared<Zuweisung>();
        init->zeile = var_tok.zeile; init->spalte = var_tok.spalte;
        init->ziel = var_tok.wert;
        init->ausdruck = init_wert;

        verbrauche(TokenTyp::STRICHPUNKT, "Erwartet ';' nach Startwert in 'fuer'-Schleife");
        auto bedingung = parse_ausdruck();
        verbrauche(TokenTyp::STRICHPUNKT, "Erwartet ';' nach Bedingung in 'fuer'-Schleife");

        // Schritt: i++, i--, oder i := i + 1
        KnotenPtr schritt;
        if (stimmt_ueberein(TokenTyp::BEZEICHNER)) {
            Token s_var = verbrauche_beliebig();
            if (stimmt_ueberein(TokenTyp::PLUS_PLUS) || stimmt_ueberein(TokenTyp::MINUS_MINUS)) {
                bool plus = stimmt_ueberein(TokenTyp::PLUS_PLUS);
                verbrauche_beliebig();
                // i++ → i := i + 1
                auto eins = std::make_shared<ZahlLiteral>();
                eins->wert = 1.0; eins->zeile = s_var.zeile; eins->spalte = s_var.spalte;
                auto var_ref = std::make_shared<Bezeichner>();
                var_ref->name = s_var.wert; var_ref->zeile = s_var.zeile; var_ref->spalte = s_var.spalte;
                auto binaer = std::make_shared<BinaerAusdruck>();
                binaer->op = plus ? "+" : "-";
                binaer->links = var_ref; binaer->rechts = eins;
                binaer->zeile = s_var.zeile; binaer->spalte = s_var.spalte;
                auto zuw = std::make_shared<Zuweisung>();
                zuw->ziel = s_var.wert; zuw->ausdruck = binaer;
                zuw->zeile = s_var.zeile; zuw->spalte = s_var.spalte;
                schritt = zuw;
            } else if (stimmt_ueberein(TokenTyp::ZUWEISUNG)) {
                verbrauche_beliebig(); // :=
                auto schritt_ausdruck = parse_ausdruck();
                auto zuw = std::make_shared<Zuweisung>();
                zuw->ziel = s_var.wert; zuw->ausdruck = schritt_ausdruck;
                zuw->zeile = s_var.zeile; zuw->spalte = s_var.spalte;
                schritt = zuw;
            } else {
                throw ParseFehler("Ungueltige Schrittangabe in 'fuer'-Schleife",
                                  s_var.zeile, s_var.spalte);
            }
        } else {
            throw ParseFehler("Erwartet Schrittanweisung (z.B. i++) in 'fuer'-Schleife",
                              aktuell().zeile, aktuell().spalte);
        }

        verbrauche(TokenTyp::TUE, "Erwartet 'tue' nach Schrittangabe");
        auto koerper = parse_block({TokenTyp::ENDE});
        verbrauche(TokenTyp::ENDE, "Erwartet 'ende' zum Abschluss von 'fuer'");

        auto knoten = std::make_shared<FuerSchleife>();
        knoten->zeile = fz; knoten->spalte = fs;
        knoten->init = init;
        knoten->bedingung = bedingung;
        knoten->schritt = schritt;
        knoten->koerper = std::move(koerper);
        return knoten;
    }

    // Bereichs-Stil: fuer i von start bis ende [schrittweite n] tue ... ende
    if (stimmt_ueberein(TokenTyp::VON)) {
        verbrauche_beliebig(); // von
        auto start = parse_ausdruck();
        verbrauche(TokenTyp::BIS, "Erwartet 'bis' nach Startwert");
        auto ende = parse_ausdruck();

        KnotenPtr schrittweite;
        if (stimmt_ueberein(TokenTyp::SCHRITTWEITE)) {
            verbrauche_beliebig();
            schrittweite = parse_ausdruck();
        }

        verbrauche(TokenTyp::TUE, "Erwartet 'tue' nach Bereichsangabe");
        auto koerper = parse_block({TokenTyp::ENDE});
        verbrauche(TokenTyp::ENDE, "Erwartet 'ende' zum Abschluss von 'fuer'");

        auto knoten = std::make_shared<BereichsSchleife>();
        knoten->zeile = fz; knoten->spalte = fs;
        knoten->variable = var_tok.wert;
        knoten->start = start;
        knoten->ende = ende;
        knoten->schrittweite = schrittweite;
        knoten->koerper = std::move(koerper);
        return knoten;
    }

    throw ParseFehler(
        "Erwartet ':=' (C-Stil) oder 'von' (Bereich) nach Laufvariable in 'fuer'",
        var_tok.zeile, var_tok.spalte);
}

KnotenPtr Parser::parse_funktion() {
    int fz = aktuell().zeile, fs = aktuell().spalte;
    verbrauche(TokenTyp::FUNKTION, "Erwartet 'funktion'");
    Token name_tok = verbrauche(TokenTyp::BEZEICHNER, "Erwartet Funktionsname");
    verbrauche(TokenTyp::LPAREN, "Erwartet '(' nach Funktionsname");

    std::vector<std::string> parameter;
    while (!stimmt_ueberein(TokenTyp::RPAREN) && !ist_ende()) {
        Token p = verbrauche(TokenTyp::BEZEICHNER, "Erwartet Parametername");
        parameter.push_back(p.wert);
        if (stimmt_ueberein(TokenTyp::KOMMA)) verbrauche_beliebig();
    }
    verbrauche(TokenTyp::RPAREN, "Erwartet ')' nach Parameterliste");

    auto koerper = parse_block({TokenTyp::ENDE});
    verbrauche(TokenTyp::ENDE, "Erwartet 'ende' zum Abschluss von 'funktion'");

    auto knoten = std::make_shared<FunktionsDefinition>();
    knoten->zeile = fz; knoten->spalte = fs;
    knoten->name = name_tok.wert;
    knoten->parameter = std::move(parameter);
    knoten->koerper = std::move(koerper);
    return knoten;
}

KnotenPtr Parser::parse_rueckgabe() {
    int rz = aktuell().zeile, rs = aktuell().spalte;
    verbrauche(TokenTyp::RUECKGABE, "Erwartet 'rueckgabe'");

    auto knoten = std::make_shared<RueckgabeAnweisung>();
    knoten->zeile = rz; knoten->spalte = rs;

    // Leere Rueckgabe, wenn naechstes Token ein Block-Abschluss oder Dateiende ist
    bool leer = ist_ende()
        || stimmt_ueberein(TokenTyp::ENDE)
        || stimmt_ueberein(TokenTyp::SONST)
        || stimmt_ueberein(TokenTyp::SONSTWENN);
    if (!leer)
        knoten->ausdruck = parse_ausdruck();

    return knoten;
}

KnotenPtr Parser::parse_ausgabe() {
    int az = aktuell().zeile, as_ = aktuell().spalte;
    verbrauche(TokenTyp::AUSGABE, "Erwartet 'ausgabe'");
    verbrauche(TokenTyp::LPAREN, "Erwartet '(' nach 'ausgabe'");
    auto ausdruck = parse_ausdruck();
    verbrauche(TokenTyp::RPAREN, "Erwartet ')' nach Ausgabeausdruck");

    auto knoten = std::make_shared<AusgabeAnweisung>();
    knoten->zeile = az; knoten->spalte = as_;
    knoten->ausdruck = ausdruck;
    return knoten;
}

KnotenPtr Parser::parse_zuweisung(const std::string& name, int zeile, int spalte) {
    auto ausdruck = parse_ausdruck();
    auto knoten = std::make_shared<Zuweisung>();
    knoten->zeile = zeile; knoten->spalte = spalte;
    knoten->ziel = name;
    knoten->ausdruck = ausdruck;
    return knoten;
}

// ------------------------------------------------------------------ //
// Ausdruck-Grammatik (Praezedenzleiter)                              //
// ------------------------------------------------------------------ //

KnotenPtr Parser::parse_ausdruck() { return parse_oder(); }

KnotenPtr Parser::parse_oder() {
    auto links = parse_und();
    while (stimmt_ueberein(TokenTyp::ODER)) {
        auto tok = verbrauche_beliebig();
        auto rechts = parse_und();
        auto knoten = std::make_shared<BinaerAusdruck>();
        knoten->op = "oder"; knoten->links = links; knoten->rechts = rechts;
        knoten->zeile = tok.zeile; knoten->spalte = tok.spalte;
        links = knoten;
    }
    return links;
}

KnotenPtr Parser::parse_und() {
    auto links = parse_nicht();
    while (stimmt_ueberein(TokenTyp::UND)) {
        auto tok = verbrauche_beliebig();
        auto rechts = parse_nicht();
        auto knoten = std::make_shared<BinaerAusdruck>();
        knoten->op = "und"; knoten->links = links; knoten->rechts = rechts;
        knoten->zeile = tok.zeile; knoten->spalte = tok.spalte;
        links = knoten;
    }
    return links;
}

KnotenPtr Parser::parse_nicht() {
    if (stimmt_ueberein(TokenTyp::NICHT)) {
        auto tok = verbrauche_beliebig();
        auto knoten = std::make_shared<UnaerAusdruck>();
        knoten->op = "nicht"; knoten->operand = parse_nicht();
        knoten->zeile = tok.zeile; knoten->spalte = tok.spalte;
        return knoten;
    }
    return parse_vergleich();
}

KnotenPtr Parser::parse_vergleich() {
    auto links = parse_addition();
    while (true) {
        std::string op;
        if      (stimmt_ueberein(TokenTyp::GLEICH))          op = "=";
        else if (stimmt_ueberein(TokenTyp::UNGLEICH))        op = "!=";
        else if (stimmt_ueberein(TokenTyp::KLEINER))         op = "<";
        else if (stimmt_ueberein(TokenTyp::GROESSER))        op = ">";
        else if (stimmt_ueberein(TokenTyp::KLEINER_GLEICH))  op = "<=";
        else if (stimmt_ueberein(TokenTyp::GROESSER_GLEICH)) op = ">=";
        else break;

        auto tok = verbrauche_beliebig();
        auto rechts = parse_addition();
        auto knoten = std::make_shared<BinaerAusdruck>();
        knoten->op = op; knoten->links = links; knoten->rechts = rechts;
        knoten->zeile = tok.zeile; knoten->spalte = tok.spalte;
        links = knoten;
    }
    return links;
}

KnotenPtr Parser::parse_addition() {
    auto links = parse_multiplikation();
    while (stimmt_ueberein(TokenTyp::PLUS) || stimmt_ueberein(TokenTyp::MINUS)) {
        auto tok = verbrauche_beliebig();
        auto rechts = parse_multiplikation();
        auto knoten = std::make_shared<BinaerAusdruck>();
        knoten->op = tok.wert; knoten->links = links; knoten->rechts = rechts;
        knoten->zeile = tok.zeile; knoten->spalte = tok.spalte;
        links = knoten;
    }
    return links;
}

KnotenPtr Parser::parse_multiplikation() {
    auto links = parse_unaer();
    while (stimmt_ueberein(TokenTyp::STERN)
        || stimmt_ueberein(TokenTyp::SLASH)
        || stimmt_ueberein(TokenTyp::MODULO)) {
        auto tok = verbrauche_beliebig();
        auto rechts = parse_unaer();
        auto knoten = std::make_shared<BinaerAusdruck>();
        knoten->op = tok.wert; knoten->links = links; knoten->rechts = rechts;
        knoten->zeile = tok.zeile; knoten->spalte = tok.spalte;
        links = knoten;
    }
    return links;
}

KnotenPtr Parser::parse_unaer() {
    if (stimmt_ueberein(TokenTyp::MINUS)) {
        auto tok = verbrauche_beliebig();
        auto knoten = std::make_shared<UnaerAusdruck>();
        knoten->op = "-"; knoten->operand = parse_unaer();
        knoten->zeile = tok.zeile; knoten->spalte = tok.spalte;
        return knoten;
    }
    if (stimmt_ueberein(TokenTyp::NICHT)) {
        auto tok = verbrauche_beliebig();
        auto knoten = std::make_shared<UnaerAusdruck>();
        knoten->op = "nicht"; knoten->operand = parse_unaer();
        knoten->zeile = tok.zeile; knoten->spalte = tok.spalte;
        return knoten;
    }
    return parse_primaer();
}

KnotenPtr Parser::parse_primaer() {
    const Token& tok = aktuell();
    KnotenPtr result;

    // Zahl
    if (stimmt_ueberein(TokenTyp::ZAHL)) {
        verbrauche_beliebig();
        auto knoten = std::make_shared<ZahlLiteral>();
        knoten->wert = std::stod(tok.wert);
        knoten->zeile = tok.zeile; knoten->spalte = tok.spalte;
        result = knoten;
    }
    // Zeichenkette
    else if (stimmt_ueberein(TokenTyp::ZEICHENKETTE)) {
        verbrauche_beliebig();
        auto knoten = std::make_shared<ZeichenkettenLiteral>();
        knoten->wert = tok.wert;
        knoten->zeile = tok.zeile; knoten->spalte = tok.spalte;
        result = knoten;
    }
    // Wahrheitswerte
    else if (stimmt_ueberein(TokenTyp::WAHR)) {
        verbrauche_beliebig();
        auto knoten = std::make_shared<WahrheitsLiteral>();
        knoten->wert = true;
        knoten->zeile = tok.zeile; knoten->spalte = tok.spalte;
        result = knoten;
    }
    else if (stimmt_ueberein(TokenTyp::FALSCH)) {
        verbrauche_beliebig();
        auto knoten = std::make_shared<WahrheitsLiteral>();
        knoten->wert = false;
        knoten->zeile = tok.zeile; knoten->spalte = tok.spalte;
        result = knoten;
    }
    // Listen-Literal: liste(elem1, elem2, ...)
    else if (stimmt_ueberein(TokenTyp::LISTE)) {
        auto liste_tok = verbrauche_beliebig();
        verbrauche(TokenTyp::LPAREN, "Erwartet '(' nach 'liste'");
        auto knoten = std::make_shared<ListenLiteral>();
        knoten->zeile = liste_tok.zeile; knoten->spalte = liste_tok.spalte;
        while (!stimmt_ueberein(TokenTyp::RPAREN) && !ist_ende()) {
            knoten->elemente.push_back(parse_ausdruck());
            if (stimmt_ueberein(TokenTyp::KOMMA)) verbrauche_beliebig();
        }
        verbrauche(TokenTyp::RPAREN, "Erwartet ')' nach Listen-Elementen");
        result = knoten;
    }
    // Bezeichner oder Funktionsaufruf
    else if (stimmt_ueberein(TokenTyp::BEZEICHNER)) {
        auto name_tok = verbrauche_beliebig();
        if (stimmt_ueberein(TokenTyp::LPAREN)) {
            verbrauche_beliebig(); // (
            std::vector<KnotenPtr> argumente;
            while (!stimmt_ueberein(TokenTyp::RPAREN) && !ist_ende()) {
                argumente.push_back(parse_ausdruck());
                if (stimmt_ueberein(TokenTyp::KOMMA)) verbrauche_beliebig();
            }
            verbrauche(TokenTyp::RPAREN, "Erwartet ')' nach Argumentliste");
            auto knoten = std::make_shared<FunktionsAufruf>();
            knoten->name = name_tok.wert;
            knoten->argumente = std::move(argumente);
            knoten->zeile = name_tok.zeile; knoten->spalte = name_tok.spalte;
            result = knoten;
        } else {
            auto knoten = std::make_shared<Bezeichner>();
            knoten->name = name_tok.wert;
            knoten->zeile = name_tok.zeile; knoten->spalte = name_tok.spalte;
            result = knoten;
        }
    }
    // Geklammert
    else if (stimmt_ueberein(TokenTyp::LPAREN)) {
        verbrauche_beliebig(); // (
        result = parse_ausdruck();
        verbrauche(TokenTyp::RPAREN, "Erwartet ')' nach Ausdruck");
    }
    else {
        throw ParseFehler(
            "Unerwartetes Token '" + tok.wert + "'",
            tok.zeile, tok.spalte);
    }

    // Postfix: Indexzugriff [expr] und Methodenaufruf .methode(args)
    while (stimmt_ueberein(TokenTyp::LBRACKET) || stimmt_ueberein(TokenTyp::PUNKT)) {
        if (stimmt_ueberein(TokenTyp::LBRACKET)) {
            auto bracket_tok = verbrauche_beliebig(); // [
            auto index = parse_ausdruck();
            verbrauche(TokenTyp::RBRACKET, "Erwartet ']' nach Index");
            auto knoten = std::make_shared<IndexZugriff>();
            knoten->zeile = bracket_tok.zeile; knoten->spalte = bracket_tok.spalte;
            knoten->objekt = result;
            knoten->index  = index;
            result = knoten;
        } else { // PUNKT
            auto punkt_tok = verbrauche_beliebig(); // .
            Token methode_tok = verbrauche(TokenTyp::BEZEICHNER, "Erwartet Methodenname nach '.'");
            verbrauche(TokenTyp::LPAREN, "Erwartet '(' nach Methodenname");
            std::vector<KnotenPtr> argumente;
            while (!stimmt_ueberein(TokenTyp::RPAREN) && !ist_ende()) {
                argumente.push_back(parse_ausdruck());
                if (stimmt_ueberein(TokenTyp::KOMMA)) verbrauche_beliebig();
            }
            verbrauche(TokenTyp::RPAREN, "Erwartet ')' nach Argumentliste");
            auto knoten = std::make_shared<MethodenAufruf>();
            knoten->zeile = punkt_tok.zeile; knoten->spalte = punkt_tok.spalte;
            knoten->objekt   = result;
            knoten->methode  = methode_tok.wert;
            knoten->argumente = std::move(argumente);
            result = knoten;
        }
    }

    return result;
}
