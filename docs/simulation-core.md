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
  `exp` wird oberhalb des Arguments 80 linear fortgesetzt. GMIN liegt zusaetzlich parallel zu jedem Uebergang.
- Bahnwiderstaende liegen ueber einen inneren Knoten des Modells (`internalNodeCount`); innere Knoten sind in
  der `Probe` nicht sichtbar und erhalten kein GMIN gegen Masse.
- Abbruch: |dV| < 1e-6 V und |dI| < 1e-9 A (jeweils zuzueglich 1e-6 relativ zum Betrag, damit Stroeme im
  Amperebereich nicht an der Rechengenauigkeit scheitern) oder max. 50 Iterationen. Eine Iteration, in der
  ein Modell die Spannung begrenzt hat, gilt nie als konvergiert.
- Bei Nichtkonvergenz: Zeitschritt halbieren (bis 1/16), dann `SimEvent::ConvergenceTrouble` und
  letzter gueltiger Zustand bleibt erhalten. Nie abstuerzen, nie NaN an die Oberflaeche geben.

### Zeitintegration
- Rueckwaerts-Euler (implizit, A-stabil) mit Begleitmodellen fuer Kondensator und Spule.
- Volta 1: feste Schrittweite 1 ms, Echtzeitbezug; Zeitraffer x1, x10, x100.
- Ab Volta 2: variable Schrittweite bis ca. 20 µs (Audio bis 20 kHz), Audioberechnung im Puffer mit kurzem Vorlauf;
  optional Trapezregel mit Daempfung gegen Klingeln.
- Nach Topologiewechsel und nach Undo: DC-Arbeitspunkt mit Quellenrampe (Source Stepping) als Startwert.
  Gespeicherte Zustaende (Kondensatorladung, Drehzahl) gehen dabei als Anfangsbedingungen ein: ein geladener
  Kondensator wirkt im Arbeitspunkt als Spannungsquelle mit seiner Ladespannung (wie SPICE `UIC`), damit
  Leitungswechsel und Undo die Ladung nicht verlieren. Umsetzung mit dem Kondensatormodell (Phase 0, Schritt 3).
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
| `batterie` | Reihe aus 1,5-V-Zellen, waehlbar 3 V / 4,5 V / 9 V (2/3/6 Zellen); Ri ≈ 0,25 Ω je Zelle (9 V ≈ 1,5 Ω) | Erwaermung bei Kurzschluss, Kapazitaet optional | `BatteryHot` |
| `gluehlampe` | R(T): kalt ≈ R_heiss/10, Faden mit eigener Zeitkonstante | Durchbrennen bei Ueberspannung | `Destroyed` |
| `led_rot`, `led_gruen`, `led_gelb`, `led_blau` | Shockley-Diode + Bahnwiderstand (innerer Knoten); Is aus `uf_V` bei 20 mA; n = 2 (blau 3), Rs = 5 Ω (blau 10 Ω); Uf rot 1,9 / gelb 2,0 / gruen 2,1 / blau 3,1 V | Imax 20 mA dauerhaft, thermisch | `Destroyed` |
| `diode_si`, `diode_ge` | Shockley-Diode + Bahnwiderstand; Si wie 1N4148 (0,70 V bei 10 mA, n = 1,75, Rs = 0,57 Ω), Ge 0,35 V bei 10 mA | Verlustleistung | `Destroyed` |
| `widerstand` | linear, E12-Werte, Farbringe | 0,25 W | `Destroyed` |
| `potentiometer` | zwei Teilwiderstaende, Stellung 0..1 | 0,25 W | – |
| `kondensator`, `elko` | C mit Leckwiderstand; Elko gepolt | Verpolung/Ueberspannung | `Destroyed` |
| `spule`, `relais` | L mit R; Relaiskontakt ab Ansprechstrom mit Hysterese | – | `RelayClick` |
| `motor` | Ra + Gegenspannung k·ω, Traegheit J, Reibung | Blockade erwaermt | `Destroyed` |
| `summer`, `piezo` | Lastwiderstand + Tonerzeugung aus Strom | – | Audio |
| `lautsprecher` | 8 Ω, Signal -> Klanganzeige (Frequenz, Pegel, Impulse) | – | Audio |
| `transistor_npn` | vereinfachtes Ebers-Moll | Verlustleistung | `Destroyed` |
| `fotowiderstand`, `heissleiter`, `fototransistor` | R bzw. I abhaengig von Licht/Temperatur (Eingabe) | – | – |
| `messgeraet` | V: Ri 10 MΩ (digital) bzw. 20 kΩ/V (analog); A: Rshunt 0,1 Ω mit Sicherung; Ω: Messstrom 1 mA | Sicherung loest aus | `FuseBlown` |

## Module

- **Teilschaltung**: Modul ist eine Unterschaltung aus Katalogbauteilen, wird beim Netzlistenaufbau expandiert
  (Blinkmodul, Tonmodul, Zeitmodul, Verstaerkermodul).
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
| `Probe` | `probe.hpp` | Knotenpotentiale und `ComponentReading` je Bauteil (inkl. Strom je Anschluss) |
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
