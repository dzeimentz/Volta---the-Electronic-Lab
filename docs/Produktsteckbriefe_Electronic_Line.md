# Produktsteckbriefe Electronic Line

_Katalogdaten für den DC Learning Navigator: Volta 1–3, Zuse 1–3, MCU Explorer 1–2_

Markdown-Fassung und Quelle des PDFs `docs/Produktsteckbriefe_Electronic_Line_v0.1.pdf` (erzeugt mit `tools/konzept_pdf.py`).

| | |
|---|---|
| Dokument | Produktsteckbriefe |
| Version | 0.1 |
| Status | ENTWURF |
| Autor | Daniel Zeimentz · DC Software Engineering |
| Datum | 14.11.2026 |
| Bezug | DC Learning Navigator Framework – Breitenkonzept v2.0 (Abschnitte 4.2, 4.3, 5, 12, 17); Konzept Volta 2 v0.4; Konzept Volta 3 v0.5; Konzept Zuse v0.3; Konzept Zuse 2 v0.1; Konzept Zuse 3 v0.1; Konzept MCU Explorer v0.2; Inhalte Volta 1 (0.9.1) |
| Zweck | Beantwortet für die Electronic Line die offene Frage aus Abschnitt 17 des Navigator-Konzepts: »Welche Produktbilder, Beschreibungen und Inhaltsgliederungen sind pro Produkt verbindlich bereitzustellen?« |

## 1. Zweck und Aufbau

Das Navigator-Konzept verlangt für jedes Produkt eine **Kachel** (Bild, Name, Kurzbeschreibung, Status, Info-Symbol; Abschnitt 4.2) und eine jederzeit zugängliche **Detailansicht** (Abschnitt 4.3). Dieses Dokument liefert die Inhalte dafür aus den Produktkonzepten. Es legt keine Datenformate fest; die Felder sind so gewählt, dass sie sich später ohne Umdeutung in eine Katalogdefinition übertragen lassen.

Jeder Steckbrief hat dieselben Felder:

| Feld | Inhalt | Verwendung |
|---|---|---|
| Kachel | Name, Untertitel, Kurzbeschreibung (höchstens 110 Zeichen) | Produktkachel |
| Beschreibung | 3–5 Sätze für Eltern, Lehrkräfte und Jugendliche | Detailansicht |
| Inhaltsgliederung | Lernstufen mit Leitfrage und Zahl der Experimente | Detailansicht |
| Hauptmodule | Geräte, Bauteile und Werkzeuge, die das Produkt ausmachen | Detailansicht |
| Lernziele | 3–5 überprüfbare Ziele | Detailansicht |
| Zielgruppe, Vorkenntnisse | Alter, Schulstufe, Einstieg | Detailansicht, Filter |
| Voraussetzungen | erforderlich / empfohlen / optional (NAV-16) | Detailansicht, Kompatibilitätsstatus |
| Plattformen | je Plattform eigener Stand | Kompatibilitätsstatus |
| Lebenszyklus | geplant, in Entwicklung, Test, verfügbar, Wartung, archiviert (Abschnitt 12) | Statusanzeige |
| Querverweise | Vorgänger, Nachfolger, verwandte Produkte (Abschnitt 9, optional) | Detailansicht |
| Bildmotiv | Vorgabe für Kachelbild und Titelgrafik | Gestaltung |

**Sprache:** Kurzbeschreibungen sprechen die Lernenden in der Du-Form an, weil Kinder und Jugendliche die Kacheln lesen. Beschreibung und Lernziele sind sachlich formuliert, weil sie auch Eltern und Lehrkräften dienen. Alle Texte werden in DE, FR, EN und LB bereitgestellt; dieses Dokument enthält die deutsche Quellfassung.

**Ehrlicher Status:** Alle Angaben geben den Stand vom 14.11.2026 wieder. Produkte im Status »geplant« zeigen keinen Startknopf (Navigator-Konzept, Abschnitt 5). Zahlen zu Stufen und Experimenten stammen aus den Konzepten und können sich bis zur Veröffentlichung ändern; der Navigator zeigt sie deshalb erst ab Status »Test« verbindlich an und davor mit dem Zusatz »geplant«.

## 2. Überblick

| Produkt | Alter | Stufen / Experimente | Lebenszyklus | Plattformen (Stand) |
|---|---|---|---|---|
| Volta 1 | 8–12 | 5 / 61 | Test (0.9.1) | Android, Windows: Test · iPad: in Vorbereitung · Mac, Linux: geplant |
| Volta 2 | 11–14 | 9 / 61 | in Entwicklung | wie Volta 1 (gemeinsame Codebasis) |
| Volta 3 | 13–16 | 9 / 61 | in Entwicklung | Windows zuerst, Tablet nach stabiler Windows-Fassung |
| Zuse 1 | 10–14 | 6 / 23 | in Entwicklung (Prototyp) | Windows zuerst, Tablet danach |
| Zuse 2 | 12–16 | 9 / 41 | geplant (Konzept v0.1) | – |
| Zuse 3 | 14–16 | 10 / 38 | geplant (Konzept v0.1) | – |
| MCU Explorer 1 | 13–16 | 11 Stufen | geplant (Konzept v0.2) | Desktop zuerst, Tablet nach Machbarkeitstest |
| MCU Explorer 2 | 15–18 | 9 Stufen | geplant (Konzept v0.2) | wie MCU Explorer 1 |
| MCU Explorer 3 | – | – | nicht konzipiert | – |

