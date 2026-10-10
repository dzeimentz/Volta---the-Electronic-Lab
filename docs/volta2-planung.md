# Planung Volta 2 – Das Elektronik-Labor (Entwurf)

Stand: 10/2026, Entwurf zur Abstimmung. Grundlage: `docs/Konzept_Volta_v3.0.pdf`, Kapitel 4.1 (Reihe), 5 (Module)
und 6.3 (Erweiterungen des Kerns). Volta 2 ist eine eigene App (`VOLTA_EDITION=volta2`,
`com.dcsoftwareengineering.volta2`) aus derselben Codebasis und mit derselben Bedienung wie Volta 1.

## Zielgruppe und Schwerpunkte
- 11–14 Jahre; begleitet den Physik- und Technikunterricht der Sekundarstufe I.
- Klassische Analogelektronik: Transistor als Verstaerker und Arbeitspunkt, mehrstufige Verstaerker,
  Operationsverstaerker (Komparator, Verstaerker, Summierer, Integrator, aktive Filter), Oszillatoren,
  hoerbarer Audioverstaerker, Gleichrichtung und Stabilisierung, Messtechnik mit Kirchhoffschen Regeln und
  Kennlinien, Optoelektronik (Infrarot, Lichtschranke, Optokoppler), **Timer-Baustein 555**.
- Module aus Volta 1 werden aus Einzelteilen nachgebaut (Gegentakt-Endstufe des Verstaerkermoduls, Oszillator
  des Tonmoduls); der 555 zeigt, wie Blink- und Zeitmodul als ein einziger Baustein aussehen.

## Neu gegenueber Volta 1
- **Bedienpult II**: zweites Dreifach-Messgeraet (Spannung und Strom gleichzeitig), Handbereichswahl,
  Kleinspannungs-Wechselquelle, Anschluesse fuer Oszilloskop und Funktionsgenerator.
- **Oszilloskop** (zwei Kanaele, Zeitbasis, Trigger; Ausbau des Zeitschreibers) und **Funktionsgenerator**
  (Sinus, Rechteck, Dreieck, ca. 1 Hz – 20 kHz).
- **Hoerbare Simulation**: der Lautsprecher gibt das simulierte Signal wieder, nicht einen erzeugten Ton.
- Integrierte Bausteine (OP, 555) in der Modulleiste: ihre schmalen Felder entsprechen den Anschluessen.
- Spannungsanzeige mit negativen Potentialen (symmetrische Versorgung), E48/E96, Kondensatorcodes,
  Kennlinienschreiber.

## Technisches Hauptrisiko: Audio-Schrittweite
Der Kern rechnet in Volta 1 mit fester Schrittweite 1 ms. Hoerbare Signale bis 20 kHz brauchen etwa 20 µs –
50-mal mehr Schritte je Sekunde, in Echtzeit auf Schultablets. Vorgesehen (Konzept 6.3): variable
Schrittweite, Wiederverwendung der (duenn besetzten) LU-Zerlegung bei unveraenderter Topologie,
Audioberechnung im Puffer mit kurzem Vorlauf. **Erster Schritt** ist deshalb ein Leistungstest auf dem
Entwicklertablet (Meswoa MES-B3, MediaTek) mit einer typischen Audioschaltung (Oszillator + Gegentaktstufe).
Rueckfallebene, falls zu langsam: hoerbare Ausgabe nur fuer kleine Schaltungen, sonst erzeugter Ton wie beim
Tonmodul (offen erklaert).

## Reihenfolge
| Schritt | Inhalt | Ergebnis |
|---|---|---|
| 1 Leistungstest Audio | Kern mit 20-µs-Schritten, Faktorisierung wiederverwenden, Messung auf dem Tablet | Machbarkeit der hoerbaren Simulation |
| 2 Band-Infrastruktur | Edition volta2, Inhaltspaket `content/volta2`, Bedienpult I/II, gemeinsamer Katalog mit Bandfreigabe | zweite App aus derselben Codebasis |
| 3 Bauteilmodelle | OP-Makromodell, 555, Z-Diode, Spannungsregler, Wechselquelle, IR-LED, Fototransistor, Optokoppler, Lautsprecher als Last | Referenztests (< 2 %) |
| 4 Messgeraete | Oszilloskop, Funktionsgenerator, zweites Messgeraet, Handbereichswahl | Bedienpult II |
| 5 Inhalte | Stufen und ca. 65 Experimente, Lexikon, Rechenblaetter | inhaltlich vollstaendiger Band |
| 6 Plattformen | Android-APK, Desktop, Pruefung auf allen Tablets | Testversion Volta 2 |

## Stufen (Entwurf)
| Stufe | Thema | Inhalte (Auswahl) |
|---|---|---|
| 1 | Genauer messen | zwei Messgeraete, Handbereichswahl, Kirchhoffsche Regeln (Knoten, Masche), Kennlinien von Widerstand, Diode, LED, Gluehlampe |
| 2 | Wechselspannung sichtbar | Funktionsgenerator und Oszilloskop, Frequenz und Periode, Kondensator und Spule an Wechselspannung, RC-Tiefpass |
| 3 | Gleichrichten und stabilisieren | Kleinspannungs-Wechselquelle, Einweg- und Brueckengleichrichter, Glaettung, Z-Diode, Spannungsregler, kleines Netzteil |
| 4 | Der Transistor als Verstaerker | Arbeitspunkt, Emitterschaltung, Verstaerkung messen, mehrstufiger Verstaerker, Gegentakt-Endstufe (Verstaerkermodul nachgebaut), Audioverstaerker |
| 5 | Operationsverstaerker | OP-Modul, Komparator, invertierender und nichtinvertierender Verstaerker, Summierer (Mischpult), Integrator, aktive Filter |
| 6 | Der Timer-Baustein 555 | siehe unten |
| 7 | Schwingungen und Klang | Oszillatoren aus Einzelteilen (astabile Kippstufe, Phasenschieber, Wien-Bruecke mit OP), Tonmodul nachgebaut, Sirene, kleiner Synthesizer |
| 8 | Licht, das man nicht sieht | IR-LED, Fototransistor, Lichtschranke, Optokoppler, Fernbedienungs-Detektor |

