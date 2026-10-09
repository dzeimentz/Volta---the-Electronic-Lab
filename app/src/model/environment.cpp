#include "environment.hpp"

#include <algorithm>
#include <cmath>

#include "anchor.hpp"
#include "volta/core/types.hpp"

namespace volta::model {

bool partCenter(const PartPlacement& part, double& col, double& row) {
    double c = 0.0, r = 0.0;
    int n = 0;
    for (const auto& pin : part.pins) {
        const std::string& f = pin.second;
        if (classifyAnchor(f) != AnchorKind::Field) continue;
        r += f[0] - 'A';
        c += std::stoi(f.substr(1));
        ++n;
    }
    if (n == 0) return false;
    col = c / n;
    row = r / n;
    return true;
}

namespace {
// Summe der Wirkung aller eingeschalteten Werkzeuge eines Typs am Bauteil (0..peak je Werkzeug)
double toolEffect(const Board& board, const PartPlacement& part, const std::string& type, double peak, double reach) {
    double col = 0.0, row = 0.0;
    if (!partCenter(part, col, row)) return 0.0;
    double sum = 0.0;
    for (const auto& t : board.desk.tools) {
        if (t.type != type || !t.on) continue;
        const double d = std::hypot(t.col - col, t.row - row);
        sum += peak / (1.0 + (d / reach) * (d / reach));
    }
    return sum;
}
} // namespace

double illuminanceAt(const Board& board, const PartPlacement& part) {
    const int room = std::clamp(board.desk.roomLight, 0, 2);
    return kRoomLux[room] + toolEffect(board, part, kToolFlashlight, kFlashLux, kFlashReach);
}

double wetnessAt(const Board& board, const PartPlacement& part) {
    return std::min(1.0, toolEffect(board, part, kToolWater, 1.0, kWaterReach));
}

double airTemperatureAt(const Board& board, const PartPlacement& part) {
    return core::kAmbientK + std::min(kDryerRiseK, toolEffect(board, part, kToolDryer, kDryerRiseK, kDryerReach));
}

} // namespace volta::model