Gemeinsam für alle Produkte der Linie: offline, ohne Konto, ohne Werbung, ohne Käufe in der App, ohne Datenerhebung; mehrsprachig (DE, FR, EN, LB); Bedienung mit Finger, Stift und Maus; Bedienziele mindestens 48 dp; Farbe nie einziger Informationsträger. Projekte, Forscherheft und Fortschritt bleiben in der jeweiligen App auf dem Gerät (NAV-19).

**Kernlinie:** Volta 1 → Volta 2 → Volta 3 → Zuse 1 → Zuse 2 → Zuse 3 → MCU Explorer. Kein Produkt setzt ein anderes als gekaufte App voraus; jedes hat einen eigenen Einstieg.

## 3. Volta – Das Elektronik-Labor

**Linienbild:** Grundplatte mit gesteckten Bauteilen, dunkelblaue Kopfzeile, Pult am Rand. Kennfarben der Bände wie im Farb- und Leitkonzept (Volta 1 Navy, Volta 2 Petrol, Volta 3 Beere). Keine fremden Markenlogos, keine echten Bauteilaufdrucke von Herstellern.

### 3.1 Volta 1

| Feld | Inhalt |
|---|---|
| Name | Volta 1 – Das Elektronik-Labor |
| Untertitel | Strom entdecken |
| Kurzbeschreibung | Bau Stromkreise mit Lämpchen, Motor und Leuchtdioden – und finde heraus, was Strom alles kann. |
| Bildmotiv | Grundplatte mit Batterie, Leuchtdiode, Taster und Motor; daneben das Dreifach-Messgerät |
| Lebenszyklus | Test (Version 0.9.1; Volta 1 ist auf 0.9 eingefroren, Korrekturen als 0.9.x) |

**Beschreibung.** Volta 1 ist ein virtueller Elektronik-Experimentierkasten für Kinder von 8 bis 12 Jahren. Die Kinder stecken Bauteile auf eine Grundplatte, schalten ein und beobachten, was passiert – berechnet von einer ehrlichen Simulation, nicht abgespielt. Jedes Experiment folgt dem Forschungskreislauf aus Frage, Vermutung, Aufbau, Beobachtung und Erklärung; zum Schluss zeigt »Jetzt in echt«, wo das Gelernte im Alltag vorkommt. Ein Forscherheft hält Vermutungen und Ergebnisse fest; Lehrkräfte können Aufgaben zusammenstellen.

**Inhaltsgliederung.**

| Stufe | Leitfrage | Experimente |
|---|---|---|
| 0 Auftakt | Wie bringe ich etwas zum Blinken? | 1 |
| 1 Strom fließt im Kreis | Wann leuchtet das Lämpchen? | 10 |
| 2 Mehrere Wege | Was passiert, wenn ich ein zweites Lämpchen einbaue? | 20 |
| 3 Messen und verstehen | Wie viel Strom fließt eigentlich? | 15 |
| 4 Kleine Elektronik | Wie kann ein Strom einen anderen steuern? | 15 |

**Hauptmodule.** Grundplatte mit Pult (Schalter, Taster, Drehregler), Batterien, Lämpchen, Leuchtdioden, Widerstände mit Farbcode, Motor, Summer, Reedkontakt, Diode, Kondensator, Licht- und Wärmesensor, Transistor, Relais; Blink-, Ton-, Verstärker- und Zeitmodul; Dreifach-Messgerät und Zeitschreiber; Werkzeugkasten mit Formelrechner, Lexikon und Fehlersuche; Freies Labor.

**Lernziele.**

- Einen geschlossenen Stromkreis aufbauen und erklären, warum ein offener Kreis nicht funktioniert.
- Reihen- und Parallelschaltung unterscheiden und ihre Wirkung vorhersagen.
- Spannung, Strom und Widerstand messen und den Zusammenhang beschreiben.
- Sensoren (Licht, Wärme, Feuchte, Magnet) als veränderliche Widerstände erkennen.
- Mit Transistor und Relais einen Strom durch einen anderen steuern.

**Zielgruppe und Vorkenntnisse.** 8–12 Jahre, Grundschule (Klasse 3–4) und Sekundarstufe I (Klasse 5–6), Familie, Arbeitsgemeinschaften. Keine Vorkenntnisse; Lesefähigkeit für kurze Texte.

**Voraussetzungen.**

