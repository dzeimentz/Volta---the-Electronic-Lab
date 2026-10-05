#pragma once

#include <QHash>
#include <QJsonObject>
#include <QString>
#include <QStringList>

#include "model/board.hpp"
#include "model/catalog.hpp"
#include "model/success.hpp"

namespace volta::services {

// Geladenes Experiment (docs/content-format.md). Die Vorlage ist bereits als Aufbau umgesetzt.
struct Experiment {
    QString id;
    int stage = 0;
    bool destructive = false;
    QString titleKey;
    model::Board templateBoard; // Vorlage (Anzeige, Schritt-fuer-Schritt-Hilfe, ContentSmokeTest)
    QStringList shownVoltages;  // "anzeigen": Felder mit vorab gesetzter Spannungsanzeige
    QJsonObject success;        // "erfolg": zustandsbasierte Bedingungen (Rohform)
    model::SuccessCondition successCondition; // ... ausgewertet
    double maxHoldS = 0.0;      // laengste Haltedauer (dauer_s) in der Bedingung
    int durationMin = 0;
    // Forschungskreislauf (Textschluessel): Frage, Vermutungen, Schritte, Hilfen, Erklaerung ...
    QString questionKey;
    QStringList guessKeys;
    QStringList stepKeys;
    QStringList hintKeys;       // die letzte Hilfe ist die Loesung
    QString explanationKey;
    QString moreKey;
    QStringList furtherKeys;
    QString realKey;
    QStringList freeParts;      // "bauteile_frei": Bauteiltypen im Bauteilfach (und "leitung")
    QStringList observationKeys; // "beobachtungen": Auswahl nach dem Versuch
    QList<int> guessMatches;     // "vermutung_passt": Vermutungen, die zum Ergebnis passen
    QList<int> guessOpen;        // "vermutung_offen": Vermutungen ohne Aussage (z. B. »weiss nicht«)
};

// Stufe des Lernpfads (content/<band>/stufen.json)
struct Stage {
    int number = 0;
    QString titleKey;
    QString questionKey;
};

// Laedt Inhaltspakete (JSON) und Bauteilkatalog aus einem Inhaltsverzeichnis. Kein Netzwerkzugriff.
class ContentRepository {
public:
    explicit ContentRepository(QString contentRoot);

    const QString& root() const { return m_root; }
    QString lastError() const { return m_error; }

    bool loadCatalog(model::Catalog& out);
    // Baende im Inhaltsverzeichnis (Verzeichnisse mit paket.json), z. B. "volta1"
    QStringList bands() const;
    // IDs aller Experimente eines Bandes (z. B. "volta1.2.4"), numerisch nach Stufe und Nummer sortiert
    QStringList experimentIds(const QString& band) const;
    // Stufen eines Bandes; leer, wenn stufen.json fehlt
    QList<Stage> stages(const QString& band);
    // Experiment nach id (z. B. "volta1.2.4") aus content/<band>/experiments/<stufe>.<nr>.json
    bool loadExperiment(const QString& id, Experiment& out);
    // Texte einer Sprache (z. B. "fr") fuer einen Band, Deutsch als Rueckfall; text() liefert den Schluessel, wenn
    // er auch auf Deutsch fehlt. false, wenn die Sprachdatei fehlt (dann gilt Deutsch).
    bool loadTexts(const QString& band, const QString& language);
    QString text(const QString& key) const;

    // Wandelt eine Erfolgsbedingung ("erfolg") in einen Bedingungsbaum um.
    static bool parseSuccess(const QJsonObject& erfolg, model::SuccessCondition& out, QString& error);
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
