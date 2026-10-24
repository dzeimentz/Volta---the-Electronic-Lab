// ContentSmokeTest (docs/testing.md, Inhalte): Jedes Experiment wird mit seiner Vorlage simuliert; die Vorlage muss
// ihre eigene Erfolgsbedingung erfuellen. Dazu Einzeltests des Bewerters fuer Erfolgsbedingungen.
#include <QJsonDocument>
#include <QtTest>

#include <algorithm>
#include <cmath>

#include "model/circuit_check.hpp"
#include "model/netlist_builder.hpp"
#include "model/success.hpp"
#include "services/content_repository.hpp"
#include "volta/core/components/switch.hpp"
#include "volta/core/simulator.hpp"
#include "volta_paths.hpp"

using namespace volta;

namespace {

model::SuccessCondition parse(const char* json) {
    model::SuccessCondition c;
    QString error;
    const bool ok = services::ContentRepository::parseSuccess(QJsonDocument::fromJson(json).object(), c, error);
    if (!ok) qWarning() << error;
    return c;
}

// Aufbau: LED mit Vorwiderstand r an 9 V (Hauptschalter an)
model::Board ledBoard(double r) {
    model::Board b;
    b.desk.mainSwitch = true;
    b.parts.push_back({"R1", "widerstand", {{"a", "B3"}, {"b", "B4"}}, r});
    b.parts.push_back({"D1", "led_rot", {{"anode", "B4"}, {"kathode", "C4"}}});
    b.wires.push_back({"L1", "P:+9V", "B3", "rot"});
    b.wires.push_back({"L2", "C4", "P:0V", "schwarz"});
    return b;
}

// Wie ein Kind drueckt der Test gedrueckte Taster der Vorlage erst kurz nach dem Einschalten (Zeitmodul: der Elko
// muss sich vorher aufladen koennen)
constexpr double kPressAfterS = 0.5;

// Simuliert bis zu maxS; liefert die Zeit, zu der der Bewerter erstmals erfuellt ist (oder -1)
double timeToSuccess(const model::Board& board, const model::Catalog& cat, model::SuccessEvaluator& eval, double maxS,
                     std::vector<std::string>* buildErrors = nullptr,
                     std::vector<model::CircuitFinding>* findings = nullptr) {
    model::Board start = board;
    std::vector<std::string> pressed;
    for (auto& p : start.parts)
        if (p.type == "taster" && p.closed) {
            p.closed = false;
            pressed.push_back(p.id);
        }
    model::Netlist net = model::buildNetlist(start, cat);
    if (buildErrors) *buildErrors = net.errors;
    core::Simulator sim;
    if (sim.setCircuit(std::move(net.circuit)) != core::Status::Ok) return -1.0;
    const int steps = static_cast<int>(std::lround(maxS / core::kDefaultStepS));
    const int pressStep = static_cast<int>(std::lround(kPressAfterS / core::kDefaultStepS));
    for (int i = 0; i < steps; ++i) {
        if (i == pressStep)
            for (const auto& id : pressed)
                if (auto* sw = dynamic_cast<core::Switch*>(&sim.circuit().component(net.parts.at(id))))
                    sw->setClosed(true);
        sim.step();
        eval.update(sim, net);
        if (eval.reached()) {
            // Fehlersuche am erreichten Ziel: mit dem Aufbau der Vorlage (Taster gedrueckt) und den Messwerten
            if (findings) {
                auto lookup = [&](const std::string& id) -> const core::ComponentReading* {
                    auto it = net.parts.find(id);
                    if (it == net.parts.end()) {
                        it = net.desk.find(id);
                        if (it == net.desk.end()) return nullptr;
                    }
                    return it->second < sim.probe().readings.size() ? &sim.probe().readings[it->second] : nullptr;
                };
                *findings = model::checkCircuit({board, cat, lookup});
            }
            return sim.timeS();
        }
    }
    return -1.0;
}

} // namespace

class ContentSmokeTest : public QObject {
    Q_OBJECT

private:
    model::Catalog m_catalog;

private slots:
    void initTestCase() {
        services::ContentRepository repo(QStringLiteral(VOLTA_CONTENT_DIR));
        QVERIFY2(repo.loadCatalog(m_catalog), qPrintable(repo.lastError()));
    }

