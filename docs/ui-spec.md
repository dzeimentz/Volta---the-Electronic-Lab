# Oberflaeche – Spezifikation

## Grundsaetze
- Gegenstaendlich: Bauteile sehen aus wie Bauteile; Schaltzeichen begleiten sie.
- Finger, Stift und Maus sind gleichwertig; Tastatur auf dem Desktop vollstaendig nutzbar.
- Bedienelemente ≥ 48 dp; Kontaktfelder ≥ 56 dp hoch, 24 dp Abstand.
- Farbe nie alleiniger Informationstraeger (Kontur, Muster, Skalenmarke).
- Animationen reduzierbar (Systemeinstellung respektieren).

## Startseite (Dashboard)
- Kacheln: Lernpfad, Freies Labor, Forscherheft, Readme (Fragezeichen), Info (i).
- **Flaggensymbol** oben rechts: Auswahl mit Flagge, Kuerzel und Eigenname:
  Deutsch (DE), Français (FR), English (EN), Lëtzebuergesch (LU). Wechsel sofort, ohne Neustart,
  laufendes Experiment bleibt erhalten. Nicht hinter der Elternschranke.
- Forscherprofil-Auswahl, wenn Profile aktiviert sind.
- Forscherprofile umgesetzt (`ProfileChooser.qml`, `services/profile_store`): im Elternbereich einschaltbar
  (Einstellung `profile/aktiv`, Vorgabe aus). Beim Start »Wer forscht heute?« mit Kacheln (Symbol, Spitzname) und
  »Neues Profil« (24 Bildzeichen, Spitzname bis 16 Zeichen, Hinweis »nicht deinen richtigen Namen«, kein Passwort);
  hoechstens 30 Profile. Je Profil eigener Ordner `profile/<id>/` mit Fortschritt, Forscherheft und Laborstand;
  Sprache und Einstellungen gelten fuer das Geraet. Abzeichen auf der Startseite wechselt das Profil. Loeschen
  einzeln oder »Alle Profile zuruecksetzen« nur im Elternbereich (mit Rueckfrage).
- Mitnahme (Elternbereich »Mitnehmen auf ein anderes Geraet«, `services/profile_transfer`): Profildatei
  `*.volta1profil` (Symbol, Spitzname, Fortschritt, Forscherheft, letzter Aufbau) ueber die Dateiauswahl des Systems
  (Android ohne Speicherberechtigung); Laden legt mit Profilen ein neues Profil an, ohne Profile ersetzt es nach
  Rueckfrage den Stand. QR-Code nur mit den erledigten Experimenten, ohne Namen: Link `volta1://fortschritt?d=1.1,...`,
  gescannt mit der Kamera-App des Systems, die Volta oeffnet (Intent-Filter, keine Kameraberechtigung); Volta fragt
  vor dem Uebernehmen (mit Profilen erst nach der Profilwahl). QR-Kodierer `third_party/qrcodegen` (MIT).
- Umgesetzt (Phase 2, D1, `Home.qml`): Kacheln Freies Labor und Forscherheft (folgt), Lernpfad mit Stufen aus
  `content/<band>/stufen.json` (Titel, Leitfrage), Fortschritt je Stufe und Experimentkarten (geschafft: Haken).
  Freischaltung zur Reduktion: Stufen 0 und 1 offen, jede weitere, wenn alle Experimente der vorigen geschafft sind;
  Einstellung »Alle Bauteile freigeben« (bis zum Elternbereich). Fortschritt: `fortschritt.json` (nur ID und Datum,
  `QSaveFile`). Bauteilfach im gefuehrten Experiment = `bauteile_frei`, im freien Labor alle Bauteile, noch gesperrte
  grau mit Schloss und Stufe (nicht ziehbar, Antippen zeigt einen Hinweis). ⌂ in der Werkzeugleiste fuehrt zur Startseite.

## Experimentansicht (Lab)
- Bereiche: Bedienpult (oben), Pultleiste, Grundplatte, Modulleiste (unten), Bauteilfach (seitlich), Werkzeugleiste.
- Bauteil- und Modulfach teilen sich die Seitenleiste: Der Knopf »Bauteilfach | Modulfach« oben schaltet um, das
  gewaehlte Fach ist fett und unterstrichen. Ohne Module bleibt es beim Bauteilfach. Die Aufbauhilfe schaltet auf das
  Fach um, das ihr naechster Schritt braucht.
- Ziehen aus dem Fach: Das Bauteil erscheint schemenhaft an dem Platz, an dem es landen wuerde, und alle Felder, die
  es belegt, sind hervorgehoben (z. B. A1 und A2). Passt es dort nicht, bleibt nur das Namensschild am Finger.
- Im Betrieb oeffnet ein langer Druck auf ein Bauteil kein Menue (ein Taster bleibt gedrueckt); Rechtsklick weiterhin.
- Werkzeugleiste: Experimente, Rueckgaengig, Wiederholen (gerahmt wie »V A Ω« und Werkzeugkasten), Spannungsanzeigen ein/aus, Schaltplan, Werkzeugkasten, Fragezeichen,
  Flaggensymbol, Hauptmenue.
- Breite der Grundplatte (8/10/12 Felder je Reihe, Befehl): runde Knoepfe »+« und »−« uebereinander rechts neben den
  Feldern, in der Hoehe zentriert, gelb mit schwarzem Ring, dazwischen »1–12«; was nicht geht, ist grau. Das
  Messgeraet neben der Grundplatte rueckt dahinter.
