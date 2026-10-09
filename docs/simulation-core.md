# Simulationskern

Der Kern (`core/`) ist eine Qt-freie C++17-Bibliothek. Er wird gegen analytisch berechnete Referenzschaltungen
getestet (Abweichung < 2 %) und ist so angelegt, dass Volta 2 (Audio, Operationsverstaerker), Volta 3
(Schwingkreis, Digitaltechnik) und Zuse 1 (Logiksimulation) ihn erweitern, ohne ihn neu zu schreiben.

## Verfahren

### Modifizierte Knotenpotentialanalyse (MNA)
- Unbekannte: Knotenpotentiale (Masse = Knoten 0, entspricht Pultfeld `0V`) und Zweigstroeme der
  Spannungsquellen sowie der Bauteile, die einen Strom als Unbekannte benoetigen (Spule, Messgeraet im A-Modus).
- Jedes Bauteil »stempelt« seinen Beitrag in Matrix `G` und Vektor `b` (`ComponentModel::stamp`).
- Volta 1: < 100 Knoten -> dichte LU-Zerlegung mit Teilpivotsuche.
- Ab Volta 2: dünn besetzte LU-Zerlegung; Wiederverwendung der symbolischen Faktorisierung bei unveraenderter
  Topologie; numerische Refaktorisierung nur bei geaenderten Werten.
- Aufbau und Loesung des Gleichungssystems sind getrennt (Vorleistung fuer Volta 2): `MnaSystem` nimmt die
  Stempel auf, `DenseLu` zerlegt und loest. Beide fordern Speicher nur beim Topologiewechsel an.
- Vorzeichen: `terminalCurrent(t)` ist der Strom, der am Anschluss `t` in das Bauteil hineinfliesst. Eine
  entladende Batterie hat daher am Pluspol einen negativen Strom und eine negative (abgegebene) Leistung.

### Nichtlineare Bauteile – Newton-Raphson
- Dioden, Leuchtdioden, Transistoren, Gluehlampe (temperaturabhaengig) werden je Iteration linearisiert.
- Spannungsbegrenzung je Iteration fuer pn-Uebergaenge (`pnjlim` nach SPICE3 ueber `Vcrit = n·Vt·ln(n·Vt/(√2·Is))`);
  die Kennlinie `Is·exp` wird dort linear fortgesetzt, wo ihre Steigung die Leitwertgrenze erreicht (Transistor
  ca. 0,85 V / 6 A). Ableitung und Kennlinie bleiben so konsistent, und auch ein Uebergang direkt an der Batterie
  konvergiert sofort (gemeinsame Hilfen in `src/components/pn_junction.hpp`). GMIN liegt zusaetzlich parallel zu jedem Uebergang.
- Bahnwiderstaende liegen ueber einen inneren Knoten des Modells (`internalNodeCount`); innere Knoten sind in
  der `Probe` nicht sichtbar und erhalten kein GMIN gegen Masse.
- Abbruch: |dV| < 1e-6 V und |dI| < 1e-9 A (jeweils zuzueglich 1e-6 relativ zum Betrag, damit Stroeme im
  Amperebereich nicht an der Rechengenauigkeit scheitern) oder max. 50 Iterationen im Arbeitspunkt bzw. 20 im
  Zeitschritt (SPICE: 10); je `step()` hoechstens 3000 Iterationen insgesamt (Echtzeitbudget). Eine Iteration, in der
  ein Modell die Spannung begrenzt hat, gilt nie als konvergiert.
- Bei Nichtkonvergenz: Zeitschritt halbieren (bis 2^-24, d. h. ca. 60 ps bei 1 ms), dann
  `SimEvent::ConvergenceTrouble` und letzter gueltiger Zustand bleibt erhalten. Nie abstuerzen, nie NaN an die
  Oberflaeche geben. Die tiefe Halbierung wird nur beim Umkippen rueckgekoppelter Schaltungen (Kippstufen)
  gebraucht: Dort waechst die Abweichung mit gm/C ≈ 1e9/s, und erst die Sperrschichtkapazitaeten der Transistoren
  geben dem Umschalten eine aufloesbare Zeitkonstante. Gemessen: Kippstufen aus Katalogteilen (RC 100–1000 Ω)
  erreichen Tiefe 21 (Reserve 3), hoechstens ca. 1000 Iterationen je Schritt; Blinkmodul 10 s Simulation in
  ca. 0,5 s (Debug-Build).