    void parsesConditions() {
        QString error;
        model::SuccessCondition c;
        QVERIFY(services::ContentRepository::parseSuccess(
            QJsonDocument::fromJson(R"({"alle":[{"bauteil":"D1","groesse":"strom_mA","zwischen":[5,25],"dauer_s":5},
                                               {"nicht":{"bauteil":"D1","zustand":"zerstoert"}}]})")
                .object(),
            c, error));
        QCOMPARE(c.kind, model::SuccessCondition::Kind::All);
        QCOMPARE(c.children.size(), std::size_t(2));
        QCOMPARE(c.children[0].holdS, 5.0);
        QVERIFY(c.children[0].hasMin && c.children[0].hasMax);
        QCOMPARE(c.children[1].kind, model::SuccessCondition::Kind::Not);
        // Fehlerhafte Bedingungen werden abgewiesen
        QVERIFY(!services::ContentRepository::parseSuccess(
            QJsonDocument::fromJson(R"({"bauteil":"D1","groesse":"strom_mA"})").object(), c, error));
        QVERIFY(!services::ContentRepository::parseSuccess(
            QJsonDocument::fromJson(R"({"bauteil":"D1","groesse":"unsinn","min":1})").object(), c, error));
        QVERIFY(!services::ContentRepository::parseSuccess(
            QJsonDocument::fromJson(R"({"feld":"B4","groesse":"strom_mA","min":1})").object(), c, error));
        QVERIFY(!services::ContentRepository::parseSuccess(QJsonDocument::fromJson(R"({"alle":[]})").object(), c,
                                                            error));
    }

    void holdDurationIsRespected() {
        model::SuccessEvaluator eval(parse(R"({"bauteil":"D1","groesse":"strom_mA","zwischen":[5,25],"dauer_s":2})"));
        const double t = timeToSuccess(ledBoard(470.0), m_catalog, eval, 5.0);
        QVERIFY2(t >= 2.0 && t < 2.1, qPrintable(QString::number(t)));
    }

    void alternativeBuildAlsoSucceeds() {
        // Zwei 220-Ohm-Widerstaende in Reihe statt 470 Ohm (docs/testing.md): gilt ebenfalls als geloest
        model::Board b = ledBoard(220.0);
        b.parts.push_back({"R2", "widerstand", {{"a", "A3"}, {"b", "B3"}}, 220.0});
        b.wires[0].to = "A3";
        model::SuccessEvaluator eval(parse(R"({"alle":[{"bauteil":"D1","groesse":"strom_mA","zwischen":[5,25],"dauer_s":1},
                                                       {"bauteil":"D1","zustand":"intakt"}]})"));
        QVERIFY(timeToSuccess(b, m_catalog, eval, 3.0) > 0.0);
    }

    void withoutResistorFails() {
        // LED ohne Widerstand: wird zerstoert, Bedingung nie erfuellt (docs/testing.md: "ohne Widerstand scheitert")
        model::Board b = ledBoard(470.0);
        b.parts.erase(b.parts.begin());
        b.wires[0].to = "B4";
        model::SuccessEvaluator eval(parse(R"({"alle":[{"bauteil":"D1","groesse":"strom_mA","zwischen":[5,25],"dauer_s":1},
                                                       {"bauteil":"D1","zustand":"intakt"}]})"));
        QCOMPARE(timeToSuccess(b, m_catalog, eval, 3.0), -1.0);
        model::SuccessEvaluator broken(parse(R"({"bauteil":"D1","zustand":"zerstoert"})"));
        QVERIFY(timeToSuccess(b, m_catalog, broken, 3.0) > 0.0);
    }

    void anyNotAndFieldVoltage() {
        model::SuccessEvaluator eval(parse(R"({"eine":[{"bauteil":"D1","groesse":"strom_mA","min":100},
                                                       {"feld":"B4","groesse":"spannung_V","zwischen":[1.5,2.5]}]})"));
        QVERIFY(timeToSuccess(ledBoard(470.0), m_catalog, eval, 0.1) > 0.0);
        model::SuccessEvaluator never(parse(R"({"nicht":{"bauteil":"D1","zustand":"intakt"}})"));
        QCOMPARE(timeToSuccess(ledBoard(470.0), m_catalog, never, 0.5), -1.0);
    }

    void unknownReferencesAreReported() {
        model::SuccessEvaluator eval(parse(R"({"alle":[{"bauteil":"X9","zustand":"intakt"},
                                                       {"feld":"E8","groesse":"spannung_V","min":1}]})"));
        QCOMPARE(timeToSuccess(ledBoard(470.0), m_catalog, eval, 0.05), -1.0);
        QCOMPARE(eval.errors().size(), std::size_t(2)); // jeder Fehler nur einmal gemeldet
    }

    // Jede Vorlage jedes Bandes erfuellt ihre eigene Erfolgsbedingung (mit eingeschaltetem Hauptschalter)
    void everyTemplateReachesItsSuccess_data() {
        QTest::addColumn<QString>("id");
        services::ContentRepository repo(QStringLiteral(VOLTA_CONTENT_DIR));
        int count = 0;
        for (const QString& band : repo.bands())
            for (const QString& id : repo.experimentIds(band)) {
                QTest::newRow(qPrintable(id)) << id;
                ++count;
            }
        QVERIFY(count > 0);
    }

    void everyTemplateReachesItsSuccess() {
        QFETCH(QString, id);
        services::ContentRepository repo(QStringLiteral(VOLTA_CONTENT_DIR));
        services::Experiment exp;
        QVERIFY2(repo.loadExperiment(id, exp), qPrintable(repo.lastError()));
        model::Board board = exp.templateBoard;
        board.desk.mainSwitch = true;
        model::SuccessEvaluator eval(exp.successCondition);
        std::vector<std::string> buildErrors;
        const double limit = std::min(exp.maxHoldS + 5.0, 60.0);
        const double t = timeToSuccess(board, m_catalog, eval, limit, &buildErrors);
        QVERIFY2(buildErrors.empty(), buildErrors.empty() ? "" : buildErrors.front().c_str());
        QVERIFY2(eval.errors().empty(), eval.errors().empty() ? "" : eval.errors().front().c_str());
        QVERIFY2(t > 0.0, qPrintable(QStringLiteral("%1: Vorlage erreicht ihre Erfolgsbedingung nicht in %2 s")
                                         .arg(id)
                                         .arg(limit)));
    }

    // LED-Farbe frei (ohne »farbe_fest«): die Vorlage erreicht ihr Ziel mit jeder LED-Farbe - sonst muss das
    // Experiment die Farbe festlegen, damit ein Kind mit einer anderen LED nicht haengen bleibt
    void templatesWorkWithAnyLedColour_data() { everyTemplateReachesItsSuccess_data(); }
    void templatesWorkWithAnyLedColour() {
        QFETCH(QString, id);
        services::ContentRepository repo(QStringLiteral(VOLTA_CONTENT_DIR));
        services::Experiment exp;
        QVERIFY2(repo.loadExperiment(id, exp), qPrintable(repo.lastError()));
        if (exp.colourFixed || exp.destructive) return;
        if (std::none_of(exp.templateBoard.parts.begin(), exp.templateBoard.parts.end(),
                         [](const model::PartPlacement& p) { return model::isLed(p.type); }))
            return;
        for (const char* colour : {"led_rot", "led_gruen", "led_gelb", "led_blau"}) {
            model::Board board = exp.templateBoard;
            for (auto& p : board.parts)
                if (model::isLed(p.type)) p.type = colour;
            board.desk.mainSwitch = true;
            model::SuccessEvaluator eval(exp.successCondition);
            std::vector<std::string> buildErrors;
            const double t = timeToSuccess(board, m_catalog, eval, std::min(exp.maxHoldS + 5.0, 60.0), &buildErrors);
            QVERIFY2(t > 0.0, qPrintable(QStringLiteral("%1 mit %2: Ziel nicht erreicht").arg(id, QLatin1String(colour))));
        }
    }

    // Fehlersuche: am erreichten Ziel jeder Vorlage keine Auffaelligkeit (sie darf nie einen funktionierenden Aufbau
    // bemaengeln); zerstoerende Experimente ausgenommen
    void templatesPassCircuitCheck_data() { everyTemplateReachesItsSuccess_data(); }

    void templatesPassCircuitCheck() {
        QFETCH(QString, id);
        services::ContentRepository repo(QStringLiteral(VOLTA_CONTENT_DIR));
        services::Experiment exp;
        QVERIFY2(repo.loadExperiment(id, exp), qPrintable(repo.lastError()));
        if (exp.destructive) QSKIP("zerstoerendes Experiment");
        model::Board board = exp.templateBoard;
        board.desk.mainSwitch = true;
        model::SuccessEvaluator eval(exp.successCondition);
        std::vector<model::CircuitFinding> findings;
        QVERIFY(timeToSuccess(board, m_catalog, eval, std::min(exp.maxHoldS + 5.0, 60.0), nullptr, &findings) > 0.0);
        QVERIFY2(findings.empty(), findings.empty() ? "" : qPrintable(QStringLiteral("%1: %2").arg(id,
                                                                QString::fromStdString(findings.front().message.text()))));
    }

    // Gegenprobe fuer Sensor-Experimente: Ohne die Werkzeuge der Vorlage (bzw. bei hellem Raum, wenn die Vorlage nur
    // das Raumlicht verdunkelt) darf die Erfolgsbedingung nicht erfuellt sein - die Schaltung reagiert wirklich
    void sensorTemplatesNeedTheirEnvironment_data() { everyTemplateReachesItsSuccess_data(); }

    void sensorTemplatesNeedTheirEnvironment() {
        QFETCH(QString, id);
        services::ContentRepository repo(QStringLiteral(VOLTA_CONTENT_DIR));
        services::Experiment exp;
        QVERIFY2(repo.loadExperiment(id, exp), qPrintable(repo.lastError()));
        model::Board board = exp.templateBoard;
        bool hasSensor = false;
        for (const auto& p : board.parts)
            if (p.type == "fotowiderstand" || p.type == "heissleiter" || p.type == "feuchtesonde") hasSensor = true;
        if (!hasSensor) QSKIP("kein Sensor");
        if (!board.desk.tools.empty()) board.desk.tools.clear();
        else if (board.desk.roomLight < 2) board.desk.roomLight = 2;
        else QSKIP("Vorlage ohne eigene Umgebung");
        board.desk.mainSwitch = true;
        model::SuccessEvaluator eval(exp.successCondition);
        const double limit = std::min(exp.maxHoldS + 5.0, 60.0);
        QVERIFY2(timeToSuccess(board, m_catalog, eval, limit) < 0.0,
                 qPrintable(QStringLiteral("%1: Erfolg auch ohne Werkzeug bzw. Raumlicht").arg(id)));
    }
};

QTEST_GUILESS_MAIN(ContentSmokeTest)
#include "test_content_smoke.moc"
