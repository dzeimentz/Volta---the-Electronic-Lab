# ADR-0003: Kontaktfelder als Knoten statt Steckbrett

- Status: angenommen
- Datum: 2026-10-04

## Entscheidung
Die Grundplatte besteht aus Kontaktfeldern mit je fuenf Steckplaetzen (urspruenglich vier, siehe Nachtrag); jedes Feld ist genau ein Knoten.
Bezeichnung als Koordinaten A1–E12 (urspruenglich A1–E8). Bauteile mit zwei festen Spannweiten. Pultleiste und Modulleiste getrennt.

## Begruendung
- Uebersichtlich fuer Achtjaehrige; Anleitungen eindeutig (»Widerstand von B3 nach B4«).
- Feld = Knoten macht den Schaltplan unmittelbar lesbar.
- Prinzip der Kosmos-X-Reihe, aber eigene Bezeichnung und Gestaltung.

## Verworfen
Eng gerastertes Steckbrett (Breadboard): zu fummelig, Knotenstruktur fuer Kinder schwer erkennbar.

## Nachtrag (2026-10-05)
Auf Wunsch des Auftraggebers wurde die Grundplatte von Volta 1 auf **12 Spalten** (A1–E12, 60 Felder) und jedes
Kontaktfeld (Grundplatte und Pultleiste) auf **fuenf Steckplaetze** erweitert: mehr Platz fuer groessere Aufbauten
und Knoten mit vielen Abzweigungen. Die Modulleiste bleibt bei einem Steckplatz je Feld. Bestehende Bezeichner
A1–E8 bleiben gueltig.
Ergaenzt: Die Breite ist je Aufbau einstellbar (8, 10, 12 Spalten, Befehl), und Beine wie Leitungsenden stecken
sichtbar in berechneten Loechern (docs/domain-model.md, Bauteile auf der Grundplatte). Leitungen bleiben unbegrenzt.
