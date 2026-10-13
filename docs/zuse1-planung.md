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
