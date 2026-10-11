// Maskottchen Tick (docs/maskottchen-tick.md): die Figur in einer Stimmung. Bilder in resources/tick (PNG, 512 px).
// Rein schmueckend; was Tick sagt, steht in der Sprechblase (TickSays) und wird von dort vorgelesen.
import QtQuick

Image {
    id: tick
    property string mood: "freundlich"
    readonly property var moods: ["erklaerend", "fragend", "freundlich", "froehlich", "gut", "motivierend",
                                  "nachdenklich", "neugierig", "ueberrascht", "warnend"]

    source: "qrc:/tick/tick_" + (moods.indexOf(mood) >= 0 ? mood : "freundlich") + ".png"
    width: 96
    height: 96
    fillMode: Image.PreserveAspectFit
    smooth: true
    mipmap: true
    Accessible.ignored: true
}
