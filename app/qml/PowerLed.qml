// Betriebslampe der Geraete (Generator im Pult, Oszilloskop): an gruen mit Schein, angehalten gelb mit Schein, aus
// dunkel ohne Schein - Helligkeit und Schein, nicht nur die Farbe; der Zustand steht zusaetzlich im Namen fuer
// Screenreader (accessibleName) und, wo Platz ist, als Text daneben
import QtQuick

Rectangle {
    id: led
    property bool on: false
    property bool paused: false
    property string accessibleName
    width: 12; height: 12; radius: 6
    color: !on ? "#26352C" : paused ? "#F2C230" : "#4CE36B"
    border.color: !on ? "#5E6E66" : paused ? "#FFE9A0" : "#B8FFC6"
    border.width: 1.5
    Rectangle { // Schein
        visible: led.on
        anchors.centerIn: parent
        width: 22; height: 22; radius: 11
        color: "transparent"
        border.color: led.paused ? "#80F2C230" : "#804CE36B"
        border.width: 4
    }
    Accessible.role: Accessible.Indicator
    Accessible.name: accessibleName
}
