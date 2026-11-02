// Hilfe mit Tick (docs/ui-spec.md, Interaktives Readme; docs/maskottchen-tick.md): Kapitel fuer Kinder, jedes mit
// Tick in passender Stimmung und kurzen Punkten. Oben Kapitelknoepfe zum Springen. Der Teil fuer Erwachsene verweist
// in die Einstellungen (hinter der PIN), statt Inhalte doppelt zu fuehren.
// Ab Volta 2 ersetzen eigene Fassungen einzelne Kapitel (Pult II, Messstellen, Rechner) und neue Kapitel kommen dazu
// (Oszilloskop, Ton und Zeitlupe). Diese Kapitel und Willkommen/Stecken zeigen eigene Teilbilder aus Volta 2
// (qrc:/help2, nur in der Ausgabe volta2); die uebrigen die Bilder aus Volta 1. Volta 1 bleibt unveraendert.
import QtQuick
import QtQuick.Controls.Basic

Popup {
    id: help
    signal parentAreaRequested()

    objectName: "helpDialog"
    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(820, parent ? parent.width - 32 : 820)
    height: parent ? parent.height - 32 : 700
    padding: 0
    closePolicy: Popup.CloseOnEscape
    background: Rectangle { radius: 12; color: "#FFFDF7"; border.color: Theme.navy; border.width: 2 }

    property string band: "" // Band der Edition; leer oder »volta1«: Hilfe von Volta 1
    readonly property bool v2: band !== "" && band !== "volta1"
    readonly property var chapters: v2 ? v2Chapters() : baseChapters

    // Kapitel: Titel, Stimmung von Tick, was Tick sagt, Punkte
    readonly property var baseChapters: [
        { key: "willkommen", title: qsTr("Willkommen"), mood: "freundlich",
          say: qsTr("Hallo, ich bin Tick! In Volta baust du Schaltungen auf dem Bildschirm. Was passiert, rechnet Volta wirklich aus: Lämpchen leuchten, Motoren drehen sich – und manchmal brennt auch etwas durch."),
          points: [qsTr("Im Lernpfad warten Experimente mit einer Forscherfrage. Stufe für Stufe kommen neue Bauteile dazu."),
                   qsTr("Im freien Labor probierst du eigene Ideen aus."),
                   qsTr("Im Forscherheft stehen deine Vermutungen, Beobachtungen und Notizen.")] },
        { key: "stecken", title: qsTr("Bauteile stecken"), mood: "erklaerend",
          say: qsTr("Ziehe ein Bauteil aus dem Bauteilfach links auf die Grundplatte. Seine Beine landen in Kontaktfeldern."),
          points: [qsTr("Alles, was im selben Feld steckt, ist miteinander verbunden."),
                   qsTr("Module wie das Blinkmodul findest du im Modulfach. Sie gehören in die Modulleiste unten."),
                   qsTr("Ein Bauteil verschiebst du, indem du es ziehst. Tippst du zweimal schnell darauf, dreht es sich."),
                   qsTr("Halte ein Bauteil lange gedrückt (am Computer: Rechtsklick). Im Menü kannst du es drehen, einen Wert wählen oder entfernen.")] },
        { key: "leitungen", title: qsTr("Leitungen ziehen"), mood: "erklaerend",
          say: qsTr("Ziehe mit dem Finger von einem Feld zu einem anderen – schon liegt dort eine Leitung."),
          points: [qsTr("Leitungen an Plus werden rot, an Minus schwarz. Das kannst du in den Einstellungen abschalten."),
                   qsTr("Das Ende einer Leitung kannst du anfassen und umstecken."),
                   qsTr("Halte eine Leitung lange gedrückt, um ihre Farbe zu ändern oder sie zu entfernen.")] },
        { key: "pult", title: qsTr("Bedienpult und Batterie"), mood: "neugierig",
          say: qsTr("Oben liegt das Bedienpult. Die Batterie liefert 3, 4,5 oder 9 Volt."),
          points: [qsTr("Mit dem Hauptschalter an der Batterie schaltest du den Strom ein und aus."),
                   qsTr("Darunter liegen im Pult die Felder: die Pole der Batterie (V+ rot, 0V/GND schwarz), Potentiometer 1 und 2 (P1a, P1s, P1b …), der Schalter (S1, S2) und der Lautsprecher (LS1, LS2). Ein Rahmen fasst jedes Bauteil mit seinen Feldern ein; in den ersten Stufen zeigt das Pult nur, was gebraucht wird."),
                   qsTr("Oben links wechselst du zwischen »Stecken« und »Betrieb«. »Betrieb« schaltet den Hauptschalter ein: Jetzt fließt Strom und du kannst Taster und Schalter bedienen. Zum Umbauen geht es zurück auf »Stecken«.")] },
        { key: "rueckgaengig", title: qsTr("Rückgängig und Löschen"), mood: "ueberrascht",
          say: qsTr("Etwas ist schiefgegangen? Kein Problem – fast alles lässt sich zurückholen."),
          points: [qsTr("Der gebogene Pfeil nach links macht den letzten Schritt rückgängig, der Pfeil nach rechts holt ihn zurück."),
                   qsTr("Auch ein durchgebranntes Bauteil wird mit »Rückgängig« wieder heil."),
                   qsTr("Der Mülleimer unten rechts entfernt, was ausgewählt ist. Du kannst Bauteile auch auf ihn ziehen. Ist nichts ausgewählt, baut er alles ab – vorher fragt Volta nach.")] },
        { key: "messen", title: qsTr("Messen und Anzeigen"), mood: "neugierig",
          say: qsTr("Wie viel Strom fließt da eigentlich? Das kannst du nachmessen."),
          points: [qsTr("»V A Ω« holt das Messgerät: Spannung in Volt, Strom in Ampere, Widerstand in Ohm."),
                   qsTr("Der Knopf mit den grünen Balken zeigt auf allen belegten Feldern, wie viel Spannung dort anliegt. Für ein einzelnes Feld geht das auch im Menü des Feldes (langer Druck)."),
                   qsTr("Das Schaltzeichen-Symbol zeigt deinen Aufbau als Schaltplan."),
                   qsTr("Kleine Pfeile zeigen, wohin der Strom fließt. Ab Stufe 3 zeichnet der Zeitschreiber Spannungen über die Zeit auf.")] },
        { key: "werkzeug", title: qsTr("Werkzeugkasten"), mood: "erklaerend",
          say: qsTr("Im Werkzeugkasten liegen meine Helfer für dich. Wische über die Reiter oder tippe auf den blauen Pfeil, dann siehst du alle."),
          points: [qsTr("Farbcode: Was bedeuten die Ringe auf einem Widerstand?"),
                   qsTr("E-Reihen: Welche Widerstandswerte gibt es zu kaufen?"),
                   qsTr("Lexikon: Was macht welches Bauteil?"),
                   qsTr("Rechner: Formeln mit Rechenweg (ab Stufe 3)."),
                   qsTr("Fehlersuche: Ich prüfe deinen Aufbau (ab Stufe 2)."),
                   qsTr("Teileliste: Was du brauchst – auch als Aufbaukarte zum Ausdrucken.")] },
        { key: "experiment", title: qsTr("Experimente"), mood: "motivierend",
          say: qsTr("So läuft ein Experiment – du schaffst das!"),
          points: [qsTr("1. Lies die Frage und wähle deine Vermutung."),
                   qsTr("2. Baue auf. Mit »Schritt für Schritt bauen« zeige ich dir jedes Teil. Kommst du nicht weiter, gibt es Tipps."),
                   qsTr("3. Wenn alles steckt und funktioniert, tippe auf »Fertig«."),
                   qsTr("4. Wähle, was du beobachtet hast. Danach erkläre ich dir, was passiert ist."),
                   qsTr("Die Lautsprecher-Taste im Experimentfeld liest dir die Texte vor.")] },
        { key: "fehler", title: qsTr("Wenn etwas nicht klappt"), mood: "fragend",
          say: qsTr("Es leuchtet nichts? Dann frag mich!"),
          points: [qsTr("Öffne im Werkzeugkasten die Fehlersuche und tippe auf »Schaltung prüfen«. Ich stelle dir Fragen zu den Stellen, die mir auffallen."),
                   qsTr("Ist der Hauptschalter an? Führt ein Weg von V+ durch die Bauteile zurück nach 0V?"),
                   qsTr("Leuchtdioden haben eine Richtung: Das lange Bein kommt zu Plus.")] },
        { key: "sicher", title: qsTr("Sicher experimentieren"), mood: "warnend",
          say: qsTr("Mit echten Bauteilen gilt immer: Sicherheit zuerst!"),
          points: [qsTr("Experimentiere nur mit Batterien bis 9 Volt – niemals an der Steckdose!"),
                   qsTr("Versuche mit »Nur in der App!« machen ein Bauteil kaputt. Die probierst du nur hier aus."),
                   qsTr("Bauteile können heiß werden. Fass sie nicht an, solange Strom fließt.")] },
        { key: "einstellungen", title: qsTr("Oben rechts"), mood: "gut",
          say: qsTr("Oben rechts kannst du Volta so einstellen, wie es dir gefällt."),
          points: [qsTr("A− und A+ machen die Schrift kleiner oder größer."),
                   qsTr("Die Flagge wählt die Sprache."),
                   qsTr("Das Fragezeichen öffnet diese Hilfe. Im Labor findest du sie im Menü mit den drei Strichen. Das i zeigt Infos über Volta."),
                   qsTr("Das Menü mit den drei Strichen enthält Einstellungen wie Ton, Stromfluss und Animationen.")] }
    ]

    // ab Volta 2: Kapitel mit eigenem Teilbild
    readonly property var v2Images: ["willkommen", "stecken", "pult", "messen", "oszilloskop", "ton"]
    // ab Volta 2: geaenderte Teile einzelner Kapitel
    readonly property var v2Changes: ({
        willkommen: {
            say: qsTr("Hallo, ich bin Tick! Willkommen in Volta 2. Hier geht es um Wechselstrom, Transistoren, Verstärker und den Timer 555. Was passiert, rechnet Volta wirklich aus – sogar Töne, die du hören kannst.") },
        stecken: {
            points: [qsTr("Alles, was im selben Feld steckt, ist miteinander verbunden."),
                     qsTr("Module wie das OP-Modul und der Timer 555 liegen im Modulfach. Sie gehören in die Modulleiste unten."),
                     qsTr("Ein Bauteil verschiebst du, indem du es ziehst. Tippst du zweimal schnell darauf, dreht es sich."),
                     qsTr("Halte ein Bauteil lange gedrückt (am Computer: Rechtsklick). Im Menü kannst du es drehen, einen Wert wählen oder entfernen.")] },
        pult: {
            say: qsTr("Oben liegt das Bedienpult. Es zeigt nur, was du gerade brauchst."),
            points: [qsTr("Im Experiment stellt Volta das Pult für dich zusammen. Im freien Labor holst du dir Teile aus dem Gerätefach im Pult."),
                     qsTr("Batterie 1 hat die Felder V+ (rot) und 0V (schwarz). Batterie 2 hat eigene Felder +B2 und −B2. Steckst du beide hintereinander, bekommst du Plus, Minus und eine Mitte."),
                     qsTr("Die Wechselquelle (W~a, W~b) liefert Wechselspannung mit 50 Hz. Der Funktionsgenerator (G, G0) macht Sinus, Dreieck oder Rechteck. Tippe auf seinen Knopf, dann stellst du Frequenz und Spannung ein."),
                     qsTr("Der Kennlinienschreiber (KS+, KS−) zeichnet, wie viel Strom bei welcher Spannung durch ein Bauteil fließt."),
                     qsTr("Das »?« an einem Pultteil erklärt das Teil und seine Felder."),
                     qsTr("Oben links wechselst du zwischen »Stecken« und »Betrieb«. »Betrieb« schaltet den Hauptschalter ein: Jetzt fließt Strom und du kannst Taster und Schalter bedienen. Zum Umbauen geht es zurück auf »Stecken«.")] },
        messen: {
            say: qsTr("Wie viel Strom fließt da eigentlich? In Volta 2 steckst du die Messstelle direkt in die Schaltung."),
            points: [qsTr("Die Messstellen V, A und Ω liegen in der Gruppe »Messen«. Du steckst sie wie einen Widerstand auf die Grundplatte."),
                     qsTr("Das Voltmeter V kommt neben das Bauteil, das Amperemeter A in den Stromkreis. Das Ohmmeter Ω misst nur bei ausgeschaltetem Strom."),
                     qsTr("»V A Ω« holt das Messgerät. Es zeigt deine Messstellen an, bis zu drei auf einmal. »V~« oder »A~« heißt: Wechselgröße, angezeigt als Effektivwert."),
                     qsTr("Der Knopf mit den grünen Balken zeigt auf allen belegten Feldern, wie viel Spannung dort anliegt. Für ein einzelnes Feld geht das auch im Menü des Feldes (langer Druck)."),
                     qsTr("Das Schaltzeichen-Symbol zeigt deinen Aufbau als Schaltplan. Kleine Pfeile zeigen, wohin der Strom fließt.")] },
        werkzeug: {
            points: [qsTr("Farbcode: Was bedeuten die Ringe auf einem Widerstand?"),
                     qsTr("E-Reihen: Welche Widerstandswerte gibt es zu kaufen?"),
                     qsTr("Lexikon: Was macht welches Bauteil und welches Modul?"),
                     qsTr("Rechner: Formeln mit Rechenweg. Mit den Stufen kommen neue Rechenblätter dazu, zum Beispiel für den Timer 555."),
                     qsTr("Fehlersuche: Ich prüfe deinen Aufbau."),
                     qsTr("Teileliste: Was du brauchst – auch als Aufbaukarte zum Ausdrucken.")] }
    })
    // ab Volta 2: neue Kapitel, hinter »Messen und Anzeigen«
    readonly property var v2Extra: [
        { key: "oszilloskop", title: qsTr("Oszilloskop"), mood: "neugierig",
          say: qsTr("Das Oszilloskop zeigt dir, wie sich eine Spannung mit der Zeit verändert – auch wenn es ganz schnell geht."),
          points: [qsTr("Stecke den Tastkopf K1 aus der Gruppe »Messen« in ein Feld. Gemessen wird immer gegen 0V. Mit K2 siehst du eine zweite Spannung."),
                   qsTr("Der Knopf mit der Kurve in der Werkzeugleiste zeigt das Oszilloskop unter der Grundplatte."),
                   qsTr("K1 ist gelb und durchgezogen, K2 türkis und gestrichelt. Daneben stehen Spitze-Spitze und Frequenz als Zahl."),
                   qsTr("Mit den Pfeilen stellst du die Zeit je Kästchen und die Volt je Kästchen ein. Der Trigger hält das Bild still."),
                   qsTr("»Rollen« schreibt die Spannung langsam von rechts nach links mit. »Standbild« hält das Bild an.")] },
        { key: "ton", title: qsTr("Ton und Zeitlupe"), mood: "erklaerend",
          say: qsTr("Schwingt deine Schaltung schnell genug, hörst du sie aus dem Lautsprecher. Ist sie zu schnell zum Zuschauen, hilft die Zeitlupe."),
          points: [qsTr("Am Lautsprecher steht, woher der Ton kommt. »♪ gerechnet« heißt: Volta rechnet den Ton Stück für Stück aus."),
                   qsTr("»♪ Ersatzton« heißt: Dein Gerät ist zu langsam dafür. Dann spielt Volta einen Ton mit der gerechneten Frequenz."),
                   qsTr("Die Lupe mit der Uhr in der Werkzeugleiste schaltet die Zeitlupe: 1/10, 1/100 oder 1/1000 so schnell wie echt. Dann ist der Ton stumm."),
                   qsTr("Neben der Lupe steht, wie langsam es gerade läuft. Tippst du weiter, geht es zurück zur Echtzeit.")] }
    ]
    function v2Chapters() {
        const out = []
        for (const c of baseChapters) {
            const change = v2Changes[c.key]
            out.push(change ? Object.assign({}, c, change) : c)
            if (c.key === "messen")
                for (const e of v2Extra) out.push(e)
        }
        return out
    }

    property string pendingKey: ""
    readonly property string language: typeof volta_languageService !== "undefined" && volta_languageService
                                       ? volta_languageService.language : "de"
    function show(key) {
        pendingKey = key || ""
        open()
    }
    function jumpTo(key) {
        for (let i = 0; i < sections.count; ++i) {
            const s = sections.itemAt(i)
            if (s && s.chapter.key === key) {
                flick.contentY = Math.min(Math.max(0, s.y - 8), Math.max(0, flick.contentHeight - flick.height))
                return
            }
        }
        if (key === "eltern") flick.contentY = Math.max(0, flick.contentHeight - flick.height)
    }
    // erst springen, wenn das Fenster steht und die Kapitel angeordnet sind
    onAboutToShow: flick.contentY = 0
    // (Zeilenumbrueche haengen von der endgueltigen Breite ab: kurz warten)
    onOpened: if (pendingKey !== "") jumpTimer.restart()
    Timer {
        id: jumpTimer
        interval: 150
        onTriggered: { help.jumpTo(help.pendingKey); help.pendingKey = "" }
    }

    // Kopf mit Tick
    Rectangle {
        id: head
        width: parent.width
        height: 64
        radius: 12
        color: Theme.navy
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 12; color: Theme.navy }
        Row {
            x: 16
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10
            Tick { visible: Theme.tick; mood: "freundlich"; width: 52; height: 52; anchors.verticalCenter: parent.verticalCenter }
            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Hilfe")
                color: "white"
                font.pixelSize: Math.round(20 * Theme.fontScale)
                font.bold: true
            }
        }
        ToolButton {
            objectName: "helpClose"
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            width: Theme.control; height: Theme.control
            contentItem: Item { DrawnIcon { anchors.centerIn: parent; kind: "close"; color: "white"; size: 20 } }
            onClicked: help.close()
            Accessible.name: qsTr("Schließen")
        }
    }

    Flickable {
        id: flick
        anchors { left: parent.left; right: parent.right; top: head.bottom; bottom: parent.bottom }
        anchors.margins: 16
        contentHeight: col.implicitHeight + 16
        clip: true
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
        Behavior on contentY { enabled: !Theme.reduceMotion; NumberAnimation { duration: 250; easing.type: Easing.OutCubic } }

        Column {
            id: col
            width: flick.width - 12
            spacing: 14

            // Kapitelknoepfe
            Flow {
                width: parent.width
                spacing: 8
                Repeater {
                    model: help.chapters
                    Button {
                        required property var modelData
                        objectName: "helpChip_" + modelData.key
                        height: Theme.control
                        text: modelData.title
                        font.pixelSize: Math.round(14 * Theme.fontScale)
                        onClicked: help.jumpTo(modelData.key)
                    }
                }
                Button {
                    objectName: "helpChip_eltern"
                    height: Theme.control
                    text: qsTr("Für Erwachsene")
                    font.pixelSize: Math.round(14 * Theme.fontScale)
                    onClicked: help.jumpTo("eltern")
                }
            }

            Repeater {
                id: sections
                model: help.chapters
                Rectangle {
                    id: section
                    required property var modelData
                    required property int index
                    readonly property var chapter: modelData
                    objectName: "helpSection_" + modelData.key
                    width: col.width
                    height: sectionCol.implicitHeight + 28
                    radius: 12
                    color: index % 2 === 0 ? "#FFFFFF" : "#F7F2E8"
                    border.color: Theme.fieldOutline
                    Column {
                        id: sectionCol
                        x: 16; y: 14
                        width: parent.width - 32
                        spacing: 10
                        Label {
                            text: section.modelData.title
                            font.pixelSize: Math.round(19 * Theme.fontScale)
                            font.bold: true
                            color: Theme.navy
                        }
                        TickSays {
                            width: parent.width
                            figure: 92
                            mood: section.modelData.mood
                            text: section.modelData.say
                            bubbleColor: section.modelData.mood === "warnend" ? "#FFE9DC" : "#EEF5FF"
                            borderColor: section.modelData.mood === "warnend" ? Theme.signalOrange : Theme.accent
                        }
                        // Teilbild der Oberflaeche mit markierten Knoepfen (nicht bei »Sicher experimentieren«)
                        Image {
                            objectName: "helpImage_" + section.modelData.key
                            visible: section.modelData.key !== "sicher"
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: Math.min(parent.width, 560)
                            height: visible ? width * 300 / 760 : 0
                            fillMode: Image.PreserveAspectFit
                            asynchronous: true
                            source: !visible ? "" : (help.v2 && help.v2Images.indexOf(section.modelData.key) >= 0 ? "qrc:/help2/" : "qrc:/help/")
                                                    + section.modelData.key + "_" + help.language + ".jpg"
                            Accessible.role: Accessible.Graphic
                            Accessible.name: section.modelData.title
                        }
                        Repeater {
                            model: section.modelData.points
                            Label {
                                required property string modelData
                                width: sectionCol.width
                                leftPadding: 18
                                wrapMode: Text.Wrap
                                // Nummerierte Punkte (»1. ...«) ohne zusaetzlichen Aufzaehlungspunkt
                                text: (/^[0-9]+\./.test(modelData) ? "" : "•  ") + modelData
                                font.pixelSize: Math.round(15 * Theme.fontScale)
                                lineHeight: 1.1
                                color: "#1E2B38"
                            }
                        }
                    }
                }
            }

            // Fuer Erwachsene: Verweis in die Einstellungen (PIN)
            Rectangle {
                objectName: "helpSection_eltern"
                width: col.width
                height: adultCol.implicitHeight + 28
                radius: 12
                color: "#EEF1F5"
                border.color: Theme.navy
                Column {
                    id: adultCol
                    x: 16; y: 14
                    width: parent.width - 32
                    spacing: 10
                    Label {
                        text: qsTr("Für Eltern und Lehrkräfte")
                        font.pixelSize: Math.round(19 * Theme.fontScale)
                        font.bold: true
                        color: Theme.navy
                    }
                    Label {
                        width: parent.width
                        wrapMode: Text.Wrap
                        font.pixelSize: Math.round(15 * Theme.fontScale)
                        color: "#1E2B38"
                        text: qsTr("Volta braucht kein Internet, legt kein Konto an und sammelt keine Daten. Datenschutz, Freigaben, Forscherprofile, Ton im Klassenzimmer, die PIN und die Erklärung, wie Volta rechnet, finden Sie in den Einstellungen. Sie sind durch eine PIN geschützt.")
                    }
                    Label {
                        width: parent.width
                        wrapMode: Text.Wrap
                        font.pixelSize: Math.round(15 * Theme.fontScale)
                        color: "#1E2B38"
                        text: qsTr("Am Computer: Strg+Z macht rückgängig, Strg+Y wiederholt, Entf entfernt die Auswahl. Mit der Maus öffnet ein Rechtsklick das Menü.")
                    }
                    Button {
                        objectName: "helpParentArea"
                        height: Theme.control
                        text: qsTr("Einstellungen öffnen")
                        font.bold: true
                        onClicked: { help.close(); help.parentAreaRequested() }
                    }
                }
            }
        }
    }
}