- Startbildschirm (Desktop): Startbild mit Tick, Schriftzug und Herstellerlogo (`resources/brand/splash.jpg`), Version
  unten rechts; nicht in der Android-Ausgabe.
- Links: **Experimentauswahl** als Aufklappfeld (»Freies Labor«, »Stufe n: Titel«); zeigt zugleich den Titel.
- Gespeichert (`AppSettings`, `einstellungen.ini`): Sprache, Schriftgroesse, Stromfluss, Animationen, Startbildschirm,
  Freigabe sowie die Arbeitsumgebung: Felder je Reihe (fuer neue Aufbauten, sofern die Vorlage passt), Messgeraet im
  freien Labor, Leitungsfarbe, Schaltplan ein/aus, Experimentfeld eingeklappt.
- Ganz rechts: **Einstellungen** (☰): Stromfluss anzeigen, Animationen reduzieren, Startbildschirm zeigen (Desktop),
  Messgeraet anzeigen, Felder je Reihe, Info. Lokal gespeichert (`AppSettings`, `einstellungen.ini`).
- Oben rechts, links vom Flaggensymbol: **Schriftgroesse** »A− / A+« in drei Stufen (80 / 90 / 100 %, Vorgabe 100 %),
  gespeichert; Schaltzeichen bleiben Grafik.
- Oben rechts wie in Visual Technic Studio: **Anzeigesprache** (runde Flagge der aktuellen Sprache; Menue mit runder
  Flagge, Kuerzel und Eigenname; LU bis Phase 6 grau mit »folgt«) und **Info** (i). Die Simulationszeit erscheint
  nur als Entwicklerhilfe (`VOLTA_SHOW_TIME=1`).
- Das Experimentfeld (rechts) laesst sich ein- und aufklappen (»/«), damit die Grundplatte mehr Platz bekommt.
- Pultleiste kompakt (12 Felder); die Buchsen des Dreifach-Messgeraets (COM, V/Ω, A) sitzen im Messgeraete-Bereich
  rechts neben der Grundplatte.

### Gesten und Eingaben
| Aktion | Touch/Stift | Maus/Tastatur |
|---|---|---|
| Bauteil setzen | aus Fach ziehen | ziehen |
| Leitung ziehen | von Feld zu Feld ziehen | ziehen |
| Leitungsende umsetzen | Ende greifen und ziehen | ziehen |
| Kontextmenue | langer Druck | Rechtsklick |
| Zoom/Verschieben | zwei Finger | Strg + Mausrad / Leertaste + Ziehen |
| Rueckgaengig | Tippen mit zwei Fingern | Strg+Z |
| Wiederholen | Tippen mit drei Fingern | Strg+Y / Strg+Umschalt+Z |
| Regler drehen | Kreisbewegung oder Wischen | Ziehen, Mausrad |
| Regler auf Mitte | Doppeltippen | Doppelklick |

Waehrend des Ziehens einer Leitung leuchtet das Zielfeld auf; Fangbereich = ganzes Feld + 8 dp.

Umgesetzt (Zoom/Verschieben, `Lab.qml`, uebernommen aus dem Cloud-Zweig `claude/mobile-android-ipad-0w8lbc` und an
den lokalen Stand angepasst): zwei Finger (`PinchHandler`; der Punkt unter den Fingern bleibt stehen, zwei Finger
gehen vor Bauteil und Leitung), Strg + Mausrad (Zoom am Zeiger), Mausrad bzw. Umschalt + Mausrad (Verschieben, nur
wenn gezoomt), Leertaste + Ziehen und mittlere Maustaste (Ausschnitt ziehen; die Leertaste wirkt, wenn die
Arbeitsflaeche den Fokus hat, z. B. nach Tippen ins Leere). Massstab zwischen »ganze Platte« und 200 %. Ansicht zu
Beginn auf allen Geraeten: ganze Platte (die Breite 8/10/12 waehlt die Automatik; am 14.10.2026 auf den Tablets
10,1" und 15,6" bestaetigt: Felder gross genug, Zoom funktioniert); Knopf oben rechts in der
Arbeitsflaeche (»Ganze Platte zeigen«) nur nach Zoomen sichtbar. Menueeintraege, Analogschalter und Gruppenlinien
mit Fangbereich >= 48 dp. Offen: Tippen mit zwei/drei Fingern fuer Rueckgaengig/Wiederholen (braucht eigene
Erkennung und Geraetetest); Hochformat mit Bauteilfach unten (Android ist auf Querformat festgelegt, Thema fuer
das iPad).

