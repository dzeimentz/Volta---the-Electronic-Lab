# Volta – Die Schaltungsbibliothek

_Konzept einer eigenen App: erprobte Schaltungen zum Suchen, Anschauen, Aufbauen und Testen – als Abschluss und Nachschlagewerk der Volta-Reihe_

Markdown-Fassung des Konzepts und Quelle des PDFs `docs/Konzept_Schaltungsbibliothek_v0.1.pdf` (erzeugt mit `tools/konzept_pdf.py`).

| | |
|---|---|
| Dokument | Konzept |
| Version | 0.1 |
| Status | ENTWURF |
| Autor | Daniel Zeimentz · DC Software Engineering |
| Datum | 04.11.2026 |
| Bezug | Konzept Volta v3.0; Konzept Volta 2 v0.4; Konzept Volta 3 v0.4 (Stufenliste freigegeben 04.11.2026); Konzept Zuse v0.3; `docs/partner/rechte-und-lizenzen.md` |
| Zielplattformen | Android (Tablet), Windows und Linux x86-64, iPadOS, macOS – aus derselben Codebasis wie Volta 1–3 |
| Name | Arbeitstitel »Schaltungsbibliothek«; der Name ist offen (Kapitel 13) und darf nicht an fremde Buchreihen erinnern |

## 1. Zusammenfassung

Mit Volta 3 ist die Elektronik-Reihe abgeschlossen und der Übergang zu Zuse gemacht. Wer die drei Bände durchgearbeitet hat, kennt Grundplatte, Messgeräte und alle Bauteile – und möchte danach **eigene Schaltungen finden, ausprobieren und nachbauen**. Genau dafür ist die Schaltungsbibliothek gedacht: eine eigene App mit einer großen Sammlung **erprobter Schaltungen**, die man wie in einem Schaltungsbuch durchblättert und über ein Register sucht – nach **Name, Thema und Anwendungsbereich (Genre)**.

Anders als ein Buch kann die Bibliothek jede Schaltung **sofort lebendig machen**: Man baut sie auf der bekannten Grundplatte **selbst auf** (mit Prüfung am Ende), lässt sich **Schritt für Schritt helfen** oder lässt sie **fertig aufbauen** und misst gleich daran. Weil der Rechenkern von Volta jede Schaltung wirklich ausrechnet, ist »erprobt« hier wörtlich zu nehmen: Jede Schaltung der Bibliothek läuft in der Simulation, mit Prüfbedingungen und Referenzwerten.

**Anregung** ist das Prinzip großer Schaltungssammlungen in Buchform, wie es die »300 Schaltungen«-Reihe von Elektor bekannt gemacht hat: viele kurze, erprobte Schaltungen zum Anschauen und Nachbauen, über ein Register erschlossen. Übernommen wird **nur dieses Prinzip** und – nach Auswahl – die **Schaltungsidee** einzelner Schaltungen. Texte, Zeichnungen, Platinenlayouts, Titel, Nummerierung, Gliederung und Aufmachung fremder Werke werden nicht übernommen; jede Schaltung wird neu entworfen, neu berechnet und neu beschrieben (Kapitel 4). Eine Verwechslung mit Elektor oder einer Elektor-Reihe muss ausgeschlossen sein.

**Empfehlung:** Konzept als Richtung festhalten; Beginn nach der Testversion von Volta 3, weil die Bibliothek dessen Bauteile (MOSFET, Thyristor, Logik, Radio) braucht. Erste Ausgabe mit **rund 100 Schaltungen**, danach Erweiterung über Inhaltsupdates.

## 2. Einordnung in die Reihe

| Angebot | Art | Alter | Rolle |
|---|---|---|---|
| Volta 1–3 | geführter Lernpfad in Stufen | 8–16 | Grundlagen Schritt für Schritt |
| Zuse 1 | geführter Lernpfad | 10–14 | Computertechnik |
| MCU Explorer | geführter Lernpfad und freies Programmieren | ab 14 | Mikrocontroller |
| **Schaltungsbibliothek** | **Nachschlagewerk zum Stöbern, Suchen und Ausprobieren** | **ab 12; ebenso Lehrkräfte, Hobby, Ausbildung** | **Anwenden, Vertiefen, Nachbauen** |

