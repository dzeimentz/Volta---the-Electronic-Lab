// Aufbauschritt mit Bildern (Stufe 0-2): Feldnamen im Text erscheinen als kleine Kontaktfelder (FieldChip), hinter
// dem ersten Namen eines Bauteils steht sein Schaltzeichen. Der Text bleibt vollstaendig erhalten (Vorlesen, Barrierefreiheit).
import QtQuick
import QtQuick.Controls.Basic

Flow {
    id: step
    property string text
    property int columns: 5
    spacing: 5

    readonly property int lineHeight: 36
    // Feld auf der Grundplatte, am Pult (V+, 0V, S1, P1a, LS1 ...) oder in der Modulleiste (M1 ...)
    readonly property var fieldPattern: /^([A-E](1[0-2]|[1-9])|V\+|0V|S[12]|P[12][asb]|LS[12]|M(1[0-6]|[1-9]))$/

    // Woerter in Stuecke zerlegen: [{kind: "text"|"field"|"symbol", value}]
    readonly property var words: {
        const stems = Texts.partWords().map(e => [e[0], e[1].split(",").map(w => w.trim().toLowerCase())])
        const used = {}
        const out = []
        const parts = step.text.split(" ")
        for (let i = 0; i < parts.length; ++i) {
            const word = parts[i]
            if (word === "") continue
            const m = word.match(/^([(„"»«“]*)(.*?)([.,;:!?)"«»“”]*)$/)
            const lead = m ? m[1] : "", core = m ? m[2] : word, trail = m ? m[3] : ""
            // Feld oder Feldbereich (A1–A2)
            const pieces = core.split(/([–-])/)
            let fields = pieces.length % 2 === 1
            for (let k = 0; k < pieces.length && fields; k += 2) fields = step.fieldPattern.test(pieces[k])
            if (fields && core !== "") {
                const w = []
                if (lead !== "") w.push({kind: "text", value: lead})
                for (let k = 0; k < pieces.length; ++k)
                    w.push(k % 2 === 0 ? {kind: "field", value: pieces[k]} : {kind: "text", value: pieces[k]})
                if (trail !== "") w.push({kind: "text", value: trail})
                out.push(w)
                continue
            }
            // Bauteilname: Wortanfang (auch nach Bindestrich oder Apostroph, z. B. »100-Ω-Widerstand«, »l'ampoule«)
            const sub = core.toLowerCase().split(/[-'’]/)
            let type = ""
            for (let t = 0; t < stems.length && type === ""; ++t) {
                if (used[stems[t][0]]) continue
                for (let k = 0; k < sub.length && type === ""; ++k)
                    if (stems[t][1].some(st => st !== "" && sub[k].startsWith(st))) type = stems[t][0]
            }
            if (type !== "") {
                used[type] = true
                out.push([{kind: "text", value: word}, {kind: "symbol", value: type}])
            } else out.push([{kind: "text", value: word}])
        }
        return out
    }

    Accessible.role: Accessible.StaticText
    Accessible.name: text

    Repeater {
        model: step.words
        Row {
            required property var modelData
            height: step.lineHeight
            spacing: 1
            Repeater {
                model: parent.modelData
                Item {
                    required property var modelData
                    width: modelData.kind === "field" ? chip.width
                         : modelData.kind === "symbol" ? symbolBox.width + 3 : label.implicitWidth
                    height: step.lineHeight
                    Label {
                        id: label
                        visible: parent.modelData.kind === "text"
                        anchors.verticalCenter: parent.verticalCenter
                        text: parent.modelData.kind === "text" ? parent.modelData.value : ""
                        font.pixelSize: Math.round(14 * Theme.fontScale)
                        color: "#1E2B38"
                    }
                    FieldChip {
                        id: chip
                        visible: parent.modelData.kind === "field"
                        anchors.verticalCenter: parent.verticalCenter
                        name: parent.modelData.kind === "field" ? parent.modelData.value : ""
                        columns: step.columns
                    }
                    Rectangle {
                        id: symbolBox
                        visible: parent.modelData.kind === "symbol"
                        x: 3
                        anchors.verticalCenter: parent.verticalCenter
                        width: 44; height: 26; radius: 5
                        color: "#EEF3F8"
                        border.color: "#9FB2C4"
                        SchematicSymbol {
                            anchors.centerIn: parent
                            width: 36; height: 18
                            type: parent.parent.modelData.kind === "symbol" ? parent.parent.modelData.value : ""
                            stroke: Theme.navy
                        }
                    }
                }
            }
        }
    }
}
