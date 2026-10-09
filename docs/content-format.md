# Format der Inhaltspakete

Inhalte liegen als JSON (UTF-8) unter `content/`. Schluessel sind deutsch. Sichtbare Texte stehen nie im
Experiment selbst, sondern als **i18n-Schluessel**, die in `content/<band>/i18n/<sprache>.json` aufgeloest werden.
Pruefung: `python3 tools/validate_content.py`. Schema: `content/schema/experiment.schema.json`.

## Verzeichnis

```
content/
  schema/experiment.schema.json
  catalog/bauteile.json          Bauteiltypen mit Anschluessen, Parametern, Freischaltstufe
  <band>/stufen.json             Stufen des Lernpfads: nr, titel_key, frage_key (Leitfrage)
  volta1/
    paket.json                   Paketkopf: band, version, schema_version, sprachen
    experiments/<id>.json        ein Experiment je Datei, Dateiname = id ohne Bandpraefix
    i18n/de.json, fr.json, en.json, lb.json
    audio/lb/<schluessel>.ogg    eigene luxemburgische Aufnahmen
```

## Experiment

```json
{
  "id": "volta1.2.4",
  "band": "volta1",
  "stufe": 2,
  "kapitel": 2,
  "titel_key": "exp_2_4_titel",
  "dauer_min": 12,
  "zerstoerend": true,
  "bauteile_frei": ["led_rot", "widerstand", "leitung"],
  "frage_key": "exp_2_4_frage",
  "vermutungen": ["exp_2_4_v1", "exp_2_4_v2", "exp_2_4_v3", "exp_2_4_v4"],
  "vorlage": {
    "pult": "bedienpult_1",
    "bauteile": [
      {"id": "R1", "typ": "widerstand", "felder": ["B3", "B4"], "wert": 470},
      {"id": "D1", "typ": "led_rot", "felder": {"anode": "B4", "kathode": "C4"}}
    ],
    "module": [],
    "leitungen": [
      {"von": "P:+9V", "nach": "B3", "farbe": "rot"},
      {"von": "C4", "nach": "P:0V", "farbe": "schwarz"}
    ],
    "anzeigen": ["B4"]
  },
  "schritte": ["exp_2_4_s1", "exp_2_4_s2"],
  "erfolg": {"alle": [
    {"bauteil": "D1", "groesse": "strom_mA", "zwischen": [5, 25], "dauer_s": 5},
    {"bauteil": "D1", "zustand": "intakt"}
  ]},
  "hilfen": ["exp_2_4_h1", "exp_2_4_h2", "exp_2_4_loesung"],
  "erklaerung_key": "exp_2_4_erklaerung",
  "mehr_wissen_key": "exp_2_4_mehr",
  "weiterforschen": ["exp_2_4_w1"],
  "jetzt_in_echt_key": "exp_2_4_echt"
}
```

### Felder
| Schluessel | Pflicht | Bedeutung |
|---|---|---|
| `id` | ja | `<band>.<stufe>.<nr>`, eindeutig |
| `stufe` | ja | 0 (Auftakt) bis 4 in Volta 1 |
| `zerstoerend` | ja | `true`, wenn im Ablauf ein Bauteil zerstoert wird -> Hinweis »Nur in der App!« |
| `beobachtungen` | nein | Auswahl nach dem Versuch (2–5 Schluessel); ohne: direkt zur Erklaerung |
| `vermutung_passt` / `vermutung_offen` | nein | Indizes der Vermutungen, die zum Ergebnis passen bzw. offen sind (»weiss nicht«); Vergleich ohne Wertung |
| `bauteile_frei` | ja | Typen aus `catalog/bauteile.json` oder `leitung` |
| `vorlage` | ja | Musteraufbau; dient Anzeige und Schritt-fuer-Schritt-Hilfe, **nicht** der Erfolgspruefung |
| `erfolg` | ja | zustandsbasierte Bedingungen (siehe unten) |
| `jetzt_in_echt_key` | ja | darf bei `zerstoerend: true` keinen zerstoerenden Nachbau beschreiben |

### Vorlage: Bauteile, Module, Leitungen
- Bauteil: `{"id", "typ", "felder", "wert"?, "geschlossen"?, "stellung"?}`. `felder` ist ein Array in der
  Reihenfolge der Katalog-Anschluesse oder ein Objekt `{anschluss: feld}`. `wert` in Ohm bzw. Farad, bei der Gluehlampe die
  Nennspannung in Volt (Katalog `varianten`: 2,5 / 6 / 10 V, Vorgabe 6 V);
  `geschlossen` fuer Schalter, Taster, Reedkontakt (Wechselschalter: Stellung B); `stellung` 0..1 fuer
  Potentiometer.
- Modul: `{"id": "BL1", "typ": "blinkmodul", "feld": 1}` – belegt die Felder der Modulleiste ab `M:<feld>` in der
  Reihenfolge der Katalog-Anschluesse (Blinkmodul: `plus`, `minus`, `aus`, `takt`).
