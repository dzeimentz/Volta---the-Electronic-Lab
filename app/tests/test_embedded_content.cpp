// Eingebettete Inhaltspakete (Android, iOS; docs/platforms.md): Die Qt-Ressource :/content enthaelt dieselben Dateien
// wie content/ im Quellbaum (ohne Schemata und CLAUDE.md), und ContentRepository laedt daraus Katalog, Stufen,
// Experimente und Texte wie aus dem Dateisystem.
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QtTest>

#include "services/content_repository.hpp"

using namespace volta;

namespace {

// Relative Pfade aller Dateien unter root, sortiert
QStringList filesBelow(const QString& root) {
    QStringList out;
    QDirIterator it(root, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) out << QDir(root).relativeFilePath(it.next());
    out.sort();
    return out;
}

} // namespace

class EmbeddedContentTest : public QObject {
    Q_OBJECT

private slots:
    void resourceMatchesSourceTree() {
        QStringList expected;
        for (const QString& f : filesBelow(QStringLiteral(VOLTA_CONTENT_DIR)))
            if (!f.startsWith(QLatin1String("schema/")) && f != QLatin1String("CLAUDE.md")) expected << f;
        QVERIFY(!expected.isEmpty());
        QCOMPARE(filesBelow(QStringLiteral(":/content")), expected);
        for (const QString& f : expected) {
            QFile a(QStringLiteral(VOLTA_CONTENT_DIR "/") + f);
            QFile b(QStringLiteral(":/content/") + f);
            QVERIFY(a.open(QIODevice::ReadOnly) && b.open(QIODevice::ReadOnly));
            QVERIFY2(a.readAll() == b.readAll(), qPrintable(f));
        }
    }

    void repositoryLoadsFromResource() {
        services::ContentRepository repo(QStringLiteral(":/content"));
        model::Catalog catalog;
        QVERIFY2(repo.loadCatalog(catalog), qPrintable(repo.lastError()));
        const QStringList bands = repo.bands();
        QVERIFY(bands.contains(QStringLiteral("volta1")));
        QVERIFY(!repo.stages(QStringLiteral("volta1")).isEmpty());
        const QStringList ids = repo.experimentIds(QStringLiteral("volta1"));
        QVERIFY(!ids.isEmpty());
        for (const QString& id : ids) {
            services::Experiment e;
            QVERIFY2(repo.loadExperiment(id, e), qPrintable(id + QStringLiteral(": ") + repo.lastError()));
        }
        QVERIFY(repo.loadTexts(QStringLiteral("volta1"), QStringLiteral("fr")));
    }
};

QTEST_GUILESS_MAIN(EmbeddedContentTest)
#include "test_embedded_content.moc"
