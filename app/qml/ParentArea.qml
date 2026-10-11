// Elternbereich (docs/ui-spec.md, docs/privacy-safety.md): nur hinter der Elternschranke erreichbar. Freigaben,
// Zuruecksetzen von Fortschritt und Forscherheft (mit Rueckfrage), Datenschutz, offengelegte Vereinfachungen der
// Simulation, Lernpfad im Ueberblick, Kontakt mit Links nach aussen (hier erlaubt, weil hinter der Schranke).
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs

Popup {
    id: area
    required property var controller
    property var settings: null
    readonly property var managed: typeof volta_managed !== "undefined" ? volta_managed : null
    readonly property bool soundManaged: managed !== null && managed.soundMode !== ""
    readonly property bool profilesManaged: managed !== null && managed.profiles >= 0
    readonly property var audioRoute: typeof volta_audioRoute !== "undefined" ? volta_audioRoute : null

    objectName: "parentArea"
    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(760, parent ? parent.width - 32 : 760)
    height: parent ? parent.height - 32 : 700
    padding: 0
    closePolicy: Popup.CloseOnEscape
    background: Rectangle { radius: 12; color: "#FFFDF7"; border.color: Theme.navy; border.width: 2 }

    component Heading: Label {
        width: parent.width
        font.pixelSize: Math.round(17 * Theme.fontScale)
        font.bold: true
        color: Theme.navy
        topPadding: 10
    }
    component Body: Label {
        width: parent.width
        wrapMode: Text.Wrap
        font.pixelSize: Math.round(14 * Theme.fontScale)
        lineHeight: 1.1
        color: "#1E2B38"
    }
    component Point: Label {
        width: parent.width
        leftPadding: 16
        wrapMode: Text.Wrap
        font.pixelSize: Math.round(14 * Theme.fontScale)
        color: "#1E2B38"
        text: "• " + body
        property string body
    }

    // Kopf
    Rectangle {
        id: head
        width: parent.width
        height: 64
        radius: 12
        color: Theme.navy
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 12; color: Theme.navy }
        Label {
            x: 20
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Elternbereich")
            color: "white"
            font.pixelSize: Math.round(20 * Theme.fontScale)
            font.bold: true
        }
        ToolButton {
            objectName: "parentAreaClose"
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            width: Theme.control; height: Theme.control
            contentItem: Item { DrawnIcon { anchors.centerIn: parent; kind: "close"; color: "white"; size: 20 } }
            onClicked: area.close()
            Accessible.name: qsTr("Schließen")
        }
    }

    ScrollView {
        id: scroll
        anchors { left: parent.left; right: parent.right; top: head.bottom; bottom: parent.bottom; margins: 20 }
        contentWidth: availableWidth
        clip: true

        Column {
            width: scroll.availableWidth
            spacing: 8

            // ---- Von der Schule verwaltet (Hinweis, wenn eine Geraeteverwaltung Vorgaben macht)
            Rectangle {
                objectName: "managedNotice"
                visible: area.managed !== null && area.managed.managed
                width: parent.width
                height: managedText.implicitHeight + 20
                radius: 8
                color: "#E6EEF7"
                border.color: Theme.accent
                Label {
                    id: managedText
                    x: 10; y: 10
                    width: parent.width - 20
                    wrapMode: Text.Wrap
                    font.pixelSize: Math.round(14 * Theme.fontScale)
                    color: "#1E2B38"
                    text: qsTr("Einige Einstellungen gibt Ihre Schule bzw. die Geräteverwaltung vor; sie lassen sich hier nicht ändern.")
                }
            }

            // ---- Klassenzimmer: Ton
            Heading { text: qsTr("Ton im Klassenzimmer") }
            Body { text: qsTr("Damit es in der Klasse ruhig bleibt, können Sie den Ton ganz abschalten oder nur über Kopfhörer erlauben. Das gilt auch für das Vorlesen.") }
            Column {
                spacing: 0
                Repeater {
                    model: [["normal", qsTr("Normal")], ["kopfhoerer", qsTr("Nur mit Kopfhörern")], ["aus", qsTr("Aus")]]
                    RadioButton {
                        required property var modelData
                        objectName: "classroomSound_" + modelData[0]
                        height: Theme.control
                        text: modelData[1]
                        enabled: !area.soundManaged && area.settings !== null
                        checked: (area.soundManaged ? area.managed.soundMode : (area.settings ? area.settings.classroomSound : "normal")) === modelData[0]
                        onClicked: area.settings.classroomSound = modelData[0]
                    }
                }
            }
            Body {
                text: area.audioRoute && area.audioRoute.headphones ? qsTr("Kopfhörer erkannt.") : qsTr("Keine Kopfhörer erkannt.")
                color: "#4A5866"
            }

            // ---- Freigaben
            Heading { text: qsTr("Freigaben") }
            Body { text: qsTr("Die Stufen des Lernpfads öffnen sich nacheinander, damit Kinder nicht mit zu vielen Bauteilen auf einmal beginnen. Für die Schule oder für ältere Kinder können Sie alles sofort freigeben.") }
            Row {
                spacing: 8
                Switch {
                    objectName: "parentUnlockAll"
                    checked: area.settings ? area.settings.unlockAll : false
                    enabled: area.settings !== null
                    onToggled: area.settings.unlockAll = checked
                    Accessible.name: qsTr("Alle Stufen und Bauteile freigeben")
                }
                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Alle Stufen und Bauteile freigeben")
                    font.pixelSize: Math.round(14 * Theme.fontScale)
                    color: "#1E2B38"
                }
            }

            // ---- Forscherprofile (geteilte Geraete)
            Heading { text: qsTr("Forscherprofile") }
            Body { text: qsTr("Teilen sich mehrere Kinder ein Gerät, bekommt jedes ein eigenes Profil mit Symbol und Spitznamen – ohne richtigen Namen und ohne Passwort. Fortschritt, Forscherheft und der letzte Aufbau werden je Profil gespeichert. Beim Start fragt Volta dann: »Wer forscht heute?«") }
            Row {
                spacing: 8
                Switch {
                    objectName: "parentProfiles"
                    checked: area.profilesManaged ? area.managed.profiles > 0 : (area.settings ? area.settings.profilesEnabled : false)
                    enabled: area.settings !== null && !area.profilesManaged
                    onToggled: area.settings.profilesEnabled = checked
                    Accessible.name: qsTr("Forscherprofile verwenden")
                }
                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Forscherprofile verwenden")
                    font.pixelSize: Math.round(14 * Theme.fontScale)
                    color: "#1E2B38"
                }
            }
            Repeater {
                model: area.controller.profiles
                Row {
                    required property var modelData
                    spacing: 10
                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        width: 260
                        elide: Text.ElideRight
                        text: modelData.symbol + "  " + modelData.name
                        font.pixelSize: Math.round(15 * Theme.fontScale)
                        color: "#1E2B38"
                    }
                    Button {
                        objectName: "parentRemoveProfile_" + modelData.id
                        height: Theme.control
                        text: qsTr("Löschen")
                        onClicked: confirm.ask(qsTr("Das Profil »%1« mit Fortschritt, Forscherheft und Aufbau löschen?").arg(modelData.name),
                                               () => area.controller.removeProfile(modelData.id))
                    }
                }
            }
            Button {
                objectName: "parentRemoveAllProfiles"
                visible: area.controller.profiles.length > 0
                height: Theme.control
                text: qsTr("Alle Profile zurücksetzen")
                onClicked: confirm.ask(qsTr("Alle Forscherprofile mit ihrem Fortschritt, Forscherheft und Aufbau löschen?"),
                                       () => area.controller.removeAllProfiles())
            }

            // ---- Mitnehmen auf ein anderes Geraet (Datei: ganzes Profil; QR-Code: nur der Fortschritt, ohne Namen)
            Heading { text: qsTr("Mitnehmen auf ein anderes Gerät") }
            Body { text: area.controller.profilesEnabled && area.controller.currentProfile.id === undefined
                         ? qsTr("Wählen Sie zuerst ein Forscherprofil, dann lässt es sich mitnehmen.")
                         : qsTr("Als Datei geht alles mit: Symbol, Spitzname, Fortschritt, Forscherheft und der letzte Aufbau. Der QR-Code enthält nur, welche Experimente erledigt sind – ohne Namen.") }
            Flow {
                width: parent.width
                spacing: 10
                readonly property bool ready: !area.controller.profilesEnabled || area.controller.currentProfile.id !== undefined
                Button {
                    objectName: "parentExportFile"
                    height: Theme.control
                    enabled: parent.ready
                    text: qsTr("Als Datei speichern")
                    onClicked: saveDialog.open()
                }
                Button {
                    objectName: "parentImportFile"
                    height: Theme.control
                    text: qsTr("Aus Datei laden")
                    onClicked: openDialog.open()
                }
                Button {
                    objectName: "parentShowQr"
                    height: Theme.control
                    enabled: parent.ready
                    text: qsTr("Fortschritt als QR-Code zeigen")
                    onClicked: qrPopup.open()
                }
            }
            Body {
                id: transferResult
                visible: text !== ""
                text: ""
                color: Theme.accent
            }

            // ---- Fortschritt und Forscherheft
            Heading { text: qsTr("Fortschritt und Forscherheft") }
            Body { text: qsTr("Der Fortschritt im Lernpfad und das Forscherheft liegen nur auf diesem Gerät. Zurücksetzen lässt sich nicht rückgängig machen.") }
            Flow {
                width: parent.width
                spacing: 10
                Button {
                    objectName: "parentResetProgress"
                    height: Theme.control
                    text: qsTr("Lernpfad-Fortschritt zurücksetzen")
                    onClicked: confirm.ask(qsTr("Den Fortschritt im Lernpfad zurücksetzen? Alle Experimente gelten danach als nicht erledigt."),
                                           () => area.controller.resetProgress())
                }
                Button {
                    objectName: "parentResetJournal"
                    height: Theme.control
                    text: qsTr("Forscherheft leeren")
                    onClicked: confirm.ask(qsTr("Das Forscherheft leeren? Alle Vermutungen, Beobachtungen und Notizen werden gelöscht."),
                                           () => area.controller.resetJournal())
                }
            }

            // ---- Datenschutz
            Heading { text: qsTr("Datenschutz") }
            Body { text: qsTr("Volta erhebt keine Daten. Es gibt kein Konto, keine Werbung, keine Käufe in der App und keine Verbindung ins Internet; Kamera und Mikrofon werden nicht verwendet.") }
            Body { text: qsTr("Nur auf diesem Gerät gespeichert werden:") }
            Point { body: qsTr("welche Experimente erledigt sind, mit Datum") }
            Point { body: qsTr("das Forscherheft mit Vermutungen, Beobachtungen und Notizen") }
            Point { body: qsTr("der zuletzt gebaute Aufbau, damit es nach einem Neustart weitergeht") }
            Point { body: qsTr("die Einstellungen (Sprache, Schriftgröße, Ton und Ähnliches)") }
            Body { text: qsTr("Beim Deinstallieren der App wird alles gelöscht.") }

            // ---- So rechnet Volta
            Heading { text: qsTr("So rechnet Volta") }
            Body { text: qsTr("Was in Volta passiert, ergibt sich aus einer echten Schaltungssimulation, nicht aus vorbereiteten Ergebnissen. Deshalb verhalten sich auch selbst erfundene Schaltungen plausibel. Einige Vereinfachungen sind für Kinder sinnvoll; wir legen sie offen:") }
            Point { body: qsTr("Das Tonmodul ist ein Verhaltensmodell: Es liefert seine Töne direkt, statt jede Schwingung zu berechnen. Blinkmodul, Zeitmodul und Verstärkermodul sind dagegen echte Transistorschaltungen.") }
            Point { body: qsTr("Elektromagnet und Relais werden ohne Induktivität gerechnet; die Spannungsspitze beim Abschalten fehlt deshalb.") }
            Point { body: qsTr("Der Lautsprecher ist eine Last von 8 Ω ohne Schwingspule.") }
            Point { body: qsTr("Bauteile erwärmen sich und kühlen schneller ab als in echt, damit man es beobachten kann.") }
            Point { body: qsTr("Die Batterie hat einen Innenwiderstand, Leitungen einen sehr kleinen Widerstand; gerechnet wird in Schritten von einer Millisekunde.") }
            Point { body: qsTr("Im Blinkmodul wird der Durchbruch der Basis-Emitter-Strecke nicht nachgebildet; in echt wird jede Hälfte des Takts dadurch etwas kürzer.") }

            // ---- Lernpfad
            Heading { text: qsTr("Der Lernpfad") }
            Repeater {
                model: area.controller.stages
                Point {
                    required property var modelData
                    body: qsTr("Stufe %1: %2 – %3").arg(modelData.number).arg(modelData.title).arg(modelData.question)
                }
            }

            // ---- Kontakt
            Heading { text: qsTr("Kontakt und Hilfe") }
            Body { text: "DC Software Engineering, Daniel Zeimentz, Irrel (DE)" }
            Flow {
                width: parent.width
                spacing: 10
                Button {
                    objectName: "parentWebsite"
                    height: Theme.control
                    text: qsTr("Website öffnen")
                    onClicked: Qt.openUrlExternally("https://dc-software-engineering.com")
                }
                Button {
                    objectName: "parentMail"
                    height: Theme.control
                    text: qsTr("E-Mail schreiben")
                    onClicked: Qt.openUrlExternally("mailto:info@dc-software-engineering.com?subject=Volta%201")
                }
            }
            Item { width: 1; height: 12 }
        }
    }

    // Profildatei speichern bzw. laden (Systemdialog; Android: Dateiauswahl ohne Speicherberechtigung)
    FileDialog {
        id: saveDialog
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("Volta-Profil (*.volta1profil)")]
        defaultSuffix: "volta1profil"
        currentFile: "file:///" + (area.controller.currentProfile.name || "volta1") + ".volta1profil"
        onAccepted: transferResult.text = area.controller.exportProfile(selectedFile)
                    ? qsTr("Gespeichert.") : qsTr("Speichern hat nicht geklappt.")
    }
    FileDialog {
        id: openDialog
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Volta-Profil (*.volta1profil)"), qsTr("Alle Dateien (*)")]
        onAccepted: {
            const info = area.controller.inspectProfileFile(selectedFile)
            if (!info.ok) { transferResult.text = qsTr("Diese Datei ist kein Volta-Profil."); return }
            const file = selectedFile
            const who = (info.symbol || "") + " " + (info.name || "")
            confirm.ask(area.controller.profilesEnabled
                        ? qsTr("Neues Profil »%1« mit %2 erledigten Experimenten anlegen?").arg(who.trim()).arg(info.done)
                        : qsTr("Den Stand auf diesem Gerät durch die Datei ersetzen (%1 erledigte Experimente)? Der bisherige Stand geht verloren.").arg(info.done),
                        () => { transferResult.text = area.controller.importProfileFile(file)
                                                      ? qsTr("Übernommen.") : qsTr("Laden hat nicht geklappt.") },
                        qsTr("Übernehmen"))
        }
    }

    // QR-Code mit dem Fortschritt: auf dem anderen Geraet mit der Kamera-App scannen
    Popup {
        id: qrPopup
        objectName: "qrPopup"
        anchors.centerIn: parent
        modal: true
        padding: 20
        readonly property var code: opened ? area.controller.qrCode(area.controller.progressLink()) : ({})
        background: Rectangle { radius: 12; color: "white"; border.color: Theme.navy; border.width: 2 }
        contentItem: Column {
            spacing: 12
            Label {
                width: 360
                wrapMode: Text.Wrap
                text: qsTr("Scannen Sie den Code auf dem anderen Gerät mit der Kamera-App. Volta öffnet sich dort und fragt, ob der Fortschritt übernommen werden soll.")
                font.pixelSize: Math.round(14 * Theme.fontScale)
            }
            Canvas {
                id: qrCanvas
                objectName: "qrCanvas"
                anchors.horizontalCenter: parent.horizontalCenter
                width: 300; height: 300
                onPaint: {
                    const ctx = getContext("2d")
                    ctx.fillStyle = "white"
                    ctx.fillRect(0, 0, width, height)
                    const c = qrPopup.code
                    if (!c.size) return
                    const quiet = 4 // Ruhezone in Modulen
                    const cell = Math.floor(width / (c.size + 2 * quiet))
                    const off = Math.floor((width - cell * c.size) / 2)
                    ctx.fillStyle = "black"
                    for (let y = 0; y < c.size; ++y)
                        for (let x = 0; x < c.size; ++x)
                            if (c.cells.charAt(y * c.size + x) === "1") ctx.fillRect(off + x * cell, off + y * cell, cell, cell)
                }
                Connections { target: qrPopup; function onCodeChanged() { qrCanvas.requestPaint() } }
            }
            Button {
                anchors.right: parent.right
                height: Theme.control
                text: qsTr("Schließen")
                onClicked: qrPopup.close()
            }
        }
    }

    // Rueckfrage vor dem Loeschen bzw. Uebernehmen (okText: Beschriftung des Bestaetigungsknopfs)
    Dialog {
        id: confirm
        property var action: null
        property string question: ""
        property string okText: ""
        function ask(text, act, ok) {
            confirm.question = text
            confirm.action = act
            confirm.okText = ok || ""
            confirm.open()
        }
        anchors.centerIn: parent
        modal: true
        width: Math.min(440, area.width - 32)
        title: confirm.okText === "" ? qsTr("Wirklich löschen?") : qsTr("Bitte bestätigen")
        contentItem: Column {
            spacing: 16
            Label {
                width: confirm.width - confirm.leftPadding - confirm.rightPadding
                wrapMode: Text.Wrap
                text: confirm.question
                font.pixelSize: Math.round(15 * Theme.fontScale)
            }
            Row {
                anchors.right: parent.right
                spacing: 12
                Button { objectName: "parentConfirmCancel"; height: Theme.control; text: qsTr("Abbrechen"); onClicked: confirm.reject() }
                Button { objectName: "parentConfirmOk"; height: Theme.control; text: confirm.okText === "" ? qsTr("Ja, löschen") : confirm.okText; onClicked: confirm.accept() }
            }
        }
        onAccepted: if (confirm.action) confirm.action()
    }
}
