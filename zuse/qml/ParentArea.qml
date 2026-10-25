import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Elternbereich wie bei Volta (nur hinter der Elternschranke): PIN aendern, alle Experimente freigeben, Vorfuehrung
// zeigen, Forscherprofile, Fortschritt und Forscherheft zuruecksetzen (mit Rueckfrage), Datenschutz, was Zuse
// vereinfacht, der Lernpfad im Ueberblick und Kontakt mit Links nach aussen (hier erlaubt, weil hinter der Schranke).
Popup {
    id: area
    objectName: "parentArea"
    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(820, parent ? parent.width - 32 : 820)
    height: Overlay.overlay ? Overlay.overlay.height - 32 : 720
    padding: 0
    closePolicy: Popup.CloseOnEscape
    onOpened: { pinField.text = ""; pinMessage.text = "" }
    background: Rectangle { radius: 12; color: "#FFFDF7"; border.color: ZTheme.navy; border.width: 2 }

    component Heading: Text {
        Layout.fillWidth: true
        Layout.topMargin: 14
        font.pixelSize: Math.round(19 * ZTheme.fontScale)
        font.bold: true
        color: ZTheme.navy
        wrapMode: Text.WordWrap
    }
    component Body: Text {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        font.pixelSize: Math.round(15 * ZTheme.fontScale)
        color: ZTheme.text
        lineHeight: 1.1
    }
    component Point: Body { property string body; text: "•  " + body; leftPadding: 8 }
    component Toggle: Switch { // Schalter mit Wort daneben, mindestens 48 dp
        id: toggle
        Layout.fillWidth: true
        implicitHeight: ZTheme.control
        font.pixelSize: Math.round(15 * ZTheme.fontScale)
        contentItem: Text {
            leftPadding: toggle.indicator.width + 12
            text: toggle.text + "  –  " + (toggle.checked ? qsTr("an") : qsTr("aus"))
            font: toggle.font
            color: ZTheme.text
            wrapMode: Text.WordWrap
            verticalAlignment: Text.AlignVCenter
        }
    }

    Popup { // Rueckfrage vor allem, was sich nicht rueckgaengig machen laesst
        id: confirm
        objectName: "parentConfirm"
        property string question
        property var action: null
        function ask(q, a) { question = q; action = a; open() }
        modal: true
        anchors.centerIn: Overlay.overlay
        width: Math.min(460, area.width - 32)
        padding: 20
        background: Rectangle { radius: 12; color: ZTheme.surface; border.color: ZTheme.bad; border.width: 2 }
        contentItem: ColumnLayout {
            spacing: 16
            Text {
                Layout.fillWidth: true
                text: confirm.question
                wrapMode: Text.WordWrap
                color: ZTheme.text
                font.pixelSize: Math.round(16 * ZTheme.fontScale)
            }
            Text {
                Layout.fillWidth: true
                text: qsTr("Das lässt sich nicht rückgängig machen.")
                color: ZTheme.bad
                font.pixelSize: Math.round(14 * ZTheme.fontScale); font.bold: true
            }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 10
                ZButton { text: qsTr("Abbrechen"); onClicked: confirm.close() }
                ZButton {
                    objectName: "parentConfirmYes"
                    text: qsTr("Ja, löschen")
                    primary: true
                    onClicked: { const a = confirm.action; confirm.close(); if (a) a() }
                }
            }
        }
    }

    Rectangle {
        id: head
        width: parent.width
        height: 64
        radius: 12
        color: ZTheme.navy
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 12; color: ZTheme.navy }
        Text {
            x: 20
            anchors.verticalCenter: parent.verticalCenter
            text: "🔒 " + qsTr("Elternbereich")
            color: "white"
            font.pixelSize: Math.round(20 * ZTheme.fontScale); font.bold: true
        }
        ZButton {
            objectName: "parentClose"
            anchors { right: parent.right; rightMargin: 12; verticalCenter: parent.verticalCenter }
            text: qsTr("Fertig")
            onClicked: area.close()
        }
    }

    Flickable {
        id: flick
        anchors { top: head.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
        contentHeight: column.implicitHeight + 40
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        ColumnLayout {
            id: column
            x: 24
            y: 12
            width: flick.width - 48
            spacing: 8

            // ---- PIN
            Heading { text: qsTr("PIN für Erwachsene") }
            Body {
                visible: settings.parentPinIsDefault
                text: "⚠ " + qsTr("Noch gilt die Start-PIN 1234. Bitte wählen Sie eine eigene, damit Kinder den Elternbereich nicht öffnen.")
                color: "#8A5A00"
                font.bold: true
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                TextField {
                    id: pinField
                    objectName: "parentPinField"
                    Layout.preferredWidth: 220
                    implicitHeight: ZTheme.control
                    echoMode: TextInput.Password
                    inputMethodHints: Qt.ImhDigitsOnly
                    maximumLength: 8
                    validator: RegularExpressionValidator { regularExpression: /[0-9]{0,8}/ }
                    placeholderText: qsTr("Neue PIN (4–8 Ziffern)")
                    font.pixelSize: Math.round(16 * ZTheme.fontScale)
                }
                ZButton {
                    objectName: "parentPinSave"
                    text: qsTr("PIN ändern")
                    enabled: pinField.text.length >= 4
                    onClicked: {
                        const ok = settings.setParentPin(pinField.text)
                        pinMessage.ok = ok
                        pinMessage.text = ok ? "✓ " + qsTr("Die neue PIN gilt ab sofort.") : qsTr("Bitte 4 bis 8 Ziffern eingeben.")
                        if (ok) pinField.text = ""
                    }
                }
            }
            Body {
                id: pinMessage
                property bool ok: true
                visible: text !== ""
                color: ok ? ZTheme.ok : ZTheme.bad
            }

            // ---- Freigaben
            Heading { text: qsTr("Freigaben") }
            Body { text: qsTr("Die Stufen des Lernpfads öffnen sich nacheinander, denn jede Erfindung baut auf den vorigen auf. Für die Schule oder für ältere Kinder können Sie alles sofort freigeben.") }
            Toggle {
                objectName: "parentUnlockAll"
                text: qsTr("Alle Experimente freigeben")
                checked: settings.unlockAll
                onToggled: settings.unlockAll = checked
            }
            Body {
                text: qsTr("Fehlt dafür eine Erfindung aus einem früheren Experiment, leiht Zuse die Musterlösung aus. Baut das Kind sie selbst, ersetzt seine eigene die geliehene.")
                color: ZTheme.muted
            }
            Toggle {
                objectName: "parentDemoVisible"
                text: qsTr("Vorführung auf der Startseite zeigen")
                checked: settings.demoVisible
                onToggled: settings.demoVisible = checked
            }
            Body {
                text: qsTr("Die Vorführung zeigt fertige Aufbauten, zum Beispiel am Beamer. Sie verrät die Lösungen; zu Hause können Sie sie ausblenden.")
                color: ZTheme.muted
            }

            // ---- Forscherprofile
            Heading { text: qsTr("Forscherprofile") }
            Body { text: qsTr("Teilen sich mehrere Kinder ein Gerät, bekommt jedes ein eigenes Profil mit Symbol und Spitznamen – ohne richtigen Namen und ohne Passwort. Fortschritt, Erfindungen und Forscherheft liegen je Profil getrennt.") }
            Toggle {
                objectName: "parentProfiles"
                text: qsTr("Forscherprofile verwenden")
                checked: settings.profilesEnabled
                onToggled: settings.profilesEnabled = checked
            }
            Repeater {
                model: workbench.profiles
                delegate: RowLayout {
                    required property var modelData
                    Layout.fillWidth: true
                    spacing: 10
                    Text {
                        Layout.fillWidth: true
                        text: modelData.symbol + "  " + modelData.name
                        elide: Text.ElideRight
                        color: ZTheme.text
                        font.pixelSize: Math.round(16 * ZTheme.fontScale)
                    }
                    ZButton {
                        objectName: "parentRemoveProfile_" + modelData.id
                        text: qsTr("Löschen")
                        onClicked: confirm.ask(qsTr("Das Profil »%1« mit Fortschritt, Erfindungen und Forscherheft löschen?").arg(modelData.name),
                                               () => workbench.removeProfile(modelData.id))
                    }
                }
            }
            ZButton {
                objectName: "parentRemoveAllProfiles"
                visible: workbench.profiles.length > 0
                text: qsTr("Alle Profile zurücksetzen")
                onClicked: confirm.ask(qsTr("Alle Forscherprofile mit ihrem Fortschritt, ihren Erfindungen und Forscherheften löschen?"),
                                       () => workbench.removeAllProfiles())
            }

            // ---- Fortschritt und Forscherheft
            Heading { text: qsTr("Fortschritt und Forscherheft") }
            Body {
                text: (workbench.profilesEnabled && workbench.currentProfile.id !== undefined
                       ? qsTr("Gilt für das Profil »%1«.").arg(workbench.currentProfile.name) + " " : "")
                      + qsTr("Fortschritt, Erfindungen und Forscherheft liegen nur auf diesem Gerät.")
            }
            Flow {
                Layout.fillWidth: true
                spacing: 10
                visible: !workbench.profilesEnabled || workbench.currentProfile.id !== undefined
                ZButton {
                    objectName: "parentResetProgress"
                    text: qsTr("Lernpfad zurücksetzen")
                    onClicked: confirm.ask(qsTr("Den Lernpfad zurücksetzen? Alle Experimente gelten danach als nicht geschafft, und die eigenen Erfindungen werden gelöscht."),
                                           () => workbench.resetProgress())
                }
                ZButton {
                    objectName: "parentResetJournal"
                    text: qsTr("Forscherheft leeren")
                    onClicked: confirm.ask(qsTr("Das Forscherheft leeren? Alle Vermutungen, Beobachtungen, Notizen und Fotos werden gelöscht."),
                                           () => workbench.resetJournal())
                }
            }
            Body {
                visible: workbench.profilesEnabled && workbench.currentProfile.id === undefined
                text: qsTr("Wählen Sie zuerst ein Forscherprofil, dann lässt sich sein Fortschritt zurücksetzen.")
                color: ZTheme.muted
            }

            // ---- Datenschutz
            Heading { text: qsTr("Datenschutz") }
            Body { text: qsTr("Zuse erhebt keine Daten. Es gibt kein Konto, keine Werbung, keine Käufe in der App und keine Verbindung ins Internet; Kamera und Mikrofon werden nicht verwendet.") }
            Body { text: qsTr("Nur auf diesem Gerät gespeichert werden:") }
            Point { body: qsTr("welche Experimente geschafft sind") }
            Point { body: qsTr("die eigenen Erfindungen und der Aufbau auf der Werkbank") }
            Point { body: qsTr("das Forscherheft mit Vermutungen, Beobachtungen, Notizen und den Fotos, die das Kind selbst hinzufügt") }
            Point { body: qsTr("die Einstellungen (Sprache, Schriftgröße und Ähnliches)") }
            Body { text: qsTr("Beim Deinstallieren der App wird alles gelöscht.") }

            // ---- So rechnet Zuse
            Heading { text: qsTr("So rechnet Zuse") }
            Body { text: qsTr("Was die Lampen zeigen, berechnet eine echte Logiksimulation, nicht vorbereitete Ergebnisse. Deshalb verhalten sich auch selbst erfundene Schaltungen richtig. Einige Vereinfachungen legen wir offen:") }
            Point { body: qsTr("Jede Leitung kennt nur 0, 1 und »unbestimmt« (zum Beispiel ein offener Eingang). Spannungen und Ströme werden nicht berechnet.") }
            Point { body: qsTr("Jedes Gatter braucht gleich lange, einen Schritt. Kurze Störimpulse durch Laufzeitunterschiede entstehen dadurch trotzdem.") }
            Point { body: qsTr("Speicher beginnen beim Einschalten mit 0. In echt ist ihr Anfangswert zufällig.") }

            // ---- Lernpfad
            Heading { text: qsTr("Der Lernpfad") }
            Repeater {
                model: workbench.levels
                delegate: Point {
                    required property var modelData
                    body: qsTr("Stufe %1: %2 – %3").arg(modelData.number).arg(modelData.title).arg(modelData.question)
                }
            }

            // ---- Kontakt
            Heading { text: qsTr("Kontakt und Hilfe") }
            Body { text: "DC Software Engineering, Daniel Zeimentz, Aufm Rothenhügel 26, 54666 Irrel, " + qsTr("Deutschland") }
            Flow {
                Layout.fillWidth: true
                spacing: 10
                ZButton {
                    objectName: "parentWebsite"
                    text: qsTr("Website öffnen")
                    onClicked: Qt.openUrlExternally("https://dc-software-engineering.com")
                }
                ZButton {
                    objectName: "parentMail"
                    text: qsTr("E-Mail schreiben")
                    onClicked: Qt.openUrlExternally("mailto:info@dc-software-engineering.com?subject=Zuse%201")
                }
            }
        }
    }
}