- Leistung (Desktop, Release, `volta_core_bench`): 42 Bauteile inkl. Blinkmodul und Messgeraet, die ersten
  1000 Schritte nach dem Einschalten (mit Anlauf von Kippstufe, Motor und Lampe) in ca. 41 ms (Vorgabe < 50 ms);
  schlechtester Einzelschritt ca. 8 ms beim Umkippen der Kippstufe (dichte LU mit ca. 45
  Unbekannten, bis ca. 780 Iterationen). Auf dem Referenztablet (Faktor 5–10) ist das ein spuerbarer Ruckler je
  Umkippen; Abhilfe ab Volta 2 mit duenn besetzter LU und Wiederverwendung der Faktorisierung, bei Bedarf frueher.
- Pendeln: Kehrt im Zeitschritt die Newton-Korrektur seit der letzten Begrenzung insgesamt viermal die Richtung um, ohne auf die Haelfte zu schrumpfen
  und ohne Begrenzung, wird sofort halbiert (Kipppunkt einer Rueckkopplung). Gedaempftes Pendeln und langsame
  monotone Konvergenz bleiben unberuehrt; auf der tiefsten Halbierungsebene wird nicht frueh abgebrochen.
- Warmstart: Scheitert ein Versuch nur an der Iterationsgrenze und kam dabei voran (letzte Korrektur kleiner als
  die erste), setzt der halbe Schritt bei der letzten Naeherung
  fort (z. B. Diodenkette nach dem Schliessen eines Schalters), statt den Fortschritt zu verwerfen.
- Zustandswechsel mit elektrischer Wirkung (Zerstoerung, ausgeloeste Sicherung) werden in `checkLimits`
  vorgemerkt und erst vor dem naechsten Loesungsvorgang wirksam; Ereignis und Anzeige erscheinen sofort. So sind die
  Messwerte jedes Schritts in sich stimmig (Kirchhoff). Nach jedem Arbeitspunkt ruft der Simulator
  `updateState(x, 0)`, damit innere Zustaende (z. B. Transistorladungen) zum Arbeitspunkt passen.
- `beginSolve(ctx)` legt vor jedem Loesungsvorgang zustandsabhaengige Kennwerte fest (R(T) der Glühlampe, C/dt der
  Sperrschichtkapazitaeten), damit Stempel und Messwerte zueinander passen.

### Zeitintegration
- Rueckwaerts-Euler (implizit, A-stabil) mit Begleitmodellen fuer Kondensator und Spule.
- Volta 1: feste Schrittweite 1 ms, Echtzeitbezug; Zeitraffer x1, x10, x100.
- Ab Volta 2: variable Schrittweite bis ca. 20 µs (Audio bis 20 kHz), Audioberechnung im Puffer mit kurzem Vorlauf;
  optional Trapezregel mit Daempfung gegen Klingeln.
- Nach Topologiewechsel und nach Undo: Arbeitspunkt zum aktuellen Zeitpunkt mit Quellenrampe (Source Stepping)
  als Rueckfall. Gespeicherte Zustaende (Kondensatorladung, spaeter Drehzahl) gehen dabei als Anfangsbedingungen
  ein: ein Kondensator wirkt im Arbeitspunkt als Spannungsquelle seiner Ladespannung (Serienwiderstand
  `kMinOhm`, wie SPICE `UIC`), damit Leitungswechsel und Undo die Ladung nicht verlieren. Ein ungeladener
  Kondensator wirkt dort folgerichtig als Kurzschluss (Einschaltstrom).
- Kondensator im Zeitschritt als Spannungszweig: `V(a) - V(b) - (dt/C)·I = U_alt` (Rueckwaerts-Euler); damit
  bleibt die Ladung zweier verbundener Kondensatoren exakt erhalten.
- Begleitmodelle beziehen ihren Vorzustand nur aus dem eigenen Bauteilzustand (`updateState`), nicht aus dem
  letzten Loesungsvektor; so stellt `restoreState` (Undo) den Zustand vollstaendig her.

### Robustheit
- Ideale Schleifen aus Quellen und Leitungen vermeiden: Innenwiderstand der Batterie, Leitungswiderstand
  0,01 Ω je Leitung (in der Darstellung vernachlaessigt).
- Offene Knoten: Ableitwiderstand 1 GΩ gegen Masse (GMIN), damit `G` regulaer bleibt.
- Kurzschlusserkennung: Strom durch Quelle > Grenzwert -> Erwaermung Batterie und Leitung, Ereignis.

## Bauteilmodelle Volta 1

