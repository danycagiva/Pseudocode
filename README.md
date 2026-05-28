# PseudoCode

Wer sich auf die Abschlussprüfung zum Fachinformatiker vorbereitet, kennt das Problem: In den schriftlichen Prüfungen werden Algorithmen in Pseudocode formuliert – und viele Azubis oder Umschüler stolpern genau da, weil sie nie wirklich damit geübt haben. Echte Programmiersprachen haben zu viel Overhead, zu viel Syntax, die vom eigentlichen Denken ablenkt.

PseudoCode ist eine kleine, interpretierte Programmiersprache mit komplett deutschen Schlüsselwörtern. Kein Semikolon-Stress, kein Typen-Wirrwarr. Einfach hinschreiben, was man meint – und es läuft.

```
Aufruf im Terminal: pseudocode datei.pseudo
```

- **Nativer C++17-Interpreter** – keine Laufzeitabhängigkeiten
- **Plattformübergreifend** – macOS, Linux, Windows (CMake)
- **VS Code Extension** – Syntax-Highlighting, Auto-Einrückung, Play-Button (F5)

---

## Installation

### Voraussetzungen

| Werkzeug | Mindestversion |
|---|---|
| CMake | 3.16 |
| C++-Compiler | C++17 (clang, gcc, MSVC) |

### Bauen & Installieren

```bash
git clone https://github.com/dein-nutzername/pseudocode-lang.git
cd pseudocode-lang
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build   # → /usr/local/bin/pseudocode
```

### VS Code Extension

```bash
cd vscode-extension
npm install
npx vsce package --no-dependencies
code --install-extension pseudocode-lang-*.vsix
```

---

## Schnellstart

```pseudocode
// hallo_welt.pseudo
ausgabe("Hallo, Welt!")

name := eingabe("Wie heißt du? ")
ausgabe("Hallo, " + name + "!")
```

```bash
pseudocode hallo_welt.pseudo
```

---

## Syntax

### Variablen & Ausgabe

```pseudocode
x    := 42
pi   := 3.14
name := "PseudoCode"

ausgabe(x)
ausgabe("Wert: " + text(x))
```

### Bedingungen

```pseudocode
wenn x > 10 dann
    ausgabe("groß")
sonstwenn x = 10 dann
    ausgabe("genau zehn")
sonst
    ausgabe("klein")
endewenn
```

### Schleifen

```pseudocode
// Solange-Schleife
solange x > 0 tue
    ausgabe(x)
    x := x - 1
endesolange

// Zählschleife (C-Stil)
fuer i := 0; i < 5; i++ tue
    ausgabe(i)
endefuer

// Bereichsschleife mit optionaler Schrittweite
fuer i von 1 bis 10 schrittweite 2 tue
    ausgabe(i)
endefuer
```

### Funktionen

```pseudocode
funktion addiere(a, b)
    rueckgabe a + b
endefunktion

ergebnis := addiere(3, 4)
ausgabe(ergebnis)   // 7
```

### Listen

```pseudocode
zahlen := liste(10, 3, 7)
zahlen.hinzufuegen(42)
zahlen[0] := 99
ausgabe(zahlen[0])          // 99
ausgabe(laenge(zahlen))     // 4
zahlen.sortieren()
letzter := zahlen.pop()
```

**Methoden:** `hinzufuegen(wert)`, `entfernen(index)`, `einfuegen(index, wert)`, `pop()`, `sortieren()`, `umkehren()`

### 2D-Listen (Matrizen)

```pseudocode
matrix := liste(
    liste(1, 2, 3),
    liste(4, 5, 6)
)
ausgabe(matrix[0][1])   // 2
matrix[1][2] := 99
```

---

## Eingebaute Funktionen

| Funktion | Rückgabe | Beschreibung |
|---|---|---|
| `ausgabe(wert)` | – | In die Konsole ausgeben |
| `eingabe(prompt?)` | Zeichenkette | Zeile von der Konsole lesen |
| `text(wert)` | Zeichenkette | Wert als Text |
| `zahl(wert)` | Zahl | Zeichenkette → Zahl |
| `datentyp(wert)` | Zeichenkette | `"zahl"` · `"zeichenkette"` · `"wahrheitswert"` · `"liste"` · `"nichts"` |
| `laenge(wert)` | Zahl | Länge einer Zeichenkette oder Liste |
| `abs(n)` | Zahl | Absolutwert |
| `runden(n)` | Zahl | Kaufmännisches Runden |
| `boden(n)` | Zahl | Abrunden (floor) |
| `decke(n)` | Zahl | Aufrunden (ceil) |
| `wurzel(n)` | Zahl | Quadratwurzel |

---

## Schlüsselwörter

| PseudoCode | Entsprechung | Hinweis |
|---|---|---|
| `:=` | `=` | Zuweisung |
| `=` | `==` | Gleichheitsvergleich |
| `!=` `<` `>` `<=` `>=` | identisch | Vergleichsoperatoren |
| `und` `oder` `nicht` | `&&` `\|\|` `!` | Logische Operatoren |
| `wenn … dann … endewenn` | `if` | |
| `sonstwenn … dann` | `else if` | |
| `sonst` | `else` | |
| `solange … tue … endesolange` | `while` | |
| `fuer … tue … endefuer` | `for` | C-Stil oder Bereich |
| `von` `bis` `schrittweite` | `range(start, stop, step)` | Bereichsschleife |
| `funktion … endefunktion` | `def` / `function` | |
| `rueckgabe` | `return` | |
| `liste(…)` | `[…]` | Liste erstellen |
| `wahr` `falsch` | `true` `false` | |

> `ende` funktioniert universell als Blockende.

---

## Projektstruktur

```
pseudocode-lang/
├── CMakeLists.txt
├── src/
│   ├── token.hpp               ← Token-Typen
│   ├── fehler.hpp              ← Fehlerklassen
│   ├── ast.hpp                 ← AST-Knotentypen
│   ├── lexer.hpp / .cpp        ← Quelltext → Token-Strom
│   ├── parser.hpp / .cpp       ← Token-Strom → AST
│   ├── interpreter.hpp / .cpp  ← Tree-Walking-Interpreter
│   └── main.cpp
├── beispiele/
│   ├── hallo_welt.pseudo
│   ├── schleifen.pseudo
│   ├── funktionen.pseudo
│   ├── fibonacci.pseudo
│   └── listen.pseudo
└── vscode-extension/
    ├── package.json
    ├── language-configuration.json
    ├── syntaxes/pseudocode.tmLanguage.json
    └── src/extension.ts
```

---

## Architektur

```
Quelldatei (.pseudo)
       │
       ▼
  ┌─────────┐
  │  Lexer  │  Quelltext → Token-Strom
  └─────────┘
       │
       ▼
  ┌────────────┐
  │   Parser   │  Token-Strom → AST (rekursiv-absteigend)
  └────────────┘
       │
       ▼
  ┌─────────────┐
  │ Interpreter │  Tree-Walking – kein Bytecode, keine VM
  └─────────────┘
```

**Werttypen zur Laufzeit:** `double`, `std::string`, `bool`, `NullWert`, `ListenPtr` (`std::shared_ptr<ListenWert>`)

---

## Fehlermeldungen

Fehler sind im VS Code-kompatiblen Format:

```
datei.pseudo:5:3: Fehler: Unbekannte Variable 'x'
```

---

## Lizenz

MIT
