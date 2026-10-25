import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Hilfe mit Zetti wie bei Volta: Kapitel fuer Kinder, jedes mit Zetti in passender Stimmung und kurzen Punkten;
// oben Kapitelknoepfe zum Springen. Der letzte Teil richtet sich an Eltern und Lehrkraefte.
Popup {
    id: help
    objectName: "helpDialog"
    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(820, parent ? parent.width - 32 : 820)
    height: parent ? parent.height - 32 : 700
    padding: 0
    closePolicy: Popup.CloseOnEscape
    background: Rectangle { radius: 12; color: "#FFFDF7"; border.color: ZTheme.navy; border.width: 2 }

    // Kapitel: Titel, Bild von Zetti, was Zetti sagt, Punkte
    readonly property var chapters: [
        { key: "willkommen", title: qsTr("Willkommen"), pose: "freundlich",
          say: qsTr("Hallo, ich bin Zetti! In Zuse 1 findest du heraus, wie ein Rechner denkt – vom Lichtschalter bis zur Rechenmaschine. Was die Lampen zeigen, rechnet Zuse wirklich aus."),
          points: [qsTr("Im Lernpfad warten Experimente in sechs Stufen. Jedes Experiment beginnt mit einer Frage aus dem Alltag."),
                   qsTr("Ein Experiment öffnet sich, wenn du das vorige geschafft hast – denn deine Werkzeuge bauen aufeinander auf."),
                   qsTr("Im freien Labor probierst du eigene Ideen aus, in der Vorführung siehst du fertige Lösungen.")] },
        { key: "kisten", title: qsTr("Rätselkisten"), pose: "nachdenklich",
          say: qsTr("In einer Rätselkiste steckt eine Regel. Welche? Das findest du durch Ausprobieren heraus."),
          points: [qsTr("Vermute zuerst: Lies jede Situation und tippe an, ob die Lampe leuchtet oder aus bleibt."),
                   qsTr("Setz eine Kiste ein und tippe die Situationen an – die Schalter stellen sich von selbst. Jede Karte zeigt, ob es so kam, wie du vermutet hast."),
                   qsTr("Passt eine Kiste zu der Geschichte, tippe auf »Diese Kiste passt!«. Die Regel kommt als Werkzeug in deine Werkzeugkiste."),
                   qsTr("Danach erklärt dir Zetti die Regel unter »Erklärung« mit einem kleinen Comic – auch in der Werkzeugkiste.")] },
        { key: "bauen", title: qsTr("Auf der Werkbank bauen"), pose: "bauen",
          say: qsTr("Links liegen deine Werkzeuge. Ziehe eines auf die Werkbank – oder tippe es an, dann landet es in der Mitte."),
          points: [qsTr("Ziehe von einem goldenen Anschluss zu einem anderen, schon liegt dort eine Leitung."),
                   qsTr("Leitungen mit 1 leuchten blau, Leitungen mit 0 sind grau gestrichelt."),
                   qsTr("Tippe einen Baustein oder eine Leitung an und dann auf »Entfernen«. »Leeren« räumt die ganze Werkbank ab."),
                   qsTr("Die gebogenen Pfeile oben machen den letzten Schritt rückgängig oder holen ihn zurück.")] },
        { key: "pruefen", title: qsTr("Prüfen und einpacken"), pose: "zeigend",
          say: qsTr("Fertig gebaut? Dann prüfe deine Schaltung!"),
          points: [qsTr("»Prüfen« stellt alle Schalterstellungen ein und vergleicht mit der Wahrheitstabelle. Falsche Zeilen werden markiert."),
                   qsTr("Bei Speichern und Zählern prüft Zuse einen Ablauf: Schritt für Schritt, wie du die Schalter und Taster bedienen würdest."),
                   qsTr("Stimmt alles, packst du deine Erfindung ein. Sie wird selbst zum Werkzeug, mit einem ★."),
                   qsTr("Tippe zweimal auf eine ★-Erfindung, dann schaust du hinein – bis zum einzelnen Gatter.")] },
        { key: "bedienfeld", title: qsTr("Schalter, Taster, Taktgeber"), pose: "erklaerend",
          say: qsTr("Links auf dem Bedienfeld sind die Eingänge, rechts die Lampen."),
          points: [qsTr("Ein Kippschalter bleibt, wie du ihn stellst. Ein Taster ist nur an, solange du drückst."),
                   qsTr("Der Taktgeber schaltet von allein im Takt um. Tippe ihn an, um ihn anzuhalten oder weiterlaufen zu lassen."),
                   qsTr("Neben jedem Schalter und jeder Lampe steht der Wert als 1 oder 0.")] },
        { key: "zahlen", title: qsTr("Zahlen: binär, dezimal, hex"), pose: "zeigend",
          say: qsTr("Ein Rechner schreibt Zahlen nur mit 0 und 1. Die Zahlenleiste unter der Werkbank übersetzt für dich."),
          points: [qsTr("Jede Stelle hat einen Wert: 8, 4, 2, 1. Leuchten 4 und 1, ist das 5."),
                   qsTr("Die Zahlenleiste zeigt Schalter und Lampen binär, dezimal und hexadezimal. Tippe auf »Warum?«, dann erkläre ich es."),
                   qsTr("Die Leiste schaltest du mit »0b 0x« oben oder im Menü ein und aus.")] },
        { key: "nichtweiter", title: qsTr("Wenn es nicht klappt"), pose: "gruebelnd",
          say: qsTr("Hängst du fest? Das gehört zum Forschen dazu!"),
          points: [qsTr("Tippe auf »Tipp«. Ich gebe dir einen Hinweis, ohne die Lösung zu verraten."),
                   qsTr("Spiel die markierten Zeilen der Tabelle an den Schaltern nach und schau, welche Leitung nicht stimmt."),
                   qsTr("Ganz zum Schluss hilft »Lösung zeigen«. Bau sie danach gern selbst nach.")] },
        { key: "forscherheft", title: qsTr("Forscherheft"), pose: "erklaerend",
          say: qsTr("Im Forscherheft hältst du fest, was du vermutet und herausgefunden hast."),
          points: [qsTr("Ich schreibe deine Vermutung hinein und nach dem Lösen, wie es wirklich war – auch, wenn es anders kam."),
                   qsTr("Mit »Notiz schreiben« fügst du eigene Gedanken hinzu, mit »Foto hinzufügen« ein Bild von deinem echten Aufbau."),
                   qsTr("Du öffnest das Heft auf der Startseite, im Experiment mit 📓 oben oder im Menü. Es bleibt auf deinem Gerät.")] },
        { key: "oben", title: qsTr("Oben rechts"), pose: "froehlich",
          say: qsTr("Oben rechts stellst du Zuse so ein, wie es dir gefällt."),
          points: [qsTr("A− und A+ machen die Schrift kleiner oder größer."),
                   qsTr("Die Flagge wählt die Sprache: Deutsch, Français oder English."),
                   qsTr("Das Fragezeichen öffnet diese Hilfe, das Blatt die Kurzanleitung. Im Experiment findest du beide im Menü mit den drei Strichen."),
                   qsTr("Das i zeigt Infos über Zuse 1. Im Menü schaltest du die Zahlenleiste und die Schaltzeichen um.")] },
        { key: "erwachsene", title: qsTr("Für Eltern und Lehrkräfte"), pose: "erklaerend",
          say: qsTr("Zuse 1 richtet sich an Kinder von 10 bis 14 Jahren (Klasse 5 bis 8)."),
          points: [qsTr("Der Lernpfad führt in sechs Stufen von Schalter und Lampe über Logik, Binärzahlen und Addierer bis zu Speicher, Zähler und einer von Hand gesteuerten Rechenmaschine."),
                   qsTr("Das Programm im Speicher – Konrad Zuses große Idee – ist Thema von Zuse 2."),
                   qsTr("Alles, was die Lampen zeigen, berechnet eine echte Logiksimulation. Vereinfachung: Speicher starten beim Einschalten mit 0; echte Bausteine starten zufällig, deshalb gibt es »Löschen«."),
                   qsTr("Im Elternbereich (Menü mit den drei Strichen, 🔒, Start-PIN 1234) geben Sie alle Experimente frei, richten Forscherprofile für Geschwister oder die Klasse ein und setzen Fortschritt oder Forscherheft zurück."),
                   qsTr("Aufgabe für die Klasse: Im Elternbereich Experimente wählen, Zuse zeigt einen QR-Code und einen Code zum Eintippen. Die Kinder geben ihn auf der Startseite unter »Aufgabe« ein; die Experimente sind dann sofort offen."),
                   qsTr("Zuse 1 erhebt keine Daten und braucht kein Internet. Fortschritt und Erfindungen bleiben auf dem Gerät.")] }
    ]

    function show(key) {
        open()
        if (key) jumpTimer.key = key, jumpTimer.restart()
    }
    function jumpTo(key) {
        for (let i = 0; i < sections.count; ++i) {
            const s = sections.itemAt(i)
            if (s && s.chapter.key === key) {
                flick.contentY = Math.min(Math.max(0, s.y - 8), Math.max(0, flick.contentHeight - flick.height))
                return
            }
        }
    }
    onAboutToShow: flick.contentY = 0
    Timer { // Zeilenumbrueche haengen von der endgueltigen Breite ab: kurz warten
        id: jumpTimer
        property string key
        interval: 150
        onTriggered: help.jumpTo(key)
    }

    Rectangle {
        id: head
        width: parent.width
        height: 64
        radius: 12
        color: ZTheme.navy
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 12; color: ZTheme.navy }
        Row {
            x: 16
            anchors.verticalCenter: parent.verticalCenter
            spacing: 12
            HelpIcon { size: 32; anchors.verticalCenter: parent.verticalCenter }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Hilfe")
                color: "white"
                font.pixelSize: Math.round(20 * ZTheme.fontScale); font.bold: true
            }
        }
        ZButton {
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            text: "✕"
            onClicked: help.close()
            Accessible.name: qsTr("Schließen")
        }
    }

    Flow { // Kapitelknoepfe
        id: tabs
        anchors { top: head.bottom; left: parent.left; right: parent.right; margins: 12 }
        spacing: 8
        Repeater {
            model: help.chapters
            ZButton {
                required property var modelData
                height: ZTheme.control
                text: modelData.title
                font.pixelSize: Math.round(14 * ZTheme.fontScale)
                onClicked: help.jumpTo(modelData.key)
            }
        }
    }

    Flickable {
        id: flick
        anchors { top: tabs.bottom; left: parent.left; right: parent.right; bottom: parent.bottom; margins: 16 }
        contentHeight: column.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
        Column {
            id: column
            width: flick.width - 12
            spacing: 22
            Repeater {
                id: sections
                model: help.chapters
                delegate: RowLayout {
                    required property var modelData
                    readonly property var chapter: modelData
                    width: column.width
                    spacing: 14
                    Image {
                        Layout.alignment: Qt.AlignTop
                        Layout.preferredWidth: 72; Layout.preferredHeight: 92
                        source: "qrc:/zuse/zetti/zetti_" + modelData.pose + ".png"
                        fillMode: Image.PreserveAspectFit
                        mipmap: true
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        Text {
                            Layout.fillWidth: true
                            text: modelData.title
                            color: ZTheme.navy
                            font.pixelSize: Math.round(19 * ZTheme.fontScale); font.bold: true
                            wrapMode: Text.WordWrap
                        }
                        Text {
                            Layout.fillWidth: true
                            text: modelData.say
                            color: ZTheme.text
                            font.pixelSize: Math.round(15 * ZTheme.fontScale); font.italic: true
                            wrapMode: Text.WordWrap
                        }
                        Repeater {
                            model: modelData.points
                            Text {
                                required property string modelData
                                Layout.fillWidth: true
                                text: "•  " + modelData
                                color: ZTheme.text
                                font.pixelSize: Math.round(15 * ZTheme.fontScale)
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }
            }
        }
    }
}
