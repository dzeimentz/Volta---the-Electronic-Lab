import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Startseite wie Volta 1: Zetti begruesst, darunter die Karten Weitermachen, Freies Labor und Vorfuehrung, dann der
// Lernpfad mit den Lernstufen (Leitfrage, Fortschritt) und ihren Experimenten. Gesperrtes traegt ein Schloss, Geschafftes
// einen Haken (nie nur ueber die Farbe).
Rectangle {
    id: home
    color: ZTheme.graphite

    signal experimentRequested(int index)
    signal freeLabRequested()
    signal demoRequested()
    signal journalRequested()

    // Karte oben (Weitermachen, Freies Labor, Vorfuehrung)
    component Tile: Rectangle {
        id: tile
        property string icon
        property string title
        property string subtitle
        signal activated()
        width: 300
        height: 104
        radius: 14
        color: tap.pressed ? "#D9F3F8" : ZTheme.surface
        border.width: 2
        border.color: ZTheme.navy
        RowLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 14
            Text { text: tile.icon; font.pixelSize: Math.round(32 * ZTheme.fontScale); font.family: "Segoe UI Emoji" }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Text {
                    Layout.fillWidth: true
                    text: tile.title
                    color: ZTheme.navy
                    font.pixelSize: Math.round(20 * ZTheme.fontScale); font.bold: true
                    elide: Text.ElideRight
                }
                Text {
                    Layout.fillWidth: true
                    text: tile.subtitle
                    color: ZTheme.muted
                    font.pixelSize: Math.round(13 * ZTheme.fontScale)
                    wrapMode: Text.WordWrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                }
            }
        }
        TapHandler { id: tap; onTapped: tile.activated() }
        Accessible.role: Accessible.Button
        Accessible.name: title
    }

    Flickable {
        anchors.fill: parent
        contentHeight: page.implicitHeight + 48
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        ColumnLayout {
            id: page
            x: 24
            y: 24
            width: parent.width - 48
            spacing: 18

            // ---- Zetti begruesst
            RowLayout {
                Layout.fillWidth: true
                spacing: 14
                Image {
                    Layout.preferredWidth: 96
                    Layout.preferredHeight: 120
                    source: "qrc:/zuse/zetti/zetti_freundlich.png"
                    fillMode: Image.PreserveAspectFit
                    mipmap: true
                }
                Rectangle {
                    Layout.preferredWidth: Math.min(620, page.width - 130)
                    Layout.preferredHeight: greeting.implicitHeight + 24
                    radius: 14
                    color: ZTheme.bubble
                    border.width: 2
                    border.color: ZTheme.accentDim
                    Text {
                        id: greeting
                        anchors { left: parent.left; right: parent.right; top: parent.top; margins: 12 }
                        wrapMode: Text.WordWrap
                        color: ZTheme.text
                        font.pixelSize: Math.round(15 * ZTheme.fontScale)
                        text: workbench.currentProfile.id !== undefined
                              ? qsTr("Hallo %1! Schön, dass du wieder forschst. Mach im Lernpfad weiter oder probiere "
                                     + "im freien Labor eigene Ideen aus.").arg(workbench.currentProfile.name)
                              : qsTr("Hallo! Ich bin Zetti. Ich zeige dir, wie ein Rechner denkt – vom Lichtschalter bis "
                                     + "zur Rechenmaschine. Fang beim Lernpfad an oder probiere im freien Labor eigene "
                                     + "Ideen aus.")
                    }
                }
                Item { Layout.fillWidth: true }
                Rectangle { // Forscherprofil: Symbol und Spitzname, Antippen wechselt das Profil
                    objectName: "profileChip"
                    visible: workbench.currentProfile.id !== undefined
                    Layout.alignment: Qt.AlignTop
                    Layout.preferredWidth: chipRow.implicitWidth + 28
                    Layout.preferredHeight: ZTheme.control + 8
                    radius: height / 2
                    color: chipTap.pressed ? "#D9F3F8" : ZTheme.surface
                    border.width: 2
                    border.color: ZTheme.navy
                    RowLayout {
                        id: chipRow
                        anchors.centerIn: parent
                        spacing: 8
                        Text { text: workbench.currentProfile.symbol ?? ""; font.pixelSize: 26; font.family: "Segoe UI Emoji" }
                        ColumnLayout {
                            spacing: 0
                            Text {
                                text: workbench.currentProfile.name ?? ""
                                color: ZTheme.navy
                                font.pixelSize: Math.round(16 * ZTheme.fontScale); font.bold: true
                            }
                            Text {
                                text: qsTr("Profil wechseln")
                                color: ZTheme.muted
                                font.pixelSize: Math.round(12 * ZTheme.fontScale)
                            }
                        }
                    }
                    TapHandler { id: chipTap; onTapped: workbench.leaveProfile() }
                    Accessible.role: Accessible.Button
                    Accessible.name: qsTr("Profil wechseln")
                }
            }

            // ---- Karten
            Flow {
                Layout.fillWidth: true
                spacing: 18
                Tile {
                    objectName: "tileContinue"
                    icon: "▶️"
                    title: qsTr("Weitermachen")
                    subtitle: workbench.resume.label !== undefined
                              ? workbench.resume.label + " " + workbench.resume.title : ""
                    onActivated: home.experimentRequested(workbench.resume.index)
                }
                Tile {
                    objectName: "tileFreeLab"
                    icon: "🔧"
                    title: qsTr("Freies Labor")
                    subtitle: qsTr("Eigene Ideen ausprobieren – mit allen deinen Werkzeugen")
                    onActivated: home.freeLabRequested()
                }
                Tile {
                    objectName: "tileJournal"
                    icon: "📓"
                    title: qsTr("Forscherheft")
                    subtitle: workbench.journal.length === 0 ? qsTr("Deine Vermutungen, Entdeckungen und Notizen")
                              : qsTr("Vermutungen, Entdeckungen und Notizen (%1)").arg(workbench.journal.length)
                    onActivated: home.journalRequested()
                }
                Tile {
                    objectName: "tileDemo"
                    visible: settings.demoVisible
                    icon: "🎬"
                    title: qsTr("Vorführung")
                    subtitle: qsTr("%1 fertige Aufbauten zum Zeigen – es wird nichts gespeichert").arg(workbench.demos.length)
                    onActivated: home.demoRequested()
                }
            }

            Text {
                Layout.topMargin: 6
                text: qsTr("Lernpfad")
                color: ZTheme.navy
                font.pixelSize: Math.round(24 * ZTheme.fontScale); font.bold: true
            }

            // ---- Lernstufen
            Repeater {
                model: workbench.levels
                delegate: Rectangle {
                    id: level
                    required property var modelData
                    readonly property int doneCount: modelData.experiments.filter(e => e.done).length
                    Layout.fillWidth: true
                    Layout.preferredHeight: levelCol.implicitHeight + 28
                    radius: 14
                    color: modelData.unlocked ? ZTheme.surface : ZTheme.surfaceHigh
                    border.color: ZTheme.line
                    ColumnLayout {
                        id: levelCol
                        x: 16
                        y: 14
                        width: parent.width - 32
                        spacing: 10
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12
                            Rectangle {
                                Layout.preferredWidth: 44; Layout.preferredHeight: 44
                                radius: 22
                                color: level.modelData.done ? ZTheme.ok
                                     : level.modelData.unlocked ? ZTheme.navy : ZTheme.zero
                                Text {
                                    anchors.centerIn: parent
                                    text: level.modelData.done ? "✓"
                                        : level.modelData.unlocked || level.modelData.planned ? level.modelData.number : "🔒"
                                    color: "white"
                                    font.pixelSize: Math.round(20 * ZTheme.fontScale); font.bold: true
                                    font.family: text === "🔒" ? "Segoe UI Emoji" : ""
                                }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 0
                                Text {
                                    Layout.fillWidth: true
                                    text: qsTr("Stufe %1: %2").arg(level.modelData.number).arg(level.modelData.title)
                                    color: ZTheme.text
                                    font.pixelSize: Math.round(19 * ZTheme.fontScale); font.bold: true
                                    wrapMode: Text.WordWrap
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: level.modelData.question
                                    color: ZTheme.muted
                                    font.pixelSize: Math.round(14 * ZTheme.fontScale); font.italic: true
                                    wrapMode: Text.WordWrap
                                }
                            }
                            ColumnLayout { // Fortschritt als Zahl und Balken
                                visible: level.modelData.experiments.length > 0
                                spacing: 4
                                Text {
                                    Layout.alignment: Qt.AlignRight
                                    text: qsTr("%1 von %2 geschafft").arg(level.doneCount)
                                                                     .arg(level.modelData.experiments.length)
                                    color: ZTheme.muted
                                    font.pixelSize: Math.round(13 * ZTheme.fontScale)
                                }
                                Rectangle {
                                    Layout.preferredWidth: 190; Layout.preferredHeight: 8
                                    radius: 4
                                    color: ZTheme.surfaceHigh
                                    Rectangle {
                                        width: parent.width * level.doneCount / Math.max(1, level.modelData.experiments.length)
                                        height: parent.height
                                        radius: 4
                                        color: ZTheme.ok
                                    }
                                }
                            }
                        }
                        Text {
                            visible: !level.modelData.unlocked && !level.modelData.planned
                            text: qsTr("Noch gesperrt – schaffe zuerst die Experimente der Stufe %1.")
                                  .arg(level.modelData.number - 1)
                            color: ZTheme.muted
                            font.pixelSize: Math.round(14 * ZTheme.fontScale)
                        }
                        Flow { // Experimente der Stufe, danach geplante mit »folgt«
                            Layout.fillWidth: true
                            spacing: 12
                            Repeater {
                                model: level.modelData.experiments
                                delegate: Rectangle {
                                    id: card
                                    required property var modelData
                                    width: 280
                                    height: 72
                                    radius: 10
                                    color: !modelData.unlocked ? ZTheme.surfaceHigh
                                         : modelData.done ? ZTheme.okBg : cardTap.pressed ? "#D9F3F8" : "#F3FAFC"
                                    border.width: 2
                                    border.color: modelData.done ? ZTheme.ok : modelData.unlocked ? ZTheme.accent : ZTheme.line
                                    opacity: modelData.unlocked ? 1 : 0.6
                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 12
                                        anchors.rightMargin: 12
                                        spacing: 10
                                        Text {
                                            text: card.modelData.label
                                            color: ZTheme.muted
                                            font.pixelSize: Math.round(13 * ZTheme.fontScale); font.bold: true
                                        }
                                        Text {
                                            Layout.fillWidth: true
                                            text: card.modelData.title
                                            color: ZTheme.text
                                            font.pixelSize: Math.round(15 * ZTheme.fontScale); font.bold: true
                                            wrapMode: Text.WordWrap
                                            maximumLineCount: 2
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            text: card.modelData.done ? "✓" : card.modelData.unlocked ? "›" : "🔒"
                                            color: card.modelData.done ? ZTheme.ok : ZTheme.navy
                                            font.pixelSize: Math.round(20 * ZTheme.fontScale); font.bold: true
                                            font.family: text === "🔒" ? "Segoe UI Emoji" : ""
                                        }
                                    }
                                    TapHandler {
                                        id: cardTap
                                        enabled: card.modelData.unlocked
                                        onTapped: home.experimentRequested(card.modelData.index)
                                    }
                                    Accessible.role: Accessible.Button
                                    Accessible.name: card.modelData.label + " " + card.modelData.title
                                                     + (card.modelData.done ? ", " + qsTr("geschafft") : "")
                                }
                            }
                            Repeater {
                                model: level.modelData.upcoming
                                delegate: Rectangle {
                                    required property var modelData
                                    width: 280
                                    height: 72
                                    radius: 10
                                    color: ZTheme.surfaceHigh
                                    border.color: ZTheme.line
                                    opacity: 0.6
                                    Text {
                                        anchors.fill: parent
                                        anchors.margins: 12
                                        verticalAlignment: Text.AlignVCenter
                                        text: parent.modelData + " – " + qsTr("folgt")
                                        color: ZTheme.text
                                        font.pixelSize: Math.round(15 * ZTheme.fontScale)
                                        wrapMode: Text.WordWrap
                                    }
                                }
                            }
                        }
                        Text { // Ausblick am Ende der Stufe (z. B. auf Zuse 2)
                            visible: level.modelData.outlook !== ""
                            Layout.fillWidth: true
                            text: "→ " + level.modelData.outlook
                            color: ZTheme.muted
                            font.pixelSize: Math.round(14 * ZTheme.fontScale); font.italic: true
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }
        }
    }
}