- **Kein Lernpfad, kein Stufenzwang:** Alle Schaltungen sind sofort zugänglich. Eine Kennzeichnung »baut auf Volta 1 / 2 / 3 auf« und eine Schwierigkeit (1–3 Sterne) helfen bei der Auswahl.
- **Verknüpfung mit den Bänden:** Jede Schaltung nennt, in welchem Band und welcher Stufe die nötigen Grundlagen stehen (»Was ist ein Schwingkreis? → Volta 3, Stufe 1«). Die Bibliothek setzt keine gekaufte Volta-App voraus; Erklärungen stehen kurz im Lexikon.
- **Schule:** Lehrkräfte finden zu einem Lehrplanthema passende Schaltungen und geben sie wie in Volta als Aufgabe (QR-Code) weiter.

## 3. Ziele und Nicht-Ziele

**Ziele**
- **Finden wie im Buch-Register:** Suche nach Name, Thema, Genre, Bauteil, Schwierigkeit und Lehrplanbezug; Blättern nach Kapiteln.
- **Jede Schaltung lebt:** aufbauen, messen, hören, verändern – mit allen Messgeräten aus Volta 1–3.
- **Erprobt heißt geprüft:** Jede Schaltung hat einen Musteraufbau, Prüfbedingungen und Referenzwerte (< 2 % Abweichung, Regel 3).
- **Vom Bildschirm in echt:** Teileliste und Aufbaukarte als PDF; später ein Streifenraster-Aufbauvorschlag zum echten Nachbauen.
- **Lehrplannah:** Auswahl nach dem, was in Sek. I und am Übergang zur Sek. II und Ausbildung vorkommt.

**Nicht-Ziele**
- **Keine Kopie fremder Werke:** keine Texte, Abbildungen, Platinenlayouts, Titel, Nummerierung oder Kapitelgliederung aus Büchern oder Zeitschriften; kein »300« im Namen, keine an eine Buchreihe angelehnte Aufmachung (Kapitel 4).
- **Keine Netz-, Hochspannungs- und Kfz-Schaltungen** (Regel 6); keine Sender (Funkrecht).
- Kein Schaltungs-Editor für beliebige eigene Bauteile, keine allgemeine Platinen-CAD-Software.
- Kein Austausch von Schaltungen über das Netz, kein Konto, keine Käufe in der App (Regel 1).

## 4. Quellen und Abgrenzung (Rechte)

Die Bibliothek lebt davon, dass ihre Schaltungen **Eigenentwicklungen** sind. Fremde Sammlungen dienen nur als Anregung, welche Schaltungsideen sich bewährt haben und lehrreich sind.

| Quelle (Privatbestand des Inhabers) | Verwendung |
|---|---|
| Elektor »300 Schaltungen« und Folgebände | Anregung für Auswahl und Themenbreite; nur Schaltungsprinzipien |
| ELRAD Schaltungssammlung und Laborblätter (`I:\ELRAD\pdf`) | Schaltungsprinzipien; Laborblätter mit Messwerten zusätzlich als Vergleich für Referenztests |
| 555-Anwendungssammlung (Zeitschriftenartikel 1973–1982) | Timer-Schaltungsideen |
| Datenblätter und Applikationsschriften der Hersteller | Grundschaltungen, Grenzwerte |
| Lehrbuchwissen und eigene Entwürfe | Grundschaltungen (Spannungsteiler, Emitterschaltung, astabiler Multivibrator …) |

**Arbeitsweise je Schaltung:**
1. **Auswahl** nach Lehrwert, Lehrplanbezug, Sicherheit und Simulierbarkeit.
2. **Prinzip** festhalten (was die Schaltung tut und warum), ohne fremden Text.
3. **Neuentwurf** mit Bauteilen aus dem Volta-Katalog: eigene Bauteilwerte, eigene Versorgung (Kleinspannung), eigene Zeichnung im Volta-Schaltplan mit Normsymbolen.
4. **Erprobung** in der Simulation: Musteraufbau, Prüfbedingungen, Referenzwerte; Abgleich mit Rechnung (Formelrechner).
5. **Eigener Text** im Volta-Stil: kurze Sätze, Du-Form, »Was passiert?«, »Warum?«, »Probier mal«.
6. **Eigener Titel** und Einordnung in die eigene Gliederung (Kapitel 6).

