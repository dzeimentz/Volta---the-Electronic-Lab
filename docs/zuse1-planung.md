# Zuse 1 – Ergaenzungen zum Konzept Zuse v0.2

Massgeblich ist **`docs/Konzept_Zuse.md`** (v0.2, 13.10.2026; PDF `docs/Konzept_Zuse_v0.2.pdf`, Entwuerfe in
`docs/zuse/`) mit **ADR-0010** (Zuse als eigene Linie mit Werkbank und Systemplatine). Diese Datei sammelt nur, was
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
| Konrad Zuse als Urheber des gespeicherten Rechenablaufs | Geschichtsfenster zu Z1 und Z3 in Stufe 10 | 10 |

## Weitere Punkte zum Konzept
- Das PDF `docs/Konzept_Zuse_v0.2.pdf` nennt noch ADR-0009 fuer die Inhalts-Einbettung; im Repository ist das
  ADR-0011 (ADR-0009 beschreibt hier die Tonausgabe). Markdown und ADR-0010 sind angepasst, das PDF beim naechsten
  Stand neu erzeugen.
- Konzept Abschnitt 9 bezieht sich auf den Cloud-Zweig `claude/mobile-android-ipad-0w8lbc` (Pull Request #1). Dieser
  Zweig setzt auf einem alten Stand auf; die Android-Arbeit ist lokal inzwischen auf anderem Weg erledigt (Test-APK,
  AAB, armv7). Touch-Bedienung (Zoom mit zwei Fingern, Hochformat), iPadOS- und macOS-Paketierung aus dem Zweig sind
  noch nicht uebernommen.
