# Zuse 1 – Computertechnik fuer Einsteiger (Entwurf)

Stand: 13.10.2026, erster Entwurf. Zuse 1 ist ein eigener Band der Reihe (Edition `zuse1`, docs/architecture.md)
aus derselben Codebasis wie Volta. Begonnen wird erst nach der Veroeffentlichung von Volta 1 (docs/roadmap.md);
das Konzept v3.0 (Abschnitt 4.3) sieht ein eigenes Konzeptdokument vor, dieser Entwurf ist dessen Vorstufe.
Eckdaten aus dem Konzept: 12–16 Jahre, Bit und Gatter bis zum selbst gebauten 4-Bit-Kleinstrechner,
Programmieren in Maschinensprache und mit Bloecken (Anbindung an den Blockeditor UBBE naheliegend), spaeter
Bruecke zu realen Mikrocontrollern (z. B. ESP32).

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
- Grundplatte und Kontaktfelder wie in Volta (fuer Einzelleitungen), Busbuchsen siehe Bus-Konzept; dazu **Bausteine** in der Modulleiste: Gatterbausteine (4 x NAND,
  4 x NOR, 6 x NICHT …), Flipflops, Zaehler, Decoder, Siebensegmentanzeige, spaeter Rechenwerk und Speicher.
  Anschluesse mit Nummer und Kurzname wie beim 555 in Volta 2.
- **Eingabeleiste** im Pult: Schalter H/L, entprellte Taster, Taktgeber (Einzeltakt per Taste, langsam,
  schnell), Ruecksetzen. **Anzeigeleiste**: Laempchen/LEDs, die H und L zusaetzlich mit 1/0 beschriften
  (Farbe nie allein).
- Pegel H/L an jedem Feld sichtbar (statt der Spannungsbalken), Leitungen mit H-Pegel hervorgehoben.
- »Blick hinein«: ein NAND aus Transistoren (Bruecke zu Volta 1/2), ein Flipflop aus zwei NAND, die
  Volladdierer im Rechenwerk.

## Bus-Konzept: Buendel statt Einzelleitungen
Die Versuchsaufbauten im Lehrgang zeigen das Problem deutlich: schon ein Zaehler mit Vergleich braucht Dutzende
Einzelkabel, die sich kreuzen und kaum noch nachzuverfolgen sind. Fuer Kinder ist das mehr Verwirrung als Lernen.
Darum ab der Stufe, in der mehrere Bits gemeinsam laufen, **Busse**:
- **Einzelleitungen nur am Anfang** (Stufen 1–3): ein Gatter, ein Flipflop – hier soll jede Verbindung
  sichtbar und einzeln gezogen werden, wie in Volta.
- **Bus ab Stufe 4**: Bausteine mit mehreren gleichartigen Anschluessen (Zaehler, Register, Addierer, Speicher,
  Anzeige) haben eine **Busbuchse** (4 bzw. 8 Bit, Bitnummern 0–3/0–7 beschriftet). Eine Busleitung wird wie
  eine Leitung gezogen, verbindet aber alle Bits auf einmal (breites Band mit Bitstreifen).
- **Wert auf dem Bus sichtbar**: am Band die Bits als 1/0 und die Zahl dazu (dual, dezimal, spaeter hexadezimal);
  Aenderungen leuchten kurz auf. Fuer den Zeitverlauf: Logikanalysator mit Bus-Spur.
- **Abzweig** fuer einzelne Bits: aus einem Bus laesst sich ein Bit herausfuehren (z. B. Bit 3 auf eine LED),
  damit Bus und Einzelleitung zusammenpassen und erklaerbar bleiben.
- **Drei Busse im eigenen Rechner** (Stufe 8): Datenbus, Adressbus, Steuerleitungen (Lesen/Schreiben, Takt) –
  farblich und mit Namen unterschieden (Farbe nie allein). Im Schaltplan als dicke Linie mit Schraegstrich und
  Bitzahl.
- Mehrere Ausgaenge an einem Bus: nur ueber schaltbare Ausgaenge (Tri-State); stehen zwei aktive Ausgaenge
  gegeneinander, meldet Volta einen **Buskonflikt** (ehrliche Physik, in der Fehlersuche erklaert).
- Modell: ein Bus ist im Kern ein Buendel einzelner Netze; die Simulation bleibt auf Gatterebene, nur Darstellung
  und Bedienung fassen zusammen. Undo/Redo wie bei Leitungen (Befehle).

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
| Stellung in der Reihe: laut Konzept nach Volta 3 (setzt bei Gattern und Kippstufen an) oder schon nach Volta 2 | vor Planungsbeginn |
| Busbreite: 4 Bit durchgehend (wie der Kleinstrechner) oder 8 Bit ab Stufe 6 | Darstellung |
| Bausteine mit echten Typnummern (7400, 7490 …) oder neutralen Namen (Gatterbaustein A …) | Stufenplanung |
| Name »Zuse«: Markenrecherche (docs/privacy-safety.md) und Namensrecht (Einverstaendnis der Erben bzw. der Zuse-Gesellschaft klaeren) | vor Planungsbeginn |
| Eigenes Bedienpult (Eingabe-/Anzeigeleiste) oder Volta-Pult mit Erweiterung | Darstellung |
