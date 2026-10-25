# Zuse 1 – Ergaenzungen zum Konzept Zuse v0.2

Massgeblich ist **`docs/Konzept_Zuse.md`** (v0.3, 15.10.2026; PDF `docs/Konzept_Zuse_v0.3.pdf` mit
`tools/konzept_pdf.py`, Entwuerfe und Bildschirmfotos in `docs/zuse/`); die Punkte unten sind dort eingearbeitet, mit **ADR-0010** (Zuse als eigene Linie mit Werkbank und Systemplatine). Diese Datei sammelt nur, was
seitdem dazugekommen ist: Entscheidungen des Inhabers und die Auswertung einer Quelle. Ein frueherer Entwurf in
dieser Datei (Zuse auf der Volta-Grundplatte mit Modulleiste, 74xx-Bausteinen und eigener Stufenliste) ist durch
das Konzept ueberholt und entfernt.

## Entscheidungen des Inhabers
- 13.10.2026: **Busbreite in Zuse 1: 4 Bit durchgehend** (Datenbus, Register, ALU, Speicherworte, Adressen 0–15).
  Deckt sich mit dem Konzept (Zuse-4; 8 Bit erst in Zuse 2).
- 13.10.2026: Zuse 1 nach Volta 3 – **am 14.10.2026 wieder offen:** Eine Marktanalyse sieht die Nachfrage bei
  Lernsystemen (Kasten und App) eher bei der Informationstechnik als bei der Elektronik. Erwogen werden deshalb
  **zwei parallele Linien** (Volta und Zuse). Die Entscheidung wartet auf die Antwort eines Verlags, den der
  Inhaber angeschrieben hat. Bis dahin gilt das Konzept (Z0 nach dem Android-Pilot von Volta 1).

## Prototyp Z0 (Stand 14.10.2026)
Umgesetzt im Repository (eigene App `zuse1` neben Volta, Edition-Frage fuer Z1 offen):
- `logic/`: Qt-freie Logiksimulation. Chip-Definition aus Anschluessen, Teilen (NAND, AND, OR, NOT, XOR oder eigene
  Chips) und Verbindungen; Aufloesung bis zum Gatter mit erhaltener Hierarchie; ereignisgesteuert mit
  Einheitsverzoegerung und Pegeln 0/1/X (Stoerimpulse entstehen echt, Schwingen wird erkannt); Pruefung mit
  vollstaendiger Wahrheitstabelle; Fehler fuer doppelte Treiber, unbekannte Anschluesse und Rekursion.
- `zuse/`: Werkbank mit fuenf Stufen (NICHT, UND, ODER aus NAND, XODER, Halbaddierer); jede Aenderung ein Befehl
  (Rueckgaengig/Wiederholen), Eingaenge antippen, Pruefen, »Chip erstellen« nur nach bestandener Pruefung,
  der neue Chip erscheint mit Stern im Fach der naechsten Stufe; »Blick hinein« per Doppeltippen ueber alle Ebenen
  mit Live-Pegeln; Musterloesung als letzte Hilfe; eigene Chips und Entwuerfe lokal (AppDataLocation, QSaveFile).
- Gestaltung nach Konzept 6: Graphit, goldene Loetpads, Cyan; Pegel 1 durchgezogen, 0 gestrichelt, ? gepunktet.
- 14.10.2026, Entscheidung des Inhabers: **Eingaenge sind Kippschalter und Taster, Ausgaenge rote LEDs** – wie auf
  dem Bedienfeld der ersten Rechner-Bausaetze (Vorbild Altair 8800). Kippschalter rastet, Taster gilt nur solange er
  gedrueckt ist; jede LED zeigt zusaetzlich 1/0. Neue Stufe 0 »Ein Bit«: Schalter und Taster direkt an LEDs, ohne
  Gatter und ohne Chip. Offen: Reihenfolge der Gatter (NAND zuerst oder UND/ODER/NICHT fertig vorgeben, NAND spaeter
  als Entdeckung) und Alltagsgeschichten je Stufe (Tresor, Klingel, Treppenhauslicht).