## Bedienpult
- **Hauptschalter** fuer die Batterie.
- **Drehregler mit Lichtring**: 16 Segmente auf 270°, Luecke unten; Segmente vom linken Anschlag bis zur Stellung
  leuchtend (Akzentfarbe Signal Orange #FF6B1A / Solder Amber #FFB07A), Rest als blasse Umrisse. Stellung intern
  stufenlos. Ab Stufe 3 zeigt langer Druck Teilwiderstand in Ω und Stellung in %.
- **Lautsprecher mit Leuchtring (Klanganzeige)**: Lautsprechersymbol im geschlossenen Ring.
  - Tonhoehe -> Farbton, logarithmisch: 100 Hz Gelb (#FFD400) ueber Orange bis ≥ 5 kHz Rot (#E8221B).
  - Signalstaerke -> Helligkeit; Stille -> dunkel.
  - Impulse unter 20 Hz -> kurzes Aufblinken je Impuls.
  - Skalenmarke wandert mit der Tonhoehe im Uhrzeigersinn (Farbfehlsichtigkeit).
  - Ab Stufe 3: langer Druck zeigt Frequenz in Hz. Bleibt im Klassenmodus aktiv.
  - Umgesetzt (`SpeakerRing.qml`, Kernmodell `Speaker`): Tonhoehe aus dem Stromverlauf (Nulldurchgaenge), Staerke aus
    dem Effektivwert des Wechselanteils, Impulse gezaehlt; reiner Gleichstrom bleibt stumm. Wegen der Schrittweite
    1 ms sind Tonhoehen nur bis ca. 250 Hz aufloesbar; hoehere Toene liefern Verhaltensmodelle (Summer, Tonmodul).
    Hz bei langem Druck im freien Labor und ab Stufe 3. Durchgebrannter Lautsprecher: antippen = reparieren (Befehl).
    Tonausgabe ueber die Soundkarte folgt mit dem Audiodienst.
- **Dreifach-Messgeraet**: Ein-/Ausblenden, Umschalter digital/analog, Drehschalter V/A/Ω, Buchsen COM, V/Ω, A.
  Zwei Plaetze, umschaltbar mit dem kleinen Knopf oben links im Geraet (gespeichert, `labor/messgeraet_im_pult`):
  neben der Grundplatte (hoch, Drehschalter, Buchsen unten im Geraet) oder oben im Pult rechts neben dem
  Lautsprecher (flach: Anzeige links, Tasten V/A/Ω, digital|analog; Buchsen in der Reihe der Pultleiste).
- Batterie im Pult: angedeutete 4,5-V-Flachbatterie (nur Umriss mit zwei Polzungen, etwas heller als das Pult)
  von Spannungswahl und Hauptschalter; ohne Spannungsaufdruck, die gewaehlte Spannung steht im Auswahlfeld.
- Steckloecher der Kontaktfelder quadratisch.
- Automatisches Speichern (`services/session_store`, `model/board_json`): Aufbau mit Pult, Messgeraet und
  Werkzeugen, laufendes Experiment mit Ablauf (Vermutung, Tipps, Beobachtung) bzw. freies Labor und ob das Labor
  offen war; 1,5 s nach jeder Aenderung und sofort beim Wechsel in den Hintergrund und beim Beenden, atomar in
  `laborstand.json` im privaten App-Speicher. Beim Start: war das Labor offen, geht es dort weiter (Hauptschalter
  aus, Steckmodus), sonst bietet die Startseite die Kachel »Weitermachen«. Rueckgaengig-Verlauf wird nicht gespeichert.
- Bauteilfach kompakt (Kacheln 92 x 44, Abstand 4), damit alle Bauteile auch auf Tablets ohne Rollen passen.
- Muelleimer unten rechts in Hoehe der Modulleiste, buendig mit dem rechten Feldrand (bei 8 Feldern je Reihe
  rechts neben M16): Antippen loescht das Ausgewaehlte (Bauteil, Leitung, Modul), ohne Auswahl »Alles abbauen«;
  Bauteile und Module lassen sich darauf ziehen (er faerbt sich rot). Immer mit Rueckfrage; Loeschen ist ein Befehl
  und laesst sich rueckgaengig machen.
- Android/iOS: keine Mindestfenstergroesse, Vollbild (Systemleisten per Wischen); Kontextmenues halten 8 dp
  Abstand zum Fensterrand und rollen, wenn sie hoeher sind als der Platz.
  Digital: 3 Stellen + Einheit, 3 Hz Aktualisierung. Analog: Zeiger mit Traegheit und leichtem Ueberschwingen.
  - Umgesetzt (aus Phase 4 vorgezogen, `MeterPanel.qml`): Geraet rechts neben der Grundplatte, ein-/ausblendbar ueber
    »V A Ω« in der Werkzeugleiste (Ausblenden nicht, solange Leitungen an den Buchsen stecken; Buchsen nur
    eingeblendet steckbar). Drehschalter V / A / Ω, Umschalter digital/analog. Zeiger in allen Messarten mit
    selbsttaetiger Bereichswahl: V 1/3/10/30 V, A 1 mA … 1 A, Ω als Zeiger-Ohmmeter (rueckwaerts, nichtlinear,
    Skalenmitte in Zehnerstufen 10 Ω … 1 MΩ). Anzeige mit Vorsatz
    (mV/V, µA/mA/A, Ω/kΩ/MΩ, `model/meter_display.hpp`), »OL«, »U !« mit »Spannung abschalten!«, durchgebrannte
    Sicherung mit »Sicherung tauschen« (Befehl).
- **Pultleiste**: Felder mit fuenf Steckplaetzen und Bezeichnung; Gruppenlinien mit Schaltzeichen
  (Batterie, P1, P2, Schalter, Lautsprecher). Tippen auf Schaltzeichen hebt Gruppe und Pultelement hervor.

## Bauteilfach
- Jede Kachel zeigt das Schaltzeichen nach DIN EN 60617 (`SchematicSymbol.qml`, einfarbig) ueber dem Namen; die
  Zeichen dienen spaeter auch der Schaltplanansicht.
- Gluehlampe: Nennspannung 2,5 / 6 / 10 V im Bauteilmenue »Nennspannung« (Befehl), Vorgabe 6 V; die Spannung steht
  an der Lampe.

## Ton und Vorlesen (Phase 2, ADR-0009)
- Tonausgabe: Pult-Lautsprecher mit seinem Ton (auch vom Tonmodul) und Summer auf der Grundplatte klingen ueber die
  Soundkarte; Impulse unter 20 Hz knacken kurz. Nur im Labor; Einstellung »Ton über Lautsprecher« im Menue.
- Vorlesen: Knopf 🔊 oben rechts im Experimentfeld liest die aktuelle Phase vor (Frage mit Vermutungen, Aufbau mit
  Schritten und Aufbauhilfe, Beobachtungen, Erklaerung); ⏹ beendet. Nur sichtbar, wenn eine Systemstimme fuer
  die Anzeigesprache vorhanden ist (LU: spaeter eigene Aufnahmen).

## Schritt-fuer-Schritt-Aufbauhilfe (Phase 2)
- Im Experimentfeld (Aufbau) Knopf »Schritt für Schritt bauen«: Die Vorlage wird in Schritte zerlegt (Module,
  Bauteile, Leitungen in Vorlagenreihenfolge, zuletzt der Hauptschalter; `model/build_guide`). Angezeigt wird immer
  der erste offene Schritt: Text mit Feldnamen (bei gepolten Bauteilen mit Hinweis auf langes Bein, Strich, Plus
  bzw. B/C; Wert bzw. Nennspannung), das Teil schemenhaft an seinem Platz und die Zielfelder mit blauem,
  pulsierendem Rahmen und Pfeil (bei reduzierter Bewegung ruhig).
- Erledigt ist ein Schritt, sobald das Teil an derselben Stelle steckt (ungepolte Bauteile in beliebiger Richtung,
  Leitungen in beiden Richtungen, Modulanschluss = Feld der Leiste); keine Reihenfolgepflicht, Entfernen holt den
  Schritt zurueck. Ob das Experiment gelingt, entscheidet weiter die Erfolgsbedingung, nicht die Aehnlichkeit.

## Forschungskreislauf und Forscherheft (D2)
- Experimentfeld: 1 Frage mit Vermutung (wird ohne Wertung ins Forscherheft geschrieben) -> 2 Aufbau mit Schritten
  und gestuften Hilfen; unten im Feld fest der Knopf »✓ Fertig«: grau und nicht klickbar, bis alle Bauteile und
  Module der Vorlage stecken (Anzahl je Typ) und die Erfolgsbedingung im aktuellen Aufbau erfuellt war (jede
  Aufbauaenderung verlangt einen neuen Nachweis, Bedienhandlungen nicht); ein Satz darunter nennt, was noch fehlt
  -> 3 nach »Fertig« »Was hast du beobachtet?« (Auswahl) -> 4 Vergleich mit der Vermutung
  (»hat gestimmt« / »Ueberraschung! …« / »Jetzt weisst du es!«, Symbol zusaetzlich zur Farbe), Erklaerung,
  Mehr wissen, Weiterforschen mit eigener Notiz, Jetzt in echt, naechstes Experiment.
- Forscherheft-Seite (Kachel auf der Startseite): je Experiment Vermutung, Beobachtung, Vergleich, Notizen, Datum;
  `forscherheft.json` im privaten App-Speicher (`JournalStore`, `QSaveFile`). Zeichnungen, Fotos, PDF folgen.

## Grundplatte
- Kontaktfelder mit Feldnamen; Spannungsanzeige (4 hellgruene Balken) je Feld zuschaltbar.
- Stromfluss: wandernde Punkte, Geschwindigkeit ~ Stromstaerke, technische Stromrichtung (Elektronenfluss ab Stufe 3).
- Waerme: Bauteile gluehen rot, Rauch bei Zerstoerung, Knopf »Reparieren«.
- Wassermodell als zuschaltbare Erklaerebene.
- Schaltplanansicht ab Stufe 2 parallel; Knoten mit Feldnamen; wechselseitige Hervorhebung.
  Umgesetzt (D4, `SchematicView.qml`, Anordnung in `model/schematic_layout`): Knoten = ueber Leitungen verbundene
  Anschlusspunkte, beschriftet mit Feldnamen; Pluspol oben, 0 V unten, weitere Knoten in Ebenen nach dem Abstand vom
  Pluspol; parallele Zweipole versetzt; Batterie als eigener Zweig links. Knopf in der Werkzeugleiste (freies Labor,
  ab Stufe 2); Bauteil im Plan antippen waehlt es auf der Platte und umgekehrt; Knoten antippen hebt seine Felder hervor.
- Leitungsfarbe: ein Knopf mit Auswahl (Farbe mit Namen) statt sechs Farbknoepfen.

## Werkzeugkasten (Seitenleiste)
- Farbcode und E-Reihen (E6, E12, E24; ab Volta 2 E48, E96; Kondensatorcodes ab Volta 2).
- Formelrechner mit Einheiten und Vorsaetzen (p, n, µ/u, m, k, M) und Rechenblaettern; Rechenweg immer sichtbar.
- Bauteillexikon, Zeitschreiber (ab Stufe 3), Fehlersuche (ab Stufe 2, antwortet mit einer Frage), Teileliste mit
  Aufbaukarte (PDF), Kennlinienschreiber (Volta 2).
- »Uebernehmen« schreibt Werte auf ein ausgewaehltes Bauteil (Befehl, undo-faehig).
- Umgesetzt (D3, `ToolboxPanel.qml`, Rechnen in `model/resistor_code.hpp`): Seitenleiste 🧰 mit Farbcode (4/5 Ringe,
  Ring fuer Ring mit Farbname und Farbfeld, Eingabe wie »4,7k«/»4k7«/»2M2«), E-Reihen E6/E12/E24 als
  logarithmischer Zahlenstrahl mit naechstem Normwert und Abweichung, Bauteillexikon mit Normsymbolen.
  »Uebernehmen« auf den ausgewaehlten Widerstand. Formelrechner, Zeitschreiber, Fehlersuche, Teileliste folgen.
- Teileliste umgesetzt (Reiter im Werkzeugkasten, `PartsListPanel.qml`, Modell `model/parts_list`, PDF
  `services/build_card`): im Experiment die Vorlage, im freien Labor der eigene Aufbau; gleiche Bauteile mit gleichem
  Wert zusammengefasst (mit Kennungen), Leitungen nach Farbe (Farbstreifen plus Farbname), Werkzeuge, Batterie.
  »Aufbaukarte speichern (PDF)« schreibt A4 mit Forscherfrage, Zeichnung der Grundplatte (Bauteile mit Kennung und
  Wert, Polaritaet als Zeichen +/−/B/C/E, Leitungen in ihrer Farbe), Teileliste zum Abhaken, nummerierten Schritten
  (wie die Schritt-fuer-Schritt-Hilfe, mit Leitungsfarbe) und Sicherheitshinweis. Zerstoerende Versuche (»Nur in der
  App!«) bekommen keine Karte. Die Reiterleiste ist wischbar.
- Fehlersuche umgesetzt (Reiter im Werkzeugkasten, ab Stufe 2; Modell `model/circuit_check`): »Schaltung pruefen«
  listet Auffaelligkeiten als Fragen mit Nummer, dieselbe Nummer erscheint als Marke an der Stelle auf der
  Grundplatte; Antippen hebt die Felder hervor bzw. waehlt das Bauteil. Am Aufbau: Kurzschluss ueber Leitungen,
  ueberbrueckte Bauteile, lose Beine und Leitungen, offener Schalter, Sackgasse, fehlende Verbindung zu +9V/0V,
  Hauptschalter aus; in der laufenden Simulation: zerstoerte und ueberlastete Bauteile, verpolte LED/Elko, sehr
  grosser LED-Strom, Kurzschlussstrom. Die Pruefung ist grosszuegig (Mehranschluss-Bauteile und Module verbinden
  alle Anschluesse); Widerstandsmessung mit dem Messgeraet braucht keine Batterie. Aenderungen am Aufbau loeschen
  die Befunde. Der ContentSmokeTest prueft, dass keine Vorlage am erreichten Ziel einen Befund hat.

## Interaktives Readme
- Rundgang beim ersten Start mit Begleitfigur; danach ueber das Fragezeichen.
- Abschnitte: Willkommen, Grundplatte, Leitungen, Bedienpult, Anzeigen, Rueckgaengig, Werkzeugkasten,
  Sprache und Einstellungen – jeweils mit Uebungsplatte (vom Lernpfad getrennt) und Aufgabe mit Bestaetigung.
- Teil fuer Erwachsene (hinter Elternschranke): Systemanforderungen, Gesten/Tastenkuerzel, Datenschutz,
  Konfigurationsschluessel fuer Schulen, Aenderungsliste. »Was ist neu« nach Updates. Kontexthilfe, Volltextsuche.
- Umgesetzt (10/2026, `HelpDialog.qml`): Hilfe mit Tick, elf Kapitel fuer Kinder (Willkommen, Bauteile stecken,
  Leitungen, Bedienpult, Rueckgaengig und Loeschen, Messen, Werkzeugkasten, Experimente, Wenn etwas nicht klappt,
  Sicher experimentieren, Oben rechts), je mit Tick in passender Stimmung; Kapitelknoepfe zum Springen. Erwachsenenteil
  als Verweis in den Elternbereich (PIN) mit Tastenkuerzeln. Aufruf: »?« oben rechts (Startseite, Forscherheft), im
  Labor im Menue (Leiste voll), »Zeig mir, wie Volta funktioniert« unter Ticks Begruessung. Offen: Uebungsplatten mit
  Aufgaben, Rundgang beim ersten Start, »Was ist neu«, Volltextsuche.
- Werkzeugkasten: verdeckte Reiter zeigt ein pulsierender Pfeil (rechts bzw. links), Antippen rollt die Leiste.

## Info-Seite
- Produkt: »Volta 1 – Das Elektronik-Labor«, Version, Build, Paket- und Kernversion.
- Hersteller: DC Software Engineering, Daniel Zeimentz, Anschrift (Anbieterkennzeichnung).
- Kontakt: dc-software-engineering.com, info@dc-software-engineering.com (Links hinter Elternschranke).
- Verwendete Techniken: Qt 6, Qt Quick (QML), Qt RHI, C++17-Simulationskern (MNA, Newton-Raphson, Rueckwaerts-Euler),
  JSON-Inhaltspakete, Systemsprachausgabe und eigene Tonaufnahmen.
- Lizenzen (alle Open-Source-Bestandteile mit Texten), Datenschutzerklaerung (offline), Nutzungsbedingungen, Danksagung.
- »Supportangaben kopieren« und vorbereitete Support-E-Mail (ohne Namen, Profile, Inhalte).
- Stand Phase 1 (`InfoDialog.qml`): Produkt, Version, Plattformen (diese Ausgabe und vorgesehene), verwendete Technik
  (Qt-Version zur Laufzeit, Uebersetzer), Hersteller mit Logo (`app/resources/brand/dc_logo.png`, sonst Schriftzug),
  Firmenangabe »DC Software Engineering, Irrel (DE)«, Name, E-Mail und Web als Text (keine Links ohne
  Elternschranke), Datenschutz. Offen: vollstaendige Anschrift (Anbieterkennzeichnung vor Phase 3), Lizenztexte,
  Supportangaben.

## Elternschranke
- Frei waehlbare PIN (Entscheidung 10/2026 statt der Rechenaufgabe); schuetzt Elternbereich, externe Links,
  Readme-Erwachsenenteil.
- Umgesetzt (`ParentGate.qml`, `AppSettings`): Start-PIN 1234, im Elternbereich aenderbar (4–8 Ziffern, zweimal
  eingeben); solange die Start-PIN gilt, zeigt der Elternbereich oben einen deutlichen Hinweis. Gespeichert nur als
  gesalzener SHA-256-Pruefwert. Eingabe verdeckt ueber Ziffernblock (48 dp); nach fuenf Fehlversuchen 30 s Sperre, die
  einen Neustart ueberdauert. »PIN vergessen?« erklaert das Zuruecksetzen ueber das Loeschen der App-Daten.
  Info-Fenster: »Website oeffnen« und »E-Mail schreiben« erst nach der Schranke.

## Elternbereich
- Erreichbar ueber das Menue (☰ »Elternbereich …«) hinter der Elternschranke (`ParentArea.qml`):
  Freigaben (»Alle Stufen und Bauteile freigeben«, aus dem Kindermenue hierher verlegt), Lernpfad-Fortschritt
  zuruecksetzen und Forscherheft leeren (je mit Rueckfrage), Datenschutz (was lokal gespeichert wird),
  »So rechnet Volta« (offengelegte Vereinfachungen der Simulation), Lernpfad im Ueberblick, Kontakt mit Links.
- Klassenmodus Ton (Elternbereich »Ton im Klassenzimmer«, `services/audio_route`): normal, nur mit Kopfhoerern
  (Android: AudioManager-Geraete Kabel/USB/Bluetooth; Desktop: Name des Ausgabegeraets), aus; gilt fuer Pult-Ton und
  Vorlesen. Die verwaltete Konfiguration kann ihn vorgeben, ebenso das Ausblenden des Elternbereichs.

## Farben (Markenwerte DC Software Engineering)
- Engineering Navy #0A2540, Pult #1A3A5C, Akzent #2E7DBE, Signal Orange #FF6B1A, Solder Amber #FFB07A,
  Spannungsbalken #8EE68A mit Kontur #3E8E3A, Grundplatte #F7F3EA, Felder #CFC5AE.

## Schaltplanansicht (DIN EN 61082-1, Schaltzeichen DIN EN 60617)
- Automatisch aus dem Aufbau (`model/schematic_layout`): jeder Knoten eine waagrechte Leitung in eigener Zeile
  (+9V oben, 0V unten, dazwischen nach Abstand vom Pluspol), jedes Bauteil senkrecht in eigener Spalte; Batterie
  links, dann in Signalrichtung Schalter, Regler, passive Bauteile, Module/Transistor, Verbraucher, Lautsprecher.
- Nur rechte Winkel; Verbindungspunkt an jeder Abzweigung; Kreuzungen ohne Punkt sind keine Verbindung.
- Mehrpolige Bauteile: Transistor C oben, E unten, B links; Potentiometer mit Schleifer seitlich; Wechselschalter
  senkrecht (Mitte oben bzw. unten); Module und Messgeraet als Block (Plus oben, Minus unten, Eingaenge links,
  Ausgaenge rechts, Anschlussnamen innen). Offene Anschluesse als kurzer Stummel.
- Kennzeichnung rechts neben dem Symbol (Kennung, Wert bzw. Nennspannung); Feldnamen klein an der Knotenleitung,
  abschaltbar (Knopf »Feldnamen«). Mindestens 0,75-fach gezeichnet, groessere Plaene lassen sich verschieben.
- Antippen eines Bauteils waehlt es auf der Grundplatte; Antippen einer Knotenleitung hebt sie und ihre Felder hervor.

## Auswaehlen, Kontextmenue, Loeschen
- Antippen waehlt ein Bauteil bzw. eine Leitung (Ende antippen) nur aus (orange); es loest nie eine Aktion aus.
  Beim ersten Auswaehlen erscheint einmal der Hinweis auf das Kontextmenue.
- Aktionen nur ueber das Kontextmenue: langer Druck (Touch) bzw. Rechtsklick. Bauteil: Drehen, Umdrehen, Lang/Kurz,
  Wert, Nennspannung, Reparieren, abgesetzt »Entfernen«. Leitung: Farben (aktuelle mit Haken), abgesetzt
  »Leitung löschen«. Ausserdem Entf- bzw. Ruecktaste fuer die Auswahl; alles rueckgaengig machbar.

## Leitungsfarben
- Farbautomatik (Einstellung »Leitungsfarbe automatisch«, Vorgabe an): Eine neue Leitung, deren Ende am Pluspol liegt
  (P:+9V oder ueber Leitungen damit verbunden), wird rot; am Minuspol (P:0V) schwarz (`model::poleColor`). Alle
  anderen bekommen die in der Werkzeugleiste gewaehlte Farbe. Beim Umsetzen eines Endes gilt die Regel erneut.
- Nachtraeglich aendern: langer Druck bzw. Rechtsklick auf ein Leitungsende -> Leitungsmenue mit den Farben (aktuelle
  mit Haken). Eine von Hand gewaehlte Farbe
  (`WirePlacement::colorFixed`) aendert die Automatik nicht mehr.

## Steck- und Betriebsmodus
- Der Modus folgt dem Hauptschalter: aus = »✋ Stecken« (aufbauen, aendern, loeschen), ein = »⚡ Betrieb«
  (Schaltung testen). Anzeige und Umschalten ueber den Modusknopf neben der Experimentauswahl (Symbol und Text).
- Im Betrieb sind Bauteile, Leitungsenden und Module fest; Taster, Schalter, Reedkontakt, Wechselschalter, Pult und
  Regler bleiben bedienbar, ebenso Beobachtendes (Blick hinein, Spannungsanzeige, Schaltplan, Messgeraet).
- Automatischer Wechsel: Wer im Betrieb umbauen will - Bauteil oder Modul aus dem Fach ziehen, Leitung aus einem
  Feld ziehen, aendernde Menueaktion, Entfernen-Taste, Rueckgaengig/Wiederholen, Felder je Reihe -, schaltet damit den
  Strom aus und ist im Steckmodus (Hinweis »Strom aus – Steckmodus …«). Wie beim echten Basteln: erst ausschalten,
  dann umbauen.

## Licht und Waerme (Stufe 3, Variante B: Werkzeuge)
- Ausklappende Werkzeuggruppe »Licht und Wärme« unten mittig (`FlyoutBar`, Bedienmuster wie in Visual Technic Studio;
  sichtbar, sobald Fotowiderstand oder Heissleiter verfuegbar oder gesteckt sind): eingeklappt eine dunkle Pille
  »Licht, Wärme, Wasser ▴ 4«, bei Mausberuehrung oder Antippen klappt darueber die Werkzeugleiste aus; sie schliesst,
  wenn der Zeiger sie verlaesst (Touch: 4 s ohne Beruehrung), nicht waehrend des Ziehens. Weitere Gruppen lassen
  sich nebeneinander setzen. Inhalt:
  Raumlicht umschalten (☀ hell 300 lx / ⛅ Daemmerung 20 lx / 🌙 dunkel 0,5 lx; die Arbeitsflaeche wird
  entsprechend gedaempft), Taschenlampe, Foehn und Wassertropfen (ab Stufe 4) auf die Grundplatte ziehen oder
  antippen (legt sie in die Mitte).
- Auf der Grundplatte: frei verschiebbar, Antippen schaltet ein/aus, langer Druck bzw. Rechtsklick: Ein-/Ausschalten,
  Wegraeumen. Ein Schein zeigt die Reichweite; die Wirkung nimmt mit dem Abstand ab (`model/environment.hpp`):
  Taschenlampe bis 5000 lx direkt darueber, Foehn bis 80 Grad C, Wassertropfen macht die Feuchtesonde direkt darunter
  nass (ein Feld weiter kaum). Bedienhandlungen (kein Verlauf), auch im Betrieb.
- Neue Bauteile der Stufe 3: Fotowiderstand (Lichtschein je nach Helligkeit), Heissleiter (Perle, die bei Waerme rot
  wird; Temperatur an der Beschriftung), Elektromagnet (Bueroklammer wird angezogen, Feldlinien), Relais auf
  2-x-2-Platine (A | B Spule oben, COM | NO Schliesser unten; Kontaktzunge zeigt die Stellung).

## Zeitschreiber (Stufe 3)
- Knopf 📈 in der Werkzeugleiste (ab Stufe 3, freies Labor, alles freigegeben) blendet einen Streifen unter der
  Arbeitsflaeche ein (`RecorderPanel`, Modell `model::Recorder`).
- Zwei Kanaele: im Feld-Menue (langer Druck bzw. Rechtsklick) »Zeitschreiber Kanal 1/2: hier messen« (Spannung des
  Felds gegen 0 V) bzw. an den Buchsen des Messgeraets dessen Anzeige (V, mA oder Ω). Kanal 1 rot durchgezogen,
  Kanal 2 blau gestrichelt; das Feld traegt das Schild K1 bzw. K2; Kanal-Schild in der Kopfzeile mit aktuellem Wert.
- Zeitfenster 2 / 10 / 30 s, Anhalten/Weiter, Loeschen; Skala Spannung 0 .. Batteriespannung, sonst automatisch.
  Zeitachse = Simulationszeit (laeuft bei Aufbauaenderungen lueckenlos weiter).

## Formelrechner (Werkzeugkasten, Reiter »Rechner«, ab Stufe 3)
- Rechenblaetter mit Rechenweg in einer Zeile (`model/formula`): Ohmsches Gesetz (Dreieck, die gesuchte Groesse
  hervorgehoben; zwei Groessen eintragen, die dritte leer), Vorwiderstand fuer Leuchtdioden (Farbe waehlbar,
  naechstgroesserer E12-Normwert, Strom und Leistung damit, Warnung ueber 0,25 W, Uebernehmen auf den ausgewaehlten
  Widerstand), Reihe und parallel, Leistung; ab Stufe 4 Spannungsteiler, Zeitkonstante (mit Ladekurve, Marke bei
  63 %), Kippstufe (Blinkfrequenz). Gesperrte Blaetter mit Schloss.
- Eingaben mit Vorsatz: 4,7k, 4k7, 470m, 100µ (auch 100u), 2,2 MΩ, 15 mA; ungueltig: rot umrandet.
- Rechenmodus mit Tastenfeld (Ziffern, Komma, + − × ÷, Klammern, Vorsaetze p n µ m k M).

## Barrierefreiheit – Pruefung 10/2026
- Bedienelemente: alle Knoepfe mit Text oder `Accessible.name`; Tippflaechen mindestens 48 dp (Formelrechner-Knoepfe
  von 40 auf 48 korrigiert; die +/- Knoepfe der Grundplatte zeigen einen kleineren Kreis in einer 48-dp-Flaeche).
- Bildschirmleser: Kontaktfelder (»Feld B3«, mit Spannungsstufe), Bauteile (Name, Kennung, Wert, kaputt/ueberlastet,
  ausgewaehlt), Leitungen (Farbe, von, nach), Gruppenlinien, Schaltplan-Knoten und Profilsymbole haben Rolle und Namen.
- Kontrast (WCAG AA 4,5:1): Akzentblau auf #2873B0 abgedunkelt (Weiss darauf 5,0:1, vorher 4,4:1); Grautexte 6–7:1;
  Warnhinweise 7:1. Unterschreiten nur deaktivierte Elemente (zulaessig), die zusaetzlich grau und ohne Wirkung sind.
- Farbe nie allein: Leitungsfarbe mit Namen, Polaritaet mit +/−, Erfolg mit ✓, Warnung mit ⚠/!, Messart mit Buchstaben.
- Bewegung: »Animationen reduzieren« stoppt Pulsieren, Hopser und Stromfluss-Animation.
- Tastatur (Desktop): Strg+Z/Strg+Y/Entf, Escape schliesst Fenster; Knoepfe per Tab erreichbar (Qt-Standard).
- Offen: Rundgang mit TalkBack am Tablet (manuelle Pruefung), Schrift groesser als Stufe 3, Systemeinstellung
  »Animationen entfernen« von Android uebernehmen.

## Aufgaben der Lehrkraft (10/2026)
- Nur Hinweg (Lehrkraft -> Kinder), ohne Netz und ohne Kamera-Berechtigung: Die Kinder scannen mit der Kamera-App des
  Geraets oder tippen einen Code ein. Format in `services/task_code`.
- Lehrkraft (Elternbereich, PIN): »Aufgabe fuer die Klasse« - Experimente aus dem Lernpfad waehlen (bis 8, nach Stufen
  aufklappbar) und/oder den aktuellen Aufbau aus dem Labor mitgeben (mit Titel), dazu ein freier Aufgabentext.
  »QR-Code zeigen« fuellt das Fenster (Beamer): grosser QR-Code, daneben der Code zum Eintippen und der Inhalt.
- Code zum Eintippen: bis 4 Experimente ohne Text und Aufbau, 8 Zeichen Crockford-Base32 mit Pruefziffer
  (»2H31-0005«; O/0 und I/L/1 gleichwertig). Passt die Aufgabe in einen Code, enthaelt auch der QR-Code nur diesen
  (`volta1://aufgabe?c=...`, grob und gut lesbar); sonst `volta1://aufgabe?d=` mit gepacktem JSON (Aufbau, Text).
- Kind: Kachel »Aufgabe« auf der Startseite (Code eingeben bzw. aktuelle Aufgabe), Link aus der Kamera-App oeffnet
  das Aufgabenfenster direkt (mit Profilen erst nach der Profilwahl). Tick zeigt Text und Experimente; Antippen startet
  sie (auch in noch gesperrten Stufen), ein eigener Aufbau oeffnet im freien Labor. Die Aufgabe bleibt je Profil
  gespeichert (`aufgabe.txt`), bis eine neue kommt oder sie entfernt wird.

## Fotos im Forscherheft und Praesentationsmodus (10/2026)
- Forscherheft: »📷 Foto hinzufuegen« je Experiment (Systemauswahl, keine Kamera-Berechtigung), hoechstens 12 je
  Eintrag. Das Bild wird verkleinert (laengste Seite 1600 px) und als JPEG neu gespeichert - Kameradaten wie der
  Aufnahmeort fallen dabei weg. Ablage `fotos/` neben `forscherheft.json` im Profil; Vorschaubilder, Antippen
  vergroessert, »Foto entfernen« mit Rueckfrage. Zuruecksetzen des Forscherhefts loescht die Fotos. Die Profildatei
  (.volta1profil) nimmt Fotos nicht mit.
- Praesentationsmodus (Elternbereich, PIN): Vollbild (Desktop), Schrift +10 %, alle Stufen offen. Gearbeitet wird in
  einem leeren, nicht gespeicherten Stand; Fortschritt, Forscherheft und Laborstand des Profils bleiben unberuehrt.
  Hinweis unten links mit »Beenden« (zurueck zur Startseite, vorheriger Stand).