| Art | Angabe |
|---|---|
| erforderlich | Android 8 oder neuer (arm64 oder armv7) bzw. Windows 10/11 64 Bit |
| empfohlen | Tablet ab 10 Zoll; Stift |
| optional | Echte Bauteile für »Jetzt in echt« (nicht Teil des Produkts) |

**Plattformen.** Android-Tablet: Test · Windows: Test · iPad: in Vorbereitung · Mac: geplant · Linux: geplant. Keine App-Berechtigungen auf Android.

**Querverweise.** Weiter mit Volta 2 (Messtechnik, Wechselspannung) oder Zuse 1 (Logik; Volta 1 enthält UND, ODER und Treppenhauslicht als Schaltung).

### 3.2 Volta 2

| Feld | Inhalt |
|---|---|
| Name | Volta 2 – Das Elektronik-Labor |
| Untertitel | Signale sehen und hören |
| Kurzbeschreibung | Mach Wechselspannung mit dem Oszilloskop sichtbar, bau Verstärker und Netzteil und hör, was du baust. |
| Bildmotiv | Grundplatte mit Timer-Baustein 555 und Lautsprecher; Oszilloskop mit Rechteck- und Sinuskurve |
| Lebenszyklus | in Entwicklung |

**Beschreibung.** Volta 2 begleitet den Physik- und Technikunterricht der Sekundarstufe I. Volta 1 zeigt, dass Strom etwas bewirkt; Volta 2 zeigt, wie Elektronik ein Signal formt. Messstellen werden als Bauteile gesteckt, ein Oszilloskop zeigt Wechselspannung, ein Funktionsgenerator liefert Sinus, Dreieck und Rechteck. Die Simulation ist hörbar: Der Lautsprecher gibt das berechnete Signal wieder. Die Module aus Volta 1 werden aus Einzelteilen nachgebaut, dazu kommen Operationsverstärker und Timer-Baustein 555.

**Inhaltsgliederung.**

| Stufe | Leitfrage | Experimente |
|---|---|---|
| 0 Auftakt | Was ist neu auf meinem Pult? | 3 |
| 1 Genauer messen | Gilt das immer – und wie genau? | 6 |
| 2 Wechselspannung sichtbar | Was zeigt ein Oszilloskop? | 8 |
| 3 Gleichrichten und stabilisieren | Wie wird aus Wechselspannung eine ruhige Gleichspannung? | 7 |
| 4 Der Transistor als Verstärker | Wie wird ein leises Signal laut? | 8 |
| 5 Operationsverstärker | Was kann ein Verstärker mit zwei Eingängen? | 8 |
| 6 Der Timer-Baustein 555 | Ein Chip für Takt und Zeit? | 9 |
| 7 Schwingungen und Klang | Wie entsteht ein Ton? | 6 |
| 8 Licht, das man nicht sieht | Wie funktioniert eine Lichtschranke? | 6 |

**Hauptmodule.** Bedienpult II; Messstellen V1/V2, A1/A2, Ω1/Ω2 und Tastköpfe K1/K2 als Bauteile; Einfach- und Doppelmessgerät; Oszilloskop mit Rollbetrieb; Funktionsgenerator; Kleinspannungs-Wechselquelle; Kennlinienschreiber; hörbare Simulation und Zeitlupe; OP-Modul; Timer-Baustein 555; Z-Diode, Spannungsregler, IR-LED, Fototransistor, Optokoppler.

**Lernziele.**

- Kirchhoffsche Regeln und Kennlinien durch Messung bestätigen.
- Periode, Frequenz und Effektivwert einer Wechselspannung am Oszilloskop bestimmen.
- Ein Netzteil aus Gleichrichter, Glättung und Regler aufbauen und unter Last prüfen.
- Arbeitspunkt, Verstärkung und Übersteuerung eines Transistorverstärkers erklären.
- Komparator, Verstärker und Kippschaltungen mit OP und 555 aufbauen.

**Zielgruppe und Vorkenntnisse.** 11–14 Jahre, Sekundarstufe I (Klasse 6–9), Physik- und Technikunterricht. Volta 1 hilfreich, aber nicht nötig: Neu Einsteigende beginnen mit Kurzanleitung und Rundgang in Stufe 0.

**Voraussetzungen.**

| Art | Angabe |
|---|---|
| erforderlich | wie Volta 1 |
| empfohlen | Lautsprecher oder Kopfhörer für die hörbare Simulation (Ton ist nie einziger Informationsträger) |
| optional | – |

**Plattformen.** Aus derselben Codebasis wie Volta 1; Android, Windows, Linux, iPadOS, macOS vorgesehen.

**Querverweise.** Vorgänger Volta 1; weiter mit Volta 3.

### 3.3 Volta 3

| Feld | Inhalt |
|---|---|
| Name | Volta 3 – Das Elektronik-Labor |
| Untertitel | Radio, Energie und Logik |
| Kurzbeschreibung | Bau dein eigenes Radio, mach aus wenig Spannung mehr und finde heraus, wie aus Spannung eine Eins wird. |
| Bildmotiv | Grundplatte mit Schwingkreis und Drehkondensator; Frequenzgang-Kurve; Siebensegmentanzeige |
| Lebenszyklus | in Entwicklung (Windows-Fassung) |

