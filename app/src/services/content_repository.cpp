#include "content_repository.hpp"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>

namespace volta::services {

namespace {
constexpr int kSchemaVersion = 1;

std::string str(const QJsonValue& v) { return v.toString().toStdString(); }
} // namespace

ContentRepository::ContentRepository(QString contentRoot) : m_root(std::move(contentRoot)) {}

bool ContentRepository::readJson(const QString& relativePath, QJsonObject& out) {
    QFile f(QDir(m_root).filePath(relativePath));
    if (!f.open(QIODevice::ReadOnly)) {
        m_error = QStringLiteral("Datei nicht lesbar: %1").arg(f.fileName());
        return false;
    }
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        m_error = QStringLiteral("JSON fehlerhaft in %1: %2").arg(f.fileName(), err.errorString());
        return false;
    }
    out = doc.object();
    return true;
}

bool ContentRepository::loadCatalog(model::Catalog& out) {
    QJsonObject root;
    if (!readJson(QStringLiteral("catalog/bauteile.json"), root)) return false;
    if (root.value(QStringLiteral("schema_version")).toInt() != kSchemaVersion) {
        m_error = QStringLiteral("Bauteilkatalog: Schema-Version nicht unterstuetzt");
        return false;
    }
    out = model::Catalog{};
    for (const QJsonValue& v : root.value(QStringLiteral("bauteile")).toArray()) {
        const QJsonObject o = v.toObject();
        model::CatalogEntry e;
        e.type = str(o.value(QStringLiteral("typ")));
        e.model = str(o.value(QStringLiteral("modell")));
        e.fromStage = o.value(QStringLiteral("ab_stufe")).toInt();
        for (const QJsonValue& p : o.value(QStringLiteral("anschluesse")).toArray()) e.pins.push_back(str(p));
        const QJsonObject params = o.value(QStringLiteral("parameter")).toObject();
        for (auto it = params.begin(); it != params.end(); ++it)
            if (it.value().isDouble()) e.params[it.key().toStdString()] = it.value().toDouble();
        out.parts[e.type] = e;
    }
    for (const QJsonValue& v : root.value(QStringLiteral("module")).toArray()) {
        const QJsonObject o = v.toObject();
        model::ModuleEntry e;
        e.type = str(o.value(QStringLiteral("typ")));
        e.kind = str(o.value(QStringLiteral("art")));
        e.fromStage = o.value(QStringLiteral("ab_stufe")).toInt();
        for (const QJsonValue& p : o.value(QStringLiteral("anschluesse")).toArray()) e.pins.push_back(str(p));
        out.modules[e.type] = e;
    }
    return true;
}

bool ContentRepository::parseTemplate(const QJsonObject& vorlage, const model::Catalog& catalog, model::Board& out,
                                      QString& error) {
    out = model::Board{};
    for (const QJsonValue& v : vorlage.value(QStringLiteral("bauteile")).toArray()) {
        const QJsonObject o = v.toObject();
        model::PartPlacement p;
        p.id = str(o.value(QStringLiteral("id")));
        p.type = str(o.value(QStringLiteral("typ")));
        p.value = o.value(QStringLiteral("wert")).toDouble(0.0);
        p.closed = o.value(QStringLiteral("geschlossen")).toBool(false);
        p.position = o.value(QStringLiteral("stellung")).toDouble(0.5);
        const model::CatalogEntry* e = catalog.part(p.type);
        if (!e) {
            error = QStringLiteral("Vorlage: unbekannter Bauteiltyp '%1'").arg(QString::fromStdString(p.type));
            return false;
        }
        const QJsonValue felder = o.value(QStringLiteral("felder"));
        if (felder.isArray()) { // Felder in Katalogreihenfolge der Anschluesse
            const QJsonArray a = felder.toArray();
            if (static_cast<std::size_t>(a.size()) != e->pins.size()) {
                error = QStringLiteral("Vorlage: Bauteil %1 braucht %2 Felder")
                            .arg(QString::fromStdString(p.id))
                            .arg(e->pins.size());
                return false;
            }
            for (std::size_t k = 0; k < e->pins.size(); ++k)
                p.pins.emplace_back(e->pins[k], str(a.at(static_cast<qsizetype>(k))));
        } else {
            const QJsonObject m = felder.toObject();
            for (const auto& name : e->pins) {
                const QString key = QString::fromStdString(name);
                if (!m.contains(key)) {
                    error = QStringLiteral("Vorlage: Bauteil %1 ohne Anschluss '%2'")
                                .arg(QString::fromStdString(p.id), key);
                    return false;
                }
                p.pins.emplace_back(name, str(m.value(key)));
            }
        }
        out.parts.push_back(std::move(p));
    }
    int wireNo = 0;
    for (const QJsonValue& v : vorlage.value(QStringLiteral("leitungen")).toArray()) {
        const QJsonObject o = v.toObject();
        model::WirePlacement w;
        w.id = o.contains(QStringLiteral("id")) ? str(o.value(QStringLiteral("id"))) : "L" + std::to_string(++wireNo);
        w.from = str(o.value(QStringLiteral("von")));
        w.to = str(o.value(QStringLiteral("nach")));
        w.color = o.contains(QStringLiteral("farbe")) ? str(o.value(QStringLiteral("farbe"))) : std::string("rot");
        out.wires.push_back(std::move(w));
    }
    for (const QJsonValue& v : vorlage.value(QStringLiteral("module")).toArray()) {
        const QJsonObject o = v.toObject();
        model::ModulePlacement m;
        m.id = str(o.value(QStringLiteral("id")));
        m.type = str(o.value(QStringLiteral("typ")));
        m.slot = o.value(QStringLiteral("feld")).toInt(1);
        out.modules.push_back(std::move(m));
    }
    return true;
}

bool ContentRepository::loadExperiment(const QString& id, Experiment& out) {
    const QStringList parts = id.split(QLatin1Char('.'));
    if (parts.size() != 3) {
        m_error = QStringLiteral("Ungueltige Experiment-ID: %1").arg(id);
        return false;
    }
    QJsonObject o;
    if (!readJson(QStringLiteral("%1/experiments/%2.%3.json").arg(parts[0], parts[1], parts[2]), o)) return false;
    model::Catalog catalog;
    if (!loadCatalog(catalog)) return false;
    out = Experiment{};
    out.id = o.value(QStringLiteral("id")).toString();
    out.stage = o.value(QStringLiteral("stufe")).toInt();
    out.destructive = o.value(QStringLiteral("zerstoerend")).toBool();
    out.titleKey = o.value(QStringLiteral("titel_key")).toString();
    out.success = o.value(QStringLiteral("erfolg")).toObject();
    const QJsonObject vorlage = o.value(QStringLiteral("vorlage")).toObject();
    for (const QJsonValue& v : vorlage.value(QStringLiteral("anzeigen")).toArray()) out.shownVoltages << v.toString();
    QString error;
    if (!parseTemplate(vorlage, catalog, out.templateBoard, error)) {
        m_error = QStringLiteral("%1: %2").arg(id, error);
        return false;
    }
    return true;
}

bool ContentRepository::loadTexts(const QString& band, const QString& language) {
    QJsonObject o;
    if (!readJson(QStringLiteral("%1/i18n/%2.json").arg(band, language), o)) return false;
    m_texts.clear();
    for (auto it = o.begin(); it != o.end(); ++it) m_texts.insert(it.key(), it.value().toString());
    return true;
}

QString ContentRepository::text(const QString& key) const { return m_texts.value(key, key); }

} // namespace volta::services