**Grundsatz:** Eine Schaltungsidee als solche ist allgemeines Fachwissen; die konkrete Darstellung (Text, Zeichnung, Layout, Auswahl und Anordnung einer Sammlung) ist geschützt. Deshalb wird weder die Auswahl noch die Reihenfolge einer fremden Sammlung nachgebildet, und keine Schaltung trägt einen Hinweis wie »nach Elektor Nr. …«. Die Namen Elektor und ELRAD erscheinen nicht in App, Store-Texten oder Inhalten. Vor der Veröffentlichung prüft eine Fachperson für Urheber- und Markenrecht Vorgehen und Namen; der Punkt wird in `docs/partner/rechte-und-lizenzen.md` aufgenommen.

## 5. Die Schaltung als Steckbrief

Jede Schaltung ist ein Eintrag im Inhaltspaket mit festem Aufbau:

| Teil | Inhalt |
|---|---|
| Titel und Kurzsatz | eigener, sprechender Name (»Dämmerungsschalter mit Rückkopplung«) und ein Satz, was sie tut |
| Einordnung | Thema, Genre, Schwierigkeit (1–3), Grundlage in Volta 1/2/3, Lehrplanbezug |
| Schaltplan | Volta-Schaltplan mit Normsymbolen und Standardformen (Brücke als Raute, Emitterschaltung …) |
| So funktioniert's | kurze Erklärung in drei bis sechs Sätzen, Verweis ins Lexikon |
| Teileliste | Bauteile mit Werten, wie die Teileliste im Werkzeugkasten |
| Messpunkte | wo man messen soll und was man sieht (Spannung, Oszilloskop, Frequenzgang …) mit Referenzwerten |
| Probier mal | zwei bis drei Abwandlungen (»Mach R2 größer – was passiert mit der Frequenz?«) |
| Jetzt in echt | Hinweise zum Nachbau, Aufbaukarte als PDF; Sicherheitshinweise |

## 6. Gliederung, Register und Suche

**Themen** (fachlich, entspricht den Kapiteln):
Stromversorgung · Verstärker · Oszillatoren und Generatoren · Filter und Klang · Timer und Zeitschaltungen · Sensoren und Messtechnik · Licht und Anzeige · Schalten und Steuern · Logik und Zähler · Empfang (Radio)

**Genres** (Anwendungsbereich, wofür man die Schaltung braucht):
Haus und Garten · Musik und Klang · Spiel und Spaß · Messen und Prüfen · Alarm und Schutz · Natur und Wetter · Fahrrad und Modellbau (nur Batteriebetrieb) · Schule und Labor

Eine Schaltung hat genau ein Thema und ein bis drei Genres. Die Gliederung ist eine Eigenentwicklung und folgt keiner fremden Sammlung.

**Suche und Register:**
- **Volltextsuche** über Titel, Kurzsatz und Stichwörter in der eingestellten Sprache (DE/FR/EN/LU), tolerant gegenüber Tippfehlern und Umlauten.
- **Filter** (kombinierbar): Thema, Genre, Schwierigkeit, Grundlage (Volta 1/2/3), **enthaltenes Bauteil** (»alle Schaltungen mit 555«, »mit LDR«), Messgerät (»zum Oszilloskop«), Lehrplanthema.
- **Register** wie am Ende eines Buches: alphabetisch nach Stichwort, nach Bauteil und nach Thema; jeder Eintrag springt zur Schaltung.
- **Blättern:** Kapitelansicht mit Kacheln (Titel, Schaltplan-Vorschau, Symbole für Schwierigkeit und Messgeräte; Farbe nie allein, Regel 7).
- **Merkliste** (lokal im Profil) und »Zuletzt angesehen«.

## 7. Drei Wege zur Schaltung

| Weg | Ablauf | Prüfung |
|---|---|---|
| **Selbst aufbauen** | leere Grundplatte, Teileliste und Schaltplan sichtbar | »Fertig« prüft den Aufbau gegen die Prüfbedingungen (wie Volta), Fehlersuche hilft |
| **Mit Aufbauhilfe** | Tick zeigt Schritt für Schritt, wohin jedes Teil gehört (Aufbauhilfe aus Volta) | jeder Schritt wird bestätigt |
| **Aufbauen lassen** | ein Tippen setzt den Musteraufbau ein (»Aufbau übernehmen« aus Volta 2), rückgängig machbar | sofort messen und verändern |

In allen drei Wegen sind alle Geräte verfügbar, die die Schaltung braucht (Pult nach Bedarf mit Gerätefach). Verändern ist ausdrücklich erwünscht: Bauteilwerte tauschen, Messstellen setzen, »Probier mal«-Vorschläge nachvollziehen. »Zurück zum Original« stellt den Musteraufbau wieder her.