**Beschreibung.** Volta 3 schließt die Reihe ab und richtet sich an 13- bis 16-Jährige am Ende der Sekundarstufe I, in Wahlpflicht- und Technikkursen und beim Übergang in die Oberstufe oder Ausbildung. Die Jugendlichen bauen einen Schwingkreis und ein Radio, das echte Mittelwellen- und UKW-Sender in der App empfängt, untersuchen Schaltwandler, Thyristor und Verstärkerklassen, messen mit Kraft- und Hall-Sensor und gehen von der Spannung zur Logik: Pegel, Gatter, Flipflop und Zähler. Am Ende steht der Übergang zur Zuse-Linie.

**Inhaltsgliederung.**

| Stufe | Leitfrage | Experimente |
|---|---|---|
| 0 Auftakt | Welche Werkzeuge braucht ein Elektroniker? | 3 |
| 1 Schwingkreis und Resonanz | Warum schwingt etwas von selbst? | 7 |
| 2 Radio | Wie kommt Musik durch die Luft? | 8 |
| 3 Stromversorgung | Wie macht man aus wenig Spannung mehr – ohne Wärme? | 7 |
| 4 Thyristor und Triac | Wie dimmt man Wechselspannung? | 6 |
| 5 Verstärkerklassen | Wie viel Leistung, wie viel Wärme? | 6 |
| 6 Sensoren und Messverstärker | Wie misst man winzige Änderungen? | 8 |
| 7 Logik aus Spannung | Wie wird aus einer Spannung eine Eins? | 8 |
| 8 Takt und Zähler | Wie zählt eine Schaltung? | 8 |

**Hauptmodule.** Bedienpult III; Frequenzgang-Schreiber; Logikanalysator mit Trigger und Zeitmarken; Generator mit Amplitudenmodulation und Wobbelbetrieb; Senderanlage (Mittelwelle und UKW); Mittelwellen- und UKW-Modul; Gatter-, Zähler- und Anzeige-Modul; Drehkondensator, Antenne, Kristallhörer, Thyristor, Triac, Diac, MOSFET, Schottky-Diode, Leistungstransistoren, Speicherdrossel, Übertrager, Kraft- und Hall-Sensor; Formelrechner mit Blättern zu Schwingkreis, Güte, Wirkungsgrad und Pegel.

**Lernziele.**

- Resonanz eines Schwingkreises vorhersagen (Thomson-Formel) und im Frequenzgang nachweisen.
- Amplitudenmodulation und Empfang an einem selbst gebauten Radio erklären.
- Wirkungsgrad von Linearregler und Schaltwandler vergleichen.
- Eine Brückenschaltung mit Messverstärker für einen Sensor auslegen.
- Logikpegel, Gatter, Flipflop und Zähler an der echten Spannung untersuchen.

**Zielgruppe und Vorkenntnisse.** 13–16 Jahre, Ende Sekundarstufe I und Übergang Sekundarstufe II, technische Ausbildung. Oszilloskop, Generator, OP und 555 werden in Stufe 0 kurz wiederholt; Volta 2 wird nicht als gekaufte App vorausgesetzt.

**Voraussetzungen.**

| Art | Angabe |
|---|---|
| erforderlich | wie Volta 1; für Tablets ein Gerät mit ausreichender Rechenleistung (Mindestklasse wird nach dem Tablet-Test festgelegt) |
| empfohlen | Lautsprecher oder Kopfhörer (Radio, Sender) |
| optional | – |

**Plattformen.** Zuerst Windows; Tablet erst, wenn die Windows-Fassung stabil läuft.

**Querverweise.** Vorgänger Volta 2; weiter mit Zuse 1 (»Wie rechnet eine Schaltung?«) und MCU Explorer 1 (gemeinsame Digitalkopplung).

## 4. Zuse – Computertechnik

**Linienbild:** Bedienfeld mit Kippschaltern und LED-Reihen im Stil der ersten Bausatz-Rechner; Werkbank mit selbst erfundenen Bausteinen. Keine Bildschirmausgabe von Text, solange das Produkt keine kennt (Zuse 1 hat kein gespeichertes Programm).

### 4.1 Zuse 1

| Feld | Inhalt |
|---|---|
| Name | Zuse 1 – Computertechnik für Einsteiger |
| Untertitel | Vom Bit zur Rechenmaschine |
| Kurzbeschreibung | Entdecke, wie ein Computer entscheidet, rechnet und sich etwas merkt – und bau deine eigene Rechenmaschine. |
| Bildmotiv | 4-Bit-Bedienfeld mit Datenschaltern und LEDs; Werkbank mit Erfindungen (Addierer, Register) |
| Lebenszyklus | in Entwicklung (Prototyp mit 23 Experimenten und Freiem Labor) |