Kennwerte sind Ausgangswerte und werden in Phase 0 an realem Material kalibriert. Jedes Modell hat
einen elektrischen Teil und – wo didaktisch relevant – ein thermisches Modell erster Ordnung
(Waermekapazitaet `Cth`, Waermewiderstand `Rth` zur Umgebung, Grenztemperatur `Tmax`).

| Typ (`typ`) | Elektrisches Modell | Thermik/Grenzen | Ereignis |
|---|---|---|---|
| `batterie` | Reihe aus 1,5-V-Zellen, waehlbar 3 V / 4,5 V / 9 V (2/3/6 Zellen); Ri ≈ 0,25 Ω je Zelle (9 V ≈ 1,5 Ω) | Erwaermung durch I²·Ri (Rth 20 K/W, τ 60 s); heiss ab 60 °C, erneut erst nach Abkuehlen unter 45 °C; Kapazitaet optional | `BatteryHot` |
| `gluehlampe` | R(T) = R_kalt·(T/T0)^1,2, R_kalt = R_heiss/10; Faden strahlt ab: Cth·dT/dt = P − k·(T⁴ − T0⁴), τ 50 ms bei Nenntemperatur ca. 2000 K; Kennlinie I ~ U^0,54 (6-V-Lampe: 34 mA bei 3 V, 62 mA bei 9 V) | ab 1,25 × U_nenn Alterung (durchgebrannt nach 5 s), ab 1,6 × U_nenn sofort | `Destroyed` |
| `led_rot`, `led_gruen`, `led_gelb`, `led_blau` | Shockley-Diode + Bahnwiderstand (innerer Knoten); Is aus `uf_V` bei 20 mA; n = 2 (blau 3), Rs = 5 Ω (blau 10 Ω); Uf rot 1,9 / gelb 2,0 / gruen 2,1 / blau 3,1 V | Imax 20 mA dauerhaft, thermisch | `Destroyed` |
| `diode_si`, `diode_ge` | Shockley-Diode + Bahnwiderstand; Si wie 1N4148 (0,70 V bei 10 mA, n = 1,75, Rs = 0,57 Ω), Ge 0,35 V bei 10 mA | Verlustleistung | `Destroyed` |
| `widerstand` | linear, E12-Werte, Farbringe | 0,25 W | `Destroyed` |
| `potentiometer` | zwei Teilwiderstaende, Stellung 0..1 | 0,25 W | – |
| `kondensator`, `elko` | C als Spannungszweig (Rueckwaerts-Euler; im Arbeitspunkt Quelle der Ladung mit 10 µs/C) mit Leckwiderstand (Folie 1 GΩ, Elko 10 MΩ); Elko gepolt, 16 V | Verpolung > 1 V oder Ueberspannung laenger als 0,2 s -> offen | `Destroyed` |
| `leitung` (`Wire`) | 0,01 Ω | glueht ab 1 A (nur Anzeige), wird nie zerstoert | – |
| `spule`, `relais` | L mit R; Relaiskontakt ab Ansprechstrom mit Hysterese | – | `RelayClick` |
| `motor` | Ra 8 Ω + Gegenspannung k·ω (k = 0,013 V·s), Traegheit J, viskose Reibung und Haftreibung; implizit als Spannungszweig; Leerlauf 3 V ca. 2080 U/min | blockiert (Bedienhandlung) erwaermt: Wicklung 30 K/W, τ 20 s, 130 °C | `Destroyed` |
| `summer`, `piezo` | gepolte Last (Diode + 330 Ω, ca. 25 mA bei 9 V); Ton als Verhaltensanteil: Lautstaerke aus der Spannung (1,5–9 V), 2,4 kHz | wie Diode | Audio |
| `schalter`, `taster`, `reedkontakt`, `wechselschalter` | Kontakt 0,02 Ω, offen 1 nS; Stellung ist Bedienhandlung | – | – |
| `lautsprecher` | 8 Ω, Signal -> Klanganzeige (Frequenz, Pegel, Impulse) | – | Audio |
| `transistor_npn` | Ebers-Moll (Transportform) wie BC547B: Is 1,8e-14 A, βF 200, βR 7; Ladungsspeicher Q = C·V + τ·I (Cbe 10 pF, Cbc 4 pF, τF 0,5 ns, τR 10 ns); pnjlim je Uebergang | groesster Anschlussstrom > 100 mA ueberlastet, > 250 mA nach 3 s zerstoert; 250 K/W, τ 5 s, 150 °C | `Destroyed` |
| `fotowiderstand`, `heissleiter`, `fototransistor` | R bzw. I abhaengig von Licht/Temperatur (Eingabe) | – | – |
| `lautsprecher` (`Speaker`, Pultfelder LS1, LS2) | ohmsch 8 Ω; Auswertung fuer die Klanganzeige: Wechselanteil = Strom − gleitender Mittelwert (τ 0,3 s), Tonhoehe aus steigenden Nulldurchgaengen mit Hysterese (Stille nach 3 Perioden, mind. 1,5 s), level = Effektivwert/0,1 A, Impulszaehler; aufloesbar bis ca. 250 Hz (Schrittweite 1 ms) | Ueberlast > 0,5 W; 120 K/W, τ 5 s, zerstoert > 180 °C | `Destroyed` |
| `messgeraet` (`Meter`, Buchsen COM, V/Ω, A) | V: Ri 10 MΩ (digital) bzw. 20 kΩ/V × Bereich 1/3/10/30 V (analog, Bereich selbsttaetig); A-Buchse immer Shunt 0,1 Ω nach COM; Ω: Messquelle 3 V ueber 3 kΩ (≤ 1 mA), Anzeige U/I, »OL« offen oder > 40 MΩ, »Spannung abschalten!« bei Fremdspannung; Messart/Anzeigeart sind Bedienhandlungen; analog Zeigerausschlag in V, A (Anzeigebereiche 1 mA … 1 A) und Ω (level = Rm/(Rm+R), Skalenmitte Rm in Zehnerstufen), Bereich in `displayRange` | Sicherung > 0,5 A fuer 10 ms, danach A-Eingang offen | `FuseBlown` |

