# Fachmodell

## Grundplatte und Kontaktfelder

- Die Grundplatte besteht aus **Kontaktfeldern** in Zeilen und Spalten. Volta 1: Zeilen `A`–`E`, Spalten `1`–`12`.
- **Breite einstellbar** (`Board::columns`): 8, 10 oder 12 Felder je Reihe ueber »−/+« in der Werkzeugleiste
  (Befehl, rueckgaengig machbar). Verkleinern nur, wenn die wegfallenden Spalten frei sind; Spannungsanzeigen dort
  entfallen. Felder rechts der eingestellten Breite sind fuer Bauteile und Leitungen ungueltig.
- Jedes Feld hat **fuenf Steckplaetze**, die leitend verbunden sind. Elektrisch ist ein Feld **ein Knoten**.
- Bezeichner: `<Zeile><Spalte>`, Regex `^[A-E](1[0-2]|[1-9])$` (Volta 1: 5 Zeilen x 12 Spalten). Die Koordinatenform ist bewusst eigenstaendig
  (das Vorbild nummeriert durchgehend).
- Freischaltung: Stufe 1 nur Zeilen `A`–`C`, ab Stufe 3 volle Flaeche.
- Mindestgroesse: Feld 56 dp hoch, 24 dp Abstand.

## Bauteile auf der Grundplatte

- Zweipolige Bauteile ueberbruecken zwei Felder, waagrecht oder senkrecht, mit Spannweite **kurz** (1 Feldabstand)
  oder **lang** (2 Feldabstaende). Ablegen zielt auf das ganze Feld; die App waehlt den freien Steckplatz.
- Der **Transistor** sitzt auf einer **quadratischen Platine** mit vier Fuessen ueber einem 2-x-2-Block von Feldern
  (Katalog `spannweite: platine_2x2`, `lage: b, c, e, e2`): oben links B, oben rechts C, unten links E, unten rechts
  ein zweiter E-Fuss (`e2`), der auf der Platine ueber eine sichtbare Leiterbahn mit E verbunden ist (Netzliste:
  Leitung `<ID>.E2`). Startfeld ist das Feld oben links; »Umdrehen« dreht die Platine um 180°.
- Der Wechselschalter sitzt auf einer **Traegerplatine** ueber drei Felder in einer Reihe (`spannweite: traeger`).
- Gepolte Bauteile tragen Anschlussnamen: Diode/LED `anode`, `kathode`; Elko `plus`, `minus`; Transistor `b`, `c`, `e` (Platine zusaetzlich `e2`).
- Ein Feld hat hoechstens fuenf Bauteilbeine. Die fuenf Steckloecher liegen mittig in einer Reihe (0 links ... 4 rechts);
  Beine und Leitungsenden stecken sichtbar in einem Loch. Die Belegung wird aus dem Aufbau berechnet
  (`assignSockets`, deterministisch, Undo-sicher):
  - Bauteilbeine zuerst, zur Seite des Partnerfelds: Partner rechts -> Loch 3, 4, 2, 1, 0; links -> 1, 0, 2, 3, 4;
    senkrecht (gleiche Spalte) -> Mitte 2, 1, 3, 0, 4. Traegerplatine: aeussere Beine nach innen, mittleres Bein Mitte; 2-x-2-Platine: Fuesse zur Mitte des Blocks.
  - Leitungsenden danach in freie Loecher: anderes Ende weiter rechts -> von aussen rechts (4, 3, ...), weiter links ->
    von aussen links (0, 1, ...), sonst (Pult, Messgeraet, Modul, gleiche Spalte) Mitte zuerst.
  - Leitungen sind nicht begrenzt: Sind alle Loecher belegt, teilt sich eine Leitung das am wenigsten belegte Loch.
  - Pultfelder und Messgeraetebuchsen haben ebenfalls fuenf Loecher, Modulleiste und Modulanschluesse eines.

## Leitungen

- Frei gezogen mit Finger, Stift oder Maus zwischen zwei Anschlusspunkten (Feld, Pultfeld, Modulanschluss,
  Messgeraetebuchse). Darstellung als weiche Kurve, rastet am Zielfeld ein.
