import QtQuick
import QtQuick.Layouts

// Zetti erzaehlt: Titel und Frage der Stufe, Sprechblase mit Geschichte, Rueckmeldung oder Tipp, Zetti in der
// passenden Stimmung (workbench.zettiPose), am Ende der Merksatz.
Rectangle {
    id: root
    property bool hintVisible: false
    radius: 18
    color: ZTheme.surface
    border.color: ZTheme.line

    Connections {
        target: workbench
        function onStageChanged() { root.hintVisible = false }
        function onMessageChanged() { root.hintVisible = false }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10
        Text {
            Layout.fillWidth: true
            text: workbench.stageTitle
            color: ZTheme.navy
            font.pixelSize: 24; font.bold: true
            wrapMode: Text.WordWrap
        }
        Text {
            Layout.fillWidth: true
            text: workbench.stageQuestion
            color: ZTheme.accentText
            font.pixelSize: 17; font.bold: true
            wrapMode: Text.WordWrap
        }
        Rectangle { // Sprechblase
            id: bubble
            Layout.fillWidth: true
            Layout.preferredHeight: bubbleText.implicitHeight + 28
            radius: 16
            color: ZTheme.bubble
            border.width: 2
            border.color: ZTheme.accentDim
            Text {
                id: bubbleText
                anchors { left: parent.left; right: parent.right; top: parent.top; margins: 14 }
                wrapMode: Text.WordWrap
                color: ZTheme.text
                font.pixelSize: 16
                lineHeight: 1.15
                text: root.hintVisible ? workbench.stageHint
                                       : (workbench.message !== "" ? workbench.message : workbench.stageStory)
            }
            Canvas { // Spitze der Blase zu Zetti
                width: 28; height: 18
                x: 30; y: parent.height - 2
                onPaint: {
                    const ctx = getContext("2d")
                    ctx.fillStyle = ZTheme.bubble
                    ctx.strokeStyle = ZTheme.accentDim
                    ctx.lineWidth = 2
                    ctx.beginPath(); ctx.moveTo(0, 0); ctx.lineTo(8, height); ctx.lineTo(width, 0); ctx.fill(); ctx.stroke()
                    ctx.fillRect(1, 0, width - 2, 2)
                }
            }
        }
        Image { // Zetti: klein unter der Sprechblase, begleitet im Hintergrund
            Layout.preferredWidth: 96
            Layout.preferredHeight: 120
            Layout.topMargin: 10
            source: "qrc:/zuse/zetti/zetti_" + workbench.zettiPose + ".png"
            fillMode: Image.PreserveAspectFit
            horizontalAlignment: Image.AlignLeft
            smooth: true
            mipmap: true
        }
        Item { Layout.fillHeight: true }
        Rectangle { // Merksatz nach dem Schaffen der Stufe
            visible: workbench.stageDone && workbench.stageMerksatz !== ""
            Layout.fillWidth: true
            Layout.preferredHeight: merk.implicitHeight + 20
            radius: 12
            color: "#FFF7E6"
            border.color: ZTheme.pad
            Text {
                id: merk
                anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; margins: 12 }
                wrapMode: Text.WordWrap
                text: qsTr("Merke: ") + workbench.stageMerksatz
                color: ZTheme.text
                font.pixelSize: 15; font.bold: true
            }
        }
        RowLayout {
            spacing: 8
            ZButton {
                text: root.hintVisible ? qsTr("Tipp aus") : qsTr("Tipp")
                onClicked: root.hintVisible = !root.hintVisible
            }
            ZButton {
                visible: workbench.stageKind === "bauen"
                text: qsTr("Lösung zeigen")
                enabled: !workbench.inside
                onClicked: workbench.showSolution()
            }
        }
    }
}
