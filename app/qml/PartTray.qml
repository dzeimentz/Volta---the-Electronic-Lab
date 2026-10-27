// Bauteilfach: Bauteile aus dem Fach auf die Grundplatte ziehen (docs/ui-spec.md, Gesten).
// Zwei Spalten, damit alle Bauteile ohne Blaettern sichtbar sind (Ziehen gehoert dem Bauteil, nicht der Liste).
import QtQuick
import QtQuick.Controls.Basic

Rectangle {
    id: tray
    required property var parts // catalogParts des LabController
    property var modules: []    // trayModules des LabController (Modulfach)
    signal dragUpdated(string type, point scenePos)
    signal dragFinished(string type, point scenePos)
    signal lockedTapped(string type, int stage) // gesperrtes Bauteil angetippt (Hinweis)
    signal moduleDragUpdated(string type, point scenePos)
    signal moduleDragFinished(string type, point scenePos)
    signal moduleLockedTapped(string type, int stage)

    color: Theme.desk
    readonly property int tileWidth: 112
    width: 2 * tileWidth + 3 * 8 + 6 // Platz fuer die Bildlaufleiste

    // Umschalter Bauteilfach / Modulfach: das gewaehlte Fach ist hervorgehoben und unterstrichen (Farbe nie allein)
    property bool showModules: false
    readonly property bool hasModules: tray.modules.length > 0
    onHasModulesChanged: if (!hasModules) showModules = false

    Button {
        id: heading
        objectName: "trayToggle"
        anchors { top: parent.top; left: parent.left; right: parent.right; margins: 8 }
        implicitHeight: Theme.control
        enabled: tray.hasModules
        onClicked: tray.showModules = !tray.showModules
        background: Rectangle {
            radius: 8
            color: heading.down ? "#33FFFFFF" : (heading.hovered && heading.enabled ? "#22FFFFFF" : "#14FFFFFF")
            border.color: heading.enabled ? "#66FFFFFF" : "transparent"
            border.width: heading.activeFocus ? 3 : 1
        }
        contentItem: Item {
          implicitHeight: trayRow.implicitHeight
          Row {
            id: trayRow
            anchors.centerIn: parent
            spacing: 6
            // lange Namen (FR »Boîte à composants | Casier à modules«) verkleinern statt abschneiden
            scale: Math.min(1, (parent.width - 4) / Math.max(1, implicitWidth))
            Repeater {
                model: [qsTr("Bauteilfach"), "|", qsTr("Modulfach")]
                Label {
                    required property int index
                    required property string modelData
                    readonly property bool chosen: index === 0 ? !tray.showModules : index === 2 && tray.showModules
                    visible: tray.hasModules || index === 0
                    anchors.verticalCenter: parent.verticalCenter
                    text: modelData
                    color: chosen || index === 1 ? "white" : "#9FB2C4"
                    font.pixelSize: Math.round(14 * Theme.fontScale)
                    font.bold: chosen
                    font.underline: chosen && tray.hasModules
                }
            }
          }
        }
        Accessible.name: tray.showModules ? qsTr("Modulfach, umschalten auf Bauteilfach")
                                          : qsTr("Bauteilfach, umschalten auf Modulfach")
    }

    // Themenleiste: »Alle« und je Thema ein Knopf in seiner Farbe mit typischem Schaltzeichen (Farbe nie allein:
    // Zeichen, Tooltip, Ueberschrift darunter). Nur Themen, die im Fach vorkommen.
    property string filter: "" // "" = alle Bauteile
    // Aufbauhilfe: das Thema des gesuchten Bauteils waehlen, damit es oben im Fach liegt
    function showType(type) {
        for (let i = 0; i < parts.length; ++i)
            if (parts[i].type === type) { filter = themesShown.some(t => t.key === parts[i].theme) ? parts[i].theme : ""; break }
        partFlick.contentY = 0
    }
    readonly property var themesShown: {
        let out = []
        for (let t = 0; t < Theme.partThemes.length; ++t)
            for (let i = 0; i < tray.parts.length; ++i)
                if (tray.parts[i].theme === Theme.partThemes[t].key) { out.push(Theme.partThemes[t]); break }
        return out
    }
    onThemesShownChanged: {
        for (let t = 0; t < themesShown.length; ++t) if (themesShown[t].key === filter) return
        filter = ""
    }
    // nach Thema sortiert (Reihenfolge der Leiste), im Thema freigeschaltete vor gesperrten
    readonly property var shownParts: {
        let out = []
        for (let t = 0; t <= Theme.partThemes.length; ++t) {
            const key = t < Theme.partThemes.length ? Theme.partThemes[t].key : null
            if (filter !== "" && key !== filter) continue
            for (let pass = 0; pass < 2; ++pass)
                for (let i = 0; i < tray.parts.length; ++i) {
                    const p = tray.parts[i]
                    const known = Theme.partThemes.some(x => x.key === p.theme)
                    if ((key === null ? !known : p.theme === key) && (p.locked === true) === (pass === 1)) out.push(p)
                }
        }
        return out
    }

    Grid {
        id: themeBar
        objectName: "themeBar"
        visible: !tray.showModules
        anchors { top: heading.bottom; topMargin: 6; left: parent.left; leftMargin: 8; right: parent.right; rightMargin: 8 }
        columns: 4
        spacing: 4
        readonly property real cell: (width - 3 * spacing) / 4
        Repeater {
            model: [{key: "", color: "#E9EEF3", symbol: ""}].concat(tray.themesShown)
            delegate: Button {
                id: themeButton
                required property var modelData
                objectName: "theme_" + (modelData.key === "" ? "alle" : modelData.key)
                readonly property bool chosen: tray.filter === modelData.key
                width: themeBar.cell
                height: Theme.control
                onClicked: tray.filter = modelData.key
                background: Rectangle {
                    radius: 8
                    color: themeButton.modelData.color
                    border.color: themeButton.chosen ? "white" : Qt.darker(themeButton.modelData.color, 1.4)
                    border.width: themeButton.chosen ? 3 : 1
                    opacity: themeButton.chosen || themeButton.hovered ? 1 : 0.85
                }
                contentItem: Item {
                    SchematicSymbol {
                        visible: themeButton.modelData.symbol !== ""
                        anchors.centerIn: parent
                        type: themeButton.modelData.symbol
                        stroke: "white"
                        width: 34
                        height: 18
                    }
                    Label {
                        visible: themeButton.modelData.symbol === ""
                        anchors.centerIn: parent
                        text: qsTr("Alle")
                        color: Theme.navy
                        font.pixelSize: Math.round(13 * Theme.fontScale)
                        font.bold: true
                    }
                }
                ToolTip.visible: hovered
                ToolTip.text: Texts.themeName(modelData.key)
                Accessible.name: Texts.themeName(modelData.key) + (chosen ? ", " + qsTr("gewählt") : "")
            }
        }
    }
    Label {
        id: themeTitle
        visible: !tray.showModules
        anchors { top: themeBar.bottom; topMargin: 4; left: parent.left; leftMargin: 10; right: parent.right; rightMargin: 8 }
        text: Texts.themeName(tray.filter)
        color: "white"
        font.pixelSize: Math.round(13 * Theme.fontScale)
        font.bold: true
        elide: Text.ElideRight
    }

    // Kacheln: oben das Bauteil wie auf dem Steckbrett, darunter das Schaltzeichen, unten der Name; Hintergrund in
    // der Farbe des Themas. Senkrecht wischen blaettert, zur Seite ziehen nimmt das Bauteil heraus.
    Flickable {
        id: partFlick
        objectName: "partFlick"
        visible: !tray.showModules
        anchors { top: themeTitle.bottom; topMargin: 4; left: parent.left; right: parent.right; bottom: parent.bottom }
        contentHeight: partGrid.height + 8
        flickableDirection: Flickable.VerticalFlick
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        ScrollBar.vertical: ScrollBar { policy: partFlick.contentHeight > partFlick.height ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff }

        Grid {
            id: partGrid
            x: 8
            columns: 2
            spacing: 4

            Repeater {
                model: tray.shownParts
                delegate: Rectangle {
                    id: entry
                    required property var modelData
                    objectName: "tray_" + modelData.type
                    width: tray.tileWidth
                    height: 108
                    radius: 8
                    readonly property bool locked: modelData.locked === true
                    readonly property var theme: Theme.partTheme(modelData.theme)
                    color: locked ? "#8E9AA8" : (drag.active ? Theme.solderAmber : theme.tint)
                    border.color: locked ? "#5D6B7A" : Qt.darker(theme.color, 1.2)
                    border.width: 2
                    opacity: locked ? 0.75 : 1

                    // Bauteil wie auf dem Steckbrett (verkleinert, damit es in die Kachel passt)
                    Item {
                        id: preview
                        x: 6; y: 4
                        width: parent.width - 12
                        height: 44
                        readonly property string type: entry.modelData.type
                        readonly property bool square: entry.modelData.pins === 4
                        readonly property bool carrier: entry.modelData.pins === 3
                        PartBody {
                            type: preview.type
                            width: preview.square ? 122 : preview.carrier ? 160 : 72
                            height: preview.square ? 102 : 34
                            feet: preview.square ? [Qt.point(13, 11), Qt.point(109, 11), Qt.point(13, 91), Qt.point(109, 91)] : []
                            value: preview.type === "widerstand" ? 1000 : 0
                            anchors.centerIn: parent
                            scale: Math.min(1, preview.width / width, preview.height / height)
                        }
                    }
                    // Schaltzeichen nach Norm (DIN EN 60617)
                    SchematicSymbol {
                        objectName: "symbol_" + entry.modelData.type
                        type: entry.modelData.type
                        stroke: entry.locked ? "#3E4955" : Theme.navy
                        anchors.horizontalCenter: parent.horizontalCenter
                        y: 50
                        width: 50
                        height: 20
                    }
                    Label {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.margins: 3
                        height: 32
                        text: Texts.partName(entry.modelData.type)
                        color: entry.locked ? "#26313C" : Theme.navy
                        font.pixelSize: Math.round((13) * Theme.fontScale)
                        font.bold: true
                        fontSizeMode: Text.Fit
                        minimumPixelSize: 9
                        wrapMode: Text.WordWrap
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    // Schloss zusaetzlich zur grauen Farbe (Farbe nie allein)
                    Label {
                        visible: entry.locked
                        x: parent.width - width - 4; y: 2
                        text: "🔒" + entry.modelData.stage // Stufe, ab der das Bauteil frei wird
                        font.pixelSize: 13
                        font.bold: true
                        color: "#1E2833"
                    }
                    TapHandler {
                        enabled: entry.locked
                        onTapped: tray.lockedTapped(entry.modelData.type, entry.modelData.stage)
                    }
                    Accessible.name: Texts.partName(entry.modelData.type) + (entry.locked ? ", " + qsTr("gesperrt bis Stufe %1").arg(entry.modelData.stage) : "")

                    property point lastScenePos
                    DragHandler {
                        id: drag
                        enabled: !entry.locked
                        target: null
                        onCentroidChanged: {
                            if (active) {
                                entry.lastScenePos = centroid.scenePosition
                                tray.dragUpdated(entry.modelData.type, entry.lastScenePos)
                            }
                        }
                        onActiveChanged: if (!active) tray.dragFinished(entry.modelData.type, entry.lastScenePos)
                    }
                }
            }
        }
    }

    // Modulfach: Module auf ein Feld der Modulleiste ziehen (erster Anschluss liegt dort)
    Grid {
        visible: tray.showModules
        anchors { top: heading.bottom; topMargin: 6; left: parent.left; leftMargin: 8 }
        columns: 2
        spacing: 4
        Repeater {
            model: tray.modules
            delegate: Rectangle {
                id: modEntry
                required property var modelData
                objectName: "trayModule_" + modelData.type
                width: 92
                height: 44 // kompakt: alle Bauteile passen auch auf Tablets
                radius: 8
                readonly property bool locked: modelData.locked === true
                color: locked ? "#8E9AA8" : (modDrag.active ? Theme.solderAmber : "#D8CCE8")
                border.color: locked ? "#5D6B7A" : Theme.navy
                opacity: locked ? 0.75 : 1
                SchematicSymbol {
                    type: modEntry.modelData.type
                    stroke: modEntry.locked ? "#3E4955" : Theme.navy
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: 2
                    width: 46
                    height: 18
                }
                Label {
                    anchors { left: parent.left; right: parent.right; bottom: parent.bottom; margins: 3 }
                    height: 22
                    text: Texts.moduleName(modEntry.modelData.type)
                    color: modEntry.locked ? "#26313C" : Theme.navy
                    font.pixelSize: Math.round((12) * Theme.fontScale)
                    fontSizeMode: Text.Fit
                    minimumPixelSize: 9
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                Label {
                    visible: modEntry.locked
                    x: parent.width - width - 4; y: 2
                    text: "🔒" + modEntry.modelData.stage
                    font.pixelSize: 13
                    font.bold: true
                    color: "#1E2833"
                }
                TapHandler {
                    enabled: modEntry.locked
                    onTapped: tray.moduleLockedTapped(modEntry.modelData.type, modEntry.modelData.stage)
                }
                Accessible.name: Texts.moduleName(modEntry.modelData.type) + (modEntry.locked ? ", " + qsTr("gesperrt bis Stufe %1").arg(modEntry.modelData.stage) : "")

                property point lastScenePos
                DragHandler {
                    id: modDrag
                    enabled: !modEntry.locked
                    target: null
                    onCentroidChanged: {
                        if (active) {
                            modEntry.lastScenePos = centroid.scenePosition
                            tray.moduleDragUpdated(modEntry.modelData.type, modEntry.lastScenePos)
                        }
                    }
                    onActiveChanged: if (!active) tray.moduleDragFinished(modEntry.modelData.type, modEntry.lastScenePos)
                }
            }
        }
    }
}
