# Volta 3 – Das Elektronik-Labor

_Konzept des dritten Bandes: Schwingkreis, Radio, Stromversorgung, Messtechnik und digitale Grundschaltungen für 13 bis 16 Jahre_

Markdown-Fassung des Konzepts und Quelle des PDFs `docs/Konzept_Volta3_v0.2.pdf` (erzeugt mit `tools/konzept_pdf.py`).

| | |
|---|---|
| Dokument | Konzept |
| Version | 0.2 |
| Status | ENTWURF |
| Autor | Daniel Zeimentz · DC Software Engineering |
| Datum | 31.10.2026 |
| Bezug | Konzept Volta v3.0 (Kap. 4.1, 4.3, 5.4); Konzept Volta 2 v0.2; Konzept Zuse v0.3; Konzept MCU Explorer v0.1; Farb- und Leitkonzept Volta 1 (0.9.1, `docs/ui-spec.md`) |
| Zielplattformen | Android (Tablet), Windows und Linux x86-64, iPadOS, macOS – aus derselben Codebasis wie Volta 1 und 2 |
| Änderungen zu v0.1 | Grundsatzentscheidungen zu Frequenzgang, Radio und Schaltwandler getroffen (31.10.2026, Kapitel 4); Masse/GND bleibt schwarz |
| Name | Arbeitstitel; die Namensfrage der Reihe ist offen (`docs/partner/rechte-und-lizenzen.md`, Abschnitt 3) |

## 1. Zusammenfassung

Volta 3 ist der dritte und letzte Band der Reihe »Volta – Das Elektronik-Labor« und eine eigene App aus derselben Codebasis (`VOLTA_EDITION=volta3`, `com.dcsoftwareengineering.volta3`). Er richtet sich an 13- bis 16-Jährige: das Ende der Sekundarstufe I, Wahlpflicht- und Technikkurse, Arbeitsgemeinschaften und den Übergang in die Sekundarstufe II oder eine technische Ausbildung.

Volta 1 hat gezeigt, **dass** Strom etwas bewirkt, Volta 2, **wie** Analogelektronik ein Signal formt. Volta 3 beantwortet die Fragen, die danach kommen: **Wie kommt Musik durch die Luft? Wie macht ein Ladegerät aus wenig Spannung mehr, ohne heiß zu werden? Wie misst eine Waage ein Gewicht? Und wie wird aus einer Spannung eine Eins?** Dafür kommen ein **Frequenzgang-Schreiber**, ein **Logikanalysator**, eine **zeitgedehnte Darstellung** für Schwingungen, die zu schnell zum Rechnen und Sehen sind, und eine kleine **Senderanlage** im Pult hinzu, die nur eigens erstellte Tonaufnahmen ausstrahlt.

Neue Bauteile sind Drehkondensator, Antenne, Kristallhörer, Thyristor, Triac, MOSFET, Schottky-Diode, Kraft- und Hall-Sensor und die Siebensegmentanzeige; neue Module sind der Mittelwellen- und der UKW-Empfänger, das Gatter-Modul (vier NAND), ein Zähler- und ein Decoder-Modul. Am Ende des Bandes steht die Brücke zur Zuse-Linie: Ein Zähler mit Anzeige ist der erste Schritt zum Rechner.

Der Lernpfad umfasst wie Volta 1 und 2 **neun Stufen (0–8) mit 61 Experimenten**. Bedienung, **Farb- und Leitkonzept**, Lernpfad, Forschungskreislauf, Forscherheft, Werkzeugkasten, Einstellungen, Profile, Aufgaben der Lehrkraft und alle Regeln zu Datenschutz, Kindersicherheit und Barrierefreiheit bleiben wie in Volta 1 und 2.

Volta 3 setzt die Technik von Volta 2 voraus (Oszilloskop, Funktionsgenerator, Wechselquelle, hörbare Simulation, OP und 555). Die größten neuen technischen Aufgaben sind die **Wechselstromanalyse** im Kern (Frequenzgang), die **ehrliche Behandlung von Hochfrequenz** (Radio) und die **Kopplung digitaler Bausteine** an den analogen Kern; die letzte teilt sich Volta 3 mit dem MCU Explorer.

**Empfehlung:** Dieses Konzept als Richtung festhalten; die drei technischen Grundsatzentscheidungen sind getroffen (31.10.2026, Kapitel 4). Vor dem Start bestätigt ein Leistungs- und Machbarkeitstest Frequenzgang, Radiomodell und Schaltwandler auf dem Tablet (Schritt 1). Begonnen wird erst, wenn Volta 2 als Testversion steht.

## 2. Einordnung in die Reihe

| Band | Alter | Schwerpunkt | Messen und Zeigen |
|---|---|---|---|
| Volta 1 | 8–12 | Stromkreis, Reihe und parallel, Wirkungen des Stroms, Sensoren, erste Elektronik mit Modulen | Dreifach-Messgerät, Zeitschreiber |
| Volta 2 | 11–14 | Messtechnik, Wechselspannung, Netzteil, Transistorverstärker, Operationsverstärker, 555, Klang, Infrarot | zwei Messgeräte, Oszilloskop, Funktionsgenerator, hörbare Simulation |
| **Volta 3** | **13–16** | **Schwingkreis und Resonanz, Radio, Schaltwandler, Thyristor bei Kleinspannung, Verstärkerklassen, Sensoren und Messverstärker, Logikpegel, Gatter, Flipflop, Zähler** | **Frequenzgang-Schreiber, Logikanalysator, zeitgedehnte Darstellung, Senderanlage** |
| Zuse 1 | 10–14 | Computertechnik: vom Gatter zum selbst gebauten Rechner (eigene Linie) | Bedienfeld, Wahrheitstabellen |
| MCU Explorer | ab 14 | echter Mikrocontroller in C/C++ an der Volta-Grundplatte | Emulator, Pinansicht |

