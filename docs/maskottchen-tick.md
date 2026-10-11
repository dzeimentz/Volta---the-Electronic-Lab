# Maskottchen Tick

Vorlage: `docs/design/tick-uebersicht.webp` (Gestaltungsblatt, 10/2026). Tick ist ein kleiner IC im DIP-Gehaeuse mit
Gluehlampe auf dem Kopf, Handschuhen und blauen Schuhen. Er begleitet durch Hilfe, Experimente und Fehlersuche.
Als achtbeiniger Baustein passt er zum NE555 in Volta 2 (Stufe 6, `docs/volta2-planung.md`).

## Pruefung des Gestaltungsblatts
- Gut: klare Figur, wiedererkennbar, zehn Ausdruecke decken alle geplanten Einsaetze ab; Warnung und Frage tragen
  zusaetzlich ein Zeichen (»!«, »?«), Farbe ist also nicht das einzige Signal.
- Beinzahl festlegen: Vorderansicht und Seitenansichten zeigen nicht ueberall gleich viele Anschluesse. Vorschlag:
  4 je Seite (DIP-8), passend zum NE555.
- Beispiele 2, 4 und 5 zeigen eine Rechteck-/Taktschaltung mit Kondensator und IC - das ist Volta-2-Stoff. Fuer
  Volta 1 gelten die Beispiele sinngemaess mit Stufe-1-4-Inhalten (Lampe, LED, Transistor, Blinkmodul).
- Beispiel 3 (»Versorgungsspannung zu niedrig oder Kurzschluss«): Tick nennt immer den konkreten Befund der
  Fehlersuche (`circuit_check`), nie eine Vermutung - »Physik ehrlich«.
- Sprechblasen nie ins Bild: Texte kommen ueber Inhaltsschluessel/qsTr (de, fr, en, lu) und koennen vorgelesen werden.
- Rechte: vor der Veroeffentlichung klaeren, mit welchem Werkzeug das Blatt entstand und ob die Nutzung im Store
  erlaubt ist; Name »Tick« auf Markenkonflikte pruefen. Eintrag im Info-Fenster (Gestaltung).

## Bildmaterial
- Zehn Einzelbilder (10/2026), Originale in `docs/design/tick/` (PNG 512 px, transparent). In der App
  (`app/resources/tick/tick_<stimmung>.png`): auf die Figur zugeschnitten, 256 Farben, zusammen ca. 360 KB.
- Stimmungen: erklaerend (Zeigestock), fragend (?), freundlich (winkt), froehlich (springt), gut (Daumen hoch),
  motivierend (Faust), nachdenklich, neugierig (Lupe), ueberrascht, warnend (Warnschild). »Idee« ist im Satz nicht
  enthalten; die leuchtende Lampe traegt Tick ohnehin in jeder Stimmung.
- PNG statt WebP: das WebP-Bildformat wird in der APK nicht ausgeliefert (Paketgroesse).

## Umsetzung
- `Tick.qml` (Figur), `TickSays.qml` (Figur mit Sprechblase; kurzer Hopser bei neuem Text, nicht bei
  »Bewegung reduzieren«). Einstellung `anzeige/tick` (Elternbereich »Tick zeigen«); aus: dieselbe Blase ohne Figur.
- Eingebaut (10/2026): Begruessung Startseite (mit Spitzname bei Profilen), Forscherfrage (neugierig), Tipps
  (nachdenklich, dann erklaerend; Figur nur beim neuesten Tipp), »Nur in der App!« (warnend), Erfolg (gut),
  Vergleich mit der Vermutung (froehlich / ueberrascht / motivierend), Fehlersuche (neugierig, gut, fragend).
- Offen: Hilfe/Readme mit Tick; »ueberrascht« beim Durchbrennen eines Bauteils; Klassenmodus ohne Hopser.

## Einsatz in Volta 1
| Ort | Ausdruck | Inhalt |
|---|---|---|
| Startbildschirm, erster Start | freundlich | Begruessung, Hinweis auf Hilfe |
| Hilfe (Kinderteil) | erklaerend | fuehrt durch Bedienung, Symbole, Fehlersuche |
| Experiment, gestufte Hilfen | idee / nachdenklich | Hilfestufen sprechen durch Tick |
| Fehlersuche | warnend / fragend | konkreter Befund, »Schau mal bei ...« |
| Fertig / Erfolg | froehlich / motivierend | Lob, naechstes Experiment |
| Lexikon, Lerntipps | neugierig | »Probier doch mal ...« |
| Unerwartetes (Bauteil durchgebrannt) | ueberrascht | Erklaerung, was passiert ist |

## Regeln
- Tick verdeckt nie das Steckfeld; erscheint am Rand, Sprechblase schliessbar (48 dp).
- Im Elternbereich abschaltbar (dann Hinweise wie bisher als Text), im Klassenmodus ruhig (keine Animation, kein Ton).
- Animation sparsam: Lampe leuchtet bei Idee/Erfolg; Systemeinstellung »Bewegung reduzieren« wird beachtet.
- Tick spricht Kinder mit »du« an, kurze Saetze, keine Fachbegriffe ohne Lexikonverweis.
