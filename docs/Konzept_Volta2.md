# Volta 2 – Das Elektronik-Labor

_Konzept des zweiten Bandes: Klassische Analogelektronik für 11 bis 14 Jahre_

Markdown-Fassung des Konzepts und Quelle des PDFs `docs/Konzept_Volta2_v0.1.pdf` (erzeugt mit `tools/konzept_pdf.py`).

| | |
|---|---|
| Dokument | Konzept |
| Version | 0.1 |
| Status | ENTWURF |
| Autor | Daniel Zeimentz · DC Software Engineering |
| Datum | 27.10.2026 |
| Bezug | Konzept Volta v3.0 (Kap. 4.1, 5, 6.3, 11.2); `docs/volta2-planung.md` (Planungsentwurf, Leistungstest Audio, Auswertung 555-Quellensammlung); DC Learning Design Standard v1.0 |
| Zielplattformen | Android (Tablet), Windows und Linux x86-64, iPadOS, macOS – aus derselben Codebasis wie Volta 1 |

## 1. Zusammenfassung

Volta 2 ist der zweite Band der Reihe »Volta – Das Elektronik-Labor« und eine eigene App aus derselben Codebasis wie Volta 1 (`VOLTA_EDITION=volta2`, `com.dcsoftwareengineering.volta2`). Er richtet sich an 11- bis 14-Jährige und begleitet den Physik- und Technikunterricht der Sekundarstufe I.

Volta 1 hat gezeigt, **dass** Strom etwas bewirkt – Licht, Bewegung, Klang – und mit Modulen erste Elektronik möglich gemacht. Volta 2 zeigt, **wie** Elektronik das tut: Was ein Transistor mit einem leisen Signal macht, warum ein Netzteil aus Wechselspannung eine ruhige Gleichspannung macht, wie ein Ton entsteht und wie man ihn sieht. Dafür kommen ein **Oszilloskop**, ein **Funktionsgenerator**, ein **zweites Messgerät** und eine **Kleinspannungs-Wechselquelle** hinzu, und die Simulation wird **hörbar**: Der Lautsprecher gibt das simulierte Signal wieder, nicht einen eingespielten Ton.

Die Module aus Volta 1 werden aus Einzelteilen **nachgebaut** (Verstärker-, Ton- und Blinkmodul), und der **Timer-Baustein 555** zeigt, wie Blink- und Zeitmodul als ein einziger Chip aussehen. Neu ist das **OP-Modul** (Operationsverstärker) für Komparator, Verstärker, Mischpult und Filter.

Bedienung, Gestaltung, Lernpfad, Forschungskreislauf, Forscherheft, Werkzeugkasten, Elternbereich, Profile, Aufgaben der Lehrkraft und alle Regeln zu Datenschutz, Kindersicherheit und Barrierefreiheit bleiben wie in Volta 1. Der Lernpfad umfasst **neun Stufen (0–8) mit 61 Experimenten**, so viele wie Volta 1.

Der technisch schwierigste Teil ist erledigt: Ein Leistungstest (14.10.2026) hat gezeigt, dass die hörbare Simulation mit 20 µs Schrittweite für Schaltungen mit 555, OP und linearen Stufen auch auf einem Schultablet in Echtzeit läuft.

**Empfehlung:** Dieses Konzept als Grundlage festschreiben, die Stufen- und Experimentliste (Kapitel 8) gegenlesen und freigeben und danach mit Schritt 2 (Band-Infrastruktur) beginnen. Volta 1 bleibt auf dem Stand 0.9; Korrekturen aus den Vorführungen fließen als 0.9.x ein.

## 2. Einordnung in die Reihe

| Band | Alter | Schwerpunkt | Messen und Zeigen |
|---|---|---|---|
| Volta 1 | 8–12 | Stromkreis, Reihe und parallel, Wirkungen des Stroms, Sensoren, erste Elektronik mit Modulen | ein Dreifach-Messgerät, Zeitschreiber |
| **Volta 2** | **11–14** | **Messtechnik, Wechselspannung, Netzteil, Transistorverstärker, Operationsverstärker, 555, Klang, Infrarot** | **zwei Messgeräte, Oszilloskop, Funktionsgenerator, hörbare Simulation** |
| Volta 3 | 13–16 | Schwingkreis und Radio, Schaltregler-Grundlagen, Verstärkerklassen, Messverstärker, digitale Grundschaltungen | Frequenzgang, Logikanalysator |
| Zuse 1 | ab 10 | Computertechnik: vom Gatter zum selbst gebauten Rechner (eigene Linie) | Bedienfeld, Wahrheitstabellen |

