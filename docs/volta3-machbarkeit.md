# Volta 3 – Machbarkeitstest (Umsetzungsplan Schritt 1)

Arbeitsgrundlage zu `docs/Konzept_Volta3.md` (Kapitel 4 und 12). Geprüft wird, ob die drei technischen
Grundsatzentscheidungen vom 31.10.2026 auf dem Tablet tragen. Zweig `volta3` (Worktree `volta-volta3`), begonnen am
04.11.2026.

Messprogramm: `core/bench/bench_volta3.cpp` → `volta_core_bench_volta3` (Release). Argumente: Dauerfaktor (Vorgabe 1),
optional `ohne` (Kern wie Volta 1/2: ohne Prädiktor, Toleranz 1e-6). Auf dem Tablet: mit NDK-Toolchain nach
`build/android-bench` bauen, per adb nach `/data/local/tmp` und dort starten.

## Neu im Kern

| Teil | Inhalt | Wirkung auf Volta 1/2 |
|---|---|---|
| Wechselstromanalyse | `Simulator::acSweep`: Linearisierung am angenommenen Zustand, komplexe Lösung je Frequenz mit derselben Knotenmatrix; `ComponentModel::stampAc` für Kondensator, Spule, Funktionsgenerator (Anregung); `ComplexMna`, komplexe LU | keine (neue Funktion) |
| Prädiktor | `Simulator::setPredictor`: Newton startet bei der linearen Fortschreibung | Vorgabe aus, bitgleich |
| Toleranz | `Simulator::setRelativeTolerance` (Vorgabe 1e-6; SPICE 1e-3) | Vorgabe unverändert, bitgleich |

Referenztests (`core/tests/test_ac_analysis.cpp`): RC-Tiefpass gegen 1/(1 + jωRC) (< 1e-4), Thomson-Frequenz eines
Parallelschwingkreises (< 2 %), Emitterschaltung: Kleinsignalverstärkung aus der Wechselstromanalyse gegen die
Amplitude im Zeitbereich (< 2 %), Zustand bleibt unverändert, Anregung wird geprüft; Prädiktor und Toleranz ändern
das Ergebnis nur im Rahmen der Toleranz und sparen Iterationen.

## Prüflinge (nur im Messprogramm)

- **Taktschalter** (Stellvertreter für MOSFET mit Ansteuerung): 0,2 Ω / 1 MΩ, Flanken über die Ereignisortung exakt.
- **Hüllkurvenquelle**: Spannung als Funktion der Zeit.
- **Gemittelter Hüllkurvendetektor**: Diodenstrom gemittelt über eine Trägerperiode,
  I(E, Vo) = Mittel über φ von g(E·cos φ − Vo); g aus Shockley-Kennlinie mit Bahnwiderstand und
  Resonanzwiderstand des Kreises (Tabelle, 32 Stützstellen dank Symmetrie). Annahme: Träger viel schneller als
  die RC-Last (bei Mittelwelle erfüllt).

## Ergebnisse (04.11.2026)

Desktop: MinGW 13, Release. Tablet: Meswoa MES-B3 (arm64, NDK 27, Release), Messung 04.11.2026. Die Rechenergebnisse sind auf beiden Geräten
ziffernweise gleich.
Desktop ist erfahrungsgemäß etwa 1,35-mal so schnell wie das Tablet (Audio-Leistungstest Volta 2).

| Prüfung | Vorgabe | Desktop | Tablet |
|---|---|---|---|
| Frequenzgang Schwingkreis, 200 Punkte | < 50 ms | 0,10 ms | 0,38 ms |
| Frequenzgang Verstärker mit Filter (20 Bauteile) | < 50 ms | 1,0 ms | 2,3 ms |
| Abstimmen (Aufbau, Arbeitspunkt, 3 Sender) | < 20 ms | 0,006 ms | 0,009 ms |
| Sendertrennung bei Kopplung 10 pF | > 10 dB | 34–40 dB | gleich |
| Abstimmung gegen Thomson-Formel | < 2 % | ≤ 0,3 % | gleich |
| Gemittelter Detektor gegen echten Träger 250 kHz (Ton) | < 2 % | −0,1 % | gleich (1,2 ms für 10 ms) |
| Einfache Hüllkurve (Spannung vor der Diode) | – | +51 … +57 % (verworfen) | gleich |
| Empfänger mit 2 Transistoren und Lautsprecher, 20 µs | ≥ 3-fach | 3,0-fach (3,4 Newton-Schritte) | **2,0-fach** (ohne Optionen 2,0-fach, 3,5 Schritte) |
| Abwärtswandler 1/2/5 kHz, 20 µs gegen 1 µs | < 2 % | 0,04–0,12 % | gleich, 8,4–14,8-fach |
| Aufwärtswandler nicht lückend, 20 µs | < 2 % | 0,79 % | gleich, 12,3-fach |
| Aufwärtswandler lückend, 20 µs | – | 6,9 % (siehe Befund 3) | gleich |
| Aufwärtswandler lückend, 5 µs | < 2 %, ≥ 3-fach | 1,85 %, 5,4-fach | 1,85 %, 4,0-fach |
| Klasse D 5 kHz, Ton 200 Hz | < 2 % | 0,17 % | gleich, 8,1-fach |

