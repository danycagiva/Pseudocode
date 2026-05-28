# PseudoCode – Sprachreferenz

Vollständige Referenz aller Sprachkonstrukte.

---

## Kommentare

```pseudocode
// Einzeiliger Kommentar
```

---

## Variablen

```pseudocode
x     := 42
pi    := 3.14
name  := "Hallo"
aktiv := wahr
leer  := nichts    // kein Wert / null
```

Jede Variable wird beim ersten `:=` automatisch erstellt. Es gibt keine Typdeklaration.

---

## Datentypen

| Typ | Beispiel | `datentyp()` |
|---|---|---|
| Zahl | `42`, `3.14`, `-7` | `"zahl"` |
| Zeichenkette | `"Hallo"` | `"zeichenkette"` |
| Wahrheitswert | `wahr`, `falsch` | `"wahrheitswert"` |
| Liste | `liste(1, 2, 3)` | `"liste"` |
| Nichts | `nichts` | `"nichts"` |

```pseudocode
ausgabe(datentyp(42))          // zahl
ausgabe(datentyp("Hallo"))     // zeichenkette
ausgabe(datentyp(wahr))        // wahrheitswert
ausgabe(datentyp(liste()))     // liste
```

---

## Operatoren

### Arithmetik

| Operator | Bedeutung |
|---|---|
| `+` | Addition (auch Zeichenketten-Verkettung) |
| `-` | Subtraktion |
| `*` | Multiplikation |
| `/` | Division |
| `%` | Modulo |

### Vergleich

| Operator | Bedeutung |
|---|---|
| `=` | Gleich |
| `!=` | Ungleich |
| `<` | Kleiner |
| `>` | Größer |
| `<=` | Kleiner oder gleich |
| `>=` | Größer oder gleich |

### Logik

| Operator | Bedeutung |
|---|---|
| `und` | Logisches UND |
| `oder` | Logisches ODER |
| `nicht` | Logisches NICHT |

### Inkrement / Dekrement

```pseudocode
i++    // i := i + 1
i--    // i := i - 1
```

---

## Bedingungen

```pseudocode
wenn <bedingung> dann
    // wenn-Zweig
sonstwenn <bedingung> dann
    // optionaler sonstwenn-Zweig (beliebig viele)
sonst
    // optionaler sonst-Zweig
endewenn
```

Beispiel:

```pseudocode
note := 85

wenn note >= 90 dann
    ausgabe("Sehr gut")
sonstwenn note >= 75 dann
    ausgabe("Gut")
sonstwenn note >= 60 dann
    ausgabe("Befriedigend")
sonst
    ausgabe("Nicht bestanden")
endewenn
```

---

## Schleifen

### Solange-Schleife (while)

```pseudocode
solange <bedingung> tue
    // Schleifenkörper
endesolange
```

```pseudocode
i := 1
solange i <= 5 tue
    ausgabe(i)
    i := i + 1
endesolange
```

### Fuer-Schleife – C-Stil (for)

```pseudocode
fuer <init>; <bedingung>; <schritt> tue
    // Schleifenkörper
endefuer
```

```pseudocode
fuer i := 0; i < 10; i++ tue
    ausgabe(i)
endefuer
```

### Fuer-Schleife – Bereich (range)

```pseudocode
fuer <variable> von <start> bis <ende> tue
    // Schleifenkörper
endefuer

fuer <variable> von <start> bis <ende> schrittweite <schritt> tue
    // Schleifenkörper
endefuer
```

```pseudocode
fuer i von 1 bis 5 tue
    ausgabe(i)    // 1 2 3 4 (Ende exklusiv)
endefuer

fuer i von 0 bis 20 schrittweite 5 tue
    ausgabe(i)    // 0 5 10 15
endefuer
```

---

## Funktionen

```pseudocode
funktion <name>(<parameter1>, <parameter2>, …)
    // Funktionskörper
    rueckgabe <wert>
endefunktion
```

