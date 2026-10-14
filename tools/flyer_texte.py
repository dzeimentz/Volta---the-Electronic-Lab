"""Uebersetzungen fuer tools/flyer_pdf.py: Schluessel ist der deutsche Text (Quelle in flyer_pdf.py).

UNVERAENDERT: Texte, die in allen Sprachen gleich bleiben (Zahlen, Namen, Adressen).
MUSTER: dynamische Zeilen (Version, Altersangaben, Experimentzahlen) als (Regex, Ersatz).
"""

UNVERAENDERT = {
    "0", "1", "2", "3", "4", "5", "6", "61", "21 + 4", "Volta 1", "Volta 2", "Volta 3", "Zuse 1", "Windows", "Mac",
    "Linux", "iPad", "dc-software-engineering.com", "info@dc-software-engineering.com",
    "DC Software Engineering · Daniel Zeimentz",
}

EN = {
    # Titel
    "Das Elektronik-Labor": "The Electronics Lab",
    "Bauteile stecken, Leitungen ziehen, einschalten – und sehen, was wirklich passiert.":
        "Plug in parts, draw wires, switch on – and see what really happens.",
    "Ein virtueller Elektronik-Experimentierkasten für Kinder von 8 bis 12 Jahren.":
        "A virtual electronics experiment kit for children aged 8 to 12.",
    "Experimente in 5 Stufen": "experiments on 5 levels",
    "Bauteile und Module": "parts and modules",
    "Sprachen: DE · FR · EN · LU": "languages: DE · FR · EN · LU",
    "Daten, Werbung, Käufe – offline": "data, ads, purchases – offline",
    "Für Kinder, Eltern und Schulen": "For children, parents and schools",
    "Lernpfad mit Forscherfrage": "Learning path with research questions",
    "Ehrliche Simulation": "Honest simulation",
    "Tick hilft": "Tick helps",
    "Aufgaben per QR-Code": "Tasks by QR code",
    "Vorführung für die Klasse": "Demo for the class",
    "Android-Tablet und Windows · iPad, Mac und Linux folgen": "Android tablet and Windows · iPad, Mac and Linux to follow",
    "Volta 1 – Das Elektronik-Labor · Produktinformation": "Volta 1 – The Electronics Lab · Product information",
    "Volta 1 – Das Elektronik-Labor · DC Software Engineering · dc-software-engineering.com":
        "Volta 1 – The Electronics Lab · DC Software Engineering · dc-software-engineering.com",
    # Seite 2
    "Forschen wie im echten Labor": "Explore like in a real lab",
    "Echte Schaltungssimulation – nichts ist vorher festgelegt": "Real circuit simulation – nothing is pre-recorded",
    "Stecken, ziehen, einschalten": "Plug in, draw, switch on",
    "Bauteile kommen aus dem Bauteilfach auf die Grundplatte; was im selben Kontaktfeld steckt, ist verbunden. "
    "Leitungen zieht man mit dem Finger von Feld zu Feld. Mit »Betrieb« fließt Strom – zwei Finger zoomen, "
    "»Rückgängig« holt jeden Schritt zurück.":
        "Parts go from the parts tray onto the base board; whatever sits in the same contact field is connected. "
        "Wires are drawn with a finger from field to field. “Running” lets the current flow – two fingers zoom, "
        "“Undo” takes back every step.",
    "Physik, die ehrlich ist": "Physics that is honest",
    "Volta rechnet jede Schaltung aus, auch selbst erfundene: Lämpchen glühen heller oder dunkler, Motoren drehen "
    "sich, Bauteile werden warm – und eine Leuchtdiode ohne Schutzwiderstand brennt durch. Ein Tippen auf "
    "»Rückgängig« macht sie wieder heil.":
        "Volta calculates every circuit, even invented ones: bulbs glow brighter or dimmer, motors turn, parts get "
        "warm – and an LED without a protective resistor burns out. One tap on “Undo” makes it whole again.",
    "Der Lernpfad": "The learning path",
    "61 Experimente in fünf Stufen, jedes mit einer Forscherfrage: Vermutung wählen, aufbauen, beobachten, "
    "verstehen. Stufe für Stufe kommen neue Bauteile dazu – vom Lämpchen bis zum Transistor, Relais und Zeitmodul.":
        "61 experiments on five levels, each with a research question: make a guess, build, observe, understand. "
        "Level by level new parts are added – from the bulb to the transistor, relay and timer module.",
    "Das Maskottchen Tick stellt die Forscherfrage, zeigt Schritt für Schritt, wohin jedes Teil gehört, und gibt "
    "Tipps, wenn es nicht weitergeht. Eine Kurzanleitung und eine Hilfe mit Tick sind immer einen Fingertipp "
    "entfernt.":
        "The mascot Tick asks the research question, shows step by step where each part goes and gives hints when "
        "children get stuck. A quick guide and help with Tick are always one tap away.",
    "Die fünf Stufen": "The five levels",
    "Auftakt": "Kick-off",
    "Wie bringe ich etwas zum Blinken?": "How do I make something blink?",
    "Strom fließt im Kreis": "Current flows in a circle",
    "Wann leuchtet das Lämpchen?": "When does the bulb light up?",
    "Mehrere Wege": "Several paths",
    "Was passiert, wenn ich ein zweites Lämpchen einbaue?": "What happens when I add a second bulb?",
    "Messen und verstehen": "Measure and understand",
    "Wie viel Strom fließt eigentlich?": "How much current actually flows?",
    "Kleine Elektronik": "Small electronics",
    "Wie kann ein Strom einen anderen steuern?": "How can one current control another?",
    # Seite 3
    "Werkzeuge wie im Elektronik-Labor – kindgerecht erklärt": "Tools like in an electronics lab – explained for children",
    "Dreifach-Messgerät": "Multimeter",
    "Spannung, Strom und Widerstand – digital oder mit Zeiger, neben der Grundplatte oder oben im Pult. "
    "Spannungsanzeigen an jedem Feld und laufende Pfeile zeigen, wohin der Strom fließt.":
        "Voltage, current and resistance – digital or with a needle, next to the board or up in the control desk. "
        "Voltage displays on every field and moving arrows show where the current flows.",
    "Zeitschreiber": "Chart recorder",
    "Zwei Kanäle zeichnen Spannungen über die Zeit auf: wie sich ein Kondensator füllt, wie ein Blinker kippt, wie "
    "ein Sensor auf Licht oder Wärme reagiert.":
        "Two channels record voltages over time: how a capacitor fills up, how a flasher flips, how a sensor reacts "
        "to light or heat.",
    "Schaltplan und »Blick hinein«": "Circuit diagram and “Look inside”",
    "Jeder Aufbau erscheint auf Wunsch als Schaltplan mit Normsymbolen. Module wie das Blinkmodul lassen sich "
    "öffnen: Innen arbeiten echte Transistoren – mit den Spannungen, die gerade gerechnet werden.":
        "Every build can be shown as a circuit diagram with standard symbols. Modules such as the flasher module "
        "can be opened: real transistors work inside – with the voltages being calculated right now.",
    "Werkzeugkasten und Fehlersuche": "Toolbox and fault finder",
    "Farbcode, E-Reihen, Bauteillexikon, Formelrechner mit Rechenweg und eine Teileliste mit Aufbaukarte zum "
    "Nachbauen. Die Fehlersuche findet Kurzschlüsse, offene Kreise und falsch gepolte Bauteile – und fragt nach, "
    "statt zu verraten.":
        "Colour code, E series, parts encyclopedia, formula calculator with workings and a parts list with a build "
        "card for building it for real. The fault finder spots short circuits, open circuits and reversed parts – "
        "and asks questions instead of giving the answer away.",
    "Licht, Wärme, Wasser": "Light, heat, water",
    "Fotowiderstand, Heißleiter und Feuchtesonde reagieren auf ihre Umgebung: Taschenlampe, Föhn und Wassertropfen "
    "zieht man direkt auf das Bauteil, das Raumlicht stellt man auf hell, Dämmerung oder dunkel. So entstehen "
    "Nachtlicht, Hitzealarm und Wassermelder.":
        "Photoresistor, thermistor and moisture probe react to their surroundings: drag the torch, hair dryer or "
        "water drop straight onto the part and set the room light to bright, dusk or dark. That is how a night "
        "light, a heat alarm and a water detector come about.",
    # Seite 4
    "Für Schule und Eltern": "For schools and parents",
    "Sicher, werbefrei, ohne Konto – und bereit für den Unterricht": "Safe, ad-free, no account – and ready for the classroom",
    "Die Lehrkraft wählt Experimente oder gibt ihren Aufbau mit – Volta zeigt einen großen QR-Code für den Beamer "
    "oder einen kurzen Code zum Eintippen. Keine Verbindung zwischen den Geräten, keine Rückmeldung an einen Server.":
        "The teacher picks experiments or hands out a build – Volta shows a large QR code for the projector or a "
        "short code to type in. No connection between devices, nothing reported to a server.",
    "Vorführung und Präsentation": "Demo and presentation",
    "Zehn fertige Aufbauten aus allen Stufen für die Klasse oder ein Gespräch, dazu ein Präsentationsmodus für "
    "Beamer und Tafel. Beides läuft ohne Spuren: Fortschritt und Forscherheft bleiben unberührt.":
        "Ten ready-made builds from all levels for the class or a meeting, plus a presentation mode for projector and "
        "whiteboard. Both leave no traces: progress and research notebook stay untouched.",
    "Forscherheft": "Research notebook",
    "Vermutungen, Beobachtungen, eigene Notizen und Fotos vom echten Aufbau – das Forscherheft wächst mit jedem "
    "Experiment. Mehrere Kinder an einem Gerät bekommen eigene Forscherprofile (Symbol und Spitzname, kein "
    "Klarname).":
        "Guesses, observations, own notes and photos of the real build – the research notebook grows with every "
        "experiment. Several children on one device get their own researcher profiles (symbol and nickname, no real "
        "name).",
    "Elternbereich und Datenschutz": "Parents' area and privacy",
    "Kein Konto, keine Werbung, keine Käufe in der App": "No account, no ads, no in-app purchases",
    "Keine Datenerhebung, keine Internetverbindung, keine Berechtigungen":
        "No data collection, no internet connection, no permissions",
    "Elternbereich mit PIN: Freigaben, Profile, Ton im Klassenzimmer":
        "PIN-protected parents' area: unlocking, profiles, classroom sound",
    "Verwaltete Geräte: Vorgaben der Schule (Sprache, Stufen, Ton)":
        "Managed devices: school settings (language, levels, sound)",
    "Bedienelemente ab 48 dp, Farbe nie allein, Vorlesen der Texte":
        "Controls of at least 48 dp, colour never alone, texts read aloud",
    "Sprachen Deutsch, Français, English, Lëtzebuergesch": "Languages: Deutsch, Français, English, Lëtzebuergesch",
    "Sicherheit: nur Batterien bis 9 V, zerstörende Versuche »Nur in der App!«":
        "Safety: batteries up to 9 V only, destructive tests “Only in the app!”",
    "Jetzt in echt": "Now for real",
    "Zu jedem Experiment gibt es eine Teileliste und eine Aufbaukarte als PDF zum Ausdrucken – damit lässt sich der "
    "Versuch mit echten Bauteilen nachbauen (nur Batterien bis 9 Volt; zerstörende Versuche gibt es nur in der App). "
    "Dazu eine Kurzanleitung in der App sowie eine ausführliche Anleitung und eine Kurzanleitung als PDF.":
        "Every experiment comes with a parts list and a printable build card (PDF) – so it can be rebuilt with real "
        "parts (batteries up to 9 volts only; destructive tests exist only in the app). Plus a quick guide in the "
        "app and a full manual and quick guide as PDF.",
    # Seite 5
    "Plattformen": "Platforms",
    "Ein Programm – auf Tablet und Computer": "One program – on tablet and computer",
    "Android-Tablet": "Android tablet",
    "Testversion verfügbar": "test version available",
    "in Vorbereitung": "in preparation",
    "geplant": "planned",
    "Android 8 und neuer, Tablets ab 10 Zoll empfohlen, Finger und Stift. 64 und 32 Bit (arm64, armv7), ohne "
    "Berechtigungen. Geplant über Google Play, auch für Schulen (verwaltete Geräte).":
        "Android 8 or later, tablets of 10 inches or more recommended, finger and stylus. 64 and 32 bit (arm64, "
        "armv7), no permissions. Planned via Google Play, also for schools (managed devices).",
    "Windows 10 und 11 (64 Bit), Maus, Tastatur und Touchscreen. Eigene Fensterleiste im Volta-Design. "
    "Installationsprogramm für die Testversion; Microsoft Store in Vorbereitung.":
        "Windows 10 and 11 (64 bit), mouse, keyboard and touchscreen. Own title bar in Volta design. Installer for "
        "the test version; Microsoft Store in preparation.",
    "iPadOS, Finger und Stift gleichwertig, alle Ausrichtungen. Paketierung vorbereitet; Veröffentlichung im App "
    "Store (Kinderkategorie, Apple School Manager) folgt.":
        "iPadOS, finger and stylus alike, all orientations. Packaging prepared; release in the App Store (Kids "
        "category, Apple School Manager) to follow.",
    "macOS auf Apple Silicon aus demselben Programmcode, mit Maus, Trackpad und Tastatur.":
        "macOS on Apple silicon from the same code base, with mouse, trackpad and keyboard.",
    "Linux 64 Bit als AppImage, für Schulrechner und Tafel-PCs; verwaltete Einstellungen über /etc/xdg.":
        "Linux 64 bit as an AppImage, for school computers and whiteboard PCs; managed settings via /etc/xdg.",
    "Auf allen Plattformen gleich: dieselben Experimente, dieselbe ehrliche Simulation, Bedienung mit Finger, Stift "
    "oder Maus, kein Netz und keine Datenerhebung. Fortschritt, Forscherheft und eigene Aufbauten bleiben auf dem "
    "Gerät und lassen sich als Datei mitnehmen.":
        "The same on every platform: the same experiments, the same honest simulation, operation by finger, stylus "
        "or mouse, no network and no data collection. Progress, research notebook and own builds stay on the device "
        "and can be taken along as a file.",
    # Seite 6
    "Die Reihe und der Weg dorthin": "The series and the road ahead",
    "Volta wächst mit – Zuse führt in die Computertechnik": "Volta grows with the children – Zuse leads into computing",
    "Testphase 0.9": "test phase 0.9",
    "Planung": "planning",
    "Konzept": "concept",
    "Stromkreis, Schalter, Leuchtdioden, Messen, Sensoren, Transistor, Module":
        "Circuits, switches, LEDs, measuring, sensors, transistor, modules",
    "Oszilloskop, Funktionsgenerator, hörbare Simulation, Operationsverstärker, Timer-Baustein 555":
        "Oscilloscope, function generator, audible simulation, operational amplifier, 555 timer",
    "Schwingkreis und Radio, Stromversorgung, Messverstärker, digitale Grundschaltungen":
        "Resonant circuit and radio, power supply, measuring amplifiers, basic digital circuits",
    "Vom Bit über Gatter, Rechenwerk und Speicher zum selbst gebauten 4-Bit-Rechner – und ihn programmieren":
        "From the bit via gates, ALU and memory to a self-built 4-bit computer – and programming it",
    "Kontakt": "Contact",
    "Aufm Rothenhügel 26 · 54666 Irrel · Deutschland": "Aufm Rothenhügel 26 · 54666 Irrel · Germany",
    "Stand: Version 0.9.0, Testphase. Erprobung mit Kindern und Schulen läuft; Veröffentlichung zuerst für "
    "Android-Tablets und Windows. Gerne zeigen wir Volta in einer Vorführung.":
        "Status: version 0.9.0, test phase. Trials with children and schools are under way; release first for "
        "Android tablets and Windows. We are happy to show Volta in a demo.",
}

TEXTE = {"en": {**{s: s for s in UNVERAENDERT}, **EN}}
MUSTER = {"en": [
    (r"(\d+) Exp\.", r"\1 exp."),
    (r"(\d+)–(\d+) Jahre", r"ages \1–\2"),
    (r"Version ([0-9.]+) \(Testphase\) · Oktober 2026 · DC Software Engineering, Irrel",
     r"Version \1 (test phase) · October 2026 · DC Software Engineering, Irrel, Germany"),
]}