**Abgrenzung zu Volta 1:** Volta 2 setzt Volta 1 nicht voraus, knüpft aber daran an. Wer Volta 1 kennt, findet seine Module wieder und sieht hinein; wer neu einsteigt, bekommt in Stufe 0 eine kurze Einführung in Grundplatte, Pult und Messgerät.

**Abgrenzung zu Volta 3:** Schwingkreis, Resonanz und Radio, Schaltregler, Thyristor und Verstärkerklassen bleiben Volta 3. Digitale Gatter, Zähler und Kippstufen als Logik gehören zu Volta 3 bzw. zur Zuse-Linie; der 555 erscheint in Volta 2 nur mit seinem Blockschaltbild (Komparatoren, Flipflop), nicht aus Gattern.

**Lehrplanbezug (Sek. I):** elektrische Leistung, Kirchhoffsche Regeln, Kennlinien, Halbleiter (Diode, Transistor), Wechselspannung (Periode, Frequenz, Effektivwert), Gleichrichtung, Kondensator und Spule im Wechselstromkreis, Signalverarbeitung. Die genaue Zuordnung für Deutschland (Länder), Luxemburg und Ostbelgien folgt im Schulpaket.

## 3. Ziele und Nicht-Ziele

**Ziele**
- Analogelektronik **verstehen statt nachbauen**: jedes Experiment beantwortet eine Frage, die Simulation zeigt das Unsichtbare (Spannung, Strom, Signalform, Klang).
- **Messen lernen**: zwei Messgeräte, Handbereichswahl, Oszilloskop; Messfehler und Grenzen der Geräte offen ansprechen.
- **Rechnen mit Rechenweg**: Formeln der Sek. I im Formelrechner (Verstärkung, Grenzfrequenz, 555-Frequenz, Leistung), immer mit dem Rechenweg in einer Zeile.
- **Hören, was man baut**: Verstärker, Filter, Oszillatoren und Sirenen klingen so, wie die Schaltung es berechnet.
- **Die Module von Volta 1 entzaubern**: Verstärker-, Ton- und Blinkmodul werden aus Einzelteilen gebaut.

**Nicht-Ziele**
- Keine Netzspannung, keine Netztransformatoren, keine Hochspannung, keine Kfz-Elektrik (Regel 6). Die Wechselquelle ist eine Kleinspannungsquelle **im Pult**, ohne Bezug zum Stromnetz.
- Keine Hochfrequenz, kein Radio (Volta 3).
- Kein Mikrofon des Geräts als Eingang (Datenschutz, Berechtigungen); Tonquellen sind Funktionsgenerator und Schaltungen.
- Keine Kopie des gestalterischen Vorbilds (Regel 2); alle Experimente sind Eigenentwicklungen.

## 4. Entscheidungen (27.10.2026)

| Frage | Entscheidung |
|---|---|
| Volta 1 | bleibt auf dem Stand 0.9 (61 Experimente, Stufen 0–4); nur Korrekturen aus Vorführungen und Pilot als 0.9.x |
| Hörbare Simulation | **hörbar mit Rückfall**: hörbar, wo die Simulation in Echtzeit läuft (555, OP, lineare Verstärker und Filter); bei Transistor-Kippstufen im Hörbereich erklingt ein Ton in der **simulierten** Frequenz, mit offenem Hinweis; zusätzlich **Zeitlupe** |
| Bedienpult II | **Pultleiste oben wie Volta 1**, erweitert um zweite Batterie, Wechselquelle und Generator; Messgeräte und Oszilloskop **andockbar** oben im Pult oder frei neben der Grundplatte (wie das Messgerät in Volta 1) |
| Umfang | **9 Stufen (0–8), 61 Experimente** |
| Timer 556 | nicht aufgenommen; Schaltungen mit zwei Timern verwenden **zwei 555** |

## 5. Bedienung und Bedienpult II

