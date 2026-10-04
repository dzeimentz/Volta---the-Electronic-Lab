---
description: Legt ein neues Experiment als Inhaltspaket an (JSON + deutsche Texte) und prueft es
argument-hint: <band>.<stufe>.<nr> <Kurzbeschreibung>
---

Lege ein neues Experiment an: $ARGUMENTS

Vorgehen:
1. Lies `docs/content-format.md`, `content/CLAUDE.md` und ein bestehendes Experiment in `content/volta1/experiments/`.
2. Pruefe in `content/catalog/bauteile.json`, ob alle benoetigten Bauteiltypen in der Stufe freigeschaltet sind.
   Fehlt ein Typ, brich ab und schlage `/neues-bauteil` vor.
3. Entwirf den Ablauf nach dem Forschungskreislauf: Frage, 3–4 Vermutungen, Aufbau, Versuch, Beobachtung,
   Erklaerung, Weiterforschen, Jetzt in echt. Ziel: 10–15 Minuten.
4. Lege `content/<band>/experiments/<stufe>.<nr>.json` an. Feldbezeichner A1–E8, Pult `P:...`, Leitungen mit Farbe
   (rot fuer Plus, schwarz fuer Minus). Erfolgsbedingungen zustandsbasiert, nicht aufbaubezogen.
5. Setze `zerstoerend` korrekt; bei `true` darf »Jetzt in echt« nur den sicheren Nachbau beschreiben.
6. Trage alle Schluessel in `content/<band>/i18n/de.json` ein: Du-Form, kurze Saetze, eigenstaendige Formulierungen.
   Keine Inhalte aus fremden Anleitungen (insbesondere nicht aus Kosmos-Handbuechern).
7. Fuehre `python3 tools/validate_content.py <band>` aus und behebe alle Fehler.
8. Rechne die erwartete Erfolgsgroesse von Hand nach (z. B. Strom durch die LED) und nenne das Ergebnis.
9. Uebergib zum Schluss an den Agenten `inhalts-pruefer`.