**Abgrenzung zu Volta 2:** Oszilloskop, Generator, OP und 555 werden vorausgesetzt und in Stufe 0 kurz wiederholt. Volta 3 setzt Volta 2 nicht als gekaufte App voraus; wer neu einsteigt, findet die Geräte und Bauteile von Volta 2 im Bauteilfach und im Lexikon.

**Abgrenzung zu Zuse:** Zuse behandelt Gatter **logisch** (Rätselkisten, Wahrheitstabelle, Rechner aus Erfindungen) und baut keine Standard-Logikbausteine nach. Volta 3 zeigt die **elektrische Seite**: Pegel, Schaltschwellen, ein NAND aus Transistoren, Prellen, Takt und Zähler an der echten Spannung. Volta 3 endet mit einer Zähler-Anzeige und verweist dann auf Zuse 1 (»Wie rechnet eine Schaltung?«).

**Abgrenzung zum MCU Explorer:** Programmierbare Bausteine kommen in Volta 3 nicht vor. Die Kopplung digitaler Ausgänge an den analogen Kern (Pegel, Ausgangswiderstand, Schaltschwelle) wird einmal gebaut und von beiden genutzt.

**Lehrplanbezug (Ende Sek. I, Übergang Sek. II):** elektromagnetische Schwingungen und Wellen (Schwingkreis, Resonanz, Thomson-Formel), Nachrichtenübertragung (Modulation, Empfang), Energie und Wirkungsgrad, Halbleiter in der Leistungselektronik, Sensorik und Messtechnik (Brückenschaltung), Grundlagen der Digitaltechnik (Pegel, Gatter, Flipflop, Zähler). Die genaue Zuordnung für Deutschland (Länder), Luxemburg und Ostbelgien folgt im Schulpaket.

## 3. Ziele und Nicht-Ziele

**Ziele**
- **Unsichtbares sichtbar machen:** Resonanz, Trägerschwingung, Hüllkurve, Schaltflanken und Logikpegel werden mit Frequenzgang, Oszilloskop und Logikanalysator gezeigt.
- **Energie bewusst machen:** Wirkungsgrad messen und vergleichen – Längsregler gegen Schaltwandler, Klasse A gegen Klasse D.
- **Messen mit Verstand:** Brückenschaltung und Messverstärker; Messfehler, Auflösung und Störungen offen ansprechen.
- **Ehrlich bei schnellen Vorgängen:** Was zu schnell für die Simulation auf dem Tablet ist, wird **erkennbar** gedehnt oder als erklärtes Verhaltensmodell gerechnet und immer so gekennzeichnet (Regel 3).
- **Brücke zur Computertechnik:** Vom Transistor über das Gatter zum Zähler mit Anzeige.

**Nicht-Ziele**
- Keine Netzspannung. Thyristor, Triac und Phasenanschnitt nur an der **Kleinspannungs-Wechselquelle** im Pult (Regel 6); Texte sagen deutlich, dass Dimmer am Stromnetz nur Fachleute anschließen.
- **Kein Senden** in der echten Welt: keine Bauanleitung für Sender (Funkrecht), kein Empfang echter Rundfunkprogramme. Die Senderanlage im Pult spielt ausschließlich eigens erstellte Tonaufnahmen ab (Morsezeichen, Sprechansagen, selbst eingespielte Klangbeispiele).
- Keine Programmierung, keine Mikrocontroller (MCU Explorer); keine Nachbildung von Standard-Logikbausteinen mit Pinbelegung als Pflichtwissen.
- Kein Mikrofon und keine Kamera des Geräts als Eingang.
- Keine Kopie des gestalterischen Vorbilds (Regel 2); alle Experimente sind Eigenentwicklungen.

## 4. Technische Grundsatzentscheidungen (31.10.2026)

Die Simulation rechnet für hörbare Signale mit 20 µs Schrittweite (Volta 2). Echte Rundfunk- und Schaltwandlerfrequenzen liegen weit darüber (Mittelwelle um 1 MHz, UKW um 100 MHz, Schaltwandler 50 kHz bis 2 MHz). Volta 3 braucht deshalb drei Grundsatzentscheidungen, die alle der Regel »Physik ist ehrlich« folgen:

| Frage | Entscheidung |
|---|---|
| Frequenzgang | **Wechselstromanalyse im Kern**: Die Schaltung wird am Arbeitspunkt linearisiert und für jede Frequenz komplex gelöst (dieselbe Knotenmatrix wie im Zeitbereich). So entstehen Resonanzkurven und Filterkennlinien von 1 Hz bis 10 MHz in Sekundenbruchteilen, exakt für lineare Schaltungen. |
| Radio | **Abstimmen echt, Hochfrequenz als Hüllkurve:** Welche Sender der Schwingkreis auf der Grundplatte durchlässt, rechnet die Wechselstromanalyse an den echten Senderfrequenzen. Die Trägerschwingung selbst wird nicht Schritt für Schritt gerechnet, sondern als Hüllkurve (Ton mal Durchlass des Kreises); Detektor, Verstärker und Lautsprecher laufen danach normal in der Simulation und sind hörbar. Das Oszilloskop zeigt den Träger **zeitgedehnt** (z. B. 1 MHz als 1 kHz, deutlich mit Lupenzeichen und Faktor beschriftet). Die UKW-Stufe (Frequenzmodulation) ist ein erklärtes Verhaltensmodell im UKW-Modul. |
| Schaltwandler und Klasse D | **echte Rechnung bei niedrigerer Schaltfrequenz:** Abwärts-, Aufwärtswandler und Klasse-D-Stufe schalten in Volta 3 mit 1–5 kHz und entsprechend großen Spulen und Kondensatoren. Das ist physikalisch dieselbe Schaltung, nur langsamer, und wird vollständig im Zeitbereich gerechnet; »Mehr wissen« erklärt, warum echte Wandler viel schneller schalten (kleinere Bauteile). |

Ebenfalls entschieden: **Digitale Module** (Gatter, Zähler, Decoder) rechnen ihre Logik ereignisgesteuert (wie die Zuse-Logiksimulation) und sind an ihren Anschlüssen als Schaltschwelle (Eingang) und Spannungsquelle mit Innenwiderstand (Ausgang) an den analogen Kern gekoppelt. Das NAND aus Einzeltransistoren auf der Grundplatte bleibt vollständig analog.

## 5. Bedienung und Bedienpult III

Grundplatte, Kontaktfelder, gezogene Leitungen, Undo/Redo, Spannungsanzeige, Stromfluss, Schaltplan, Aufbauhilfe, Werkzeugkasten, Messgeräte 1 und 2, Oszilloskop und Funktionsgenerator sind dieselben wie in Volta 2.

### 5.1 Pultleiste

| Pultfeld | Bauteil | Bedeutung |
|---|---|---|
| `V+`, `0V` | Batterie 1 | wie Volta 2; zusätzlich **1,5 V (eine Zelle)** für Aufwärtswandler und Detektorversuche |
| `+B2`, `−B2` | Batterie 2 | wie Volta 2 (symmetrische Versorgung) |
| `W~a`, `W~b` | Wechselquelle | wie Volta 2, 6 / 9 V~, 50 Hz – Quelle für Thyristor und Triac |
| `G`, `G0` | Funktionsgenerator | wie Volta 2; neu: **Wobbelbetrieb** (Frequenz läuft selbst durch) und **Amplitudenmodulation** mit einem Ton |
| `ANT` | Senderanlage (neu) | drei Sender auf Mittelwelle und zwei auf UKW mit eigenen Aufnahmen; ein Feld für die Antenne auf der Grundplatte. Bedienung im Senderfenster: Sender an/aus, Inhalt wählen |
| `P1…`, `P2…` | Drehregler | wie Volta 1 und 2 |
| `S1`, `S2` | Schalter | wie Volta 1 und 2 |
| `LS1`, `LS2` | Lautsprecher | wie Volta 2 (hörbar) |

### 5.2 Neue Mess- und Anzeigegeräte

- **Frequenzgang-Schreiber** (Werkzeug, ab Stufe 0): Eingang am Generator, Ausgang an einem beliebigen Feld; zeichnet Verstärkung in dB und wahlweise die Phase über der Frequenz (logarithmisch, 1 Hz … 10 MHz). Markiert Resonanzfrequenz, Grenzfrequenz (−3 dB) und Bandbreite und nennt sie als Zahl. Bei nichtlinearen Schaltungen (Gleichrichter, Komparator) zeigt er einen Hinweis statt einer falschen Kurve.
- **Logikanalysator** (ab Stufe 7): acht Kanäle mit Messspitze auf ein beliebiges Feld; zeigt H und L als Zeitdiagramm, Schaltschwellen einstellbar (Vorgabe nach Betriebsspannung), Trigger auf Flanke oder Muster, Zeitmarken mit Abstand in µs/ms. Der Zustand jedes Kanals steht zusätzlich als Buchstabe H/L bzw. 1/0.
- **Zeitgedehnte Darstellung** (Lupe): Oszilloskop und Logikanalysator zeigen Vorgänge, die schneller sind, als die Simulation Schritt für Schritt rechnet, mit einem Dehnungsfaktor (×10 … ×1000). Gedehnte Anzeigen tragen immer Lupenzeichen, Faktor und einen gestrichelten Rahmen.
- **Frequenzzähler** (im Messgerät 1, ab Stufe 8): zählt Flanken in einer Torzeit von 0,1 / 1 s – derselbe Grundgedanke wie Experiment 8.6.

### 5.3 Senderanlage und Inhalte

Die Sender spielen ausschließlich eigens erstellte Aufnahmen: Morsezeichen (»VOLTA«), Sprechansagen in DE/FR/EN/LU (»Hier ist Sender 1 auf 1000 Kilohertz«), Zeitzeichen-Piepen und selbst eingespielte Melodien. Keine fremden Rundfunkinhalte, keine Verbindung ins Netz. Die Tonaufnahmen werden wie die übrigen Tonaufnahmen nach `docs/i18n.md` erstellt und lizenziert.

## 6. Bauteile und Module

Neu in Volta 3 (Katalog `content/catalog/bauteile.json`, Feld `ab_band: 3`); alle Bauteile aus Volta 1 und 2 bleiben verfügbar.

