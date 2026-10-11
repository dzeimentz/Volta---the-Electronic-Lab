#pragma once

#include <QIODevice>
#include <QMap>
#include <QString>
#include <QStringList>

#include "model/board.hpp"
#include "model/catalog.hpp"

namespace volta::services {

// Aufbaukarte als PDF (Werkzeugkasten »Teileliste«, docs/ui-spec.md): Kopf mit Experiment und Forscherfrage,
// Zeichnung der Grundplatte mit allen Bauteilen und Leitungen, Teileliste und nummerierte Aufbauschritte - zum
// Ausdrucken und Nachbauen mit echten Bauteilen. Die Texte kommen fertig uebersetzt aus der Oberflaeche; die
// Zeichnung entsteht hier aus dem Aufbau. Kein Netz, nur die Zieldatei wird geschrieben.
struct BuildCardTexts {
    QString brand;         // Kopfzeile, z. B. "Volta 1 - Aufbaukarte"
    QString title;         // Experiment oder "Freies Labor"
    QString subtitle;      // Stufe
    QString question;      // Forscherfrage (leer: ohne)
    QString warning;       // z. B. "Nur in der App!" (leer: ohne)
    QString boardHeading;
    QString partsHeading;
    QString stepsHeading;
    QStringList parts;     // Zeilen der Teileliste
    QStringList steps;     // Aufbauschritte
    QString footer;        // Sicherheitshinweis und Herkunft
    QMap<QString, QString> labels; // Beschriftung in der Zeichnung je Bauteil/Modul (Kennung -> Text)
};

// Schreibt die Karte (A4 hoch, bei Bedarf mehrere Seiten). false, wenn das Geraet nicht beschreibbar ist.
bool writeBuildCardPdf(QIODevice& out, const model::Board& board, const model::Catalog& catalog,
                       const BuildCardTexts& texts);

} // namespace volta::services
