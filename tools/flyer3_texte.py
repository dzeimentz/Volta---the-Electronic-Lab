"""Uebersetzungen fuer tools/flyer3_pdf.py: Schluessel ist der deutsche Text (Quelle in flyer3_pdf.py).

Ergaenzt die Tabellen von tools/flyer_texte.py und tools/flyer2_texte.py (gemeinsame Texte wie Datenschutz, Kontakt,
Reihe stehen dort). Stufentitel und Forscherfragen kommen aus content/volta3/i18n. MUSTER3: dynamische Zeilen
(Version) als (Regex, Ersatz).
"""

EN = {
    # Titel
    "Volta 3": "Volta 3",
    "Schwingen, senden, schalten, zählen – ehrlich gerechnet.": "Oscillate, transmit, switch, count – honestly calculated.",
    "Der dritte Band des virtuellen Elektronik-Experimentierkastens, für 13 bis 16 Jahre.":
        "The third volume of the virtual electronics experiment kit, for ages 13 to 16.",
    "Projekte zum Nachbauen": "projects to build",
    "Neu in Volta 3": "New in Volta 3",
    "Frequenzgang-Schreiber": "Frequency response plotter",
    "Logikanalysator": "Logic analyser",
    "Lupe ×10 … ×1000": "Magnifier ×10 … ×1000",
    "Senderanlage und Radio": "Transmitter and radio",
    "Modulation und Wobbeln": "Modulation and sweep",
    "Gatter und Zähler": "Gates and counters",
    "Zuerst für Windows · Android-Tablet und iPad folgen": "Windows first · Android tablet and iPad to follow",
    "Volta 3 – Das Elektronik-Labor · Produktinformation": "Volta 3 – The Electronics Lab · Product information",
    "Volta 3 – Das Elektronik-Labor · DC Software Engineering · dc-software-engineering.com":
        "Volta 3 – The Electronics Lab · DC Software Engineering · dc-software-engineering.com",
    # Seite 2
    "Messgeräte für schnelle Signale": "Instruments for fast signals",
    "Was im Oszilloskop zu schnell oder zu viel ist, zeigen neue Geräte":
        "What is too fast or too much for the oscilloscope, new instruments show",
    "Generator an den Eingang, Tastkopf an den Ausgang: Volta zeichnet die Verstärkung in dB von 1 Hz bis 10 MHz, "
    "auf Wunsch mit Phase. Resonanz, Grenzfrequenzen, Bandbreite und Güte stehen als Zahl daneben. Bei Schaltungen, "
    "die schalten, erscheint ein Hinweis statt einer falschen Kurve.":
        "Generator at the input, probe at the output: Volta plots the gain in dB from 1 Hz to 10 MHz, with phase if "
        "wanted. Resonance, cut-off frequencies, bandwidth and quality factor appear as numbers next to it. For "
        "circuits that switch, a note appears instead of a wrong curve.",
    "Lupe im Oszilloskop": "Magnifier in the oscilloscope",
    "Vorgänge, die schneller sind als die Simulation in Echtzeit rechnet, dehnt die Lupe ×10, ×100 oder ×1000. Volta "
    "rechnet dann in feineren Schritten – die Zeit auf dem Schirm bleibt die echte. Gedehnte Bilder tragen einen "
    "gestrichelten Rahmen und den Faktor.":
        "Events faster than the simulation can calculate in real time are stretched ×10, ×100 or ×1000 by the "
        "magnifier. Volta then calculates in finer steps – the time on the screen stays the real time. Stretched "
        "images carry a dashed frame and the factor.",
    "Bis zu acht Logik-Tastköpfe, jeder mit einer Zeile H und L. Trigger auf Flanke oder Muster, zwei Zeitmarken mit "
    "Abstand in µs und ms, Schwellen zum Einstellen. Im feinen Fenster sieht man, wie ein Taster prellt.":
        "Up to eight logic probes, each with a row of H and L. Trigger on edge or pattern, two time markers with the "
        "distance in µs and ms, adjustable thresholds. In the fine window you can see a push button bounce.",
    "Eine kleine Senderanlage im Pult sendet eigene Aufnahmen auf Mittelwelle und UKW – nur in Volta, ohne Netz. Mit "
    "Spule, Drehkondensator und Diode entsteht ein Radio ohne Batterie, danach mit Verstärker und Modul.":
        "A small transmitter in the desk sends its own recordings on medium wave and FM – only inside Volta, without "
        "a network. With a coil, a variable capacitor and a diode you build a radio without a battery, then with an "
        "amplifier and a module.",
    # Seite 3
    "Vom Schwingkreis bis zum Zähler": "From the tuned circuit to the counter",
    "Die Stufen": "The levels",
    "Gatter, Zähler, Anzeige": "Gates, counters, display",
    "Aus Transistoren wird ein NICHT, aus zwei NAND ein UND, aus zwei Gattern ein Gedächtnis. Danach zählen Zähler- "
    "und Dekadenzähler-Modul, das Anzeige-Modul zeigt Ziffern – bis zum elektronischen Würfel und zum Frequenzzähler.":
        "Transistors become a NOT, two NANDs an AND, two gates a memory. Then the counter and decade counter modules "
        "count and the display module shows digits – up to the electronic dice and the frequency counter.",
    "Jedes Experiment beginnt mit einer Forscherfrage und einer Vermutung. Tick zeigt Schritt für Schritt, wohin jedes "
    "Teil gehört, gibt Tipps und erklärt am Ende, warum es so ist. Wer mehr wissen will, liest »Mehr wissen« und "
    "»Jetzt in echt«.":
        "Every experiment starts with a research question and a guess. Tick shows step by step where each part goes, "
        "gives hints and explains at the end why it is so. Anyone who wants to know more reads »Learn more« and "
        "»Now for real«.",
    # Seite 4
    "Werkzeugkasten und Projekte": "Toolbox and projects",
    "Rechnen, nachschlagen, Fehler finden – und am Ende ein Gerät": "Calculate, look up, find faults – and build a device in the end",
    "Formelrechner": "Formula calculator",
    "Schwingkreis (Thomson-Formel)": "Tuned circuit (Thomson formula)",
    "Bandbreite und Güte": "Bandwidth and quality factor",
    "Wirkungsgrad und Verlust": "Efficiency and loss",
    "Pegel in dB": "Level in dB",
    "Tastverhältnis und Mittelwert": "Duty cycle and average",
    "Frequenzteiler": "Frequency divider",
    "Lexikon": "Glossary",
    "alle Bauteile und Module": "all parts and modules",
    "Modulation, Resonanz, Wirkungsgrad": "modulation, resonance, efficiency",
    "Pegel, Flanke, Prellen": "level, edge, contact bounce",
    "Fehlersuche": "Troubleshooting",
    "Thyristor ohne Haltestrom": "Thyristor without holding current",
    "MOSFET-Gate offen": "MOSFET gate left open",
    "Spule ohne Freilaufdiode": "Coil without flyback diode",
    "Gatter-Eingang offen": "Gate input left open",
    "Taster ohne Entprellung": "Push button without debouncing",
    "Schwingkreis ohne Widerstand": "Tuned circuit without resistor",
    "Die Fehlersuche stellt Fragen statt Lösungen – und bemängelt nie einen Aufbau, der funktioniert.":
        "Troubleshooting asks questions instead of giving solutions – and never criticises a build that works.",
    "Sieben Projekte": "Seven projects",
    "Der Klangschalen-Oszillator": "The singing-bowl oscillator",
    "Ein UKW-Radio": "An FM radio",
    "Eine Taschenlampe mit einer Zelle": "A torch with a single cell",
    "Ein Lampendimmer für Kleinspannung": "A lamp dimmer for low voltage",
    "Der sparsamste Verstärker": "The most economical amplifier",
    "Eine Waage mit Leuchtbalken": "A scale with a light bar",
    "Ein Reaktionszeit-Messer": "A reaction timer",
    "Volta rechnet jede Schaltung wirklich aus – nichts ist vorher aufgezeichnet. Die Abweichung zu den Formeln bleibt "
    "unter 2 %.":
        "Volta really calculates every circuit – nothing is pre-recorded. The deviation from the formulas stays below "
        "2 %.",
    # Seite 5
    "Sicher, werbefrei, ohne Konto – wie Volta 1 und 2": "Safe, ad-free, no account – like Volta 1 and 2",
    "Die Senderanlage sendet nur eigene Aufnahmen in der App – kein Empfang echter Sender":
        "The transmitter only sends its own recordings inside the app – no reception of real stations",
    "Sicherheit: nur Kleinspannung aus Batterie und Pult – auch der Dimmer":
        "Safety: only low voltage from battery and desk – the dimmer too",
    "13–16 Jahre": "ages 13–16",
    "Schwingkreis und Radio, Stromversorgung, Thyristor, Verstärkerklassen, Messverstärker, Logik und Zähler":
        "Tuned circuits and radio, power supplies, thyristor, amplifier classes, measuring amplifiers, logic and "
        "counters",
    "in Entwicklung": "in development",
}

TEXTE3 = {"en": EN}

MUSTER3 = {
    "en": [
        (r"[0-9]+", r"\g<0>"),  # Zahlen in den Kacheln
        (r"^Version (\S+) \(Entwicklung\) · November 2026 · DC Software Engineering, Irrel$",
         r"Version \1 (in development) · November 2026 · DC Software Engineering, Irrel"),
        (r"^Stand: Version (\S+), in Entwicklung\. Volta 3 baut auf Volta 1 und 2 auf; gerne zeigen wir es in einer "
         r"Vorführung\.$",
         r"Status: version \1, in development. Volta 3 builds on Volta 1 and 2; we are happy to show it in a "
         r"demonstration."),
    ],
}