Grundplatte, Kontaktfelder, gezogene Leitungen, Undo/Redo, Spannungsanzeige, Stromfluss, Schaltplan, Aufbauhilfe und Werkzeugkasten sind dieselben wie in Volta 1. Volta 2 beginnt mit der breiten Grundplatte (12 Spalten); 8 und 10 bleiben wählbar.

### 5.1 Pultleiste

| Pultfeld | Bauteil | Bedeutung |
|---|---|---|
| `+9V`, `0V` | Batterie 1 | wie Volta 1, wählbar 3 / 4,5 / 9 V |
| `+B2`, `−B2` | Batterie 2 (neu) | zweite, getrennte Batterie 4,5 / 9 V. In Reihe mit Batterie 1 gesteckt ergibt sie eine **symmetrische Versorgung** (+4,5 / 0 / −4,5 V) für den OP – das Kind steckt sie selbst (Experiment 0.3) |
| `W~a`, `W~b` | Wechselquelle (neu) | Kleinspannung, wählbar 6 / 9 V~ (Effektivwert), 50 Hz; Hauptschalter wie Batterie 1 |
| `G`, `G0` | Funktionsgenerator (neu) | Ausgang und Masse; Bedienung im Generatorfenster |
| `P1a`, `P1s`, `P1b`, `P2…` | Drehregler P1, P2 | wie Volta 1 |
| `S1`, `S2` | Schalter | wie Volta 1 |
| `LS1`, `LS2` | Lautsprecher | wie Volta 1; gibt in Volta 2 das **simulierte** Signal wieder |

Die Pultleiste wird bei schmalen Fenstern zweizeilig; jedes Feld behält Namen und Schaltzeichen (Gruppenlinie wie Volta 1).

### 5.2 Messgeräte und Anzeigegeräte

- **Zwei Dreifach-Messgeräte** (Volt, Ampere, Ohm), farblich und mit »1« / »2« beschriftet. Neu: **Handbereichswahl** (200 mV … 20 V, 2 mA … 2 A, 200 Ω … 2 MΩ) mit Überlaufanzeige »1«, wahlweise selbsttätig wie in Volta 1; Gleich- und Wechselspannung (Effektivwert) umschaltbar.
- **Oszilloskop** (Ausbau des Zeitschreibers): zwei Kanäle (K1, K2) mit Messspitze auf ein beliebiges Kontaktfeld, Bezug ist `0V`; Zeitbasis 1 s … 100 µs je Teilung, Spannung je Teilung, Trigger (Flanke, Pegel, Kanal), Standbild, XY-Betrieb für Kennlinien. Kanäle sind durch Farbe **und** Beschriftung unterschieden; Periode, Frequenz und Spitzenwert stehen zusätzlich als Zahl daneben.
- **Funktionsgenerator**: Sinus, Dreieck, Rechteck; 1 Hz … 10 kHz (in Dekaden mit Feineinstellung); Amplitude 0 … 4 V Spitze; Gleichanteil −4 … +4 V. Bei 20 µs Schrittweite sind Signalformen bis etwa 5 kHz sauber dargestellt; darüber werden sie eckiger, hörbar bleiben sie (offengelegt unter »So rechnet Volta«).
- **Kennlinienschreiber** (Werkzeug): misst Strom über Spannung eines Bauteils zwischen zwei Feldern und zeichnet die Kennlinie (Widerstand, Glühlampe, Diode, LED, Z-Diode, Transistor).
- Alle Geräte sind **andockbar** (oben im Pult) oder frei neben der Grundplatte; auf kleinen Tablets wird das Oszilloskop als Blatt über der Grundplatte geöffnet.

### 5.3 Hörbare Simulation und Zeitlupe

- Liegt ein Lautsprecher in einer Schaltung, die in Echtzeit läuft, rechnet der Kern mit 20 µs Schrittweite und gibt das Signal über einen kurzen Vorlaufpuffer (ca. 50 ms) aus.
- Läuft die Schaltung langsamer als Echtzeit (Transistor-Kippstufen im Hörbereich, sehr große Aufbauten), schaltet Volta auf **Ton aus der simulierten Frequenz** um und sagt das offen: »Diese Schaltung ist zu aufwendig, um sie Schritt für Schritt hörbar zu rechnen. Du hörst einen Ton mit der Frequenz, die die Simulation ergibt.« Im Elternbereich steht es unter »So rechnet Volta«.
- **Zeitlupe** (1/10, 1/100, 1/1000) verlangsamt die Simulation; das Oszilloskop zeigt dann die Kippflanken, die Lampen blinken langsam. Ton ist in der Zeitlupe stumm.
- Lautstärke über das Gerät; ein Begrenzer schützt vor plötzlich lauten Signalen (Kinderohren).