| Bauteil / Modul | Modell im Kern | Referenz (Abweichung < 2 %) |
|---|---|---|
| Drehkondensator 10–500 pF | Kondensator mit Drehregler im Bauteil | Resonanzfrequenz |
| Spulen 100 µH, 1 mH, 10 mH | Induktivität mit Wicklungswiderstand und Güte | Schwingkreis, Wandler |
| Antenne (Draht) | Signalquelle der Senderanlage mit Innenwiderstand und kleiner Kapazität | Detektorempfänger |
| Kristallhörer | hochohmiger Hörer (Kapazität mit Parallelwiderstand), hörbar | Detektorempfänger |
| Schottky- und Germaniumdiode | Diodenmodell mit niedriger Schwellspannung | Hüllkurvendetektor |
| Thyristor | Zwei-Transistor-Modell mit Zünd- und Haltestrom | Halten, Löschen, Phasenanschnitt |
| Triac und Diac | Thyristorpaar gegenparallel, Diac als Kippdiode | Phasenanschnitt beider Halbwellen |
| MOSFET N-Kanal (Logikpegel) | Quadratisches Modell mit Durchlasswiderstand und Gate-Kapazität | Schalter, Wandler, Klasse D |
| Kraftsensor (Dehnungsmessstreifen-Brücke) | vier Widerstände mit Kraftanteil | Waage |
| Hall-Sensor | Spannungsausgang proportional zum Magnetfeld | Magnetfeld messen |
| Siebensegmentanzeige | sieben LEDs mit gemeinsamer Kathode | Anzeige |
| **MW-Modul** | Verhaltensmodell: Abstimmung mit Drehkondensator, Hüllkurve, Tonausgang | Empfang der Senderanlage |
| **UKW-Modul** | Verhaltensmodell: Abstimmung, Frequenzdemodulation erklärt, Tonausgang | Empfang der Senderanlage |
| **Gatter-Modul** | vier NAND, ereignisgesteuert, Ein-/Ausgänge an den Kern gekoppelt | Wahrheitstabelle, Schaltschwellen |
| **Zähler-Modul** | 4-Bit-Binärzähler mit Takt und Rücksetzen | Teiler, Zählfolge |
| **Dekadenzähler-Modul** | zehn Ausgänge nacheinander | Lauflicht |
| **Decoder-Modul** | Binärzahl → Siebensegment | Anzeige 0–9 |

**Umgebungsleiste »Licht, Wärme, Wasser«** (Volta 1) wird um **Gewichte** (10 g … 1 kg, auf den Kraftsensor ziehen) und einen **Magneten** (Abstand zum Hall-Sensor) ergänzt.

**»Blick hinein«:** Beim MW- und UKW-Modul zeigt er die Empfängerschaltung mit Erklärung (Verhaltensmodell, offen so benannt), beim Gatter-Modul ein NAND aus Transistoren mit den aktuellen Pegeln, beim Zähler die Flipflop-Kette mit Zuständen.

## 7. Werkzeugkasten

Wie Volta 2, ergänzt um:
- **Formelrechner:** Thomson-Formel f = 1 / (2π √(L C)), Güte und Bandbreite, Wirkungsgrad η = P_ab / P_zu, Verlustleistung, Brückenspannung, Verstärkung in dB, Tastverhältnis und Mittelwert, Frequenzteilung. Immer mit Rechenweg.
- **Zahlensysteme** (binär, dezimal, hexadezimal) in einer Zeile – wie die Zahlenleiste in Zuse 1.
- **Lexikon:** Modulation, Resonanz, Wirkungsgrad, Pegel, Flanke, Prellen, mit Verweis auf Zuse 1 für Logik.
- **Fehlersuche** erweitert: Thyristor ohne Haltestrom, MOSFET-Gate offen, Spule ohne Freilaufdiode, Gatter-Eingang offen (schwebender Pegel), Taster ohne Entprellung, Schwingkreis ohne Dämpfung.

## 8. Lernpfad

Aufbau wie Volta 1 und 2: Stufen mit Leitfrage, Experimente mit Forschungskreislauf (Frage → Vermutung → Aufbau → Beobachtung → Erklärung → »Mehr wissen« → »Jetzt in echt«), Freischaltung der Reihe nach, Projekte am Ende einer Stufe. Die Liste ist ein Entwurf zur Freigabe; Bezeichnungen sind Arbeitstitel.

### Stufe 0 – Auftakt: Welche Werkzeuge braucht ein Elektroniker? (3)
- 0.1 **Der Frequenzgang-Schreiber** – der Tiefpass aus Volta 2 als Kurve; die Grenzfrequenz wird markiert.
- 0.2 **Der Logikanalysator** – zwei 555-Takte als Folge von H und L.
- 0.3 **Zu schnell für das Auge** – eine schnelle Schwingung in der Lupe gedehnt; was der Dehnungsfaktor bedeutet.

### Stufe 1 – Schwingkreis und Resonanz: Warum schwingt etwas von selbst? (7)
- 1.1 **Kondensator trifft Spule** – die Ladung pendelt hin und her: eine gedämpfte Schwingung am Oszilloskop.
- 1.2 **Größer heißt tiefer** – mehr Kapazität oder Induktivität, tiefere Frequenz; Thomson-Formel im Formelrechner.
- 1.3 **Der Widerstand bremst** – Dämpfung: Die Schwingung klingt schneller ab.
- 1.4 **Im Takt anstoßen** – Resonanz im Reihenkreis: Bei einer Frequenz fließt am meisten Strom.
- 1.5 **Der Parallelkreis als Filter** – eine Frequenz kommt durch, die anderen nicht (Frequenzgang).
- 1.6 **Schmal oder breit** – die Güte bestimmt, wie scharf der Kreis trennt.
- 1.7 **Projekt: der Klangschalen-Oszillator** – Transistor und Schwingkreis erzeugen einen hörbaren Ton, der Drehkondensator stimmt ihn.