- Tests: 11 Testfaelle Logik (Catch2), 5 Werkbank (Qt Test, u. a. alle Musterloesungen bestehen).

Offen fuer die Erprobung mit Jugendlichen: Uebersetzungen (FR, LU; Texte DE/EN vorhanden, .ts-Dateien fehlen),
Android-APK des Prototyps (Ziel `zuse_app`, Option `VOLTA_BUILD_ZUSE` im Android-Build), eigenes Symbol,
Hoch- und Querformat, Zoom mit zwei Fingern, Tastaturbedienung der Anschluesse, Testfolgen fuer Speicher (Stufe 6).

## Neuausrichtung des Prototyps (15.10.2026)
Rueckmeldung des Inhabers: Werkbank und »Chip erstellen« waren technisch richtig, aber zu abstrakt; gewuenscht ist
ein spielerischer, heller Einstieg wie bei Volta 1. Umgesetzt:
- **Jede Stufe beginnt mit einer Frage aus dem Alltag** (Licht an, Nachtlicht, Tresor, Klingel, Alarmanlage,
  Treppenhauslicht, 1 + 1 = 10); Schalter und LEDs tragen Bedeutungen (»Haustuer«, »Klingel«).
- **Entdecken mit Raetselkisten:** NICHT, UND, ODER werden nicht vorgegeben, sondern durch Ausprobieren dreier
  verschlossener Kisten gefunden; falsche Kisten erklaeren konkret, wo sie versagen. Erst danach Name und Zeichen.
- **Werkzeugkiste statt »Chip erstellen«:** Entdeckte Regeln (mit Merksatz) und eingepackte Erfindungen
  (»Alarmanlage«) sind Werkzeuge fuer spaetere Stufen; Bauen kommt erst, wenn ein Problem es verlangt.
- **Maskottchen Zetti** (eigene Figur der Zuse-Linie) erzaehlt in einer Sprechblase, zeigt Stimmungen.
- **Zahlensysteme:** zuschaltbare Zahlenleiste (Schalter und LEDs binaer mit Stellenwerten, dezimal, hexadezimal)
  und Erklaerung »Warum rechnet ein Computer binaer, wozu Hex?« mit Vergleichstabelle 0–15.
- Helles Design, Akzent Cyan, Knoepfe in Zettis Muetzenblau; NAND wird spaeter als Bonus-Entdeckung eingefuehrt.

## Leitbild Bedienfeld (nach dem Altair 8800) – Fahrplan der Reihe (Entwurf 15.10.2026)
Der Inhaber bestaetigt die Linie (Geschichten, Raetselkisten, Werkzeugkiste, Zahlensysteme, Zetti klein) als
gemeinsame Basis mit Volta 1. Weiterentwicklung der Reihe am Vorbild der ersten Bausatz-Rechner: Ein Bedienfeld aus
Kippschaltern (Adresse, Daten), LEDs (Adresse, Daten, Zustand) und Tastern (Untersuchen, Ablegen, Einzelschritt,
Start/Stopp, Ruecksetzen) waechst mit jedem Band. Nur das Prinzip wird uebernommen; der Name »Altair« erscheint nicht
im Produkt, hoechstens sachlich in einem Geschichtsfenster (neben Konrad Zuse, Z1/Z3).