Die Reihenfolge der Stufen 5 und 6 ist zwingend: der 555 wird mit den Komparatoren aus Stufe 5 erklaert.

## Stufe 6: Der Timer-Baustein 555
Der 555 ist seit 1972 der Standardbaustein fuer Takt und Zeit (Datenquelle u. a.
elektronik-kompendium.de, Datenblaetter der Hersteller). Er kombiniert zwei Funktionen: eine **Zeitfunktion**
(Laden eines Kondensators bis zu einer Schwelle) und das **Speichern eines Zustands** (Flipflop).

### Darstellung
- Baustein mit 8 Anschluessen (DIL8) in der Modulleiste: 1 Masse, 2 Trigger, 3 Ausgang, 4 Reset,
  5 Steuerspannung, 6 Schwelle, 7 Entladung, 8 +U. Die Felder tragen Nummer und Kurzname.
- »Blick hinein« als Blockschaltbild: Spannungsteiler aus drei gleichen Widerstaenden (1/3 und 2/3 der
  Betriebsspannung), zwei Komparatoren, Flipflop, Entladetransistor, Endstufe; im Betrieb mit den aktuellen
  Spannungen und dem Zustand des Flipflops. Das vollstaendige Innenleben aus Gattern folgt in Volta 3.

### Modell (Verhaltensmodell im Kern, wie das Tonmodul)
- Schwellen: Trigger < 1/3 U setzt, Schwelle > 2/3 U setzt zurueck; Steuerspannung (Pin 5) verschiebt die
  Schwellen (2/3 U = Pin 5, 1/3 U = Pin 5 / 2); Reset (Pin 4) unter ca. 0,7 V setzt zurueck und hat Vorrang.
- Innenwiderstand des Teilers 3 x 5 kOhm (belastet Pin 5 realistisch).
- Entladung (Pin 7): Transistor mit offenem Kollektor, leitet bei Ausgang LOW.
- Ausgang (Pin 3): Gegentaktstufe der bipolaren Ausfuehrung, HIGH ca. U − 1,7 V, LOW ca. 0,1–0,4 V je nach
  Strom; Strombegrenzung, Ueberlast bei mehr als 200 mA (Waerme, wie bei Volta-1-Bauteilen).
- Betriebsspannung 4,5–16 V: darunter arbeitet der Baustein nicht zuverlaessig (bei 3 V aus dem Pult: Ausgang
  bleibt aus – ehrliche Physik, im Experiment erklaert); ueber 16 V nicht vorgesehen (Pult hoechstens 9 V).
- Stromaufnahme ca. 3–10 mA im Ruhezustand (je nach Betriebsspannung). Schaltstoesse der bipolaren Ausfuehrung und der empfohlene
  Stuetzkondensator werden in »Mehr wissen« erwaehnt, nicht simuliert (Vereinfachung, offengelegt).
- Langsame Anwendungen (Blinken, Zeitschalter) laufen mit der Volta-1-Schrittweite; hoerbare Anwendungen
  brauchen die Audio-Schrittweite (siehe oben).

### Experimente (Entwurf, ca. 10)
1. Der 555 als Blinker (astabil): LED am Ausgang, Frequenz mit R1, R2 und C; Rechnung f ≈ 1,44 / ((R1 + 2 R2) C)
   im Formelrechner.
2. Gleiche Zeit an und aus? Puls und Pause messen (Tastverhaeltnis), Diode parallel zu R2 fuer 50 %.
3. Blick hinein: Kondensatorspannung pendelt zwischen 1/3 und 2/3 U (Oszilloskop an Pin 2/6).
4. Der 555 als Zeitschalter (monostabil): Treppenhauslicht aus Volta 1 mit einem Baustein; t ≈ 1,1 R C.
5. Reset: Stopp-Taster an Pin 4, LED gegen +U (leuchtet bei LOW) und gegen Masse (leuchtet bei HIGH).
6. Ein Ton aus dem Chip: Lautsprecher am Ausgang, Tonhoehe mit Drehregler P1 (hoerbar).
7. Zweitoensirene: zweiter 555 steuert ueber Pin 5 die Tonhoehe des ersten.
8. Lichtorgel/Lichtton: Fotowiderstand statt R2 – die Tonhoehe folgt dem Licht.
9. Motor stufenlos: Pulsweitensteuerung mit Transistorstufe am Ausgang, Pulse am Oszilloskop sichtbar.
10. Schmitt-Trigger: Daemmerungsschalter ohne Flackern (Schwellen 1/3 und 2/3 U).

## Offene Entscheidungen
| Entscheidung | Spaetestens |
|---|---|
| Reihenfolge: Volta 1 erst veroeffentlichen (Phase 3) oder Volta 2 parallel | vor Schritt 2 |
| Hoerbare Simulation zwingend oder Rueckfall auf erzeugten Ton bei langsamen Geraeten | nach Schritt 1 |
| Bedienpult II: Anordnung zweites Messgeraet, Wechselquelle, Generator (oben im Pult oder seitlich) | Schritt 4 |
| Stufenplanung und Experimentliste freigeben | vor Schritt 5 |
| 556 (zwei Timer in einem Gehaeuse) aufnehmen oder zwei 555 verwenden | Schritt 3 |
