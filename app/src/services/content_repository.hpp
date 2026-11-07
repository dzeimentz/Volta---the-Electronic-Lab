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
    bool colourFixed = false; // »farbe_fest«: die LED-Farbe der Vorlage ist Teil des Versuchs
    bool scopeShown = false;  // Vorlage stellt das Oszilloskop ein: beim Start eingeblendet (ab Volta 2)
    // »start_frequenz_Hz« des Generators (ab Volta 3): Frequenz beim Start, wenn das Kind sie selbst finden soll
    // (Resonanz); die Vorlage behaelt ihre Frequenz (Musteraufbau, Inhaltstest). 0 = wie die Vorlage
    double startGenFrequencyHz = 0.0;
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
    // "aufbau_von": frueheres Experiment, auf dessen Aufbau dieses aufbaut (ab Volta 2); "aufbau_von_schritt": Schritt
    // (ab 1), in dem der Knopf »Aufbau uebernehmen« steht, intern ab 0
    QString buildsOn;
    int buildsOnStep = 0;
};

// Stufe des Lernpfads (content/<band>/stufen.json)
struct Stage {
    int number = 0;
    QString titleKey;
    QString questionKey;
};

// Pultteil im Geraetefach (paket.json "pult_nach_bedarf", Pult nach Bedarf ab Volta 2)
// Frei wird es ab einer Stufe ("ab_stufe"), mit einem Experiment ("ab_experiment", sobald es freigeschaltet ist)
// oder mit dem ersten freigeschalteten Experiment, das es benutzt ("bei_nutzung").
struct DeskOffer {
    QString part; // Pultteil (model::deskGroupNames), z. B. "batterie2"
    int fromStage = 0;
    QString fromExperiment;
    bool onUse = false;
};

// Laedt Inhaltspakete (JSON) und Bauteilkatalog aus einem Inhaltsverzeichnis. Kein Netzwerkzugriff.
class ContentRepository {
public:
    explicit ContentRepository(QString contentRoot);

    const QString& root() const { return m_root; }
    QString lastError() const { return m_error; }

    // Bauteilkatalog; mit band nur die Eintraege dieses Bandes und seiner Grundbaende (paket.json "basis"),
    // Eintraege eines Grundbandes sind von Anfang an frei (Stufe 0)
    bool loadCatalog(model::Catalog& out, const QString& band = {});
    // Baende im Inhaltsverzeichnis (Verzeichnisse mit paket.json), z. B. "volta1"
    QStringList bands() const;
    // Grundbaende und Band selbst, vom aeltesten an (z. B. volta2 -> volta1, volta2)
    QStringList lineage(const QString& band);
    // IDs aller Experimente eines Bandes (z. B. "volta1.2.4"), numerisch nach Stufe und Nummer sortiert
    QStringList experimentIds(const QString& band) const;
    // Pultteile des Geraetefachs; leer: festes Pult wie Volta 1
    QList<DeskOffer> deskOffers(const QString& band);
    // Startbreite der Grundplatte (paket.json "start_spalten": 8, 10 oder 12); 0: wie Volta 1 (kleine Bretter der
    // ersten Stufen, freies Labor 12 Spalten)
    int startColumns(const QString& band);
    // Stufen eines Bandes; leer, wenn stufen.json fehlt
    QList<Stage> stages(const QString& band);
    // Experiment nach id (z. B. "volta1.2.4") aus content/<band>/experiments/<stufe>.<nr>.json
    bool loadExperiment(const QString& id, Experiment& out);
    // Texte einer Sprache (z. B. "fr") fuer einen Band, Deutsch als Rueckfall; text() liefert den Schluessel, wenn
    // er auch auf Deutsch fehlt. false, wenn die Sprachdatei fehlt (dann gilt Deutsch). Texte der Grundbaende
    // (lineage) gelten mit, der Band selbst hat Vorrang.
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
