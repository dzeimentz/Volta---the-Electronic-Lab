"""Uebersetzungen fuer tools/flyer2_pdf.py: Schluessel ist der deutsche Text (Quelle in flyer2_pdf.py).

Ergaenzt die Tabellen von tools/flyer_texte.py (gemeinsame Texte wie Datenschutz, Kontakt, Reihe stehen dort).
Stufentitel und Forscherfragen kommen aus content/volta2/i18n. MUSTER2: dynamische Zeilen (Version) als (Regex, Ersatz).
"""

UNVERAENDERT = {"7", "8", "15 + 2"}

EN = {
    # Titel
    "Messen, sehen, hören: Oszilloskop, Generator und Verstärker – ehrlich gerechnet.":
        "Measure, see, hear: oscilloscope, generator and amplifier – honestly calculated.",
    "Der zweite Band des virtuellen Elektronik-Experimentierkastens, für 11 bis 14 Jahre.":
        "The second volume of the virtual electronics experiment kit, for ages 11 to 14.",
    "Experimente in 9 Stufen": "experiments on 9 levels",
    "neue Bauteile und Module": "new parts and modules",
    "Neu in Volta 2": "New in Volta 2",
    "Oszilloskop": "Oscilloscope",
    "Funktionsgenerator": "Function generator",
    "Kennlinienschreiber": "Curve tracer",
    "Hörbare Simulation": "Audible simulation",
    "Schaltplan nach Norm": "Standard circuit diagrams",
    "Aufbau übernehmen": "Reuse a build",
    "Volta 2 – Das Elektronik-Labor · Produktinformation": "Volta 2 – The Electronics Lab · Product information",
    "Volta 2 – Das Elektronik-Labor · DC Software Engineering · dc-software-engineering.com":
        "Volta 2 – The Electronics Lab · DC Software Engineering · dc-software-engineering.com",
    # Seite 2
    "Messgeräte wie im echten Labor": "Instruments like in a real lab",
    "Was Volta 1 begonnen hat, wird jetzt sichtbar und hörbar": "What Volta 1 started now becomes visible and audible",
    "Zwei Kanäle mit Tastköpfen direkt auf der Grundplatte. Zeit, Volt je Kästchen, Lage und Trigger stellt man wie "
    "am echten Gerät ein – oder tippt auf »Auto« je Kanal, Zeit und Trigger. Dazu Rollbetrieb und XY-Darstellung. Es "
    "schaltet mit dem Hauptschalter ein und aus.":
        "Two channels with probes placed right on the base board. Time base, volts per division, position and trigger "
        "are set as on a real instrument – or with “Auto” for each channel, the time base and the trigger. Plus roll "
        "mode and XY display. It switches on and off with the main switch.",
    "Sinus, Dreieck und Rechteck von 1 Hz bis 10 kHz, mit einstellbarer Spitzenspannung und Gleichanteil. Was er "
    "erzeugt, rechnet Volta durch die ganze Schaltung – und über den Lautsprecher im Pult hört man das Ergebnis.":
        "Sine, triangle and square from 1 Hz to 10 kHz, with adjustable peak voltage and DC offset. Volta calculates "
        "its signal through the whole circuit – and the speaker in the control desk lets you hear the result.",
    "Er fährt die Spannung an einem Bauteil hoch und zeichnet den Strom dazu: die Gerade des Widerstands, die Kurve "
    "der Glühlampe, den Knick der Diode. So wird sichtbar, was die Formeln aus Volta 1 meinen – und wo sie aufhören "
    "zu gelten.":
        "It sweeps the voltage across a part and plots the current: the straight line of the resistor, the curve of "
        "the bulb, the knee of the diode. This shows what the formulas from Volta 1 mean – and where they stop "
        "being true.",
    "Pult nach Bedarf": "Control desk on demand",
    "Neue Geräte kommen erst ins Pult, wenn sie gebraucht werden: zweite Batterie, Wechselquelle, Generator und "
    "Kennlinienschreiber. Mit »+« holt man sie aus dem Gerätefach, mit »−« nimmt man sie wieder heraus. Leuchten "
    "zeigen, was eingeschaltet ist.":
        "New instruments only join the control desk when they are needed: second battery, AC source, generator and "
        "curve tracer. “+” takes them out of the instrument tray, “−” puts them back. Lamps show what is switched on.",
    # Seite 3
    "Der Lernpfad": "The learning path",
    "Neun Stufen – vom genauen Messen bis zur Lichtschranke": "Nine levels – from precise measuring to the light barrier",
    "Die neun Stufen": "The nine levels",
    "Viele Versuche bauen aufeinander auf. Wer den Arbeitspunkt geschafft hat, findet ihn im nächsten Experiment als "
    "beschrifteten Knopf in der Aufbaubeschreibung: ein Tippen setzt den fertigen Aufbau ein, nur das Neue kommt "
    "dazu. Alles bleibt rückgängig machbar.":
        "Many experiments build on each other. Once the operating point is done, the next experiment offers it as a "
        "labelled button in the build steps: one tap places the finished build, only the new parts are added. "
        "Everything can still be undone.",
    "Tick erklärt weiter": "Tick keeps explaining",
    "Jedes Experiment beginnt mit einer Forscherfrage und einer Vermutung. Tick zeigt Schritt für Schritt, wohin "
    "jedes Teil gehört, gibt Tipps und erklärt am Ende, warum es so ist. Ein »?« an jedem Gerät öffnet eine kurze "
    "Anleitung.":
        "Every experiment starts with a research question and a guess. Tick shows step by step where each part goes, "
        "gives hints and explains at the end why it works that way. A “?” on every instrument opens a short guide.",
    # Seite 4
    "Projekte, Schaltpläne, Blick hinein": "Projects, circuit diagrams, a look inside",
    "Vom einzelnen Versuch zum fertigen Gerät": "From a single experiment to a finished device",
    "Jeder Aufbau erscheint als Schaltplan mit Normsymbolen. Bekannte Grundschaltungen zeichnet Volta so, wie sie in "
    "jedem Buch stehen: den Brückengleichrichter als Raute, die Emitterschaltung mit dem Transistor in der Mitte, die "
    "Darlingtonschaltung. Der Plan lässt sich vergrößern und verkleinern.":
        "Every build appears as a circuit diagram with standard symbols. Well-known basic circuits are drawn the way "
        "every textbook shows them: the bridge rectifier as a diamond, the common-emitter stage with the transistor in "
        "the middle, the Darlington pair. The diagram can be zoomed in and out.",
    "Blick in den 555": "Inside the 555",
    "Der Timer-Baustein 555 lässt sich öffnen: Ein Blockbild zeigt Spannungsteiler, zwei Vergleicher und das "
    "Flipflop – mit den Spannungen, die gerade gerechnet werden. Blink- und Tonmodul aus Volta 1 baut man nach und "
    "sieht, wie sie innen arbeiten.":
        "The 555 timer chip can be opened: a block diagram shows the voltage divider, two comparators and the "
        "flip-flop – with the voltages being calculated right now. The flasher and tone modules from Volta 1 are "
        "rebuilt from parts to see how they work inside.",
    "Der Transistor als Verstärker": "The transistor as an amplifier",
    "Arbeitspunkt einstellen, Emitterschaltung aufbauen, übersteuern und wieder sauber machen – am Oszilloskop sieht "
    "man Eingang und Ausgang übereinander. Am Ende steht ein Audioverstärker.":
        "Set the operating point, build the common-emitter stage, overdrive it and clean it up again – the "
        "oscilloscope shows input and output one above the other. At the end there is an audio amplifier.",
    "Sechs Projekte": "Six projects",
    "Ein Netzteil mit Gleichrichter, Elko und 5-Volt-Regler": "A power supply with rectifier, capacitor and 5-volt regulator",
    "Ein Audioverstärker mit Gegentakt-Endstufe": "An audio amplifier with push-pull output stage",
    "Ein Klangregler mit Operationsverstärkern": "A tone control with operational amplifiers",
    "Ein Reaktionstester mit dem 555": "A reaction tester with the 555",
    "Ein kleiner Synthesizer": "A small synthesizer",
    "Eine Alarmanlage mit Lichtschranke": "An alarm system with a light barrier",
    "Volta rechnet jede Schaltung wirklich aus – nichts ist vorher aufgezeichnet. Wer genau hinschauen will, nimmt "
    "die Zeitlupe.":
        "Volta really calculates every circuit – nothing is pre-recorded. To look closely, use slow motion.",
    # Seite 5
    "Sicher, werbefrei, ohne Konto – wie schon bei Volta 1": "Safe, ad-free, no account – just like Volta 1",
    "Aufgaben per QR-Code und Forscherheft wie in Volta 1": "Tasks by QR code and research notebook as in Volta 1",
    "Sicherheit: nur Kleinspannung aus Batterie und Pult": "Safety: extra-low voltage only, from battery and control desk",
    "Die Reihe": "The series",
    "Testphase": "test phase",
    "in Entwicklung": "in development",
    "Oszilloskop, Funktionsgenerator, Kennlinien, Verstärker, Operationsverstärker, Timer-Baustein 555, Infrarot":
        "Oscilloscope, function generator, characteristic curves, amplifiers, operational amplifiers, 555 timer, "
        "infrared",
}

TEXTE2 = {"en": {**{s: s for s in UNVERAENDERT}, **EN}}
MUSTER2 = {"en": [
    (r"Version ([0-9.]+) \(Testphase\) · November 2026 · DC Software Engineering, Irrel",
     r"Version \1 (test phase) · November 2026 · DC Software Engineering, Irrel, Germany"),
    (r"Stand: Version ([0-9.]+), Testphase\. Volta 2 baut auf Volta 1 auf; gerne zeigen wir es in einer Vorführung\.",
     r"Status: version \1, test phase. Volta 2 builds on Volta 1; we are happy to show it in a demo."),
]}