## 8. Bauteile, Geräte und Technik

- **Bauteile und Module:** der vollständige Katalog von Volta 1–3 (`ab_band` 1–3 freigegeben). Neue Bauteile entstehen nur, wenn eine ausgewählte Schaltung sie zwingend braucht, und dann im gemeinsamen Katalog.
- **Geräte:** Dreifach- und Doppelmessgerät, Oszilloskop, Funktionsgenerator, Kennlinienschreiber, Frequenzgang-Schreiber, Logikanalysator, hörbare Simulation.
- **Grundplatte:** 10 bzw. 12 Spalten wie Volta 2/3; große Schaltungen dürfen eine breitere Grundplatte nutzen (Grenze im Machbarkeitstest festlegen).

| Bereich | Umsetzung | Stand |
|---|---|---|
| Edition | `VOLTA_EDITION=bibliothek` (Arbeitsname), eigene App-ID, Inhaltspaket `content/bibliothek` | wie Volta 2/3 |
| Inhaltsformat | neuer Typ »Schaltung« neben »Experiment«: Steckbrief, Thema, Genres, Stichwörter, Musteraufbau, Prüfbedingungen, Messpunkte; JSON-Schema in `content/schema/` | neu |
| Suche | Index beim Laden des Inhaltspakets, lokal, ohne Netz; Umlaut- und Tippfehlertoleranz | neu |
| Bibliotheksoberfläche | Kapitel, Kacheln, Filterleiste, Register, Steckbriefansicht | neu (QML) |
| Aufbauen lassen / Aufbauhilfe | aus Volta 1/2 (»Aufbau übernehmen«, Guide) | vorhanden |
| Prüfung | Prüfbedingungen und Musterlöser aus Volta | vorhanden |
| Schaltplan | Schaltplan mit Standardformen aus Volta 2 | vorhanden, je nach Schaltung erweitern |
| Aufbaukarte, Teileliste (PDF) | aus Volta 1 | vorhanden |
| Streifenraster-Vorschlag | Umsetzung des Aufbaus auf eine Streifenrasterplatine als Zeichnung (später) | Ausbaustufe |
| Referenztests | jede Schaltung als Testfall in `check_all.py` (Musteraufbau rechnen, Messpunkte gegen Referenz < 2 %) | Erweiterung des ContentSmokeTest |

Alle Regeln von Volta gelten unverändert: Kern ohne Qt, jede Aufbauänderung ein Befehl, Physik ehrlich, kein Netz, keine Datenerhebung, Bedienelemente ≥ 48 dp, Kontaktfelder ≥ 56 dp, Farbe nie allein, DE/FR/EN/LU.

## 9. Auswahl der ersten Ausgabe

Rund **100 Schaltungen**, gleichmäßig über die zehn Themen verteilt (je 8–12). Auswahlkriterien, gewichtet:

| Kriterium | Frage |
|---|---|
| Lehrwert | Zeigt die Schaltung ein Prinzip, das man danach verstanden hat? |
| Lehrplanbezug | Kommt das Thema in Sek. I / Übergang Sek. II / Ausbildung vor? |
| Erlebnis | Sieht, hört oder misst man etwas Überraschendes? |
| Sicherheit | nur Kleinspannung; »Jetzt in echt« ohne Gefahr (Regel 6) |
| Simulierbarkeit | mit dem Volta-Kern ehrlich rechenbar, ggf. mit den gekennzeichneten Wegen aus Volta 3 (Kapitel 4 dort) |
| Nachbaubarkeit | Bauteile im Handel üblich, Aufbau auf Steckbrett oder Streifenraster möglich |

Projekte und Experimente aus Volta 1–3 werden nicht doppelt aufgenommen; die Bibliothek verweist auf sie und bietet Abwandlungen und Weiterführungen an.

## 10. Datenschutz und Kindersicherheit

- Wie Volta: kein Konto, keine Analyse, keine Werbung, keine Käufe in der App, kein Netz; Merkliste und Fortschritt nur lokal im Profil.
- Keine Netzspannung, keine Hochspannung, keine Kfz-Bordnetze, keine Sender; Grenzwerte im Katalog.
- »Nur in der App!« für Abwandlungen, die Bauteile überlasten; »Jetzt in echt« schlägt nie zerstörende Versuche vor.
- Erweiterungen der Bibliothek kommen als App-Update, nicht als Kauf in der App.

## 11. Umsetzungsplan