### Stufe 2 – Radio: Wie kommt Musik durch die Luft? (8)
- 2.1 **Träger und Ton** – Amplitudenmodulation mit dem Generator, am Oszilloskop gedehnt: Die Hüllkurve ist der Ton.
- 2.2 **Den Sender wählen** – der Schwingkreis mit Drehkondensator lässt einen der drei Sender durch.
- 2.3 **Die Diode holt den Ton heraus** – der Hüllkurvendetektor.
- 2.4 **Radio ohne Batterie** – Antenne, Schwingkreis, Diode und Kristallhörer: der Detektorempfänger.
- 2.5 **Lauter mit Verstärker** – eine Transistor- oder OP-Stufe treibt den Lautsprecher.
- 2.6 **Zwei Sender nah beieinander** – Trennschärfe und Güte.
- 2.7 **Das MW-Modul von innen** – Blick hinein: Wie ein fertiger Empfänger aufgebaut ist.
- 2.8 **Projekt: UKW-Radio** – das UKW-Modul mit Lautstärke und Senderwahl; »Mehr wissen«: Frequenzmodulation.

### Stufe 3 – Stromversorgung: Wie macht man aus wenig Spannung mehr – ohne Wärme? (7)
- 3.1 **Der Regler wird warm** – Verlustleistung am Längsregler aus Volta 2 messen.
- 3.2 **Schalten statt bremsen** – Pulsweite an einer Lampe: Der Mittelwert zählt.
- 3.3 **Die Spule wehrt sich** – Abschaltspannung und Freilaufdiode (»Nur in der App!« ohne Diode).
- 3.4 **Der Abwärtswandler** – weniger Spannung, kaum Wärme; Wirkungsgrad gegen den Längsregler.
- 3.5 **Eine Zelle, eine weiße LED** – der Aufwärtswandler mit 1,5 V.
- 3.6 **Minus aus dem Takt** – Ladungspumpe mit 555, Dioden und Kondensatoren.
- 3.7 **Projekt: Taschenlampe mit einer Zelle** – möglichst hell bei möglichst langer Laufzeit, Wirkungsgrad im Vergleich.

### Stufe 4 – Thyristor und Triac: Wie dimmt man Wechselspannung? (6)
- 4.1 **Einmal gezündet, bleibt er an** – Thyristor an der Batterie mit Taster.
- 4.2 **Wie bekommt man ihn wieder aus?** – Haltestrom und Löschen.
- 4.3 **An Wechselspannung** – der Thyristor löscht in jeder Halbwelle von selbst.
- 4.4 **Später zünden, weniger Licht** – Phasenanschnitt mit RC-Verzögerung an 9 V~; die angeschnittene Kurve am Oszilloskop.
- 4.5 **Beide Halbwellen** – Triac und Diac.
- 4.6 **Projekt: Lampendimmer für Kleinspannung** – mit dem Hinweis, dass Dimmer am Stromnetz nur Fachleute anschließen.

### Stufe 5 – Verstärkerklassen: Wie viel Leistung, wie viel Wärme? (6)
- 5.1 **Klasse A** – immer Strom, sauberer Klang, viel Wärme: Wirkungsgrad messen.
- 5.2 **Klasse B** – Gegentakt spart Strom, aber Übernahmeverzerrung (sehen und hören).
- 5.3 **Klasse AB** – zwei Dioden schaffen die Lücke weg.
- 5.4 **Wohin geht die Leistung?** – Leistung am Lautsprecher und Wärme in den Transistoren im Vergleich.
- 5.5 **Klasse D** – schalten mit Pulsweite, Tiefpass am Ausgang (langsamere Schaltfrequenz, offen erklärt).
- 5.6 **Projekt: der sparsamste Verstärker** – gleiche Lautstärke, kleinste Batterieleistung.

### Stufe 6 – Sensoren und Messverstärker: Wie misst man winzige Änderungen? (8)
- 6.1 **Die Messbrücke** – zwei Spannungsteiler, Nullabgleich mit P1.
- 6.2 **Wärme in der Brücke** – Heißleiter in der Brücke, kleine Spannung, große Bedeutung.
- 6.3 **Ein Gewicht verbiegt** – der Kraftsensor mit Gewichten aus der Umgebungsleiste.
- 6.4 **Den Unterschied verstärken** – der Differenzverstärker mit dem OP.
- 6.5 **Der Instrumentenverstärker** – drei OPs, ein Regler für die Verstärkung.
- 6.6 **Die Diode als Thermometer** – rund 2 mV weniger je Grad.
- 6.7 **Magnetfeld messen** – der Hall-Sensor und der Magnet.
- 6.8 **Projekt: Waage mit Leuchtbalken** – Kraftsensor, Messverstärker und eine Komparatorkette mit LEDs.

