// Rendert das App-Symbol (SVG) in PNG-Dateien verschiedener Groessen (Qt Svg, ohne Fenster).
// Aufruf: render_icon <eingabe.svg> <ausgabeverzeichnis> <groesse>...
// Erzeugt <ausgabeverzeichnis>/volta_<groesse>.png. Das ICO fuer Windows baut tools/make_icons.py daraus.
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QSvgRenderer>
#include <cstdio>

int main(int argc, char* argv[]) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app(argc, argv);
    if (argc < 4) {
        std::fprintf(stderr, "Aufruf: render_icon <eingabe.svg> <ausgabeverzeichnis> <groesse>...\n");
        return 2;
    }
    QSvgRenderer svg(QString::fromLocal8Bit(argv[1]));
    if (!svg.isValid()) {
        std::fprintf(stderr, "SVG ungueltig: %s\n", argv[1]);
        return 1;
    }
    for (int i = 3; i < argc; ++i) {
        const int size = QByteArray(argv[i]).toInt();
        QImage img(size, size, QImage::Format_ARGB32_Premultiplied);
        img.fill(Qt::transparent);
        QPainter p(&img);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        svg.render(&p);
        p.end();
        // Dateiname nach dem SVG: volta.svg -> volta_<n>.png, zuse1.svg -> zuse1_<n>.png
        const QString base = QFileInfo(QString::fromLocal8Bit(argv[1])).completeBaseName();
        const QString out = QStringLiteral("%1/%2_%3.png").arg(QString::fromLocal8Bit(argv[2]), base).arg(size);
        if (!img.save(out)) return 1;
    }
    return 0;
}
