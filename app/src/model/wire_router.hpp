#pragma once

#include <vector>

namespace volta::model {

// Leitungsfuehrung auf der Arbeitsflaeche (docs/ui-spec.md, Leitungen): Leitungen sollen nicht ueber Bauteile laufen.
// Der Router sucht je Leitung einen Weg auf einem Raster (A*, acht Richtungen) um die Bauteilkoerper herum, mit
// Abstand zu ihnen, wenigen Knicken und moeglichst ohne Strecken anderer Leitungen; danach wird der Weg zu wenigen
// Wegpunkten gestrafft (Sichtlinie). Ist der gewohnte Bogen frei, bleibt er (arc = true).
// Reine Geometrie in Pixeln der Arbeitsflaeche, ohne Qt.
struct RouterPoint {
    double x = 0.0, y = 0.0;
};
struct RouterRect {
    double x = 0.0, y = 0.0, w = 0.0, h = 0.0;
};
struct RoutedWire {
    std::vector<RouterPoint> points; // Start, Wegpunkte, Ende
    bool arc = false;                // true: Bogen wie bisher (frei von Bauteilen), points = Start und Ende
};

struct RouterOptions {
    double cell = 8.0;      // Rasterweite
    double clearance = 9.0; // Abstand der Leitungsmitte zu Bauteilen
    double arcLift = 60.0;  // Hoehe der Kontrollpunkte des Bogens (WireItem.qml)
};

class WireRouter {
public:
    // bounds: Arbeitsflaeche; obstacles: Bauteilkoerper; wires: je Leitung Start und Ende
    static std::vector<RoutedWire> route(const RouterRect& bounds, const std::vector<RouterRect>& obstacles,
                                         const std::vector<std::pair<RouterPoint, RouterPoint>>& wires,
                                         const RouterOptions& options = {});
};

} // namespace volta::model