- Leitung: `{"von", "nach", "farbe"?, "id"?}`; ohne `id` vergibt das Laden `L1`, `L2`, …
- Pultstellung (optional): `"pult_stellung": {"batterie_V": 3 | 4.5 | 9, "schalter": bool, "p1": 0..1, "p2": 0..1}`.
  Gilt beim Start des gefuehrten Experiments (leere Grundplatte) und fuer den Musteraufbau; der Hauptschalter
  bleibt aus – Einschalten ist Sache des Kindes. Fehlt der Schluessel: 9 V, Pultschalter offen, Regler in der Mitte.

### Anschlussbezeichner
- Kontaktfeld: `A1` … `E12` (5 Zeilen x 12 Spalten, je Feld 5 Steckplaetze)
- Pultfeld: `P:+9V`, `P:0V`, `P:P1a`, `P:P1s`, `P:P1b`, `P:P2a`, `P:P2s`, `P:P2b`, `P:S1`, `P:S2`, `P:LS1`, `P:LS2`
- Modulanschluss: `<Modul-ID>.<Anschluss>` (z. B. `BL1.takt`)
- Messgeraet: `MESS:COM`, `MESS:VOHM`, `MESS:A`

### Erfolgsbedingungen
Bedingungen werden ueber den simulierten Zustand geprueft; jeder funktionierende Aufbau gilt.

| Form | Beispiel |
|---|---|
| Groesse im Bereich | `{"bauteil": "D1", "groesse": "strom_mA", "zwischen": [5, 25], "dauer_s": 5}` |
| Zustand | `{"bauteil": "D1", "zustand": "intakt"}` |
| Leuchten | `{"bauteil": "L1", "groesse": "helligkeit", "min": 0.5}` |
| Knotenspannung | `{"feld": "B4", "groesse": "spannung_V", "zwischen": [1.5, 2.5]}` |
| Ton | `{"bauteil": "LS", "groesse": "frequenz_Hz", "min": 200}` |
| Verknuepfung | `{"alle": [...]}`, `{"eine": [...]}`, `{"nicht": {...}}` |

Groessen: `strom_mA`, `spannung_V`, `leistung_mW`, `helligkeit` (0..1), `drehzahl`, `frequenz_Hz`, `temperatur_C`.
Bauteilbezug ueber die Rolle in der Vorlage: Das Kind darf andere Bezeichner verwenden; die Zuordnung erfolgt ueber
Typ und Funktion (z. B. »die einzige rote LED«). Bei Mehrdeutigkeit `rolle` statt `bauteil` verwenden.

Umsetzung (Phase 1, `LabSession::setSuccess`): Ein Vorlagenbauteil gilt als das Bauteil gleicher Kennung, wenn
dieses denselben Typ hat; sonst als das naechste noch nicht zugeordnete Bauteil desselben Typs (Reihenfolge des
Aufbaus). Die Pruefung laeuft im Simulations-Thread nach jedem Schritt; nach einer Aufbauaenderung beginnen die
Haltedauern (`dauer_s`) neu. `rolle` (Funktion statt Typ) folgt mit der Lernpfad-Engine in Phase 2.

Messgeraet in Bedingungen: `{"bauteil": "MESS", "groesse": "anzeige", "zwischen": [8.5, 9.1]}` prueft die Anzeige
des Dreifach-Messgeraets (Betrag, in V, A bzw. Ohm je nach Messart) - so laesst sich pruefen, ob richtig gemessen
wird. Vorlage: `pult_stellung` kennt zusaetzlich `messart` ("V", "A", "Ohm") und `raumlicht` ("hell", "daemmerung",
"dunkel"); `werkzeuge` legt Taschenlampe bzw. Foehn bereit: `[{"typ": "taschenlampe", "feld": "B3", "an": true}]`.

Module in Bedingungen: `bauteil` darf auch eine Modulkennung der Vorlage sein (z. B. `TM1`); gemessen werden die
Werte des Moduls (Tonmodul: `frequenz_Hz` = aktueller Ton, `strom_mA` = Strom in den Ausgang `aus`; Blinkmodul:
Ausgangstransistor). Zuordnung wie bei Bauteilen ueber Kennung und Typ.

## Bauteilkatalog (`catalog/bauteile.json`)

```json
{"typ": "led_rot", "anschluesse": ["anode", "kathode"], "spannweite": ["kurz", "lang"],
 "modell": "led", "parameter": {"uf_V": 1.9, "imax_mA": 20}, "ab_band": "volta1", "ab_stufe": 2,
 "lexikon_key": "lex_led"}
```

## i18n-Dateien

`content/volta1/i18n/de.json` ist ein flaches Objekt `{ "schluessel": "Text" }`. Jede im Paket verwendete
Schluessel muss in `de.json` existieren; fehlende Uebersetzungen in anderen Sprachen sind Warnungen, keine Fehler.
Kindertexte: kurze Saetze, Du-Form, keine Fachwoerter ohne Erklaerung, keine Inhalte aus fremden Anleitungen.

## Versionierung

`paket.json` traegt `schema_version` (aktuell `1`) und `version` (SemVer). Inkompatible Formataenderungen erhoehen
`schema_version` und erfordern eine Migration im `ContentRepository`.