**Beschreibung.** Zuse 1 führt Kinder ohne Vorwissen in die Computertechnik. In Rätselkisten entdecken sie die Grundbausteine NICHT, UND, ODER und NAND, kombinieren sie auf der Werkbank zu eigenen Erfindungen und machen aus Erfindungen neue Werkzeuge. So entstehen Addierer, Speicher, Zähler und schließlich eine Rechenmaschine, die das Kind Schritt für Schritt selbst steuert. Zahlen erscheinen binär, dezimal und hexadezimal nebeneinander. Eine Zeitleiste erzählt die Geschichte der Logik und der Rechenmaschinen.

**Inhaltsgliederung.**

| Stufe | Leitfrage | Experimente |
|---|---|---|
| 0 Auftakt | Was versteht ein Rechner überhaupt? | 1 |
| 1 Logik entdecken | Wie entscheidet ein Rechner? | 4 |
| 2 Erfinden mit Bausteinen | Was kann ich aus meinen Bausteinen bauen? | 3 |
| 3 Zahlen und Rechnen | Wie rechnet ein Rechner mit 0 und 1? | 4 |
| 4 Speichern und Zählen | Wie merkt sich ein Rechner etwas – und wie zählt er? | 8 |
| 5 Die Rechenmaschine | Wie rechnet eine Maschine – und wer sagt ihr, was sie tun soll? | 3 |

**Hauptmodule.** Rätselkisten (entdecken), Werkbank mit Werkzeugkiste und eigenen Erfindungen (bauen), 4-Bit-Bedienfeld (zahlen), Wahrheitstabelle und Ablaufprüfung, Taktgeber, Zahlenleiste binär/dezimal/hex, »Blick hinein«, Freies Labor, Forscherheft, Historische Entwicklung.

**Lernziele.**

- Grundbausteine durch Ausprobieren finden und mit Wahrheitstabelle beschreiben.
- Aus Grundbausteinen eigene Schaltungen entwerfen und prüfen.
- Binärzahlen lesen und mit einem selbst gebauten Addierer addieren und subtrahieren.
- Erklären, wie Speicher, Takt und Zähler eine Maschine »sich merken« lassen.
- Die Schritte eines Rechenablaufs selbst als Steuerwerk ausführen.

**Zielgruppe und Vorkenntnisse.** 10–14 Jahre, Sekundarstufe I (Klasse 5–8), Informatik- und Technikunterricht, Arbeitsgemeinschaften. Keine Vorkenntnisse.

**Voraussetzungen.**

| Art | Angabe |
|---|---|
| erforderlich | Windows 10/11 64 Bit (erste Fassung) |
| empfohlen | Touchscreen oder Stift |
| optional | – |

**Plattformen.** Zuerst Windows; Android-Tablet, iPad, Mac und Linux danach aus derselben Codebasis.

**Querverweise.** Brücke aus Volta 3 (Zähler mit Anzeige) und Volta 1 (UND, ODER als Schaltung); weiter mit Zuse 2.

### 4.2 Zuse 2

| Feld | Inhalt |
|---|---|
| Name | Zuse 2 (Zusatz offen) |
| Untertitel | Der Rechner steuert sich selbst |
| Kurzbeschreibung | Bau Speicher, Bus und Steuerwerk – und bring deinen eigenen Rechner dazu, Programme selbst auszuführen. |
| Bildmotiv | Bedienfeld mit Daten- und Adressschaltern, Status-LEDs; Pixelmatrix 8 × 8 mit einfachem Spiel |
| Lebenszyklus | geplant (Konzept v0.1 vom 14.11.2026) |

**Beschreibung.** Zuse 2 beantwortet die Frage, mit der Zuse 1 endet: Wer gibt die Schritte, wenn niemand am Bedienfeld steht? Ausgehend von Konrad Zuses Idee des gespeicherten Ablaufs bauen die Jugendlichen einen Speicher mit Adressen, einen Bus und ein Steuerwerk, das Befehle holt und ausführt. So läuft zuerst der Zuse-4, ein 4-Bit-Rechner mit fest verdrahtetem Steuerwerk. In der zweiten Hälfte entsteht der Zuse-8 mit Mikroprogramm, eigenem Befehlssatz, Stapel, Tasten und Pixelmatrix. Programme werden erst am Bedienfeld eingegeben, zum Schluss in Assembler geschrieben.

**Inhaltsgliederung.**

| Stufe | Leitfrage | Experimente |
|---|---|---|
| 0 Auftakt | Wer gibt die Schritte? | 2 |
| 1 Speicher mit Adressen | Welcher Platz ist gemeint? | 5 |
| 2 Ein Bus für alle | Wie reden viele Bausteine über eine Leitung? | 3 |
| 3 Das Steuerwerk | Wie holt sich der Rechner seinen nächsten Befehl? | 5 |
| 4 Läuft er? | Was kann der Zuse-4? | 7 |
| 5 Mehr Bits | Wie wird aus 4 Bit ein 8-Bit-Rechner? | 4 |
| 6 Das Steuerwerk als Tabelle | Wie baut man ein Steuerwerk, das man ändern kann? | 5 |
| 7 Merken, Zurückkehren, Antworten | Wie arbeitet ein Programm mit Listen, Unterprogrammen und Tasten? | 6 |
| 8 Vom Schalter zum Assembler | Warum schreibt man Programme als Text? | 4 |

