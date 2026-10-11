// Hilfe mit Tick (docs/ui-spec.md, Interaktives Readme; docs/maskottchen-tick.md): Kapitel fuer Kinder, jedes mit
// Tick in passender Stimmung und kurzen Punkten. Oben Kapitelknoepfe zum Springen. Der Teil fuer Erwachsene verweist
// in den Elternbereich (hinter der PIN), statt Inhalte doppelt zu fuehren.
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

    // Kapitel: Titel, Stimmung von Tick, was Tick sagt, Punkte
    readonly property var chapters: [
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
                   qsTr("Die Pultleiste darunter führt zu den Polen der Batterie (+9V und 0V), zu den Drehreglern P1 und P2, zum Schalter S und zum Lautsprecher LS."),
                   qsTr("Oben links wechselst du zwischen »Stecken« und »Betrieb«. »Betrieb« schaltet den Hauptschalter ein: Jetzt fließt Strom und du kannst Taster und Schalter bedienen. Zum Umbauen geht es zurück auf »Stecken«.")] },
        { key: "rueckgaengig", title: qsTr("Rückgängig und Löschen"), mood: "ueberrascht",
          say: qsTr("Etwas ist schiefgegangen? Kein Problem – fast alles lässt sich zurückholen."),
          points: [qsTr("Der gebogene Pfeil nach links macht den letzten Schritt rückgängig, der Pfeil nach rechts holt ihn zurück."),
                   qsTr("Auch ein durchgebranntes Bauteil wird mit »Rückgängig« wieder heil."),
                   qsTr("Der Mülleimer unten rechts entfernt, was ausgewählt ist. Du kannst Bauteile auch auf ihn ziehen. Ist nichts ausgewählt, baut er alles ab – vorher fragt Volta nach.")] },
        { key: "messen", title: qsTr("Messen und Anzeigen"), mood: "neugierig",
          say: qsTr("Wie viel Strom fließt da eigentlich? Das kannst du nachmessen."),
          points: [qsTr("»V A Ω« holt das Messgerät: Spannung in Volt, Strom in Ampere, Widerstand in Ohm."),
                   qsTr("Im Menü eines Feldes (langer Druck) schaltest du dort eine Spannungsanzeige ein."),
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
                   qsTr("Ist der Hauptschalter an? Führt ein Weg von +9V durch die Bauteile zurück nach 0V?"),
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

    property string pendingKey: ""
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

            // Fuer Erwachsene: Verweis in den Elternbereich (PIN)
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
                        text: qsTr("Volta braucht kein Internet, legt kein Konto an und sammelt keine Daten. Datenschutz, Freigaben, Forscherprofile, Ton im Klassenzimmer, die PIN und die Erklärung, wie Volta rechnet, finden Sie im Elternbereich. Er ist durch eine PIN geschützt.")
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
                        text: qsTr("Elternbereich öffnen")
                        font.bold: true
                        onClicked: { help.close(); help.parentAreaRequested() }
                    }
                }
            }
        }
    }
}
