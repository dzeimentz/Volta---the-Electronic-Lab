// QR-Code zeichnen (LabController::qrCode: size, cells als "0"/"1"), schwarz auf weiss mit Ruhezone.
import QtQuick

Canvas {
    id: canvas
    property var code: ({})
    onCodeChanged: requestPaint()
    onWidthChanged: requestPaint()
    onPaint: {
        const ctx = getContext("2d")
        ctx.fillStyle = "white"
        ctx.fillRect(0, 0, width, height)
        const c = canvas.code
        if (!c || !c.size) return
        const quiet = 4 // Ruhezone in Modulen
        const cell = Math.floor(Math.min(width, height) / (c.size + 2 * quiet))
        const offX = Math.floor((width - cell * c.size) / 2)
        const offY = Math.floor((height - cell * c.size) / 2)
        ctx.fillStyle = "black"
        for (let y = 0; y < c.size; ++y)
            for (let x = 0; x < c.size; ++x)
                if (c.cells.charAt(y * c.size + x) === "1") ctx.fillRect(offX + x * cell, offY + y * cell, cell, cell)
    }
}
