// Startseite (docs/ui-spec.md, Startseite): Kacheln Freies Labor und Forscherheft, darunter der Lernpfad mit seinen
// Stufen (Leitfrage, Fortschritt, Freischaltung) und den Experimenten als Karten. Oben rechts Sprache, Info und
// Einstellungen wie im Labor. Die Freischaltung dient der Reduktion, nicht der Belohnung (Konzept 5.5).
import QtQuick
import QtQuick.Controls.Basic

Rectangle {
    id: home
    required property var controller
    property var languageService: null
    property var settings: null
    signal experimentRequested(string id)
    signal freeLabRequested()
    signal continueRequested()
    signal profileSwitchRequested() // Forscherprofil wechseln // zurueck ins Labor mit dem aktuellen (gespeicherten) Stand
    signal journalRequested()
    signal infoRequested()
    signal helpRequested()
    signal taskRequested()
    signal parentAreaRequested()

    color: Theme.board

    // ---------------------------------------------------------------- Kopf
    Rectangle {
        id: header
        anchors { left: parent.left; right: parent.right; top: parent.top }
        height: Theme.control + 8
        color: Theme.desk
        Row {
            anchors.left: parent.left
            anchors.leftMargin: Theme.margin
            anchors.verticalCenter: parent.verticalCenter
            spacing: 12
            Image {
                anchors.verticalCenter: parent.verticalCenter
                source: "qrc:/icons/volta_64.png"
                width: 40; height: 40
            }
            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: home.controller.appInfo().name
                color: "white"
                font.pixelSize: Math.round((20) * Theme.fontScale)
                font.bold: true
            }
        }
        // Forscherprofil: Symbol und Spitzname; Antippen fuehrt zur Profilauswahl
        Button {
            id: profileBadge
            objectName: "profileBadge"
            visible: home.controller.profilesEnabled && home.controller.currentProfile.id !== undefined
            anchors.right: topActions.left
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            height: Theme.control
            text: (home.controller.currentProfile.symbol || "") + "  " + (home.controller.currentProfile.name || "")
            font.pixelSize: Math.round(15 * Theme.fontScale)
            font.bold: true
            onClicked: home.profileSwitchRequested()
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Profil wechseln")
            Accessible.name: qsTr("Profil wechseln: %1").arg(home.controller.currentProfile.name || "")
        }
        TopBarActions {
            id: topActions
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            controller: home.controller
            languageService: home.languageService
            settings: home.settings
            onInfoRequested: home.infoRequested()
            onHelpRequested: home.helpRequested()
            onParentAreaRequested: home.parentAreaRequested()
        }
    }

    component Tile: Rectangle {
        id: tile
        property string title
        property string subtitle
        property string icon
        property bool available: true
        signal activated()
        width: 300
        height: 104
        radius: 14
        color: !available ? "#D9D4C7" : (tap.pressed ? "#F2C894" : "#F7E3C2")
        border.color: available ? Theme.navy : "#A8A091"
        border.width: 2
        Row {
            anchors.verticalCenter: parent.verticalCenter
            x: 18
            spacing: 14
            Label { anchors.verticalCenter: parent.verticalCenter; text: tile.icon; font.pixelSize: Math.round((38) * Theme.fontScale) }
            Column {
                anchors.verticalCenter: parent.verticalCenter
                width: tile.width - 90
                // lange Titel (»Aufgabe deiner Lehrkraft«) passen sich der Kachel an statt hinauszuragen
                Label {
                    width: parent.width
                    text: tile.title
                    font.pixelSize: Math.round((20) * Theme.fontScale)
                    fontSizeMode: Text.HorizontalFit
                    minimumPixelSize: 13
                    elide: Text.ElideRight
                    font.bold: true
                    color: tile.available ? Theme.navy : "#6E6A60"
                }
                Label { width: parent.width; text: tile.subtitle; wrapMode: Text.Wrap; font.pixelSize: Math.round((13) * Theme.fontScale); color: "#4A5866" }
            }
        }
        TapHandler { id: tap; enabled: tile.available; onTapped: tile.activated() }
        Accessible.role: Accessible.Button
        Accessible.name: tile.title
    }

    ScrollView {
        id: scroll
        anchors { left: parent.left; right: parent.right; top: header.bottom; bottom: parent.bottom }
        contentWidth: availableWidth
        clip: true

        Column {
            x: Theme.margin
            width: scroll.availableWidth - 2 * Theme.margin
            topPadding: Theme.margin
            bottomPadding: Theme.margin
            spacing: 18

            // Tick begruesst (mit Spitzname, wenn ein Forscherprofil aktiv ist); darunter der Weg in die Hilfe
            Item {
                width: parent.width
                height: Math.max(homeTick.height, homeHelp.y + homeHelp.height)
                TickSays {
                    id: homeTick
                    objectName: "homeTick"
                    width: Math.min(parent.width, 720)
                    figure: 128
                    mood: "freundlich"
                    text: (home.controller.profilesEnabled && home.controller.currentProfile.name
                           ? qsTr("Hallo %1! Ich bin Tick.").arg(home.controller.currentProfile.name) : qsTr("Hallo! Ich bin Tick."))
                          + " " + qsTr("Ich begleite dich durch deine Experimente. Fang beim Lernpfad an oder probiere im freien Labor eigene Ideen aus.")
                }
                Button {
                    id: homeHelp
                    objectName: "homeHelp"
                    x: homeTick.bubbleLeft
                    y: homeTick.bubbleBottom + 10
                    height: Theme.control
                    text: qsTr("Zeig mir, wie Volta funktioniert")
                    font.pixelSize: Math.round(15 * Theme.fontScale)
                    font.bold: true
                    leftPadding: 8
                    rightPadding: 16
                    onClicked: home.helpRequested()
                    // gut sichtbar: rotes Fragezeichen auf gelbem Kreis, heller Knopf mit blauem Rand
                    contentItem: Row {
                        spacing: 10
                        HelpIcon { size: 32; anchors.verticalCenter: parent.verticalCenter }
                        Label {
                            anchors.verticalCenter: parent.verticalCenter
                            text: homeHelp.text
                            font: homeHelp.font
                            color: Theme.navy
                        }
                    }
                    background: Rectangle {
                        radius: Theme.control / 2
                        color: homeHelp.down ? "#FFE9A8" : (homeHelp.hovered ? "#FFF4CC" : "#FFFBEA")
                        border.color: Theme.accent
                        border.width: 2
                    }
                    Accessible.name: text
                }
            }

            Flow {
                width: parent.width
                spacing: 18
                Tile {
                    objectName: "tileContinue"
                    visible: home.controller.freeLab || home.controller.experimentId !== ""
                    icon: "▶️"
                    title: qsTr("Weitermachen")
                    subtitle: home.controller.title
                    onActivated: home.continueRequested()
                }
                // Aufgabe der Lehrkraft: zeigt die aktuelle Aufgabe oder fuehrt zur Eingabe des Codes
                Tile {
                    objectName: "tileTask"
                    icon: "📋"
                    title: home.controller.task.ok === true ? qsTr("Aufgabe deiner Lehrkraft") : qsTr("Aufgabe")
                    subtitle: home.controller.task.ok === true
                              ? (home.controller.task.text || qsTr("Antippen und loslegen"))
                              : qsTr("Code der Lehrkraft eingeben")
                    onActivated: home.taskRequested()
                }
                Tile {
                    objectName: "tileFreeLab"
                    icon: "🔧"
                    title: qsTr("Freies Labor")
                    subtitle: qsTr("Eigene Ideen ausprobieren – mit allen freigeschalteten Bauteilen")
                    onActivated: home.freeLabRequested()
                }
                Tile {
                    objectName: "tileJournal"
                    icon: "📓"
                    title: qsTr("Forscherheft")
                    subtitle: qsTr("Deine Vermutungen, Beobachtungen und Notizen")
                    onActivated: home.journalRequested()
                }
            }

            Label {
                text: qsTr("Lernpfad")
                font.pixelSize: Math.round((24) * Theme.fontScale)
                font.bold: true
                color: Theme.navy
            }

            // Stufen
            Repeater {
                model: home.controller.stages
                Rectangle {
                    id: stage
                    required property var modelData
                    objectName: "stage_" + modelData.number
                    width: parent.width
                    height: stageColumn.implicitHeight + 28
                    radius: 14
                    color: modelData.unlocked ? "#FFFDF7" : "#EFECE4"
                    border.color: modelData.unlocked ? Theme.fieldOutline : "#C8C2B5"

                    Column {
                        id: stageColumn
                        x: 16
                        y: 14
                        width: parent.width - 32
                        spacing: 10

                        Row {
                            spacing: 12
                            width: parent.width
                            Rectangle {
                                width: 44; height: 44; radius: 22
                                color: stage.modelData.unlocked ? Theme.accent : "#9AA3AD"
                                Label {
                                    anchors.centerIn: parent
                                    // Schloss zusaetzlich zur Farbe (Farbe nie allein)
                                    text: stage.modelData.unlocked ? stage.modelData.number : "🔒"
                                    color: "white"
                                    font.pixelSize: Math.round((20) * Theme.fontScale)
                                    font.bold: true
                                }
                            }
                            Column {
                                width: parent.width - 260
                                Label {
                                    text: qsTr("Stufe %1: %2").arg(stage.modelData.number).arg(stage.modelData.title)
                                    font.pixelSize: Math.round((18) * Theme.fontScale)
                                    font.bold: true
                                    color: Theme.navy
                                }
                                Label {
                                    width: parent.width
                                    text: stage.modelData.question
                                    font.pixelSize: Math.round((14) * Theme.fontScale)
                                    font.italic: true
                                    color: "#4A5866"
                                    wrapMode: Text.Wrap
                                }
                            }
                            // Fortschritt als Zahl und Balken
                            Column {
                                visible: stage.modelData.total > 0
                                width: 190
                                spacing: 4
                                Label {
                                    text: qsTr("%1 von %2 geschafft").arg(stage.modelData.done).arg(stage.modelData.total)
                                    font.pixelSize: Math.round((13) * Theme.fontScale)
                                    color: "#4A5866"
                                }
                                Rectangle {
                                    width: parent.width; height: 10; radius: 5
                                    color: "#E2DDD2"
                                    Rectangle {
                                        width: parent.width * stage.modelData.done / Math.max(1, stage.modelData.total)
                                        height: parent.height
                                        radius: 5
                                        color: "#2E7D32"
                                    }
                                }
                            }
                        }

                        Label {
                            visible: !stage.modelData.unlocked
                            text: qsTr("Noch gesperrt – schaffe zuerst die Experimente der Stufe %1.").arg(stage.modelData.number - 1)
                            font.pixelSize: Math.round((13) * Theme.fontScale)
                            color: "#6E6A60"
                        }
                        Label {
                            visible: stage.modelData.unlocked && stage.modelData.total === 0
                            text: qsTr("Experimente folgen.")
                            font.pixelSize: Math.round((13) * Theme.fontScale)
                            color: "#6E6A60"
                        }

                        // Experimente als Karten
                        Flow {
                            visible: stage.modelData.unlocked && stage.modelData.total > 0
                            width: parent.width
                            spacing: 12
                            Repeater {
                                model: stage.modelData.experiments
                                Rectangle {
                                    id: card
                                    required property var modelData
                                    objectName: "card_" + modelData.id
                                    width: 290
                                    height: 84
                                    radius: 10
                                    color: cardTap.pressed ? "#E3EEF8" : "#EEF4FA"
                                    border.color: card.modelData.done ? "#2E7D32" : Theme.accent
                                    border.width: card.modelData.done ? 3 : 1.5
                                    Column {
                                        x: 12
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: parent.width - 52
                                        spacing: 4
                                        Label {
                                            width: parent.width
                                            text: card.modelData.title
                                            font.pixelSize: Math.round((14) * Theme.fontScale)
                                            font.bold: true
                                            color: Theme.navy
                                            wrapMode: Text.Wrap
                                            maximumLineCount: 2
                                            elide: Text.ElideRight
                                        }
                                        Label {
                                            visible: card.modelData.destructive
                                            text: "⚠ " + qsTr("Nur in der App!")
                                            font.pixelSize: Math.round((11) * Theme.fontScale)
                                            color: "#8A3200"
                                        }
                                    }
                                    // erledigt: Haken (zusaetzlich zur gruenen Kontur)
                                    Label {
                                        anchors.right: parent.right
                                        anchors.rightMargin: 10
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: card.modelData.done ? "✓" : "›"
                                        font.pixelSize: Math.round((card.modelData.done ? 26 : 30) * Theme.fontScale)
                                        font.bold: true
                                        color: card.modelData.done ? "#2E7D32" : Theme.accent
                                    }
                                    TapHandler { id: cardTap; onTapped: home.experimentRequested(card.modelData.id) }
                                    Accessible.role: Accessible.Button
                                    Accessible.name: card.modelData.title + (card.modelData.done ? ", " + qsTr("geschafft") : "")
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