Insgesamt 41 Experimente, geplant (Konzept Zuse 2, Kapitel 5).

**Hauptmodule.** Decoder, Register mit Adressen, Bus mit Zustand Z, Befehlszähler, Befehlsregister, Phasenzähler, Steuermatrix; Zuse-4 (16 Wörter zu 8 Bit, 9 Befehle); Zuse-8 (256 Byte, Register A, X, PC, SP, etwa 24 Befehle, Mikroprogramm-Tabelle); Tasten, Pixelmatrix 8 × 8, Siebensegmentanzeige; Assembler mit Marken, Haltepunkte, Intel-HEX-Datei.

**Lernziele.**

- Den Befehlszyklus Holen – Entschlüsseln – Ausführen an einem selbst gebauten Rechner erklären.
- Erklären, warum ein Bus Bustreiber mit Zustand Z braucht.
- Ein fest verdrahtetes und ein mikroprogrammiertes Steuerwerk vergleichen.
- Programme mit Sprüngen, Stapel und Unterprogrammen schreiben.
- Ein Programm in Assembler schreiben, übersetzen und auf Fehler untersuchen.

**Zielgruppe und Vorkenntnisse.** 12–16 Jahre, Sekundarstufe I und Beginn Sekundarstufe II, Informatikunterricht. Gatter, Binärzahlen, Addierer und Register wie in Zuse 1; Stufe 0 wiederholt die Rechenmaschine.

**Voraussetzungen.** Wie Zuse 1; keine zusätzliche Hardware.

**Plattformen.** Vorgesehen wie Zuse 1; App-Kennung `com.dcsoftwareengineering.zuse2`.

**Querverweise.** Vorgänger Zuse 1; weiter mit Zuse 3.

### 4.3 Zuse 3

| Feld | Inhalt |
|---|---|
| Name | Zuse 3 (Zusatz offen) |
| Untertitel | Im Inneren des Prozessors |
| Kurzbeschreibung | Bau Unterbrechungen, Cache und ein Fließband in deinen Prozessor und miss, wie viel schneller er wird. |
| Bildmotiv | Prozessorkern Zuse-K mit Fließband aus drei Stufen und Messleiste (Takte); zwei Kerne an einem Speicher |
| Lebenszyklus | geplant (Konzept v0.1 vom 14.11.2026) |

**Beschreibung.** Zuse 3 öffnet den Blick auf die Bauweise heutiger Prozessoren. Aus dem Zuse-8 wird der Zuse-K, ein kleiner Prozessorkern mit Unterbrechungen, Zeitgeber und Ports, umschaltbar zwischen Von-Neumann- und Harvard-Bauweise. Die Jugendlichen bauen einen Zwischenspeicher (Cache), ein Fließband und vergleichen zwei Kerne an einem Speicher. Jede Verbesserung wird am eigenen Programm in Takten gemessen. Zum Schluss übersetzt die Lehrsprache Zuse-Text Programme sichtbar in Prozessorbefehle, und die letzte Stufe zeigt die Register eines echten Mikrocontrollers.

**Inhaltsgliederung.**

| Stufe | Leitfrage | Experimente |
|---|---|---|
| 0 Auftakt | Warum ruckelt mein Spiel? | 2 |
| 1 Unterbrechungen | Wie meldet sich die Welt beim Rechner? | 5 |
| 2 Zeitgeber | Wie misst ein Rechner Zeit, ohne zu warten? | 4 |
| 3 Ports als Speicheradressen | Wie spricht ein Programm mit Geräten? | 4 |
| 4 Programm und Daten | Darf ein Programm sich selbst verändern? | 4 |
| 5 Speicherhierarchie (Vertiefung) | Warum ist Speicher langsamer als der Prozessor? | 4 |
| 6 Fließband | Kann ein Prozessor mehrere Befehle gleichzeitig bearbeiten? | 4 |
| 7 Mehrere Kerne (Vertiefung) | Werden zwei Kerne doppelt so schnell? | 4 |
| 8 Vom Text zum Befehl | Was macht ein Übersetzer aus meinem Text? | 4 |
| 9 Alles auf einem Chip | Was steckt in einem Mikrocontroller? | 3 |

Insgesamt 38 Experimente, geplant (Konzept Zuse 3, Kapitel 5). Stufen 5 und 7 sind als abwählbare Vertiefung vorgeschlagen, aber noch nicht entschieden.

**Hauptmodule.** Zuse-K mit Unterbrechungen (EI, DI, RETI), Zeitgeber, Ports mit Richtungsregister; Von-Neumann/Harvard-Umschaltung; Cache mit 4 oder 8 Zeilen; Fließband mit drei Stufen; zwei Kerne mit Schiedsrichter und Sperrbefehl; Messleiste; Lehrsprache Zuse-Text mit den Ansichten Wörter, Baum, Befehle und Speicher.