```pseudocode
funktion potenz(basis, exponent)
    ergebnis := 1
    fuer i := 0; i < exponent; i++ tue
        ergebnis := ergebnis * basis
    endefuer
    rueckgabe ergebnis
endefunktion

ausgabe(potenz(2, 8))   // 256
```

- Funktionen ohne `rueckgabe` geben `nichts` zurück.
- Rekursion wird unterstützt.

---

## Listen

### Erstellen

```pseudocode
leer    := liste()
zahlen  := liste(1, 2, 3, 4, 5)
gemischt := liste("a", 1, wahr)
```

### Zugriff & Zuweisung

```pseudocode
ausgabe(zahlen[0])      // erstes Element (Index 0)
zahlen[0] := 99         // Element schreiben
```

Negativer Index: `zahlen[-1]` liefert das letzte Element.

### Methoden

| Methode | Beschreibung |
|---|---|
| `liste.hinzufuegen(wert)` | Element ans Ende anfügen |
| `liste.entfernen(index)` | Element an Position entfernen |
| `liste.einfuegen(index, wert)` | Element an Position einfügen |
| `liste.pop()` | Letztes Element entfernen und zurückgeben |
| `liste.sortieren()` | Aufsteigend sortieren (nur Zahlen oder nur Zeichenketten) |
| `liste.umkehren()` | Reihenfolge umkehren |

### 2D-Listen

```pseudocode
matrix := liste(
    liste(1, 2, 3),
    liste(4, 5, 6),
    liste(7, 8, 9)
)

ausgabe(matrix[1][2])    // 6
matrix[0][0] := 99
```

### Dynamisch aufgebaut

```pseudocode
tabelle := liste()
fuer zeile von 0 bis 3 schrittweite 1 tue
    reihe := liste()
    fuer spalte von 0 bis 4 schrittweite 1 tue
        reihe.hinzufuegen(zeile * 10 + spalte)
    endefuer
    tabelle.hinzufuegen(reihe)
endefuer
ausgabe(text(tabelle))
```

---

## Ein- und Ausgabe

```pseudocode
ausgabe("Hallo")           // Ausgabe ohne Prompt-Zeilenumbruch
eingabe()                  // Liest eine Zeile → Zeichenkette
eingabe("Dein Name: ")     // Mit Prompt-Text
```

Zahlen aus Eingaben müssen explizit umgewandelt werden:

```pseudocode
eingabe_text := eingabe("Zahl eingeben: ")
n := zahl(eingabe_text)
ausgabe(n * 2)
```

---

## Eingebaute Funktionen

| Funktion | Signatur | Beschreibung |
|---|---|---|
| `ausgabe` | `ausgabe(wert)` | Wert ausgeben (mit Zeilenumbruch) |
| `eingabe` | `eingabe(prompt?)` → Zeichenkette | Konsoleneingabe |
| `text` | `text(wert)` → Zeichenkette | Wert als Zeichenkette |
| `zahl` | `zahl(wert)` → Zahl | In Zahl umwandeln |
| `datentyp` | `datentyp(wert)` → Zeichenkette | Typname |
| `laenge` | `laenge(wert)` → Zahl | Länge von Zeichenkette oder Liste |
| `abs` | `abs(n)` → Zahl | Absolutwert |
| `runden` | `runden(n)` → Zahl | Runden (kaufmännisch) |
| `boden` | `boden(n)` → Zahl | Abrunden |
| `decke` | `decke(n)` → Zahl | Aufrunden |
| `wurzel` | `wurzel(n)` → Zahl | Quadratwurzel (n ≥ 0) |

---

## Fehlermeldungen

| Fehlertyp | Auslöser |
|---|---|
| `Lexer-Fehler` | Unbekanntes Zeichen |
| `Parse-Fehler` | Ungültige Syntax |
| `Laufzeit-Fehler` | Division durch 0, unbekannte Variable, Typfehler, Index außerhalb des Bereichs |

Format (VS Code-kompatibel):
```
datei.pseudo:5:3: Fehler: Unbekannte Variable 'x'
```