| Band | Bedienfeld | Rechner | Neu |
|---|---|---|---|
| Zuse 1 | 4 Datenschalter, 4 Adressschalter, LEDs fuer Adresse, Daten, Uebertrag, Null, Lauf/Halt | Zuse-4 (4 Bit, 16 Speicherplaetze) aus selbst gebauten Werkzeugen | Programm per Schalter eintippen (Ablegen, Weiter) und im Einzelschritt laufen lassen |
| Zuse 2 | 8 Daten-, 8 Adressschalter, Status-LEDs | Zuse-8 (8 Bit, eigener Befehlssatz, Stapel, Ein-/Ausgabe-Ports, Pixelmatrix) | Assembler neben dem Bedienfeld, Intel HEX |
| Zuse 3 | Bedienfeld wird zur Anschlussleiste | Mikrocontroller als fertiger Chip | Bloecke und Text, Sensoren und Aktoren ueber den Volta-Kern |

Stufen Zuse 1 nach dem aktuellen Prototyp (0 bis 6), Vorschlag:

| Stufe | Frage | Ergebnis |
|---|---|---|
| 7 | Wie zaehlt ein Rechner mit vier Lampen? | Binaerzahlen am 4-Bit-Bedienfeld (Zahlenleiste) |
| 8 | Geht alles mit einem einzigen Baustein? | NAND als Bonus-Entdeckung |
| 9 | Rechnen bis 15 – und was, wenn es nicht mehr passt? | Volladdierer, 4-Bit-Addierer, Uebertrags-LED |
| 10 | Hat jemand geklingelt, waehrend ich weg war? | Speicher: Flipflop, Register |
| 11 | Wie zaehlt ein Rechner von allein? | Takt (Taster prellt), Zaehler, Befehlszaehler |
| 12 | Wo wohnt ein Programm? | Speicher 16 x 4 mit Adressschaltern: Untersuchen und Ablegen am Bedienfeld |
| 13 | Laeuft er? | Zuse-4: Programm eintippen, Einzelschritt, Start (Steuerwerk teilweise vorgegeben) |