## Thermikmodell

Jedes Bauteil mit `ThermalParams` (Rth, Cth, Tmax) rechnet `Cth·dT/dt = P - (T - Tamb)/Rth` mit der exakten
Loesung je Schritt (stabil fuer jede Schrittweite). Rth folgt Datenblattwerten, die Zeitkonstanten sind
**didaktisch** gewaehlt, damit Erwaermung sichtbar wird (Zehntelsekunden bis Sekunden statt Millisekunden).
`overloaded` (Inhaltsformat: Zustand `ueberlastet`) zeigt eine Ueberschreitung des Dauergrenzwerts an,
`destroyed` folgt bei T > Tmax; zerstoerte Widerstaende und Dioden sind offen.

| Modell | Rth | τ | Tmax | Ueberlastet ab | Kennfall |
|---|---|---|---|---|---|
| Widerstand 0,25 W | 340 K/W | 10 s | 232 °C | P > 0,25 W | 100 Ω an 9 V brennt nach ca. 16 s durch |
| LED | 350 K/W | 12,6 s | 125 °C | I > 20 mA | ohne Vorwiderstand an 9 V zerstoert nach ca. 0,5 s; 50 ms Stoss: +11 K; ueber 50 mA (2,5 × Imax) zerstoert nach 3 s (z. B. 100 Ω an 9 V) |
| Si-Diode | 350 K/W | 5 s | 200 °C | I > 200 mA | – |
| Ge-Diode | 500 K/W | 3 s | 90 °C | I > 50 mA | – |
| Batterie | 20 K/W | 60 s | – | heiss > 60 °C | Kurzschluss ueber Leitung: heiss nach ca. 2,3 s |

Dioden und LEDs werden zusaetzlich zeitbasiert zerstoert, wenn der Strom laenger als 3 s ueber dem 2,5-fachen
Dauergrenzstrom liegt (vereinfachte Alterung; die reine Thermik wuerde eine LED erst ab ca. 120 mA zerstoeren).
Zwischen Dauergrenzwert und Zerstoerung liegt bewusst ein Bereich, in dem das Bauteil nur als
ueberlastet gilt (z. B. LED bei 40 mA). Die Oberflaeche zeigt ihn deutlich an (Gluehen), damit kein Kind
eine dauerhafte Ueberlast fuer unbedenklich haelt.

## Module