## 6. Bauteile und Module

Neu in Volta 2 (Katalog `content/catalog/bauteile.json`, Feld `ab_band: 2`); alle Bauteile aus Volta 1 bleiben verfügbar.

| Bauteil / Modul | Modell im Kern | Referenz (Abweichung < 2 %) |
|---|---|---|
| Transistor PNP (BC557-artig) | Ebers-Moll wie NPN in Volta 1 | Kennlinie, Gegentaktstufe |
| Z-Diode 4,7 V und 5,6 V | Diode mit Durchbruch in Sperrrichtung | Kennlinie, Stabilisierung |
| Spannungsregler 5 V (78L05-artig) | Verhaltensmodell: Ausgang 5 V, Mindest-Spannungsabfall 1,7 V, Strombegrenzung, Wärme | Lastkennlinie |
| Spule 100 mH / 1 H | Induktivität mit Wicklungswiderstand | Strom an Wechselspannung |
| Folienkondensatoren 1 nF … 1 µF | wie Kondensator in Volta 1, neue Werte (Kondensatorcode) | RC-Glied |
| Trimmer 10 kΩ / 100 kΩ | Potentiometer auf der Grundplatte | – |
| IR-LED, Fototransistor | LED mit Strahlung ohne Licht; Fototransistor wie NPN mit Lichtstrom | Lichtschranke |
| Optokoppler | IR-LED und Fototransistor in einem Gehäuse, Übertragungsfaktor | Kennlinie |
| **OP-Modul** (neu) | Makromodell: Leerlaufverstärkung, Aussteuergrenzen (ca. 1,5 V unter der Versorgung), Anstiegsrate, Verstärkungs-Bandbreite-Produkt (1 MHz), Ausgangsstrombegrenzung | invertierender Verstärker, Integrator, Komparator |
| **Timer 555** (neu) | Verhaltensmodell (Teiler 3 × 5 kΩ, zwei Komparatoren, Flipflop, Entladung, Gegentakt-Ausgang) mit **Ereignisortung** im Schritt; Betriebsspannung 4,5–16 V | Frequenz und Tastverhältnis astabil, Zeit monostabil |

**Module in der Modulleiste** zeigen ihre Anschlüsse als schmale Felder mit Nummer und Kurzname (555: 1 GND, 2 TR, 3 OUT, 4 R, 5 CV, 6 THR, 7 DIS, 8 +U). »Blick hinein« zeigt beim 555 das Blockschaltbild, beim OP-Modul den Differenzverstärker mit Endstufe – jeweils mit den aktuellen Spannungen.

**Nachgebaute Module aus Volta 1:** Verstärkermodul (Gegentakt-Endstufe, Stufe 4), Tonmodul (Stufe 7), Blinkmodul (astabile Kippstufe, Stufe 7); das Zeitmodul wird in Stufe 6 durch den 555 ersetzt. Die Module aus Volta 1 bleiben in Volta 2 verfügbar, damit Projekte nicht an Nebensachen scheitern.

## 7. Werkzeugkasten

Wie Volta 1 (Farbcode, E-Reihen, Lexikon, Formelrechner, Fehlersuche, Teileliste, Zeitschreiber), ergänzt um:
- **Formelrechner**: Leistung P = U · I; Verstärkung (Transistor, OP invertierend/nichtinvertierend); Grenzfrequenz f = 1 / (2π R C); 555 astabil f ≈ 1,44 / ((R1 + 2 R2) C) und monostabil t ≈ 1,1 R C; Effektivwert; Brummspannung (Näherung). Immer mit Rechenweg.
- **E-Reihen bis E48**, **Kondensatorcodes** (104 = 100 nF).
- **Kennlinienschreiber** (siehe 5.2).
- **Fehlersuche** erweitert: OP ohne Versorgung, 555 unter 4,5 V, Elko an Wechselspannung, Z-Diode falsch herum, Transistor ohne Basiswiderstand.