**Lernziele.**

- Unterbrechung und Abfrage vergleichen und den Unterschied in Takten belegen.
- Einen Zeitgeber aus Zähler, Vorteiler und Vergleicher bauen und nutzen.
- Von-Neumann- und Harvard-Bauweise unterscheiden.
- Wirkung von Cache, Fließband und mehreren Kernen messen und ihre Grenzen erklären.
- Nachvollziehen, wie ein Übersetzer aus Text Prozessorbefehle macht.

**Zielgruppe und Vorkenntnisse.** 14–16 Jahre, Sekundarstufe I/II, Informatik-Wahlpflicht und Oberstufe. Befehlszyklus, Assembler und Stapel wie in Zuse 2.

**Voraussetzungen.** Wie Zuse 1; keine zusätzliche Hardware.

**Plattformen.** Vorgesehen wie Zuse 1.

**Querverweise.** Vorgänger Zuse 2; weiter mit MCU Explorer 1 (Experiment 9.3 »Der echte Mikrocontroller«).

## 5. MCU Explorer – Mikrocontroller verstehen

**Linienbild:** Codefenster neben der Volta-Grundplatte; ein neutral gezeichneter Mikrocontroller mit Pinbeschriftung, LED mit Vorwiderstand, Oszilloskop. Keine Logos von Board- oder Chipherstellern; »Arduino-Uno-kompatibel« nur als sachliche Kompatibilitätsangabe im Text (Konzept MCU Explorer, Kapitel 8).

### 5.1 MCU Explorer 1

| Feld | Inhalt |
|---|---|
| Name | MCU Explorer 1 |
| Untertitel | Mikrocontroller programmieren |
| Kurzbeschreibung | Schreib echte Programme für einen Mikrocontroller und sieh sofort, was dein Code an der Schaltung bewirkt. |
| Bildmotiv | Codefenster mit Blinkprogramm; Mikrocontroller an der Grundplatte mit LED und Taster; Rechteck im Oszilloskop |
| Lebenszyklus | geplant (Konzept v0.2 vom 14.11.2026) |

**Beschreibung.** Im MCU Explorer schreiben Jugendliche Programme in C/C++ für einen Arduino-Uno-kompatiblen Mikrocontroller (ATmega328P). Das Programm wird wirklich übersetzt und läuft befehlsgenau in einem Emulator, dessen Anschlüsse mit der Volta-Simulation verbunden sind. Deshalb hat Elektrik Folgen: Eine LED ohne Vorwiderstand überlastet den Pin, ein offener Eingang liefert unbestimmte Pegel, ein Motor braucht Transistor und Freilaufdiode – wie in der Wirklichkeit, aber ohne Risiko. Der Lernpfad führt vom Blinklicht bis zu Interrupts und einem eigenen Projekt.

**Inhaltsgliederung.**

| Stufe | Forscherfrage |
|---|---|
| 1 | Wie bringe ich einen Chip zum Blinken? |
| 2 | Wie merkt der Chip einen Tastendruck? |
| 3 | Wie läuft ein Licht von selbst? |
| 4 | Wie dimmt man eine LED? |
| 5 | Wie misst der Chip eine Spannung? |
| 6 | Was denkt der Chip gerade? |
| 7 | Wie spielt der Chip Musik? |
| 8 | Wie schaltet ein schwacher Pin einen starken Motor? |
| 9 | Wie wartet man, ohne stillzustehen? |
| 10 | Wie reagiert der Chip sofort? |
| 11 | Projekt: Ampel, Nachtlicht oder Messstation |

**Hauptmodule.** Codeeditor mit Einzelschritt; Übersetzung mit avr-gcc (Desktop); Emulator ATmega328P (16 MHz); Pinansicht mit Grenzwerten; Volta-Grundplatte mit Messgeräten, Oszilloskop und Logikanalysator; serieller Monitor; Register als LED-Reihe.

**Lernziele.**

- Ein Programm mit setup/loop schreiben, übersetzen und ausführen.
- Digitale Ein- und Ausgänge mit Pull-up und Vorwiderstand richtig beschalten.
- PWM und Analog-Digital-Wandlung messen und erklären.
- Lasten über Transistor und Freilaufdiode schalten.
- Zeitsteuerung ohne Blockieren und einfache Interrupts nutzen.

**Zielgruppe und Vorkenntnisse.** 13–16 Jahre, Sekundarstufe I/II, Maker-Gruppen. Keine Programmiervorkenntnisse; Grundbegriffe Spannung, Strom und Widerstand (z. B. aus Volta 1).

**Voraussetzungen.**

| Art | Angabe |
|---|---|
| erforderlich | Desktop (Windows, macOS, Linux); Tablet erst nach Machbarkeitstest |
| empfohlen | Tastatur zum Programmieren |
| optional | Echtes Arduino-Uno-kompatibles Board mit USB-Kabel (spätere Ausbaustufe, nicht Teil des ersten Produkts) |

