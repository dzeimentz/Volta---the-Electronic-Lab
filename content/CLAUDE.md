# content/ – Regeln fuer Inhaltspakete

Lies vor Aenderungen `docs/content-format.md`.

## Harte Regeln
- **Eigenstaendigkeit:** Keine Texte, Versuchsaufbauten, Kapitelueberschriften, Abbildungen oder Figuren aus
  Kosmos-Handbuechern oder anderen fremden Anleitungen. Auch nicht umformuliert. Der Name »Kosmos« erscheint nicht.
- **Sicherheit:** `zerstoerend: true` setzen, sobald im Ablauf ein Bauteil zerstoert werden kann.
  `jetzt_in_echt_key` beschreibt dann ausschliesslich den sicheren Nachbau. Keine Netzspannung, keine Akkus.
- **Physik:** Jede Vorlage muss in der Simulation ihre eigene Erfolgsbedingung erfuellen (ContentSmokeTest).
- **Sprache:** Kindertexte in Du-Form, kurze Saetze, Fachwoerter erklaeren. Deutsch ist Quellsprache, `de.json`
  vollstaendig. Luxemburgisch: Code `lb`, Anzeige »LU«.

## Pruefen
```bash
python3 tools/validate_content.py            # alle Pakete
python3 tools/validate_content.py volta1     # ein Band
```
Fehler muessen behoben sein; Warnungen (fehlende Uebersetzungen) sind vor Phase 6 zulaessig.

## Neues Experiment
Slash-Befehl `/neues-experiment` verwenden. Danach Agent `inhalts-pruefer` laufen lassen.