## 8. Lernpfad

Aufbau wie Volta 1: Stufen mit Leitfrage, Experimente mit Forschungskreislauf (Frage → Vermutung → Aufbau → Beobachtung → Erklärung → »Mehr wissen« → »Jetzt in echt«), Freischaltung der Reihe nach, Projekte am Ende einer Stufe. Die Liste ist ein Entwurf zur Freigabe; Bezeichnungen der Experimente sind Arbeitstitel.

### Stufe 0 – Auftakt: Was ist neu auf meinem Pult? (3)
- 0.1 **Zwei Messgeräte zugleich** – Spannung und Strom an einer Lampe, daraus die Leistung.
- 0.2 **Den Messbereich selbst wählen** – Handbereichswahl, Überlauf »1«, warum ein kleiner Bereich genauer ist.
- 0.3 **Plus, Minus und die Mitte** – zwei Batterien in Reihe, die Mitte als Bezug: Die Spannungsanzeige zeigt zum ersten Mal negative Spannungen.

### Stufe 1 – Genauer messen: Gilt das immer – und wie genau? (6)
- 1.1 **Die Knotenregel** – Ströme an einer Verzweigung messen: Was hineinfließt, fließt hinaus.
- 1.2 **Die Maschenregel** – Spannungen in einem Kreis zusammenzählen.
- 1.3 **Kennlinie eines Widerstands** – der Kennlinienschreiber zeichnet eine Gerade.
- 1.4 **Warum die Glühlampe keine Gerade hat** – der Widerstand wächst mit der Temperatur.
- 1.5 **Diode und LED** – Schwellspannung je Farbe; Durchlass und Sperre.
- 1.6 **Die Batterie gibt nach** – Innenwiderstand: Die Spannung sinkt unter Last.

### Stufe 2 – Wechselspannung sichtbar: Was zeigt ein Oszilloskop? (8)
- 2.1 **Das Oszilloskop** – Gleichspannung als Strich, der Blinker als Rechteck.
- 2.2 **Der Funktionsgenerator** – Sinus, Dreieck, Rechteck; Periode ablesen, Frequenz berechnen.
- 2.3 **Sehen und hören** – Generator am Lautsprecher: Die Tonhöhe ist die Frequenz (hörbar).
- 2.4 **Spitze oder Mittel?** – Das Messgerät zeigt bei Wechselspannung weniger als die Spitze am Oszilloskop: der Effektivwert.
- 2.5 **Kondensator an Wechselspannung** – je höher die Frequenz, desto mehr Strom.
- 2.6 **Spule an Wechselspannung** – genau umgekehrt.
- 2.7 **Höhen weg: der Tiefpass** – RC-Glied; der Ton klingt dumpfer, Grenzfrequenz im Formelrechner.
- 2.8 **Bässe weg: der Hochpass** – Kondensator und Widerstand vertauscht.

### Stufe 3 – Gleichrichten und stabilisieren: Wie wird aus Wechselspannung eine ruhige Gleichspannung? (7)
- 3.1 **Die Wechselquelle** – zwei LEDs gegeneinander: Plus und Minus wechseln ständig, 50 Schwingungen je Sekunde (die Zeitlupe zeigt es).
- 3.2 **Nur die Hälfte** – der Einweggleichrichter mit einer Diode.
- 3.3 **Beide Hälften** – der Brückengleichrichter aus vier Dioden.
- 3.4 **Glätten mit dem Elko** – Brummspannung am Oszilloskop; mehr Last, mehr Brumm.
- 3.5 **Die Z-Diode hält fest** – Kennlinie in Sperrrichtung, einfache Stabilisierung.
- 3.6 **Immer 5 Volt** – der Spannungsregler, auch wenn die Batterie nachlässt.
- 3.7 **Projekt: ein Netzteil** – Wechselquelle (9 V~), Gleichrichter, Elko, Regler, Kontroll-LED; Messung unter Last. Mit 6 V~ reicht die Spannung nach Gleichrichter und Brumm nicht für den Regler – ein Grund zum Nachrechnen.