**Plattformen.** Desktop zuerst; Android-Tablet und iPad nach Machbarkeitstest (Übersetzung auf dem Gerät oder über den Desktop als Übergang).

**Querverweise.** Vorgänger Zuse 3 (Prozessorinneres) und Volta 1–3 (Elektrik); weiter mit MCU Explorer 2.

### 5.2 MCU Explorer 2

| Feld | Inhalt |
|---|---|
| Name | MCU Explorer 2 |
| Untertitel | Den Chip von innen steuern |
| Kurzbeschreibung | Steuer Timer, Wandler und Schnittstellen direkt über die Register und sieh, was der Übersetzer aus deinem Code macht. |
| Bildmotiv | Registeransicht als Bedienfeld; Logikanalysator mit serieller Übertragung; Disassembler neben C-Code |
| Lebenszyklus | geplant (Konzept v0.2) |

**Beschreibung.** MCU Explorer 2 richtet sich an die Oberstufe und die Berufsausbildung. Statt Bibliotheksfunktionen werden Timer, Analog-Digital-Wandler, Interrupts und serielle Schnittstellen direkt über die Register des Mikrocontrollers eingestellt. SPI und I²C verbinden simulierte Sensoren und Anzeigen; Schlafmodi zeigen, wie ein Gerät Strom spart. Ein Disassembler macht sichtbar, welche Befehle der Übersetzer erzeugt.

**Inhaltsgliederung.** 1 Blick in den Chip · 2 Timer selbst einstellen · 3 ADC ohne Bibliothek · 4 Interrupts im Detail · 5 Serielle Schnittstelle auf Registerebene · 6 Sparsam rechnen · 7 Bausteine am Bus · 8 Was der Übersetzer daraus macht · 9 Projekt.

**Lernziele.**

- Peripherie über Register und Bitmasken konfigurieren.
- Timer, ADC und Interrupts ohne Bibliothek einsetzen.
- Serielle Protokolle im Logikanalysator lesen.
- Stromaufnahme durch Schlafmodi senken.
- Übersetzten Maschinencode lesen und Befehlszyklen abschätzen.

**Zielgruppe und Vorkenntnisse.** 15–18 Jahre, Oberstufe, Berufsausbildung. MCU Explorer 1 oder gleichwertige Erfahrung mit C/C++ auf Mikrocontrollern.

**Voraussetzungen und Plattformen.** Wie MCU Explorer 1.

**Querverweise.** Vorgänger MCU Explorer 1; inhaltliche Nähe zu Zuse 3 (Register, Unterbrechungen).

### 5.3 MCU Explorer 3

Im Navigator-Konzept als dritte Stufe genannt, in den Produktkonzepten bisher nicht beschrieben. Bis zu einer Entscheidung zeigt der Navigator keine Kachel für MCU Explorer 3, oder höchstens eine Kachel mit Status »geplant« ohne Inhaltsgliederung (Navigator-Konzept, Abschnitt 14: wahrheitsgemäßer Status).

## 6. Vorgaben für Bilder

- Jedes Bild zeigt eine Szene aus dem Produkt selbst (Grundplatte, Bedienfeld, Codefenster), keine Fotos fremder Geräte.
- Keine fremden Marken- oder Firmenlogos (Board-, Chip-, Betriebssystem- oder KI-Hersteller), keine echten Herstelleraufdrucke auf Bauteilen.
- Nur Inhalte zeigen, die das Produkt wirklich hat: keine Textausgabe bei Zuse 1, kein ESP32 beim MCU Explorer (festgelegt ist der ATmega328P).
- Status sichtbar und nicht nur über Farbe: Textchip »geplant«, »in Entwicklung«, »Test«, »verfügbar«.
- Die Kachel bleibt ohne Lizenz voll lesbar; gesperrt wird nur der Startknopf (NAV-04 bis NAV-06).
- Gestaltung nach dem DC Learning Design Standard mit Volta als Vorlage; Maskottchen nur bei Volta 1 und Zuse 1 im Vordergrund.

## 7. Offene Punkte

| Punkt | Vorschlag |
|---|---|
| Alter Zuse 1: Konzept 10–14, Volta-1-Flyer 12–16 | Konzept gilt (10–14); Flyer bei nächster Auflage angleichen |
| Produktzusätze Zuse 2 und Zuse 3 | Untertitel aus diesem Dokument als Arbeitsstand |
| MCU Explorer 3 | erst konzipieren oder aus dem Navigator-Katalog streichen |
| Stufen- und Experimentzahlen geplanter Produkte | im Navigator erst ab Status »Test« verbindlich anzeigen |
| Mindestgeräteklasse Volta 3 auf Tablets | nach dem Tablet-Test festlegen |
| Übersetzung der Steckbriefe in FR, EN, LB | nach Freigabe der deutschen Fassung |
