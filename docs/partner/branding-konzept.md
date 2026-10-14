# Volta – Branding-Konzept für Partnerausgaben

Stand: 14.10.2026. Wie eine Ausgabe für einen Verlag oder Lehrmittelhersteller angepasst werden kann, was fest
bleibt und was das technisch kostet.

## Grundsatz

Eine Partnerausgabe ist eine eigene **Edition** aus derselben Codebasis (wie heute `volta1`; künftig z. B.
`<partner>-elektronik`). Sie trägt Name, Farben und Logo des Partners und kann eigene Inhalte enthalten. Der
Simulationskern, die Sicherheits- und Datenschutzregeln und der **Urhebervermerk** bleiben unverändert.

## Was angepasst werden kann

| Bereich | Anpassung | Heute | Aufwand |
|---|---|---|---|
| Name und App-Kennung | Produktname, Paketname (z. B. `com.partner.elektronik`), Store-Eintrag | Edition über `VOLTA_EDITION`, Name in CMake | gering |
| Programmsymbol, Startbild | Logo, Startbildschirm, Store-Grafiken | aus `volta.svg` bzw. `splash.jpg` erzeugt (`tools/make_icons.py`) | gering |
| Farben der Oberfläche | Kopfleiste, Knöpfe, Kacheln, Akzentfarben, Fensterleiste | etwa 25 Grundfarben zentral (`Theme.qml`), weitere ca. 200 Oberflächenfarben noch in einzelnen Dateien | mittel: einmalig alle Oberflächenfarben ins Theme holen (ca. 3–5 PT), danach je Partner gering |
| Bauteilfarben | bleiben (Normfarben: Farbcode, LED-Farben, Leitungsfarben) | – | – |
| Figur | Tick ersetzen durch eine Figur des Partners oder ohne Figur | Tick als Bildsatz (10 Stimmungen), abschaltbar; ohne Figur erscheinen die Hinweise als Kästen | gering (neue Bilder); neue Figur: Gestaltung durch Partner |
| Texte | Ansprache, Begriffe der Partnerreihe, Hilfetexte | alle Texte über Übersetzungsschlüssel bzw. Inhaltspakete | gering bis mittel |
| Inhalte | zusätzliche Experimente passend zu einem echten Kasten des Partners (Bauteile, Feldbezeichnungen der Aufbaukarte, »Jetzt in echt«) | Inhaltspakete (JSON), Aufbaukarte als PDF | je Experiment ca. 0,5 PT inkl. Prüfung |
| Sprachen | Auswahl und Reihenfolge, weitere Sprachen | DE, FR, EN, LU | je Sprache Übersetzung + Prüfung |
| Schulfunktionen | Vorgaben für verwaltete Geräte, Präsentationsmodus, Vorführung, Aufgaben per QR-Code | vorhanden, im Elternbereich schaltbar | gering |
| Unterlagen | Anleitung, Kurzanleitung, Flyer im Design des Partners | per Skript erzeugt (`tools/manual_pdfs.py`, `tools/flyer_pdf.py`) | gering bis mittel |

## Was fest bleibt

- **Simulationskern** und das Prinzip »ehrliche Physik« (kein Vortäuschen von Ergebnissen).
- **Sicherheit:** keine Netzspannung, zerstörende Versuche nur in der App, Links hinter der Elternschranke.
- **Datenschutz:** keine Datenerhebung, kein Netz, keine Werbung, keine Käufe.
- **Barrierefreiheit:** Bedienelemente ≥ 48 dp, Farbe nie alleiniger Informationsträger.
- **Urhebervermerk:** im Info-Fenster und Impressum der Ausgabe, z. B. »Software: DC Software Engineering« bzw.
  »Entwickelt von DC Software Engineering«; Wortlaut und Platz (auch Startbild, Store-Text, Verpackung) regelt
  der Vertrag.

## Modelle der Sichtbarkeit

| Modell | Nach außen | DC Software Engineering |
|---|---|---|
| Eigenmarke des Partners | Name, Farben, Logo des Partners | Info-Fenster, Impressum, Store-Beschreibung |
| Co-Branding | Partner vorne, »mit Volta-Technik« | Startbild, Verpackung, Info |
| Volta mit Vertriebspartner | Volta, Logo des Partners als Vertrieb | voll sichtbar |

## Technischer Weg

1. **Branding-Paket je Partner** (Vorschlag): Ordner `branding/<partner>/` mit `marke.json` (Name, App-Kennung,
   Farben, Texte für Info/Impressum, Figur ja/nein), Symbol-SVG, Startbild, optional Figurenbilder; Auswahl beim
   Bauen (`-DVOLTA_BRAND=<partner>`). Ohne Angabe entsteht die DC-Ausgabe.
2. **Theme vervollständigen:** alle Oberflächenfarben in `Theme.qml`, gespeist aus `marke.json` (einmalig).
3. **Inhaltspaket des Partners** neben dem Volta-Paket (eigene Stufen oder zusätzliche Experimente).
4. **Unterlagen und Store-Grafiken** aus denselben Skripten mit den Markenwerten.
5. **Beispiel zur Vorführung:** eine Umgestaltung mit einer erfundenen Marke, um die Möglichkeiten zu zeigen –
   nie mit Namen oder Logos echter Verlage ohne deren Zustimmung.

Grobe Schätzung für die erste Partnerausgabe: 8–15 PT (Branding-Paket und Theme einmalig, dann Anpassung,
Prüfung, Paketierung); weitere Ausgaben deutlich weniger. Eigene Experimente zu einem echten Kasten kommen
hinzu.

## Wartung und Weiterentwicklung

Alle Ausgaben teilen Kern, Fehlerbehebungen und neue Funktionen. Updates liefert DC Software Engineering für
alle Editionen; die Veröffentlichung im Store kann über das Konto des Partners oder des Entwicklers laufen
(Vertragssache, Wartungsvereinbarung).
