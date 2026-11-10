// Aufbauschritt mit Bildern (Stufe 0-4): Feldnamen im Text erscheinen als kleine Kontaktfelder (FieldChip), hinter
// dem ersten Namen eines Bauteils steht sein Bild wie im Bauteilfach (Bauteil oder Schaltzeichen, Umschalter im Fach).
// Die Steckfelder eines Bauteils (»A2 und A3«, »A2–A3«, »B3/B4/C3/C4«, »von A3 nach A4«) stehen unmittelbar nebeneinander.
// Ein Wert ohne Bauteilnamen (»470 Ω A2–A3«) zeigt das Bild des Widerstands; die Kennung nach einem Bauteilnamen
// (»Elko C1 D3–D4«) bleibt Text.
// Der Text bleibt vollstaendig erhalten (Vorlesen, Barrierefreiheit).
import QtQuick
import QtQuick.Controls.Basic

Flow {
    id: step
    property string text
    property int columns: 5
    property bool partImages: true   // true: Bauteil wie auf dem Steckbrett, false: Schaltzeichen
    property var templateParts: []   // [{type, fields}] des Musteraufbaus
    property var catalog: []         // catalogParts (Anschlusszahl fuer das Bauteilbild)
    spacing: 5

    readonly property int lineHeight: 36
    // Feld auf der Grundplatte, am Pult (V+, 0V, S1, P1a, LS1 ...; ab Volta 2 +B2, −B2, W~a, G0, KS+ ...) oder in
    // der Modulleiste (M1 ...)
    readonly property var fieldPattern:
        /^([A-E](1[0-2]|[1-9])|V\+|0V|S[12]|P[12][asb]|LS[12]|\+B2|−B2|W~[ab]|G0?|KS[+−]|M(1[0-6]|[1-9]))$/
    // Bindewoerter zwischen den zwei Feldern eines Bauteils bzw. davor (DE, EN, FR, LU)
    readonly property var joinWords: ["und", "and", "et", "an", "a", "nach", "to", "à", "op", "bis", "mit", "with"]
    readonly property var fromWords: ["von", "from", "de", "vu", "vun"]
    // »mit A1 und A2 mit KS-«: folgt auf das zweite Feld ein solches Wort und wieder ein Feld, sind es zwei Leitungen
    readonly property var wireWords: ["mit", "to", "à", "mat"]

    function pinsOf(type) {
        for (let i = 0; i < catalog.length; ++i) if (catalog[i].type === type) return catalog[i].pins
        return 2
    }
    // beide Felder gehoeren zu einem zweipoligen Bauteil (bei Relais und Transistor meint »B3 und B4« zwei Leitungen)
    function samePart(a, b) {
        for (let i = 0; i < templateParts.length; ++i) {
            const f = templateParts[i].fields
            if (f.length === 2 && f.indexOf(a) >= 0 && f.indexOf(b) >= 0) return true
        }
        return false
    }

    // Woerter in Stuecke zerlegen: [{kind: "text"|"field"|"symbol", value}]; field: einzelnes Feld des Wortes
    readonly property var words: {
        // Namen aus mehreren Woertern (»variable capacitor«) bleiben ein Wort, sonst traefe »capacitor« allein
        const stems = Texts.partWords().map(e => [e[0], e[1].split(",").map(w => w.trim().toLowerCase().replace(/ /g, " "))])
        let text = step.text
        stems.forEach(e => e[1].forEach(st => {
            if (st.indexOf(" ") < 0) return
            const re = new RegExp(st.replace(/[.*+?^${}()|[\]\\]/g, "\\$&").replace(/ /g, " "), "gi")
            text = text.replace(re, m => m.replace(/ /g, " "))
        }))
        const used = {}
        const list = []
        // Zahl und Einheit bleiben zusammen (»10 kΩ« nicht ueber zwei Zeilen)
        const parts = text.replace(/([0-9]) ([kM]?Ω|µF|V)(?=[\s.,;:)]|$)/g, "$1 $2").split(" ")
        for (let i = 0; i < parts.length; ++i) {
            const word = parts[i]
            if (word === "") continue
            const m = word.match(/^([(„"»«“]*)(.*?)([.,;:!?)"«»“”]*)$/)
            const lead = m ? m[1] : "", core = m ? m[2] : word, trail = m ? m[3] : ""
            // Feld oder Feldbereich (A1–A2, B3/B4/C3/C4): die Felder eines Bereichs stehen direkt nebeneinander
            const pieces = core.split(/[–\-\/]/)
            let fields = core !== ""
            for (let k = 0; k < pieces.length && fields; ++k) fields = step.fieldPattern.test(pieces[k])
            if (fields) {
                // mehr als zwei Felder (Relais B3/B4/C3/C4): je zwei zusammen, damit die Zeile umbrechen kann
                for (let s = 0; s < pieces.length; s += 2) {
                    const chunk = pieces.slice(s, s + 2)
                    const first = s === 0, last = s + 2 >= pieces.length
                    const w = []
                    if (first && lead !== "") w.push({kind: "text", value: lead})
                    for (let k = 0; k < chunk.length; ++k) w.push({kind: "field", value: chunk[k]})
                    if (last && trail !== "") w.push({kind: "text", value: trail})
                    list.push({pieces: w, field: pieces.length === 1 ? pieces[0] : "", lead: first ? lead : "",
                               trail: last ? trail : "", raw: word})
                }
                continue
            }
            // Bauteilname: Wortanfang (auch nach Bindestrich oder Apostroph, z. B. »100-Ω-Widerstand«, »l'ampoule«)
            const sub = core.toLowerCase().split(/[-'’]/)
            let type = "", named = false
            for (let t = 0; t < stems.length && type === ""; ++t) {
                for (let k = 0; k < sub.length && type === ""; ++k)
                    if (stems[t][1].some(st => st !== "" && sub[k].startsWith(st))) {
                        named = true
                        if (!used[stems[t][0]]) type = stems[t][0]
                    }
            }
            // Widerstandswert ohne Namen (»470 Ω«, »10 kΩ«): Bild des Widerstands
            if (type === "" && !used["widerstand"] && /^[0-9][0-9,.]* [kM]?Ω$/.test(core)
                    && step.templateParts.some(p => p.type === "widerstand"))
                type = "widerstand"
            // Kondensator oder Elko: im Englischen und Franzoesischen heissen beide gleich, der Musteraufbau entscheidet
            if (type === "kondensator" || type === "elko") {
                const other = type === "kondensator" ? "elko" : "kondensator"
                if (!step.templateParts.some(p => p.type === type) && step.templateParts.some(p => p.type === other))
                    type = other
            }
            if (type !== "") used[type] = true
            // Spule: Wert aus den folgenden Woertern (»Spule 1 H«), sonst die Spule aus dem Musteraufbau
            if (type === "spule_100mh") {
                const after = ((parts[i + 1] || "") + " " + (parts[i + 2] || "")).toLowerCase()
                const coils = step.templateParts.map(p => p.type).filter(t => t.startsWith("spule_"))
                let pick = coils.length > 0 ? coils[0] : ""
                for (let k = 0; k < coils.length; ++k) {
                    const v = Texts.partName(coils[k]).toLowerCase().replace(/^.* ([0-9]+ [µm]?h)$/, "$1")
                    if (after.startsWith(v)) { pick = coils[k]; break }
                }
                if (pick !== "") type = pick
            }
            // LED: die Farbe aus dem Nachbarwort (»grüne Leuchtdiode«, »LED verte«), sonst aus dem Musteraufbau
            if (type === "led_rot") {
                const near = [parts[i - 1] || "", parts[i + 1] || ""].map(w => w.toLowerCase())
                let first = ""
                for (let k = 0; k < step.templateParts.length; ++k) {
                    const t = step.templateParts[k].type
                    if (!t.startsWith("led_")) continue
                    if (first === "") first = t
                    const colour = Texts.colorName(t.substring(4)).toLowerCase()
                    if (near.some(w => w.startsWith(colour))) { first = t; break }
                }
                if (first !== "") type = first
            }
            list.push({pieces: type !== "" ? [{kind: "text", value: word}, {kind: "symbol", value: type}]
                                           : [{kind: "text", value: word}],
                       field: "", raw: word, part: named})
        }
        // »A2 und A3«, »von A3 nach A4«: gehoeren beide Felder zu einem Bauteil, stehen sie direkt nebeneinander
        const out = []
        for (let i = 0; i < list.length; ++i) {
            const a = list[i], j = list[i + 1], b = list[i + 2]
            // Kennung direkt nach dem Bauteilnamen und vor seinen Feldern (»Elko C1 D3–D4«): kein Feld
            if (a.field !== "" && a.lead === "" && a.trail === "" && i > 0 && list[i - 1].part === true
                    && j && j.pieces.some(p => p.kind === "field")) {
                out.push({pieces: [{kind: "text", value: a.raw}], field: "", raw: a.raw})
                continue
            }
            const c = list[i + 3], d = list[i + 4]
            const twoWires = b && b.trail === "" && c && d && d.pieces.some(p => p.kind === "field")
                    && step.wireWords.indexOf(c.raw.toLowerCase()) >= 0
            if (a.field !== "" && a.trail === "" && j && b && b.field !== "" && b.lead === "" && !twoWires
                    && step.joinWords.indexOf(j.raw.toLowerCase()) >= 0 && step.samePart(a.field, b.field)) {
                const prev = out.length > 0 ? out[out.length - 1] : null
                if (prev && prev.field === "" && step.fromWords.indexOf(prev.raw.toLowerCase()) >= 0) out.pop()
                const w = []
                if (a.lead !== "") w.push({kind: "text", value: a.lead})
                w.push({kind: "field", value: a.field}, {kind: "field", value: b.field})
                if (b.trail !== "") w.push({kind: "text", value: b.trail})
                out.push({pieces: w, field: "", raw: ""})
                i += 2
                continue
            }
            out.push(a)
        }
        return out.map(w => w.pieces)
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
                    id: piece
                    required property var modelData
                    readonly property bool isText: modelData.kind === "text"
                    readonly property bool isField: modelData.kind === "field"
                    readonly property bool isSymbol: modelData.kind === "symbol"
                    readonly property bool isModule: isSymbol && modelData.value.endsWith("modul")
                    // Bauteilbild nur fuer Bauteile der Grundplatte (Module und Pultteile: Schaltzeichen)
                    readonly property bool showPart: isSymbol && step.partImages && !isModule && modelData.value !== "lautsprecher"
                    // Module in der Bauteil-Ansicht: Platte in ihrer Farbe mit Piktogramm
                    readonly property bool showModule: isSymbol && step.partImages && isModule
                    width: isField ? chip.width : isSymbol ? symbolBox.width + 3 : label.implicitWidth
                    height: step.lineHeight
                    Label {
                        id: label
                        visible: piece.isText
                        anchors.verticalCenter: parent.verticalCenter
                        text: piece.isText ? piece.modelData.value : ""
                        font.pixelSize: Math.round(14 * Theme.fontScale)
                        color: "#1E2B38"
                    }
                    FieldChip {
                        id: chip
                        visible: piece.isField
                        anchors.verticalCenter: parent.verticalCenter
                        name: piece.isField ? piece.modelData.value : ""
                        columns: step.columns
                    }
                    Rectangle {
                        id: symbolBox
                        visible: piece.isSymbol
                        x: 3
                        anchors.verticalCenter: parent.verticalCenter
                        width: piece.showPart ? 62 : 44
                        height: piece.showPart ? 30 : 26
                        radius: 5
                        color: piece.showModule ? Theme.modulePlate(piece.modelData.value)
                             : piece.showPart ? Theme.partTheme(step.themeOf(piece.modelData.value)).tint : "#EEF3F8"
                        border.color: "#9FB2C4"
                        ModuleIcon {
                            visible: piece.showModule
                            anchors.centerIn: parent
                            height: 20
                            type: piece.showModule ? piece.modelData.value : ""
                        }
                        SchematicSymbol {
                            visible: !piece.showPart && !piece.showModule
                            anchors.centerIn: parent
                            width: 36; height: 18
                            type: piece.isSymbol && !piece.showPart && !piece.showModule ? piece.modelData.value : ""
                            stroke: Theme.navy
                        }
                        PartPreview {
                            visible: piece.showPart
                            anchors.fill: parent
                            anchors.margins: 2
                            type: piece.showPart ? piece.modelData.value : ""
                            pins: piece.showPart ? step.pinsOf(piece.modelData.value) : 2
                        }
                    }
                }
            }
        }
    }

    function themeOf(type) {
        for (let i = 0; i < catalog.length; ++i) if (catalog[i].type === type) return catalog[i].theme
        return ""
    }
}
