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
- Werkzeugleiste: Experimente, Rueckgaengig, Wiederholen, Breite der Grundplatte »− 1–12 +« (8/10/12 Felder je Reihe), Spannungsanzeigen ein/aus, Schaltplan, Werkzeugkasten, Fragezeichen,
  Flaggensymbol, Hauptmenue.
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
- Batterie im Pult: angedeutete 4,5-V-Flachbatterie (Gehaeuse, Banderole, zwei Messingfahnen) als Hintergrund
  von Spannungswahl und Hauptschalter; ohne Spannungsaufdruck, die gewaehlte Spannung steht im Auswahlfeld.
- Steckloecher der Kontaktfelder quadratisch.
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

## Interaktives Readme
- Rundgang beim ersten Start mit Begleitfigur; danach ueber das Fragezeichen.
- Abschnitte: Willkommen, Grundplatte, Leitungen, Bedienpult, Anzeigen, Rueckgaengig, Werkzeugkasten,
  Sprache und Einstellungen – jeweils mit Uebungsplatte (vom Lernpfad getrennt) und Aufgabe mit Bestaetigung.
- Teil fuer Erwachsene (hinter Elternschranke): Systemanforderungen, Gesten/Tastenkuerzel, Datenschutz,
  Konfigurationsschluessel fuer Schulen, Aenderungsliste. »Was ist neu« nach Updates. Kontexthilfe, Volltextsuche.

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
- Einfache Rechenaufgabe mit Zahlen in Worten; schuetzt Elternbereich, externe Links, Readme-Erwachsenenteil.

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
