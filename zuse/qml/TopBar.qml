import QtQuick
import QtQuick.Controls

// Knoepfe oben rechts wie bei Volta: Schriftgroesse (A−/A+), Anzeigesprache (runde Flagge mit Kuerzel im Menue),
// Hilfe, Kurzanleitung, Info und das Menue mit den drei Strichen (ganz rechts). Im Experiment ist die Kopfzeile
// voll: Hilfe und Kurzanleitung stehen dann im Menue.
Row {
    id: bar
    property bool helpInMenu: false
    signal helpRequested()
    signal quickGuideRequested()
    signal infoRequested()
    signal journalRequested()
    signal parentAreaRequested()
    spacing: 4

    component BarButton: ToolButton {
        width: ZTheme.control; height: ZTheme.control
        ToolTip.visible: hovered
        Accessible.name: ToolTip.text
        background: Rectangle {
            radius: 6
            color: parent.down ? "#33FFFFFF" : (parent.hovered ? "#1FFFFFFF" : "transparent")
        }
    }

    Repeater { // Schrift kleiner bzw. groesser (drei Stufen, groesste ist die Vorgabe)
        model: [{label: "A−", step: -1}, {label: "A+", step: 1}]
        BarButton {
            required property var modelData
            objectName: modelData.step < 0 ? "fontSmaller" : "fontLarger"
            enabled: modelData.step < 0 ? settings.fontLevel > 0 : settings.fontLevel < 2
            onClicked: settings.fontLevel = settings.fontLevel + modelData.step
            ToolTip.text: modelData.step < 0 ? qsTr("Schrift kleiner") : qsTr("Schrift größer")
            contentItem: Text {
                text: parent.modelData.label
                color: parent.enabled ? ZTheme.textOnShell : "#6F8399"
                font.pixelSize: parent.modelData.step < 0 ? 13 : 16 // feste Groesse: die Knoepfe wachsen nicht mit
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    BarButton {
        objectName: "languageButton"
        onClicked: languageMenu.open()
        ToolTip.text: qsTr("Anzeigesprache wählen")
        contentItem: Item {
            Image {
                anchors.centerIn: parent
                width: 26; height: 26
                sourceSize: Qt.size(64, 64)
                source: "qrc:/flags/" + settings.language + ".svg"
            }
        }
        Menu {
            id: languageMenu
            objectName: "languageMenu"
            margins: 8
            y: parent.height
            x: parent.width - width
            Repeater {
                model: settings.languages
                MenuItem {
                    id: langItem
                    required property var modelData
                    height: ZTheme.control
                    checkable: true
                    checked: settings.language === modelData.code
                    // Flagge nie allein: Kuerzel und Eigenname
                    text: modelData.short + "  " + modelData.name
                    contentItem: Row {
                        spacing: 10
                        leftPadding: 28
                        Image {
                            width: 24; height: 24
                            sourceSize: Qt.size(48, 48)
                            source: "qrc:/flags/" + langItem.modelData.code + ".svg"
                        }
                        Text {
                            text: langItem.text
                            font.pixelSize: Math.round(15 * ZTheme.fontScale)
                            color: ZTheme.text
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    onTriggered: settings.setLanguage(modelData.code)
                }
            }
        }
    }
    BarButton {
        objectName: "helpButton"
        visible: !bar.helpInMenu
        onClicked: bar.helpRequested()
        ToolTip.text: qsTr("Hilfe")
        contentItem: Item { HelpIcon { anchors.centerIn: parent; size: 28 } }
    }
    BarButton {
        objectName: "quickGuideButton"
        visible: !bar.helpInMenu
        onClicked: bar.quickGuideRequested()
        ToolTip.text: qsTr("Kurzanleitung")
        contentItem: Item { GuideIcon { anchors.centerIn: parent; size: 28 } }
    }
    BarButton {
        objectName: "infoButton"
        onClicked: bar.infoRequested()
        ToolTip.text: qsTr("Info")
        contentItem: Item {
            Rectangle {
                anchors.centerIn: parent
                width: 24; height: 24; radius: 12
                color: "transparent"
                border.color: "white"
                border.width: 2
                Text {
                    anchors.centerIn: parent
                    text: "i"; color: "white"
                    font.pixelSize: 15; font.bold: true; font.italic: true
                }
            }
        }
    }
    BarButton { // Menue mit den drei Strichen (ganz rechts)
        objectName: "settingsButton"
        onClicked: settingsMenu.open()
        ToolTip.text: qsTr("Einstellungen")
        contentItem: Item {
            Column {
                anchors.centerIn: parent
                spacing: 4
                Repeater {
                    model: 3
                    Rectangle { width: 22; height: 3; radius: 1.5; color: "white" }
                }
            }
        }
        Menu {
            id: settingsMenu
            objectName: "settingsMenu"
            margins: 8
            y: parent.height
            x: parent.width - width
            width: 330
            MenuItem {
                visible: bar.helpInMenu
                height: visible ? ZTheme.control : 0
                text: "?  " + qsTr("Hilfe")
                font.bold: true
                onTriggered: bar.helpRequested()
            }
            MenuItem {
                visible: bar.helpInMenu
                height: visible ? ZTheme.control : 0
                text: "»  " + qsTr("Kurzanleitung")
                font.bold: true
                onTriggered: bar.quickGuideRequested()
            }
            MenuItem {
                objectName: "menuProfile"
                visible: workbench.currentProfile.id !== undefined
                height: visible ? ZTheme.control : 0
                text: workbench.currentProfile.id !== undefined
                      ? workbench.currentProfile.symbol + "  " + qsTr("Profil wechseln") : ""
                onTriggered: workbench.leaveProfile()
            }
            MenuItem {
                objectName: "menuJournal"
                visible: workbench.mode !== "vorfuehrung" && !(workbench.profilesEnabled && workbench.currentProfile.id === undefined)
                height: visible ? ZTheme.control : 0
                text: "📓  " + qsTr("Forscherheft")
                font.bold: true
                onTriggered: bar.journalRequested()
            }
            MenuItem {
                height: ZTheme.control
                checkable: true
                checked: workbench.numbersVisible
                text: qsTr("Zahlenleiste anzeigen")
                onTriggered: workbench.numbersVisible = !workbench.numbersVisible
            }
            MenuItem {
                height: ZTheme.control
                checkable: true
                checked: workbench.symbolStyle === "us"
                text: qsTr("Schaltzeichen nach US-Norm (IEEE)")
                onTriggered: workbench.symbolStyle = workbench.symbolStyle === "us" ? "din" : "us"
            }
            MenuItem {
                objectName: "menuParentArea"
                height: ZTheme.control
                text: "🔒  " + qsTr("Elternbereich")
                onTriggered: bar.parentAreaRequested()
            }
        }
    }
}