- **Teilschaltung**: Modul ist eine Unterschaltung aus Katalogbauteilen, wird beim Netzlistenaufbau expandiert
  (Blinkmodul, Zeitmodul, Verstaerkermodul). Die Aufbaufunktionen liegen im Kern (`modules/`), damit sie getestet
  und in VTS wiederverwendet werden koennen; die App ruft sie im `NetlistBuilder` auf. Bauteilkennungen
  `<Modul-ID>.<Name>` (z. B. `BL1.T1`), der belegte Bereich wird fuer »Blick hinein« zurueckgegeben.
  - Blinkmodul (`addBlinkModule`): astabile Kippstufe aus zwei NPN (RC 1 kΩ, RB 22/27 kΩ, 2 × 22 µF) mit
    Ausgangstransistor (offener Kollektor an `aus`); ca. 0,85 Hz, ein Widerstand von `takt` nach `plus`
    beschleunigt (4,7 kΩ: ca. 1,1 Hz). Die Basen schwingen beim Umkippen bis ca. −8 V; der reale
    Basis-Emitter-Durchbruch (BC547 meist 8–9 V) wird nicht nachgebildet und verkuerzt real die Halbperiode
    nur wenig. Bei »Blick hinein« und im Elternbereich erwaehnen.
  - Tonmodul (`addToneModule`, Klasse `ToneModule`): Ein Ton im Hoerbereich ist mit 1 ms Schrittweite nicht als
    Schwingung simulierbar. In Volta 1 deshalb ein Verhaltensmodell: Ersatzschaltbild aus Ruhestrom (4,7 kΩ
    plus–minus), Eingang (100 kΩ ein–minus, Schwelle 1,5 V mit Hysterese bis 1,0 V, Versorgung ab 3 V) und Endstufe
    (aus–minus, 100 Ω einschliesslich Schutzwiderstand, gerechnet mit dem Mittelwert bei 50 % Tastverhaeltnis, in
    Melodiepausen gesperrt). Die Melodie (8 Toene zu 0,2 s, eigene Tonfolge) liefert das Modell als Frequenz.
    Treibt die Endstufe den Pult-Lautsprecher (gleicher Strom in Lautsprecher und Ausgang, Abweichung hoechstens
    20 %), uebernimmt die Klanganzeige den Ton (`LabSession::speakerReading`). Im »Blick hinein« und im
    Elternbereich offengelegt; als Teilschaltung erst ab Volta 2 mit Audioschrittweite.
- **Makromodell** (ab Volta 2): Operationsverstaerker mit Aussteuergrenzen, Anstiegsrate, GBW.
- **Verhaltensmodell** (Volta 3): Empfangsmodule; liefern ein Tonsignal aus eigenen Aufnahmen, abhaengig von
  Abstimmung und Antenne. Im Elternbereich als solches offengelegt.
- **Logikmodell** (Volta 3, Zuse 1): ereignisgesteuerte Gatter mit Kopplung an analoge Pegel.

## Programmierschnittstelle (Phase 0)

| Typ | Datei | Aufgabe |
|---|---|---|
| `Circuit` | `circuit.hpp` | Knoten und Bauteile; verschiebbar, wird je Topologiewechsel neu uebergeben |
| `ComponentModel` | `component_model.hpp` | Bauteilschnittstelle inkl. `ComponentState` fuer Undo |
| `MnaSystem`, `DenseLu` | `mna_system.hpp`, `dense_lu.hpp` | Aufbau bzw. Loesung des Gleichungssystems |
| `Simulator` | `simulator.hpp` | `setCircuit` (Topologie, Arbeitspunkt), `step` (allokationsfrei), `probe`, `events` |
| `Probe` | `probe.hpp` | Knotenpotentiale und `ComponentReading` je Bauteil (Strom je Anschluss, Temperatur, `overloaded`, `destroyed`) |
| `EventQueue` | `sim_event.hpp` | Ringpuffer fester Groesse (256); bei Ueberlauf verworfen und gezaehlt |

Bedienhandlungen (z. B. Zellenzahl, spaeter Schalter und Regler) aendern Parameter ueber `Simulator::circuit()`
zwischen zwei Schritten; die Kopplung an den UI-Thread uebernimmt die App.

## Messschnittstelle

`Probe` liefert fuer jeden Knoten das Potential und fuer jede Bauteilinstanz Strom, Spannung, Leistung,
Temperatur und Zustand. Die Spannungsanzeige (4 Balken) und das Dreifach-Messgeraet lesen ausschliesslich ueber
`Probe`. Das Messgeraet selbst ist ein Bauteil im Gleichungssystem und belastet die Schaltung realistisch.

## Erweiterungen (Vorleistungen in Phase 0)

1. Variable Schrittweite im Integrator (Schnittstelle schon in Volta 1).
2. Trennung Topologie/Werte fuer Wiederverwendung der Faktorisierung.
3. Messschnittstelle fuer beliebige Knoten und Zweige.
4. Audiopuffer-Schnittstelle (`AudioSink`), in Volta 1 nur fuer Summer/Lautsprecher genutzt.
