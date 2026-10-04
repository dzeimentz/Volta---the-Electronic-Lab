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
- Aufbau und Loesung des Gleichungssystems sind getrennt (Vorleistung fuer Volta 2).

### Nichtlineare Bauteile – Newton-Raphson
- Dioden, Leuchtdioden, Transistoren, Gluehlampe (temperaturabhaengig) werden je Iteration linearisiert.
- Spannungsbegrenzung je Iteration fuer pn-Uebergaenge (Begrenzung nach SPICE-Art ueber `Vcrit`).
- Abbruch: |dV| < 1e-6 V und |dI| < 1e-9 A oder max. 50 Iterationen.
- Bei Nichtkonvergenz: Zeitschritt halbieren (bis 1/16), dann `SimEvent::ConvergenceTrouble` und
  letzter gueltiger Zustand bleibt erhalten. Nie abstuerzen, nie NaN an die Oberflaeche geben.

### Zeitintegration
- Rueckwaerts-Euler (implizit, A-stabil) mit Begleitmodellen fuer Kondensator und Spule.
- Volta 1: feste Schrittweite 1 ms, Echtzeitbezug; Zeitraffer x1, x10, x100.
- Ab Volta 2: variable Schrittweite bis 10 µs (Audio bis 20 kHz), Audioberechnung im Puffer mit kurzem Vorlauf;
  optional Trapezregel mit Daempfung gegen Klingeln.
- Nach Topologiewechsel und nach Undo: DC-Arbeitspunkt mit Quellenrampe (Source Stepping) als Startwert.

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
| `batterie_9v` | Quelle 9 V (umschaltbar 4,5 V) mit Ri ≈ 1,5 Ω | Erwaermung bei Kurzschluss | `BatteryHot` |
| `gluehlampe` | R(T): kalt ≈ R_heiss/10, Faden mit eigener Zeitkonstante | Durchbrennen bei Ueberspannung | `Destroyed` |
| `led_rot`, `led_gruen`, `led_gelb`, `led_blau` | Shockley-Diode + Bahnwiderstand, Uf 1,8–3,2 V | Imax 20 mA dauerhaft, thermisch | `Destroyed` |
| `diode_si`, `diode_ge` | Shockley-Diode | Verlustleistung | `Destroyed` |
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

## Messschnittstelle

`Probe` liefert fuer jeden Knoten das Potential und fuer jede Bauteilinstanz Strom, Spannung, Leistung,
Temperatur und Zustand. Die Spannungsanzeige (4 Balken) und das Dreifach-Messgeraet lesen ausschliesslich ueber
`Probe`. Das Messgeraet selbst ist ein Bauteil im Gleichungssystem und belastet die Schaltung realistisch.

## Erweiterungen (Vorleistungen in Phase 0)

1. Variable Schrittweite im Integrator (Schnittstelle schon in Volta 1).
2. Trennung Topologie/Werte fuer Wiederverwendung der Faktorisierung.
3. Messschnittstelle fuer beliebige Knoten und Zweige.
4. Audiopuffer-Schnittstelle (`AudioSink`), in Volta 1 nur fuer Summer/Lautsprecher genutzt.
