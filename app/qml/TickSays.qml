// Tick spricht: Figur links, Sprechblase rechts (docs/maskottchen-tick.md). Ist Tick im Elternbereich
// abgeschaltet (Theme.tick), bleibt nur die Blase als Hinweiskasten - der Inhalt geht nie verloren.
// Beim Wechsel von Text oder Stimmung hopst Tick kurz (nicht bei »Bewegung reduzieren«).
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Shapes

Item {
    id: says
    property string mood: "freundlich"
    property string text
    property int figure: 88
    property color bubbleColor: "#FFFFFF"
    property color borderColor: Theme.fieldOutline
    property color textColor: "#1E2B38"
    property bool bold: false
    property bool figureShown: true // false: nur die Blase (z. B. aeltere Tipps)
    readonly property bool showFigure: Theme.tick && figureShown

    implicitHeight: Math.max(showFigure ? figure : 0, bubble.height)
    height: implicitHeight

    function hop() {
        if (!Theme.reduceMotion && showFigure) hopAnimation.restart()
    }
    onTextChanged: hop()
    onMoodChanged: hop()

    Tick {
        id: figureItem
        visible: says.showFigure
        mood: says.mood
        width: says.figure
        height: says.figure
        y: (says.height - height) / 2
        transformOrigin: Item.Bottom
        SequentialAnimation {
            id: hopAnimation
            NumberAnimation { target: figureItem; property: "scale"; to: 1.08; duration: 120; easing.type: Easing.OutQuad }
            NumberAnimation { target: figureItem; property: "scale"; to: 1.0; duration: 180; easing.type: Easing.OutBack }
        }
    }

    Rectangle {
        id: bubble
        x: says.showFigure ? says.figure + 10 : 0
        width: says.width - x
        height: label.implicitHeight + 20
        y: (says.height - height) / 2
        radius: 12
        color: says.bubbleColor
        border.color: says.borderColor
        border.width: 1.5

        // Spitze der Sprechblase zur Figur hin
        Shape {
            visible: says.showFigure
            x: -9
            y: bubble.height / 2 - 8
            width: 10
            height: 16
            preferredRendererType: Shape.CurveRenderer
            ShapePath {
                fillColor: says.bubbleColor
                strokeColor: says.borderColor
                strokeWidth: 1.5
                startX: 10; startY: 0
                PathLine { x: 0; y: 8 }
                PathLine { x: 10; y: 16 }
            }
        }
        Label {
            id: label
            x: 12
            width: bubble.width - 24
            anchors.verticalCenter: parent.verticalCenter
            wrapMode: Text.Wrap
            text: says.text
            color: says.textColor
            font.bold: says.bold
            font.pixelSize: Math.round(14 * Theme.fontScale)
        }
    }

    Accessible.role: Accessible.StaticText
    Accessible.name: text
}
