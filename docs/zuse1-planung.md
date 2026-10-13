# Zuse 1 – Computertechnik fuer Einsteiger (Entwurf)

Stand: 13.10.2026, erster Entwurf. Zuse 1 ist ein eigener Band der Reihe (Edition `zuse1`, docs/architecture.md)
aus derselben Codebasis wie Volta. Begonnen wird erst nach der Veroeffentlichung von Volta 1 (docs/roadmap.md).

## Leitidee
Vom Schalter zum eigenen Rechner: Kinder bauen Schritt fuer Schritt aus einfachen Verknuepfungen Speicher,
Zaehler und Rechenwerk – und am Ende den **kleinsten denkbaren Rechenautomaten** (Speicher, Befehlszaehler,
Befehlsregister, Befehlsdecoder, Multiplexer, Rechenwerk), der ein selbst eingegebenes Programm im Einzeltakt
ausfuehrt. Konrad Zuse und seine Z1–Z3 sind das Geschichtsfenster des Bandes: die Idee, den Ablauf der
Rechenschritte selbst zu speichern.

## Quelle und Umgang damit
Grundlage der Gliederung ist der Lehrgang »Digitaltechnik im Experiment – Lehrgang Elektronische
Datenverarbeitung« (H. Weidner, ELRAD 11/77–05/82, Sonderheft Heise 2005; private Sammlung des Inhabers, nicht
im Repository). Er fuehrt in zehn Teilen von UND/ODER bis zum Konzept eines Rechners, jeweils mit Versuchen auf
steckbaren Experimentierbrettern (Eingabe mit Schaltern und Tastern, Anzeige mit Laempchen, Bausteinbretter).
Uebernommen werden nur **Prinzipien und Reihenfolge**; Texte, Abbildungen, Brett-Layouts und Aufgaben werden
nicht nachgebildet, Experimente sind Eigenentwicklungen (CLAUDE.md, Regel 2). Ausgeschlossen ist das Kapitel
zum Experimentiernetzteil mit Netztrafo (Regel 6): die 5 V kommen aus dem Pult.

## Darstellung (Vorschlag)
- Grundplatte und Kontaktfelder wie in Volta; dazu **Bausteine** in der Modulleiste: Gatterbausteine (4 x NAND,
  4 x NOR, 6 x NICHT …), Flipflops, Zaehler, Decoder, Siebensegmentanzeige, spaeter Rechenwerk und Speicher.
  Anschluesse mit Nummer und Kurzname wie beim 555 in Volta 2.
- **Eingabeleiste** im Pult: Schalter H/L, entprellte Taster, Taktgeber (Einzeltakt per Taste, langsam,
  schnell), Ruecksetzen. **Anzeigeleiste**: Laempchen/LEDs, die H und L zusaetzlich mit 1/0 beschriften
  (Farbe nie allein).
- Pegel H/L an jedem Feld sichtbar (statt der Spannungsbalken), Leitungen mit H-Pegel hervorgehoben.
- »Blick hinein«: ein NAND aus Transistoren (Bruecke zu Volta 1/2), ein Flipflop aus zwei NAND, die
  Volladdierer im Rechenwerk.

## Stufen (Entwurf)
| Stufe | Thema | Inhalte (Auswahl) |
|---|---|---|
| 1 | Zwei Zustaende | Schalter und Laempchen, H/L und 1/0, UND als Reihen-, ODER als Parallelschaltung (Bruecke zu Volta 1), NICHT |
| 2 | Gatter | Wahrheitstabellen selbst aufnehmen (Forscherheft), NAND als Allzweckbaustein, alle Verknuepfungen aus NAND, Exklusiv-ODER |
| 3 | Speichern | RS-Flipflop aus zwei NAND, Taster prellt (ehrliche Physik: Zaehler springt mehrfach) und Entprellen, getaktetes, D- und JK-Flipflop |
| 4 | Zaehlen | Dualzahlen, Zaehler aus Flipflops, Teiler durch 2, 4, 8 …, Dezimalzaehler mit Ruecksetzen, Sekundentakt und kleine Uhr, Zaehler mit einstellbarem Endwert |
| 5 | Codieren und Anzeigen | Dezimal-Dual-Codierer, Decoder, Siebensegmentanzeige, Vergleicher (gleich, groesser, kleiner), Vereinfachen von Schaltungen (Rechenregeln der Logik) |
| 6 | Rechnen | Halb- und Volladdierer, 4-bit-Addierer, Subtrahieren, Schieberegister, serielles Addieren mit Akkumulator |
| 7 | Das Rechenwerk | Rechenwerk mit Steuerleitungen (logische und arithmetische Funktionen), Akkumulator, Zwischenspeicher |
| 8 | Der eigene Rechner | Speicher mit Adressen (Lesen/Schreiben), Befehlszaehler, Befehlsregister und -decoder, Programme im Einzeltakt: addieren, zaehlen, Lauflicht; Zuse und die Z3 |

## Simulation
- **Logikmodell** im Kern (docs/simulation-core.md): ereignisgesteuerte Gatter mit Laufzeit, Kopplung an
  analoge Pegel dort, wo Volta-Bauteile (Taster, LED, Transistor) angeschlossen sind.
- Ehrliche Physik auch digital: prellende Taster, offene Eingaenge (Verhalten wie TTL: offen = H, im Lexikon
  erklaert), Ueberlast eines Ausgangs, kurze Stoerimpulse durch Laufzeiten (»Glitch«) im Zeitdiagramm sichtbar.
- Versorgung 5 V aus dem Pult; Bausteine ohne Versorgung arbeiten nicht.

## Werkzeugkasten (Vorschlag)
Wahrheitstabelle (Aufnahme per Knopf), Zahlensysteme (dual, dezimal, hexadezimal), Zeitdiagramm/Logikanalysator
(Nachfolger des Zeitschreibers), Befehlstabelle des eigenen Rechners, Lexikon der Bausteine, Fehlersuche.

## Offene Entscheidungen
| Entscheidung | Spaetestens |
|---|---|
| Altersgruppe (Vorschlag 11–15) und Stellung in der Reihe (nach Volta 2 oder 3) | vor Planungsbeginn |
| Bausteine mit echten Typnummern (7400, 7490 …) oder neutralen Namen (Gatterbaustein A …) | Stufenplanung |
| Name »Zuse«: Markenrecherche (docs/privacy-safety.md) und Namensrecht (Einverstaendnis der Erben bzw. der Zuse-Gesellschaft klaeren) | vor Planungsbeginn |
| Eigenes Bedienpult (Eingabe-/Anzeigeleiste) oder Volta-Pult mit Erweiterung | Darstellung |