### Stufe 7 – Logik aus Spannung: Wie wird aus einer Spannung eine Eins? (8)
- 7.1 **Hoch, tief und dazwischen** – Logikpegel und Schaltschwelle; der verbotene Bereich.
- 7.2 **Ein Transistor sagt NEIN** – das NICHT aus einem Transistor.
- 7.3 **Das NAND aus Transistoren** – Wahrheitstabelle am Logikanalysator.
- 7.4 **Das Gatter-Modul** – vier NAND in einem Gehäuse; Blick hinein.
- 7.5 **Alles aus NAND** – UND und ODER aus NAND; Verweis auf die Rätselkisten in Zuse 1.
- 7.6 **Zwei Gatter merken sich etwas** – das Flipflop aus zwei NAND.
- 7.7 **Der Taster prellt** – ein Druck, viele Flanken; Entprellen mit RC-Glied und Schmitt-Trigger.
- 7.8 **Projekt: Quizknopf** – wer zuerst drückt, leuchtet; die anderen sind gesperrt.

### Stufe 8 – Takt und Zähler: Wie zählt eine Schaltung? (8)
- 8.1 **Der Takt** – der 555 als Taktgeber am Logikanalysator.
- 8.2 **Teilen durch zwei** – jede zweite Flanke kippt das Flipflop.
- 8.3 **Binär zählen** – das Zähler-Modul mit vier LEDs; Zahlensysteme im Werkzeugkasten.
- 8.4 **Das Lauflicht** – der Dekadenzähler.
- 8.5 **Ziffern anzeigen** – Decoder und Siebensegmentanzeige.
- 8.6 **Frequenz messen** – Flanken in einer Sekunde zählen: so arbeitet ein Frequenzzähler.
- 8.7 **Der elektronische Würfel** – schneller Takt, Taster hält an.
- 8.8 **Projekt: Reaktionszeit-Messer** – Lichtsignal, Zähler mit Anzeige, Stopp-Taster; Ausblick: »Wie rechnet eine Schaltung? – Zuse 1«.

**Summe:** 3 + 7 + 8 + 7 + 6 + 6 + 8 + 8 + 8 = **61 Experimente**, davon 8 Projekte (je eines am Ende der Stufen 1–8). Weitere Ideen für das **Freie Labor** und Aufgaben der Lehrkraft: Metalldetektor (Schwingkreis wird verstimmt), Lichtorgel mit Filtern, Lauflicht mit Geschwindigkeitsregler, Morse-Übungsgerät mit dem Sender.

## 9. Gestaltung, Farb- und Leitkonzept

Gestaltung wie Volta 1 und 2 (hell, ruhige Flächen, DC Learning Design Standard), Reihensymbol `docs/design/symbole/volta3.svg` (drei leuchtende Felder). Tick bleibt Begleitfigur, sachlicher im Ton; Texte duzen weiterhin. Die Ebene »Mehr wissen« trägt in Volta 3 Formeln und Hintergründe für die Oberstufe.

### 9.1 Grundsatz: dasselbe Farb- und Leitkonzept wie Volta 1 und 2

Volta 3 übernimmt das Farb- und Leitkonzept aus Volta 1 (0.9.1) mit den Erweiterungen aus Volta 2 (Konzept Volta 2, Kapitel 9) **unverändert**: sechs Themen im Bauteilfach mit Farbe und Schaltzeichen, Reihenfarben der Grundplatte (A rot … E blau), Felder im Text als kleine Kontaktfelder in ihrer Farbe, automatische Leitungsfarben (Plus rot, Masse/GND schwarz, negative Versorgung blau, sonst Lieblingsfarbe), Rahmen, Schaltzeichen und »?« je Pultbauteil, eigene Trägerplattenfarbe je Modul, Kanalfarben der Messgeräte (1 Blau, 2 Orange) und des Oszilloskops (K1 gelb, K2 türkis), Stufenanpassung von Steckbrett und Pult. Grundregel: **Farbe nie allein**.

### 9.2 Einordnung der neuen Bauteile

| Thema (Farbe) | neu in Volta 3 |
|---|---|
| Licht (gelb) | Siebensegmentanzeige |
| Schalten (blau) | – (Taster und Schalter aus Volta 1 genügen) |
| Widerstände und Sensoren (grün) | Kraftsensor, Hall-Sensor |
| Dioden und Transistoren (violett) | Schottky- und Germaniumdiode, MOSFET, Thyristor, Triac, Diac |
| Kondensatoren und Spulen (türkis) | Drehkondensator, Spulen 100 µH … 10 mH |
| Bewegung und Ton (orange) | Kristallhörer, Antenne; das Thema heißt in Volta 3 »Ton und Empfang« |

Das Thema »Dioden und Transistoren« wird in Volta 3 mit rund 15 Bauteilen groß. Vorgeschlagen ist, es im Fach in zwei Gruppen zu zeigen (»Dioden« und »Transistoren und Thyristoren«), mit derselben Farbe und einer Zwischenüberschrift – statt eines siebten Themas, damit die Farben der Reihe gleich bleiben (offene Entscheidung, Kapitel 14).

**Module:** MW-Modul und UKW-Modul (Trägerplatte dunkelblau, Bildzeichen Antenne mit Wellen, »MW« bzw. »UKW« aufgedruckt), Gatter-Modul (grau, Bildzeichen NAND-Symbol nach DIN EN 60617 mit »&«), Zähler-, Dekadenzähler- und Decoder-Modul (dunkelgrün, Bildzeichen Ziffernfolge 0-1-2-3 bzw. Siebensegment-Ziffer).

### 9.3 Neue Geräte im selben Konzept