## Quelle: ELRAD-Lehrgang »Digitaltechnik im Experiment« (Auswertung 13.10.2026)
Lehrgang Elektronische Datenverarbeitung von H. Weidner (ELRAD 11/77–05/82, Sonderheft Heise 2005; private
Sammlung, `I:\ELRAD\pdf\sonderhefte\digitaltechnik\`, nicht im Repository). Zehn Teile vom UND/ODER mit Schaltern
und Laempchen ueber Flipflops, Zaehler, Codierer, Addierer und ein Rechenwerk mit Akkumulator bis zum »kleinsten
denkbaren Rechenautomaten« (Speicher 16 x 4, Befehlszaehler, Befehlsregister und -decoder, Multiplexer).
Er bestaetigt den Aufbau des Konzepts (Abschnitt 7.1) fast Stufe fuer Stufe. Uebernommen werden nur Prinzipien;
der Lehrgang arbeitet mit 74xx-Bausteinen auf Steckbrettern – genau das ist im Konzept ein Nicht-Ziel (3.2).
Ausgeschlossen ist sein Kapitel zum Netzteil mit Netztrafo (Volta-Regel 6).

Die Fotos seiner Versuchsaufbauten zeigen anschaulich, warum Einzelleitungen nicht tragen (schon ein Zaehler mit
Vergleich wird zum Kabelknaeuel) – ein gutes Argument fuer die Busse und die Abstraktionsleiter des Konzepts.

Ergaenzungen, die das Konzept noch nicht nennt (Vorschlag, Zuordnung zu den Stufen in 7.1):

| Idee aus dem Lehrgang | Vorschlag fuer Zuse | Stufe |
|---|---|---|
| Wahrheitstabelle selbst aufnehmen (erst messen, dann eintragen) | Pruefung umkehrbar: das Kind fuellt die Tabelle zuerst aus, dann laeuft die Simulation dagegen | 1–2 |
| Alle Verknuepfungen aus NAND; Rechenregeln der Logik (De Morgan), Vereinfachen | Werkzeug »Vereinfachen«: zwei Chips mit gleicher Wahrheitstabelle sind gleichwertig – der mit weniger Gattern gewinnt einen Stern | 1–4 |
| Prellender Taster und Entprellen mit RS-Flipflop | Taster-Baustein mit (abschaltbarem) Prellen: der Zaehler springt mehrfach – Anlass fuer das erste Flipflop | 6–7 |
| Teiler 2, 4, 8 … aus Flipflops, Sekundentakt | Takt-Baustein mit Teilern; im Logikanalysator sichtbar | 7 |
| Vergleicher (gleich, groesser, kleiner) | Null-Erkennung bzw. Vergleicher als Chip – Voraussetzung fuer den Befehl JZ | 5 oder 9 |
| Codierer, Decoder, Siebensegmentanzeige | Decoder als eigener Chip (Stufe 8) und optional Siebensegment-Decoder fuer die Ausgabe der Systemplatine | 8 / frei |
| Serielles Addieren mit Schieberegister und Akkumulator | Vergleich »parallel gegen seriell« als Weiterforschen; Schieberegister eher Zuse 2 | Weiterforschen |
| Laufzeiten und Stoerimpulse | Die Einheitsverzoegerung der Logiksimulation erzeugt echte kurze Stoerimpulse; im Logikanalysator zeigen und erklaeren statt verstecken | 4–7 |
| Konrad Zuse als Urheber des gespeicherten Rechenablaufs | Geschichtsfenster zu Z1 und Z3 in Stufe 4 (Speichern und Programmieren) | 10 |

## Abgleich mit dem ELRAD-Lehrgang (Stand 25.10.2026: Lernstufen 0–4)
Grundlage: Sonderheft »Digitaltechnik im Experiment« (elrad 1979, zehn Teile, H. Weidner; Ablage des Inhabers
`J:\DC Learning EcoSystem\ELRAD - Digitaltechnik.pdf`, nicht im Repository). Uebernommen werden nur Prinzipien.

### Gang des Lehrgangs
| Teil | Inhalt |
|---|---|
| 1 | Binaere Groessen (Dinge mit und ohne »Gedaechtnis«: Lichtschalter gegen Klingelknopf), Wahrheitstabelle mit 2^n Zeilen, UND/ODER/NICHT an Alltagsentscheidungen, NAND als Inverter und UND aus NAND, XOR; erstes Flipflop (RS aus NAND) |
| 2 | Schaltzeiten als Grund fuer den Takt: getaktetes RS-Flipflop, Taktgenerator, Dualsystem (Potenzen von 2, oktal, hexadezimal), JK-Flipflop |
| 3 | Asynchroner 4-Bit-Zaehler mit Ruecksetztaste, BCD-Zaehler, Synchronzaehler, selbstanhaltende Zaehler, Schieberegister, Wahrheitstabelle Zeile fuer Zeile in Gatter umsetzen |
| 4 | Boolesche Algebra, De Morgan, Vereinfachungsaufgaben; Technik (Transistor als Schalter, TTL, Schmitt-Trigger); Teiler bis Sekunden-, Minuten-, Stundentakt |
| 5 | Netzteil (ausgeschlossen, Volta-Regel 6); Dezimal-Dual-Codierer, Dual-Dezimal-Decoder, Siebensegment-Decoder |
| 6 | De-Morgan-Uebungen, D-Flipflop, umlaufendes Schieberegister, programmierbarer Zaehler, Vergleicher (Gleichheit mit XOR) |
| 7 | 4-Bit-Vergleicher, groessere Zahl erkennen, Vorwahlzaehler, Rechts-Links-Schieberegister (mal 2, durch 2), Halb- und Volladdierer, BCD-Addition |
| 8 | Serielles Addieren mit Schieberegister und Akkumulator, Paralleladdierer in zwei Schritten |
| 9 | Rechenwerk mit ALU-Baustein: logische Funktionen, Steuerleitungen, Akkumulator; eine Aufgabe von Hand nach einem Ablaufschema durchrechnen (der Mensch ist das Steuerwerk) |
| 10 | Der Rechenautomat: Konrad Zuse und der gespeicherte Ablauf, Speicher mit Adresse und Inhalt, Maschinencode mit etwa zehn Befehlen (schieben, +1, −1, negieren, laden, speichern, addieren, subtrahieren, UND, ODER), Programm »zwei Zahlen addieren«, Befehl holen und ausfuehren |

### Gemeinsamkeiten
- Gleicher Bogen: vom Schalter und Laempchen ueber Gatter, Addierer, Speicher und Zaehler bis zum kleinen Rechner mit
  Programm im Speicher. Teil 10 entspricht fast genau den geplanten Experimenten 4.6/4.7 (Zuse-4).
- Gleicher Einstieg: Der Lehrgang beginnt mit Lichtschalter (merkt sich etwas) und Klingelknopf (merkt sich nichts) –
  genau Kippschalter und Taster aus Experiment 0.1.
- Alltagsentscheidungen statt Formeln (Ampel, Hindernis, Stoppschild bzw. bei Zuse Tresor, Klingel, Alarmanlage).
- Wahrheitstabelle selbst ausfuellen, bevor man misst (bei Zuse: Vermutung, dann Vergleich).
- NAND als Baustein, aus dem sich alles bauen laesst (Zuse 2.3).

### Unterschiede
| Thema | ELRAD-Lehrgang | Zuse 1 (Stand jetzt) | Bewertung / Vorschlag |
|---|---|---|---|
| Zielgruppe und Weg | Jugendliche und Erwachsene; erst Erklaerung, dann Versuch nach Schaltplan | 10–14 Jahre; Frage – Vermutung – Ausprobieren – Benennen, Raetselkisten, eigene Erfindungen als Werkzeuge | Zuse-Weg beibehalten |
| Reihenfolge | Flipflop schon in Teil 1, Takt und Zaehler (Teil 2–3) vor dem Rechnen (Teil 7) | Rechnen (Stufe 3) vor Speichern (Stufe 4) | beibehalten: Zahlen motivieren; das Dualsystem kommt bei Zuse ueber »Vier Lampen« statt ueber den Zaehler |
| Speicherbaustein | RS aus zwei NAND (ueber Kreuz), danach fertige JK- und D-Flipflops als IC | Merker aus ODER/UND/NICHT mit Rueckfuehrung, Speicherbit, Umschalter – alles selbst gebaut | Zuse geht tiefer (eigener Umschalter); RS aus NAND als »Blick hinein« oder Weiterforschen anbieten |
| Einschalten | Zustand nach dem Einschalten unvorhersehbar, daher Ruecksetztaste an jedem Zaehler | Speicher starten mit 0 (Vereinfachung) | **Luecke:** Register und Zaehler bekommen einen Eingang »Loeschen«; der Merksatz sagt, dass echte Speicher zufaellig starten |
| Takt | Taktgenerator, Teiler bis zum Sekundentakt (Uhr) | Takt nur per Taster | **Luecke:** Taktgeber-Baustein (langsam blinkend) und Teiler durch 2 – ein »Sekundenblinker« ist kindgerecht |
| Zaehlerarten | asynchron, synchron, BCD (Ruecksetzen bei 10), selbstanhaltend, Vorwahl | nur asynchroner 4-Bit-Zaehler | BCD und Vorwahl als Weiterforschen; fuer Zuse 1 genuegt der Binaerzaehler |
| Schieberegister | ausfuehrlich: Lauflicht, mal 2/durch 2, serielles Addieren | fehlt | **Vorschlag:** ein Experiment »Lauflicht« (Register, dessen Bits weiterwandern) – anschaulich, und »schieben = mal 2« verbindet mit den Zahlen |
| Vergleicher | Gleichheit (XOR), groesser/kleiner, 4 Bit | fehlt | **Vorschlag:** »Sind beide Zahlen gleich?« mit XODER und NICHT – baut auf 2.2 auf und wird spaeter fuer den Sprungbefehl gebraucht |
| Codierer, Decoder, Siebensegment | eigener Teil (5) | Zahlenleiste zeigt binaer, dezimal, hex; kein Decoder | **Luecke vor 4.6:** Decoder »1 aus 4« ist Voraussetzung fuer Speicheradressen; Siebensegment eher Zuse 2 |
| Boolesche Algebra | Gesetze, De Morgan, Aufgaben »auf n Verknuepfungen reduzieren« | nur NAND-Universalitaet | fuer 10–14 hoechstens spielerisch: Stern fuer die Loesung mit den wenigsten Bausteinen; Formeln in Zuse 2 |
| Technik | Transistor als Schalter, TTL, Schmitt-Trigger, Netzteil | bewusst nicht (Nicht-Ziel); kein Netz | Bruecke ins Ecosystem: Transistor als Schalter steht in Volta 3 |
| Laufzeiten | Schaltzeiten begruenden den Takt | Simulation hat echte Laufzeiten (Stoerimpulse), werden aber nicht gezeigt | mit dem Logikanalysator (F11) zeigen |
| Rechenwerk | ALU mit Rechen- und Logikfunktionen, Subtraktion ueber Komplement | 4-Bit-Addierer mit Uebertrag | Der Arbeitsentwurf Zuse-4 (Konzept 5.6: LDA, ADD, SUB, STA, OUT, JMP, JZ, HLT) braucht SUB – also ein Experiment »Abziehen durch Addieren« (Komplement); sonst SUB streichen (offene Entscheidung, Konzept 15) |
| Vom Rechenwerk zum Automaten | erst von Hand nach Ablaufschema bedienen (Mensch = Steuerwerk), dann automatisieren | geplant: 4.6 Speicher, 4.7 Zuse-4 | **Vorschlag:** Experiment »Du bist das Steuerwerk« vor 4.7 – das Kind gibt die Steuersignale selbst, dann uebernimmt der Rechner |
| Geschichte | Konrad Zuse als Urheber des gespeicherten Ablaufs | Name der Reihe, aber noch kein Geschichtsfenster | Geschichtsfenster in Stufe 4 |
| Darstellung | H/L-Pegel, Steckbrett, Kabelknaeuel | 0/1, Werkbank mit Raster, Erfindungen als Chips | Zuse-Weg beibehalten |

### Folgerung fuer Stufe 4 (Vorschlag, noch nicht umgesetzt)
1. Register und Zaehler mit »Loeschen« (ehrlich zum Einschalten).
2. Neue Experimente in dieser Reihenfolge: Lauflicht (Schieberegister) nach dem Register; Taktgeber mit Teiler nach
   dem Zaehler; Vergleicher (gleich) und Decoder 1 aus 4 als Vorbereitung auf den Speicher.
3. »Du bist das Steuerwerk« vor »Laeuft er?«.
4. Wird Stufe 4 damit zu gross (etwa zehn Experimente), sie teilen: Stufe 4 »Speichern und Zaehlen«, Stufe 5 »Der
   Rechenautomat« – Entscheidung des Inhabers.

## Weitere Punkte zum Konzept
- Das fruehere PDF v0.2 nannte ADR-0009 fuer die Inhalts-Einbettung; richtig ist ADR-0011 (in v0.3 behoben).
- Konzept Abschnitt 9 bezieht sich auf den Cloud-Zweig `claude/mobile-android-ipad-0w8lbc` (Pull Request #1). Dieser
  Zweig setzt auf einem alten Stand auf; die Android-Arbeit ist lokal inzwischen auf anderem Weg erledigt (Test-APK,
  AAB, armv7). Touch-Bedienung (Zoom mit zwei Fingern, Hochformat), iPadOS- und macOS-Paketierung aus dem Zweig sind
  noch nicht uebernommen.