- Eigenschaften: `von`, `nach`, `farbe` (`rot`, `schwarz`, `blau`, `gelb`, `gruen`, `weiss`), optional `zwischenpunkt`.
- Beide Enden lassen sich greifen und umsetzen. Loslassen ausserhalb eines Ziels verwirft die Leitung.
- Leitungen sind ideale Verbindungen mit 0,01 Ω (nur numerisch).

## Bedienpult und Pultleiste

| Pultfeld | Bauteil | Bedeutung |
|---|---|---|
| `+9V`, `0V` | Batterie | Plus, Minus (Masse); Batterie aus 1,5-V-Zellen, waehlbar 3 / 4,5 / 9 V. Der Feldname `+9V` bleibt bei jeder Wahl gleich |
| `P1a`, `P1s`, `P1b` | Potentiometer P1 | Anschluss a, Schleifer, Anschluss b |
| `P2a`, `P2s`, `P2b` | Potentiometer P2 | wie P1 |
| `S1`, `S2` | Schalter | Kontakte |
| `LS1`, `LS2` | Lautsprecher | Anschluesse |

- Jedes Pultfeld ist ein Kontaktfeld mit fuenf Steckplaetzen und eigenem Knoten.
- **Gruppenlinie**: graue waagrechte Linie ueber den Feldern eines Bauteils mit dessen Schaltzeichen in der Mitte;
  zeigt Zugehoerigkeit, ist **keine** Leitung. Felder einer Gruppe sind elektrisch getrennte Knoten.
- Pultelemente: Hauptschalter (Batterie), zwei Drehregler mit 270°-Lichtring (16 Segmente), Lautsprecher mit
  Leuchtring (Klanganzeige), Dreifach-Messgeraet.
- In Bezeichnern von Inhaltspaketen: Praefix `P:` (z. B. `P:+9V`, `P:P1s`).

## Modulleiste

- Schmale Felder `M1`–`M16` am unteren Rand; Bezeichner `M:<n>`.
- Ein Modul belegt einen zusammenhaengenden Bereich; seine Anschluesse tragen Namen (z. B. Blinkmodul `+`, `-`,
  `takt`, `aus`). Adressierung im Inhaltspaket: `<Modul-ID>.<Anschluss>`.

## Dreifach-Messgeraet

- Buchsen `COM`, `VOHM`, `A`; Bezeichner `MESS:COM`, `MESS:VOHM`, `MESS:A`.
- Messart `V`, `A`, `OHM`; Anzeige `digital` oder `analog`; Bereich automatisch (Volta 1), manuell ab Volta 2.
- Verhalten bei Fehlbedienung: A-Modus parallel zur Quelle -> Sicherung loest aus (`FuseBlown`);
  Ω-Modus unter Spannung -> Anzeige »Spannung abschalten!«.

## Spannungsanzeige

- Je Kontakt- oder Pultfeld zuschaltbar: vier hellgruene Balken (Kontur fuer Farbfehlsichtige).
- Anzahl leuchtender Balken = Potential gegen `0V` im Verhaeltnis zur Batteriespannung:
  0 Balken < 1/8, 1 Balken < 3/8, 2 Balken < 5/8, 3 Balken < 7/8, sonst 4; Hysterese 2 % der Batteriespannung.
- Ab Volta 2 bei negativen Potentialen Balken nach unten.

## Zustaende eines Bauteils

`intakt`, `ueberlastet` (gluehend, noch funktionsfaehig), `zerstoert` (Rauch, ohne Funktion), `repariert`
(nach Tippen auf »Reparieren«, Befehl im Verlauf).

## Bezeichner im Code

| Fachbegriff | C++/QML |
|---|---|
| Kontaktfeld | `Field`, `FieldId` |
| Pultfeld | `DeskField` |
| Leitung | `Wire` |
| Bauteil (Instanz) | `Placement` / `ComponentInstance` |
| Bauteiltyp | `ComponentType` |
| Modul | `ModuleInstance` |
| Bedienpult | `ControlDesk` |
| Pultleiste | `DeskStrip` |
| Modulleiste | `ModuleBar` |
| Dreifach-Messgeraet | `Meter` |
| Spannungsanzeige | `VoltageBars` |
| Klanganzeige | `SoundRing` |
| Lichtring | `KnobRing` |