- **Logikanalysator:** Die acht Kanäle tragen die Farben des Widerstands-Farbcodes in der Reihenfolge 0 bis 7 (schwarz, braun, rot, orange, gelb, grün, blau, violett) – dieselben Farben, die die Kinder seit Volta 1 aus dem Farbcode kennen – und immer zusätzlich die Kanalnummer D0 … D7 an Fähnchen, Spur und Beschriftung.
- **Logikpegel an den Feldern** (Stufen 7–8): Neben dem grünen Spannungsbalken aus Volta 1 kann ein Feld seinen Pegel zeigen: ein Kästchen mit **H** (gefüllt) bzw. **L** (leer), im verbotenen Bereich ein **?** mit gestricheltem Rand.
- **Frequenzgang-Schreiber:** Kurve in der Farbe des Messkanals (K1 gelb), Resonanz- und Grenzfrequenz als senkrechte Marke mit Zahl.
- **Zeitgedehnte Darstellung:** gestrichelter Rahmen, Lupenzeichen und Faktor (»×1000 gedehnt«) – nie nur eine andere Farbe.
- **Senderanlage im Pult:** Rahmen, Schaltzeichen Antenne, »?« wie jedes Pultbauteil; jeder Sender hat Nummer und Frequenz als Text.
- **Leitungsfarben:** Die Leitung zur Antenne bekommt keine eigene Farbe (Lieblingsfarbe); es gilt dieselbe Automatik wie in Volta 2.

### 9.4 Bedienpult III und Stufenanpassung

- Das Pult wächst mit den Stufen: Stufe 0 Batterie 1, Messgeräte, Generator, Oszilloskop, Frequenzgang-Schreiber, Logikanalysator; ab Stufe 2 Senderanlage; ab Stufe 4 Wechselquelle; Batterie 2 ab Stufe 6 (Messverstärker mit symmetrischer Versorgung); Drehregler, Schalter und Lautsprecher, sobald ein Experiment sie nutzt; dazu immer, was Vorlage oder Aufbau verwenden.
- Steckbrett: 5 × 10 Felder zu Beginn, wächst mit der Vorlage; im freien Labor 12 Spalten. Radio- und Zählerprojekte nutzen die Modulleiste, damit die Grundplatte übersichtlich bleibt.
- Schmale Fenster: Pultleiste zweizeilig; Frequenzgang und Logikanalysator öffnen sich auf kleinen Tablets als Blatt über der Grundplatte (wie das Oszilloskop in Volta 2).

## 10. Technik

| Bereich | Umsetzung | Stand |
|---|---|---|
| Edition | `VOLTA_EDITION=volta3`, eigene App-ID, Inhaltspaket `content/volta3`, Katalog mit Bandfreigabe (`ab_band`) | wie Volta 2 |
| Wechselstromanalyse | Linearisierung am Arbeitspunkt, komplexe Lösung je Frequenz mit derselben Knotenmatrix; Kern bleibt Qt-frei | neu |
| Radiomodell | Senderanlage als Quellen mit Frequenz; Durchlass aus der Wechselstromanalyse; Hüllkurve als Quelle für Detektor und Verstärker im Zeitbereich | neu, Machbarkeitstest in Schritt 1 |
| Schaltwandler, Klasse D | Zeitbereich mit 1–5 kHz Schaltfrequenz, Ereignisortung an Schaltflanken (aus dem 555 von Volta 2) | auf Grundlage Volta 2 |
| Thyristor, Triac | Zünd- und Haltelogik als Zustand im Bauteil, für Undo im Zustandsschnappschuss | neu |
| Digitale Module | ereignisgesteuerte Logik (Wiederverwendung aus `logic/`), Kopplung an den Kern über Schaltschwelle und Ausgangsquelle | gemeinsam mit MCU Explorer |
| Logikanalysator | Ringpuffer je Kanal im Kern, Schwellen im Modell, Darstellung in QML | Oszilloskop aus Volta 2 als Grundlage |
| Senderanlage | eigene Tonaufnahmen als Qt-Ressource je Sprache | neu |

Alle Regeln von Volta 1 gelten unverändert: Kern ohne Qt, jede Aufbauänderung ein Befehl, Physik ehrlich (Referenztests < 2 %), kein Netz, keine Datenerhebung, Bedienelemente ≥ 48 dp, Farbe nie allein, DE/FR/EN/LU.

## 11. Datenschutz und Kindersicherheit

- Wie Volta 1 und 2: kein Konto, keine Analyse, keine Werbung, keine Käufe in der App, kein Netz.
- Kein Zugriff auf Mikrofon, Kamera oder Funkhardware des Geräts.
- **Keine Netzspannung:** Thyristor, Triac und Dimmer nur an der Kleinspannungs-Wechselquelle. Jedes Experiment der Stufe 4 trägt den Hinweis, dass Geräte am Stromnetz nur Fachleute anschließen; »Jetzt in echt« schlägt dort nur Versuche mit Batterie und Lämpchen vor.
- **Kein Senden:** »Jetzt in echt« beschreibt nur Empfangen (Detektorempfänger mit Drahtantenne) und Schaltungen ohne Sender. In vielen Ländern sind Mittelwellensender inzwischen weitgehend abgeschaltet; die Texte sagen das offen, statt Empfang zu versprechen.
- »Nur in der App!« für zerstörende Versuche (Spule ohne Freilaufdiode, MOSFET ohne Gatewiderstand überlastet, Thyristor überlastet).
- Lautstärkebegrenzer wie Volta 2, auch für den Kristallhörer.