### Stufe 4 – Der Transistor als Verstärker: Wie wird ein leises Signal laut? (8)
- 4.1 **Kleiner Strom steuert großen Strom** – Basis- und Kollektorstrom mit zwei Messgeräten, Stromverstärkung.
- 4.2 **Der Arbeitspunkt** – Basisspannungsteiler: Kollektor auf halber Betriebsspannung.
- 4.3 **Die Emitterschaltung** – Wechselsignal verstärken; Eingang und Ausgang am Oszilloskop (umgedreht!).
- 4.4 **Übersteuert!** – zu großes Signal wird abgeschnitten, hörbar verzerrt.
- 4.5 **Weniger ist stabiler** – Gegenkopplung mit Emitterwiderstand.
- 4.6 **Zwei Stufen** – Koppelkondensator zwischen zwei Verstärkerstufen.
- 4.7 **Das Verstärkermodul von innen** – Gegentakt-Endstufe aus NPN und PNP; Übernahmeverzerrung sehen und hören.
- 4.8 **Projekt: Audioverstärker** – Generator, Vorstufe, Endstufe, Lautsprecher, Lautstärke mit P1.

### Stufe 5 – Operationsverstärker: Was kann ein Verstärker mit zwei Eingängen? (8)
- 5.1 **Das OP-Modul vergleicht** – Plus- gegen Minus-Eingang: der Komparator.
- 5.2 **Dämmerungsschalter** – Komparator mit Fotowiderstand.
- 5.3 **Kein Flackern mehr** – Schmitt-Trigger: zwei Schaltschwellen.
- 5.4 **Der invertierende Verstärker** – Verstärkung = R2 / R1, mit Rechenweg.
- 5.5 **Nichtinvertierend und Spannungsfolger** – verstärken, ohne umzudrehen; entkoppeln.
- 5.6 **Das Mischpult** – Summierer für zwei Töne.
- 5.7 **Aus Rechteck wird Dreieck** – der Integrator; »Mehr wissen«: Grenzen des OP (Anstiegsrate).
- 5.8 **Projekt: Klangregler** – aktiver Tief- und Hochpass, Bässe und Höhen mit P1 und P2.

### Stufe 6 – Der Timer-Baustein 555: Ein Chip für Takt und Zeit? (9)
Reihenfolge nach Stufe 5 zwingend: Der 555 wird mit den Komparatoren erklärt.
- 6.1 **Der 555 blinkt** – astabil; Frequenz aus R1, R2 und C im Formelrechner.
- 6.2 **Puls und Pause** – Tastverhältnis messen; mit einer Diode gleich lang.
- 6.3 **Blick hinein** – die Kondensatorspannung pendelt zwischen 1/3 und 2/3 der Betriebsspannung.
- 6.4 **Der Zeitschalter** – monostabil: das Treppenhauslicht aus Volta 1 mit einem Chip.
- 6.5 **Stopp und Wechsel** – Reset-Taster; zwei LEDs blinken im Wechsel.
- 6.6 **Ein Speicher mit zwei Tastern** – bistabil: das Flipflop im Inneren wird sichtbar.
- 6.7 **Ein Ton aus dem Chip** – Lautsprecher am Ausgang, Tonhöhe mit P1 (hörbar).
- 6.8 **Helligkeit stufenlos** – Pulsweite mit Dioden und Drehregler; am Oszilloskop sichtbar.
- 6.9 **Projekt: Reaktionstester** – kurzer Lichtblitz, wer trifft ihn mit dem Taster?

### Stufe 7 – Schwingungen und Klang: Wie entsteht ein Ton? (6)
- 7.1 **Das Blinkmodul von innen** – astabile Kippstufe aus zwei Transistoren; in Zeitlupe die Kippflanken.
- 7.2 **Das Tonmodul von innen** – dieselbe Kippstufe, schneller (Ton aus der simulierten Frequenz, offen erklärt).
- 7.3 **Zweitonsirene** – ein zweiter 555 steuert über Pin 5 die Tonhöhe des ersten.
- 7.4 **Heulton** – auf- und abschwellende Tonhöhe.
- 7.5 **Ein reiner Sinus** – Wien-Brücken-Oszillator mit dem OP.
- 7.6 **Projekt: kleiner Synthesizer** – Tasten S1/S2, Tonhöhe, Lautstärke und Klangfilter.

