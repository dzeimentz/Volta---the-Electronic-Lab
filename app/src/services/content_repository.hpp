#pragma once

#include <QHash>
#include <QJsonObject>
#include <QString>
#include <QStringList>

#include "model/board.hpp"
#include "model/catalog.hpp"

namespace volta::services {

// Geladenes Experiment (docs/content-format.md). Die Vorlage ist bereits als Aufbau umgesetzt.
struct Experiment {
    QString id;
    int stage = 0;
    bool destructive = false;
    QString titleKey;
    model::Board templateBoard; // Vorlage (Anzeige, Schritt-fuer-Schritt-Hilfe, ContentSmokeTest)
    QStringList shownVoltages;  // "anzeigen": Felder mit vorab gesetzter Spannungsanzeige
    QJsonObject success;        // "erfolg": zustandsbasierte Bedingungen
};

// Laedt Inhaltspakete (JSON) und Bauteilkatalog aus einem Inhaltsverzeichnis. Kein Netzwerkzugriff.
class ContentRepository {
public:
    explicit ContentRepository(QString contentRoot);

    const QString& root() const { return m_root; }
    QString lastError() const { return m_error; }

    bool loadCatalog(model::Catalog& out);
    // Experiment nach id (z. B. "volta1.2.4") aus content/<band>/experiments/<stufe>.<nr>.json
    bool loadExperiment(const QString& id, Experiment& out);
    // Texte einer Sprache (z. B. "de") fuer einen Band; text() liefert den Schluessel, wenn er fehlt.
    bool loadTexts(const QString& band, const QString& language);
    QString text(const QString& key) const;

    // Wandelt eine Vorlage ("vorlage" eines Experiments) in einen Aufbau um.
    static bool parseTemplate(const QJsonObject& vorlage, const model::Catalog& catalog, model::Board& out,
                              QString& error);

private:
    bool readJson(const QString& relativePath, QJsonObject& out);

    QString m_root;
    QString m_error;
    QHash<QString, QString> m_texts;
};

} // namespace volta::services