## 12. Umsetzungsplan

| Schritt | Inhalt | PT | Ergebnis |
|---|---|---|---|
| 1 Machbarkeitstest | Wechselstromanalyse an Referenzschaltungen, Radiomodell (Abstimmen, Hüllkurve, hörbar), Wandler mit 1–5 kHz auf dem Tablet | 10 | Grundsatzentscheidungen (Kapitel 4) bestätigt |
| 2 Band-Infrastruktur | Edition volta3, Inhaltspaket, Bandfreigabe, Pultleiste III, Senderanlage, App-Symbol | 12 | dritte App aus derselben Codebasis |
| 3 Bauteilmodelle | Drehkondensator, Spulen, Antenne, Kristallhörer, Schottky/Ge, MOSFET, Thyristor, Triac/Diac, Kraft- und Hall-Sensor, Siebensegment; MW/UKW-Modul; Gatter-, Zähler-, Decoder-Modul mit Kopplung | 35 | Referenztests (< 2 %) |
| 4 Geräte | Frequenzgang-Schreiber, Logikanalysator, zeitgedehnte Darstellung, Frequenzzähler, Wobbel- und AM-Betrieb des Generators, Umgebungsleiste (Gewichte, Magnet) | 30 | Bedienpult III |
| 5 Inhalte | 61 Experimente mit Musteraufbau und Prüfbedingungen, Tonaufnahmen der Sender, Lexikon, Formelrechner, Hilfe; FR/EN/LU | 55 | inhaltlich vollständiger Band |
| 6 Plattformen | Android-APK, Windows, Mac; Prüfung auf allen Tablets; Anleitung und Flyer | 15 | Testversion Volta 3 (0.9) |
| | **Summe** | **157** | (Volta 2: 145 PT) |

Voraussetzung ist Volta 2 als Testversion (Oszilloskop, Generator, Wechselquelle, OP, 555, hörbare Simulation). Die Kopplung der digitalen Module (Schritt 3) kann zusammen mit dem MCU Explorer entstehen und senkt dort den Aufwand.

## 13. Risiken und Maßnahmen

| Risiko | Maßnahme |
|---|---|
| Hochfrequenz lässt sich auf dem Tablet nicht in Echtzeit rechnen | Abstimmen über Wechselstromanalyse, Hüllkurve im Zeitbereich, Träger nur gedehnt gezeigt – offen gekennzeichnet (Kapitel 4); Machbarkeitstest vor dem Start |
| Gedehnte oder langsamere Darstellung wirkt wie »geschummelt« | Lupenzeichen, Faktor und gestrichelter Rahmen an jeder gedehnten Anzeige; »So rechnet Volta« in den Einstellungen; »Mehr wissen« nennt die echten Frequenzen |
| Thyristor- und Dimmerversuche verleiten zu Netzspannung | nur Kleinspannungsquelle im Pult, Warnhinweise in Stufe 4, »Jetzt in echt« ohne Netzbezug (Regel 6) |
| Überschneidung mit Zuse 1 in der Digitaltechnik | Volta 3 elektrisch (Pegel, Schwellen, Prellen, Zähler), Zuse logisch (Rätselkisten, Rechner); Verweise in beide Richtungen |
| Mittelwelle gibt es kaum noch | Senderanlage im Pult statt echtem Empfang; »Jetzt in echt« ehrlich zu den heutigen Empfangsmöglichkeiten |
| Bauteilfach wird zu voll | Gruppen innerhalb eines Themas, Stufenanpassung zeigt nur die Bauteile der Stufe, Modulfach für Radio und Zähler |
| Inhaltlicher Umfang (61 Experimente) | Musteraufbau und Prüfbedingungen mit den Werkzeugen aus Volta 1 und 2; Projekte nutzen Teilschaltungen aus früheren Stufen |

## 14. Offene Entscheidungen

| Entscheidung | Spätestens |
|---|---|
| Freigabe der Stufen- und Experimentliste (Kapitel 8) | vor Schritt 2 |
| Gruppen im Thema »Dioden und Transistoren« statt eines siebten Themas (9.2) | Schritt 2 |
| Kennfarbe von Volta 3 (App-Symbol entschieden: Reihensymbol `volta3.svg`) | Schritt 2 |
| Name der Reihe (Markenfrage Volta) | vor Schritt 6 |
| Verkauf: eigene App oder Freischaltung in einer gemeinsamen App (abhängig von Verlagsantworten) | vor Schritt 6 |
| Lehrplanzuordnung DE/LU/Ostbelgien | Schulpaket |

## 15. Fazit

Volta 3 schließt die Elektronik-Reihe ab: vom Schwingkreis über Radio, Stromversorgung und Messtechnik bis zum Zähler mit Anzeige, der direkt in die Zuse-Linie führt. Inhaltlich ist der Band mit 61 Experimenten in neun Stufen umrissen; gestalterisch übernimmt er das Farb- und Leitkonzept von Volta 1 und 2 vollständig. Technisch ist er anspruchsvoller als Volta 2, weil er Vorgänge zeigt, die schneller sind, als ein Tablet Schritt für Schritt rechnen kann – dafür sind ehrliche, gekennzeichnete Wege entschieden (Kapitel 4), die ein Machbarkeitstest vor dem Start bestätigt. Der Aufwand liegt bei rund 157 Personentagen nach Abschluss von Volta 2.
