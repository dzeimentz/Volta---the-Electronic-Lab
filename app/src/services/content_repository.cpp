#include "content_repository.hpp"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>

#include <algorithm>
#include <cmath>

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

QStringList ContentRepository::bands() const {
    QStringList out;
    const QDir root(m_root);
    for (const QString& d : root.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name))
        if (QFile::exists(root.filePath(d + QStringLiteral("/paket.json")))) out << d;
    return out;
}

QStringList ContentRepository::experimentIds(const QString& band) const {
    QStringList out;
    const QDir dir(QDir(m_root).filePath(band + QStringLiteral("/experiments")));
    for (const QString& f : dir.entryList({QStringLiteral("*.json")}, QDir::Files, QDir::Name))
        out << band + QLatin1Char('.') + f.chopped(5);
    return out;
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
        for (const QJsonValue& p : o.value(QStringLiteral("spannweite")).toArray()) e.spans.push_back(str(p));
        for (const QJsonValue& p : o.value(QStringLiteral("lage")).toArray()) e.layout.push_back(str(p));
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
    for (const QJsonValue& v : vorlage.value(QStringLiteral("anzeigen")).toArray()) out.voltageDisplays.push_back(str(v));
    // Pultstellung zu Beginn (Hauptschalter bleibt aus: Einschalten ist Sache des Kindes)
    const QJsonObject pult = vorlage.value(QStringLiteral("pult_stellung")).toObject();
    if (pult.contains(QStringLiteral("batterie_V"))) {
        const double u = pult.value(QStringLiteral("batterie_V")).toDouble();
        if (std::abs(u - 3.0) < 1e-6)
            out.desk.cells = 2;
        else if (std::abs(u - 4.5) < 1e-6)
            out.desk.cells = 3;
        else if (std::abs(u - 9.0) < 1e-6)
            out.desk.cells = 6;
        else {
            error = QStringLiteral("Vorlage: batterie_V muss 3, 4.5 oder 9 sein");
            return false;
        }
    }
    out.desk.switchClosed = pult.value(QStringLiteral("schalter")).toBool(false);
    out.desk.p1 = pult.value(QStringLiteral("p1")).toDouble(0.5);
    out.desk.p2 = pult.value(QStringLiteral("p2")).toDouble(0.5);
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

bool ContentRepository::parseSuccess(const QJsonObject& o, model::SuccessCondition& out, QString& error) {
    using C = model::SuccessCondition;
    out = C{};
    auto listOf = [&](const QString& key, C::Kind kind) {
        out.kind = kind;
        const QJsonArray a = o.value(key).toArray();
        if (a.isEmpty()) {
            error = QStringLiteral("Erfolg: '%1' braucht eine nicht leere Liste").arg(key);
            return false;
        }
        for (const QJsonValue& v : a) {
            C child;
            if (!parseSuccess(v.toObject(), child, error)) return false;
            out.children.push_back(std::move(child));
        }
        return true;
    };
    if (o.contains(QStringLiteral("alle"))) return listOf(QStringLiteral("alle"), C::Kind::All);
    if (o.contains(QStringLiteral("eine"))) return listOf(QStringLiteral("eine"), C::Kind::Any);
    if (o.contains(QStringLiteral("nicht"))) {
        out.kind = C::Kind::Not;
        C child;
        if (!parseSuccess(o.value(QStringLiteral("nicht")).toObject(), child, error)) return false;
        out.children.push_back(std::move(child));
        return true;
    }
    if (o.contains(QStringLiteral("rolle"))) {
        error = QStringLiteral("Erfolg: 'rolle' wird erst mit der Lernpfad-Engine (Phase 2) unterstuetzt");
        return false;
    }
    out.part = o.value(QStringLiteral("bauteil")).toString().toStdString();
    out.field = o.value(QStringLiteral("feld")).toString().toStdString();
    out.holdS = o.value(QStringLiteral("dauer_s")).toDouble(0.0);
    if (o.contains(QStringLiteral("zustand"))) {
        out.kind = C::Kind::State;
        const QString z = o.value(QStringLiteral("zustand")).toString();
        if (z == QStringLiteral("intakt")) out.state = C::State::Intact;
        else if (z == QStringLiteral("ueberlastet")) out.state = C::State::Overloaded;
        else if (z == QStringLiteral("zerstoert")) out.state = C::State::Destroyed;
        else {
            error = QStringLiteral("Erfolg: unbekannter Zustand '%1'").arg(z);
            return false;
        }
        return true;
    }
    out.kind = C::Kind::Quantity;
    const QString g = o.value(QStringLiteral("groesse")).toString();
    if (g == QStringLiteral("strom_mA")) out.quantity = C::Quantity::CurrentMilliA;
    else if (g == QStringLiteral("spannung_V")) out.quantity = C::Quantity::VoltageV;
    else if (g == QStringLiteral("leistung_mW")) out.quantity = C::Quantity::PowerMilliW;
    else if (g == QStringLiteral("helligkeit")) out.quantity = C::Quantity::Brightness;
    else if (g == QStringLiteral("drehzahl")) out.quantity = C::Quantity::Rpm;
    else if (g == QStringLiteral("frequenz_Hz")) out.quantity = C::Quantity::FrequencyHz;
    else if (g == QStringLiteral("temperatur_C")) out.quantity = C::Quantity::TemperatureC;
    else {
        error = QStringLiteral("Erfolg: unbekannte Groesse '%1'").arg(g);
        return false;
    }
    if (!out.field.empty() && out.quantity != C::Quantity::VoltageV) {
        error = QStringLiteral("Erfolg: am Feld ist nur spannung_V moeglich");
        return false;
    }
    const QJsonArray range = o.value(QStringLiteral("zwischen")).toArray();
    if (range.size() == 2) {
        out.hasMin = out.hasMax = true;
        out.min = range.at(0).toDouble();
        out.max = range.at(1).toDouble();
    }
    if (o.contains(QStringLiteral("min"))) {
        out.hasMin = true;
        out.min = o.value(QStringLiteral("min")).toDouble();
    }
    if (o.contains(QStringLiteral("max"))) {
        out.hasMax = true;
        out.max = o.value(QStringLiteral("max")).toDouble();
    }
    if (!out.hasMin && !out.hasMax) {
        error = QStringLiteral("Erfolg: Groesse ohne 'zwischen', 'min' oder 'max'");
        return false;
    }
    return true;
}

namespace {
double maxHold(const model::SuccessCondition& c) {
    double m = c.holdS;
    for (const auto& ch : c.children) m = std::max(m, maxHold(ch));
    return m;
}
} // namespace

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
    out.durationMin = o.value(QStringLiteral("dauer_min")).toInt();
    auto keys = [&o](const char* name) {
        QStringList l;
        for (const QJsonValue& v : o.value(QLatin1String(name)).toArray()) l << v.toString();
        return l;
    };
    out.questionKey = o.value(QStringLiteral("frage_key")).toString();
    out.guessKeys = keys("vermutungen");
    out.stepKeys = keys("schritte");
    out.hintKeys = keys("hilfen");
    out.explanationKey = o.value(QStringLiteral("erklaerung_key")).toString();
    out.moreKey = o.value(QStringLiteral("mehr_wissen_key")).toString();
    out.furtherKeys = keys("weiterforschen");
    out.realKey = o.value(QStringLiteral("jetzt_in_echt_key")).toString();
    const QJsonObject vorlage = o.value(QStringLiteral("vorlage")).toObject();
    for (const QJsonValue& v : vorlage.value(QStringLiteral("anzeigen")).toArray()) out.shownVoltages << v.toString();
    QString error;
    if (!parseTemplate(vorlage, catalog, out.templateBoard, error)) {
        m_error = QStringLiteral("%1: %2").arg(id, error);
        return false;
    }
    if (!parseSuccess(out.success, out.successCondition, error)) {
        m_error = QStringLiteral("%1: %2").arg(id, error);
        return false;
    }
    out.maxHoldS = maxHold(out.successCondition);
    return true;
}

bool ContentRepository::loadTexts(const QString& band, const QString& language) {
    // Deutsch ist Quellsprache und vollstaendig (docs/i18n.md); fehlende Uebersetzungen fallen darauf zurueck
    QJsonObject de;
    if (!readJson(QStringLiteral("%1/i18n/de.json").arg(band), de)) return false;
    m_texts.clear();
    for (auto it = de.begin(); it != de.end(); ++it) m_texts.insert(it.key(), it.value().toString());
    if (language == QLatin1String("de")) return true;
    QJsonObject o;
    if (!readJson(QStringLiteral("%1/i18n/%2.json").arg(band, language), o)) return false;
    for (auto it = o.begin(); it != o.end(); ++it)
        if (!it.value().toString().isEmpty()) m_texts.insert(it.key(), it.value().toString());
    return true;
}

QString ContentRepository::text(const QString& key) const { return m_texts.value(key, key); }

} // namespace volta::services