| Schritt | Inhalt | PT | Ergebnis |
|---|---|---|---|
| 1 Quellenauswertung und Auswahl | Sammlungen sichten, Kandidatenliste (~150), Auswahl der 100 nach Kapitel 9, Gliederung festlegen | 10 | freigegebene Schaltungsliste |
| 2 Infrastruktur | Edition, Inhaltstyp »Schaltung« mit Schema und Validierung, Suchindex, Referenztests in `check_all.py` | 12 | lauffähige Bibliothek mit 5 Musterschaltungen |
| 3 Oberfläche | Kapitel, Kacheln, Filter, Register, Steckbrief, drei Wege, Merkliste, Aufgaben per QR-Code | 15 | bedienbare Bibliothek |
| 4 Inhalte | 100 Schaltungen: Neuentwurf, Musteraufbau, Prüfbedingungen, Messpunkte, Texte DE (≈ 0,5 PT je Schaltung) | 50 | inhaltlich vollständige erste Ausgabe |
| 5 Übersetzung | FR/EN/LU | 10 | vier Sprachen |
| 6 Plattformen | Android, Windows, Mac, iPad; Anleitung, Flyer, Testpaket | 10 | Testversion (0.9) |
| | **Summe** | **107** | |
| später | Streifenraster-Vorschläge, weitere Schaltungen als Updates | offen | |

Voraussetzung: Volta 3 als Testversion (Bauteile und Geräte). Schritte 1 und 2 können bereits parallel zu Volta 3 beginnen.

## 12. Risiken und Maßnahmen

| Risiko | Maßnahme |
|---|---|
| Rechte: Nähe zu einer fremden Sammlung | nur Prinzipien, eigene Auswahl und Gliederung, eigene Werte, Zeichnungen und Texte; keine Quellennummern; Rechtsprüfung vor Veröffentlichung (Kapitel 4) |
| Verwechslung mit Elektor | kein »300« und keine Elektor-Anmutung in Name und Gestaltung; Volta-Gestaltung durchgehend |
| Schaltung nicht ehrlich simulierbar | nicht aufnehmen oder mit den gekennzeichneten Wegen aus Volta 3; Referenztest Pflicht |
| Umfang der Inhalte | Steckbrief mit festem Aufbau, Werkzeuge zum Musteraufbau aus Volta; Erweiterung in Ausgaben statt alles auf einmal |
| Grundplatte zu klein für größere Schaltungen | Grenze im Schritt 2 festlegen; große Schaltungen in Teilschaltungen oder mit Modulen |
| Bibliothek ersetzt die Bände | Bibliothek setzt Grundlagen voraus und verweist auf Volta 1–3; kein Lernpfad |

## 13. Offene Entscheidungen

| Entscheidung | Spätestens |
|---|---|
| Name der App (ohne »300«, ohne Anklang an fremde Reihen; Markenfrage Volta) | vor Schritt 6 |
| Umfang der ersten Ausgabe (100?) und Takt der Erweiterungen | Schritt 1 |
| Altersangabe (ab 12? ab 13?) und Ansprache (Du-Form wie Volta oder neutral für Hobby und Ausbildung) | Schritt 1 |
| Gliederung: Themen und Genres wie in Kapitel 6 | Schritt 1 |
| Ablageort der Elektor-Bände und der ELRAD-Schaltungssammlung für die Auswertung | Schritt 1 |
| Verkauf: eigene App oder gemeinsame App mit den Bänden (abhängig von Verlagsantworten) | vor Schritt 6 |
| Streifenraster-Vorschläge in der ersten Ausgabe oder später | Schritt 3 |
| Rechtsprüfung (Urheber- und Markenrecht) | vor Veröffentlichung |

## 14. Fazit

Die Schaltungsbibliothek setzt den Schlusspunkt hinter die Volta-Reihe: Nach dem geführten Weg durch drei Bände öffnet sie eine große, durchsuchbare Sammlung erprobter Schaltungen, die man nicht nur ansieht, sondern auf der vertrauten Grundplatte aufbaut, misst, hört und verändert. Fast die gesamte Technik ist mit Volta 1–3 schon vorhanden; der Aufwand liegt vor allem in sorgfältig ausgewählten, neu entworfenen und geprüften Inhalten. Mit rund 107 Personentagen für die erste Ausgabe ist sie das kleinste der großen Vorhaben – und das, mit dem die Reihe über die Schulzeit hinaus nützlich bleibt.