### Stufe 8 – Licht, das man nicht sieht: Wie funktioniert eine Lichtschranke? (6)
- 8.1 **Unsichtbares Licht** – die IR-LED leuchtet, aber man sieht nichts; »Jetzt in echt«: die Kamera vieler Handys (oft die vordere) zeigt das Leuchten einer Fernbedienung.
- 8.2 **Der Fototransistor** – Strom wächst mit dem Licht.
- 8.3 **Die Lichtschranke** – Unterbrechung löst den 555-Ton aus.
- 8.4 **Zwei Kreise ohne Verbindung** – der Optokoppler (Vergleich mit dem Relais aus Volta 1).
- 8.5 **Ton über Licht** – Generator, IR-LED, Fototransistor, Verstärker: Musik über eine Lichtstrecke.
- 8.6 **Projekt: Alarmanlage** – Lichtschranke, Zeitschalter und Sirene.

**Summe:** 3 + 6 + 8 + 7 + 8 + 8 + 9 + 6 + 6 = **61 Experimente**, davon 7 Projekte. Weitere Ideen aus der 555-Quellensammlung (Morsetaste, Frequenzteiler, Kondensatoren messen, Herzschlag-Wächter) stehen für das **Freie Labor** und Aufgaben der Lehrkraft bereit.

## 9. Gestaltung und Begleitfigur

Gestaltung wie Volta 1 (hell, ruhige Flächen, DC Learning Design Standard). Volta 2 und Volta 3 tragen von Anfang an das neue Reihensymbol (`docs/design/symbole/volta2.svg`, `volta3.svg`): das Volta-Symbol mit LED-Balkenanzeige, in der zwei bzw. drei Felder leuchten (entschieden 27.10.2026). Volta 1 behält sein bisheriges Symbol und wechselt erst mit Version 1.0 auf `volta1.svg`. Offen ist nur noch eine Kennfarbe für den Band. Die Begleitfigur **Tick** bleibt, mit etwas sachlicherem Ton für die ältere Zielgruppe; die Ebene »Mehr wissen« wird in Volta 2 öfter genutzt (Formeln, Hintergründe). Die Texte duzen weiterhin.

## 10. Technik

| Bereich | Umsetzung | Stand |
|---|---|---|
| Edition | `VOLTA_EDITION=volta2`, eigene App-ID, eigenes Inhaltspaket `content/volta2`, Katalog mit Bandfreigabe (`ab_band`) | im Build-System vorgesehen |
| Audio-Schrittweite | 20 µs, Faktorisierung wiederverwenden, solange die Matrix gleich bleibt | erledigt (Leistungstest 14.10.2026) |
| 555 | Verhaltensmodell mit Ereignisortung (Teilschritt bis zum Schwellendurchgang) | Prototyp im Leistungstest; Ereignisortung offen |
| OP | Makromodell (siehe Kapitel 6) | offen |
| Quellen | Wechselquelle, Funktionsgenerator als zeitabhängige Quellen im Kern | offen |
| Tonausgabe | Rechnung im Puffer mit Vorlauf, Ausgabe über Qt Multimedia; Echtzeitprüfung und Rückfall | offen |
| Oszilloskop | Ringpuffer je Kanal im Kern, Trigger im Modell, Darstellung in QML (Qt Quick Shapes) | Zeitschreiber als Grundlage vorhanden |
| Messwerte | Veröffentlichung an die Oberfläche höchstens 60-mal je Sekunde, unabhängig von der Schrittweite | offen |

Alle Regeln von Volta 1 gelten unverändert: Kern ohne Qt, jede Aufbauänderung ein Befehl, Physik ehrlich (Referenztests < 2 %), kein Netz, keine Datenerhebung, Bedienelemente ≥ 48 dp, Farbe nie allein, DE/FR/EN/LU.

## 11. Datenschutz und Kindersicherheit

- Wie Volta 1: kein Konto, keine Analyse, keine Werbung, keine Käufe in der App, kein Netz.
- Kein Zugriff auf Mikrofon oder Kamera des Geräts (Fotos im Forscherheft über die Systemauswahl wie in Volta 1).
- Die Wechselquelle ist ausdrücklich eine **Kleinspannung im Pult**; Texte und »Jetzt in echt« verweisen nie auf Netzteile zum Selbstbau am Stromnetz.
- Lautstärkebegrenzer für die hörbare Simulation.
- »Nur in der App!« für zerstörende Versuche (z. B. Spannungsregler ohne Kühlung überlasten, Transistor ohne Basiswiderstand).

