// Knoepfe oben rechts wie in Visual Technic Studio (Startseite und Labor): Anzeigesprache (runde Flagge), Info,
// Einstellungen (ganz rechts).
import QtQuick
import QtQuick.Controls.Basic

Row {
    id: bar
    required property var controller
    property var languageService: null
    property var settings: null
    signal infoRequested()
    signal helpRequested()
    property bool helpInMenu: false // im Labor ist die Leiste voll: Hilfe dann im Menue statt als Knopf
    readonly property var managed: typeof volta_managed !== "undefined" ? volta_managed : null
    signal parentAreaRequested() // hinter der Elternschranke (Main.qml)
    spacing: 6

    // Schriftgroesse: A- kleiner, A+ groesser (drei Stufen; groesste ist die Vorgabe)
    Row {
        visible: bar.settings !== null
        anchors.verticalCenter: parent.verticalCenter
        spacing: 0
        Repeater {
            model: [{label: "A−", step: -1, name: "fontSmaller"}, {label: "A+", step: 1, name: "fontLarger"}]
            ToolButton {
                required property var modelData
                objectName: modelData.name
                width: Theme.control; height: Theme.control
                enabled: bar.settings !== null && (modelData.step < 0 ? bar.settings.fontLevel > 0 : bar.settings.fontLevel < 2)
                onClicked: bar.settings.fontLevel = bar.settings.fontLevel + modelData.step
                ToolTip.visible: hovered
                ToolTip.text: modelData.step < 0 ? qsTr("Schrift kleiner") : qsTr("Schrift größer")
                Accessible.name: ToolTip.text
                contentItem: Label {
                    text: parent.modelData.label
                    color: parent.enabled ? "white" : "#6F8399"
                    // feste Groesse: die Knoepfe selbst wachsen nicht mit
                    font.pixelSize: parent.modelData.step < 0 ? 13 : 16
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 6
                    color: parent.down ? "#33FFFFFF" : (parent.hovered ? "#1FFFFFFF" : "transparent")
                }
            }
        }
    }

    ToolButton {
        id: languageButton
        objectName: "languageButton"
        // verwaltete Konfiguration: Sprache gesperrt -> kein Flaggensymbol
        visible: bar.languageService !== null && !(bar.managed && bar.managed.languageLocked)
        width: Theme.control; height: Theme.control
        onClicked: languageMenu.open()
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Anzeigesprache wählen")
        Accessible.name: ToolTip.text
        background: Rectangle { // wie VTS: ohne Flaeche, nur Hervorhebung beim Zeigen/Druecken
            radius: 6
            color: parent.down ? "#33FFFFFF" : (parent.hovered ? "#1FFFFFFF" : "transparent")
        }
        contentItem: Item {
            Image {
                anchors.centerIn: parent
                width: 26; height: 26
                sourceSize: Qt.size(64, 64)
                source: bar.languageService ? "qrc:/flags/" + bar.languageService.language + ".svg" : ""
            }
        }
        Menu {
            id: languageMenu
            margins: 8 // immer ganz im Fenster
            objectName: "languageMenu"
            y: parent.height
            x: parent.width - width
            Repeater {
                model: bar.languageService ? bar.languageService.languages : []
                MenuItem {
                    id: langItem
                    required property var modelData
                    objectName: "language_" + modelData.code
                    height: Theme.control
                    enabled: modelData.available
                    checkable: true
                    checked: bar.languageService && bar.languageService.language === modelData.code
                    // Flagge nie allein: Kuerzel und Eigenname; LU folgt (grau)
                    text: modelData.short + "  " + modelData.name + (modelData.available ? "" : "  – " + qsTr("folgt"))
                    contentItem: Row {
                        spacing: 10
                        leftPadding: 28
                        Image {
                            width: 24; height: 24
                            sourceSize: Qt.size(48, 48)
                            source: "qrc:/flags/" + langItem.modelData.code + ".svg"
                            opacity: langItem.enabled ? 1 : 0.35
                        }
                        Label {
                            text: langItem.text
                            font.pixelSize: Math.round((15) * Theme.fontScale)
                            color: langItem.enabled ? "#1E2B38" : "#9AA3AD"
                        }
                    }
                    onTriggered: bar.languageService.setLanguage(modelData.code)
                }
            }
        }
    }
    // Hilfe mit Tick (Fragezeichen)
    ToolButton {
        objectName: "helpButton"
        visible: !bar.helpInMenu
        width: Theme.control; height: Theme.control
        onClicked: bar.helpRequested()
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Hilfe")
        Accessible.name: qsTr("Hilfe")
        background: Rectangle {
            radius: 6
            color: parent.down ? "#33FFFFFF" : (parent.hovered ? "#1FFFFFFF" : "transparent")
        }
        contentItem: Item { HelpIcon { anchors.centerIn: parent; size: 28 } }
    }
    ToolButton {
        objectName: "infoButton"
        width: Theme.control; height: Theme.control
        onClicked: bar.infoRequested()
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Info")
        Accessible.name: qsTr("Info")
        background: Rectangle { // wie VTS: ohne Flaeche, nur Hervorhebung beim Zeigen/Druecken
            radius: 6
            color: parent.down ? "#33FFFFFF" : (parent.hovered ? "#1FFFFFFF" : "transparent")
        }
        contentItem: Item {
            Rectangle {
                anchors.centerIn: parent
                width: 24; height: 24; radius: 12
                color: "transparent"
                border.color: "white"
                border.width: 2
                Label { anchors.centerIn: parent; text: "i"; color: "white"; font.pixelSize: 15; font.bold: true; font.italic: true }
            }
        }
    }
    // Einstellungen (ganz rechts)
    ToolButton {
        objectName: "settingsButton"
        width: Theme.control; height: Theme.control
        contentItem: Item { DrawnIcon { anchors.centerIn: parent; kind: "menu"; color: "white"; size: 24 } }
        onClicked: settingsMenu.open()
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Einstellungen")
        Accessible.name: qsTr("Einstellungen")
        background: Rectangle {
            radius: 6
            color: parent.down ? "#33FFFFFF" : (parent.hovered ? "#1FFFFFFF" : "transparent")
        }
        Menu {
            id: settingsMenu
            margins: 8 // immer ganz im Fenster
            objectName: "settingsMenu"
            y: parent.height
            x: parent.width - width
            width: 300
            MenuItem {
                objectName: "menuHelp"
                visible: bar.helpInMenu
                height: visible ? Theme.control : 0
                text: "?  " + qsTr("Hilfe")
                font.bold: true
                onTriggered: bar.helpRequested()
            }
            MenuItem {
                objectName: "settingFlow"
                height: Theme.control
                text: qsTr("Stromfluss anzeigen")
                checkable: true
                enabled: bar.settings !== null
                checked: bar.settings ? bar.settings.showCurrentFlow : true
                onTriggered: bar.settings.showCurrentFlow = checked
            }
            MenuItem {
                objectName: "settingSound"
                height: Theme.control
                text: qsTr("Ton über Lautsprecher")
                checkable: true
                enabled: bar.settings !== null
                checked: bar.settings ? bar.settings.soundOn : true
                onTriggered: bar.settings.soundOn = checked
            }
            MenuItem {
                objectName: "settingAutoWireColor"
                height: Theme.control
                text: qsTr("Leitungsfarbe automatisch (Plus rot, Minus schwarz)")
                checkable: true
                enabled: bar.settings !== null
                checked: bar.settings ? bar.settings.autoWireColor : true
                onTriggered: bar.settings.autoWireColor = checked
            }
            MenuItem {
                objectName: "settingAutoColumns"
                height: Theme.control
                text: qsTr("Steckfeld-Breite automatisch")
                checkable: true
                enabled: bar.settings !== null
                checked: bar.settings ? bar.settings.autoBoardColumns : true
                onTriggered: bar.settings.autoBoardColumns = checked
                ToolTip.visible: hovered
                ToolTip.delay: 600
                ToolTip.text: qsTr("Lernpfad 8, freies Labor 12 Felder je Reihe; Messgerät daneben 10, im Pult 12")
            }
            MenuItem {
                objectName: "settingSilverFields"
                height: Theme.control
                text: qsTr("Kontaktfelder silbern (sonst Messing)")
                checkable: true
                enabled: bar.settings !== null
                checked: bar.settings ? bar.settings.silverFields : true
                onTriggered: bar.settings.silverFields = checked
            }
            MenuItem {
                objectName: "settingMotion"
                height: Theme.control
                text: qsTr("Animationen reduzieren")
                checkable: true
                enabled: bar.settings !== null
                checked: bar.settings ? bar.settings.reduceMotion : false
                onTriggered: bar.settings.reduceMotion = checked
            }
            MenuItem {
                height: Theme.control
                visible: bar.settings !== null && bar.settings.splashAvailable
                text: qsTr("Startbildschirm zeigen")
                checkable: true
                checked: bar.settings ? bar.settings.showSplash : true
                onTriggered: bar.settings.showSplash = checked
            }
            MenuItem {
                height: Theme.control
                text: qsTr("Messgerät anzeigen")
                checkable: true
                checked: bar.controller.meterVisible
                onTriggered: bar.controller.meterVisible = checked
            }
            Menu {
                margins: 8 // immer ganz im Fenster
                title: qsTr("Felder je Reihe")
                Repeater {
                    model: [8, 10, 12]
                    MenuItem {
                        required property int modelData
                        height: Theme.control
                        text: qsTr("1–%1").arg(modelData)
                        checkable: true
                        checked: bar.controller.boardColumns === modelData
                        onTriggered: bar.controller.setBoardColumns(modelData)
                    }
                }
            }
            MenuSeparator {}
            MenuItem {
                objectName: "settingParentArea"
                visible: !bar.managed || bar.managed.parentArea
                height: visible ? Theme.control : 0
                text: qsTr("Elternbereich …")
                onTriggered: bar.parentAreaRequested()
            }
            MenuItem {
                height: Theme.control
                text: qsTr("Info")
                onTriggered: bar.infoRequested()
            }
        }
    }
}