## Befunde

1. **Frequenzgang trägt.** Die Wechselstromanalyse ist exakt für lineare Schaltungen und um Größenordnungen
   schneller als nötig; der Frequenzgang-Schreiber kann beim Drehen am Regler live neu zeichnen.
2. **Radio: Abstimmen echt, Detektor nur gemittelt ehrlich.** Die Hüllkurve einfach als Spannung vor die Diode zu
   legen, verfälscht um über 50 % (die Diode leitet beim echten Träger nur an den Spitzen). Der gemittelte
   Detektor trifft den echten Träger auf 0,1 % (Ton) bzw. vier Stellen (Gleichanteil). Er wird als Teil des
   MW-Moduls bzw. als Betriebsart der Detektordiode im Hochfrequenzzweig umgesetzt und in »So rechnet Volta«
   offen erklärt.
3. **Schaltwandler: 20 µs reichen, außer im Lückbetrieb.** Das Rückwärtsverfahren zählt die Ladung je Schritt mit
   dem Strom am Schrittende; endet der Spulenstrom dreieckförmig (Lückbetrieb), fehlt je Periode etwa ein halber
   Schritt Ladung (Fehler erster Ordnung, ohne Lücken hebt er sich auf). Eine Ortung des Sperrbeginns der Diode
   ändert daran nichts. Abhilfe: Schrittweite 5 µs, solange ein Schaltwandler bzw. Taktschalter steckt
   (wie die App heute schon 20 µs nur bei hörbaren Schaltungen wählt, `board_edit.cpp`). Später möglich:
   Trapezverfahren (zweite Ordnung) – größerer Eingriff, nicht für Volta 1/2.
4. **Schrittweitenfehler bei 20 µs** an der RC-Last des Detektors: −2,1 % im Ton (1 kHz, τ = 0,22 ms). Das gilt für
   jede hörbare Schaltung (Volta 2: Tonhöhe −2 … −3 %); kein Fehler des Radiomodells. Das Trapezverfahren würde ihn
   ebenfalls beseitigen.
5. **Transistorschaltungen im Hörbereich sind knapp.** Der Empfänger mit zwei Transistoren erreicht auf dem Desktop
   gerade die dreifache Echtzeit (Volta-2-Vorgabe). Hilfreich: Toleranz 1e-4 statt 1e-6 (4,0 → 3,1 Newton-Schritte,
   SPICE rechnet mit 1e-3), Prädiktor gering. **Auf dem Tablet 2,0-fach** – hörbar in Echtzeit, aber unter der
   Reserve von Volta 2 (3-fach). Mit dem gemittelten Detektor bringen Toleranz und Prädiktor kaum noch etwas
   (3,5 → 3,4 Schritte). Rückfallebene der App: angesagter Ersatzton, wenn die Rechnung langsamer als Echtzeit ist.
   Zu entscheiden: Reserve 2-fach für Empfänger mit Transistorverstärker annehmen, oder den Verstärker im
   MW-Modul als Verhaltensmodell (wie der 555 in Volta 2) und nur Detektor und Kopfhörer echt auf der Grundplatte.

## Entscheidungen (04.11.2026)

- **Empfänger:** 2-fache Echtzeit auf dem Tablet genügt; der Verstärker bleibt echte Schaltung. Wird es zu knapp,
  greift der angesagte Ersatzton der App.
- **Schaltwandler:** Schrittweite 5 µs, solange ein Schaltwandler steckt; Trapezverfahren als spätere Option.
- **Radiomodell:** gemittelter Hüllkurvendetektor; Konzept Volta 3 auf v0.5 angepasst (Kapitel 4).

## Offene Punkte

- Prädiktor und Toleranz 1e-4 für Volta 3 einschalten – nach Messung mit echten Experimenten (Schritt 3).
- Drehkondensator als Bauteil mit Bedienparameter (der Test baut den Aufbau je Reglerschritt neu; 0,009 ms).