## 12. Umsetzungsplan

| Schritt | Inhalt | PT | Ergebnis |
|---|---|---|---|
| 1 Leistungstest Audio | 20-µs-Schritte, Faktorisierung wiederverwenden, Messung auf dem Tablet | – | **erledigt** |
| 2 Band-Infrastruktur | Edition volta2, Inhaltspaket, Bandfreigabe im Katalog, Pultleiste II, App-Symbol | 15 | zweite App aus derselben Codebasis |
| 3 Bauteilmodelle | PNP, Z-Diode, Regler, Spule, Trimmer, IR-LED, Fototransistor, Optokoppler, OP-Makromodell, 555 mit Ereignisortung, Wechselquelle, Generator | 30 | Referenztests (< 2 %) |
| 4 Geräte und Ton | zweites Messgerät mit Handbereichswahl, Oszilloskop, Funktionsgenerator, Kennlinienschreiber, Tonausgabe mit Rückfall, Zeitlupe | 35 | Bedienpult II |
| 5 Inhalte | 61 Experimente mit Musteraufbau und Prüfbedingungen, Lexikon, Formelrechner, Hilfe; FR/EN/LU | 50 | inhaltlich vollständiger Band |
| 6 Plattformen | Android-APK, Windows, Mac; Prüfung auf allen Tablets; Anleitung und Flyer | 15 | Testversion Volta 2 (0.9) |
| | **Summe** | **145** | (Konzept v3.0: 130–150 PT) |

Schritte 2 und 3 können beginnen, sobald die Experimentliste freigegeben ist; Schritt 5 setzt 3 und 4 voraus.

## 13. Risiken und Maßnahmen

| Risiko | Maßnahme |
|---|---|
| Hörbare Simulation auf langsamen Tablets zu langsam | Echtzeitprüfung je Schaltung, Rückfall auf Ton aus der simulierten Frequenz (entschieden), Messwerte seltener veröffentlichen |
| Oszilloskop auf kleinen Tablets zu klein | Blattansicht über der Grundplatte, Werte zusätzlich als Zahl |
| OP-Modell zu ideal oder zu kompliziert | Makromodell mit wenigen, im Lexikon offengelegten Grenzen; Referenztests gegen Datenblattwerte |
| Inhaltlicher Umfang (61 Experimente) | Musteraufbau und Prüfbedingungen mit denselben Werkzeugen wie Volta 1; Projekte wiederverwenden Teilschaltungen |
| Abgrenzung zu Volta 3 und Zuse verschwimmt | Digitaltechnik nur über den 555 (Blockschaltbild); Radio und Schwingkreis bleiben Volta 3 |
| Rückmeldungen der Vorführungen zu Volta 1 betreffen die gemeinsame Bedienung | Korrekturen zuerst in Volta 1 (0.9.x), Volta 2 übernimmt sie automatisch (gemeinsame Codebasis) |

## 14. Offene Entscheidungen

| Entscheidung | Spätestens |
|---|---|
| Freigabe der Stufen- und Experimentliste (Kapitel 8) | vor Schritt 2 |
| Kennfarbe von Volta 2 (App-Symbol entschieden: Reihensymbol `volta2.svg`) | Schritt 2 |
| Wechselquelle: fest 50 Hz oder einstellbar (für die Experimente reicht fest) | Schritt 3 |
| Verkauf: eigene App oder Freischaltung in einer gemeinsamen App (abhängig von Verlagsantworten) | vor Schritt 6 |
| Lehrplanzuordnung DE/LU/Ostbelgien | Schulpaket |

## 15. Fazit

Volta 2 ist technisch abgesichert (hörbare Simulation geprüft), inhaltlich umrissen (61 Experimente in neun Stufen) und baut vollständig auf der fertigen Plattform von Volta 1 auf. Der Aufwand liegt bei rund 145 Personentagen. Nächster Schritt ist die Freigabe der Experimentliste; danach folgen Band-Infrastruktur und Bauteilmodelle.
