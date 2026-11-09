// Auswertung eines Frequenzgangs (Volta 3, Frequenzgang-Schreiber): Grenzfrequenzen liegen dort, wo die Kurve
// 3 dB unter ihren Hoechstwert faellt; zwischen zwei Punkten wird logarithmisch in der Frequenz interpoliert.
#include "volta/core/frequency_response.hpp"

#include <algorithm>
#include <cmath>

namespace volta::core {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kCutDb = 3.0;     // Grenzfrequenz: 3 dB unter dem Hoechstwert (1/sqrt(2) der Spannung)
constexpr double kEdgeDb = 1.0;    // Rand gilt als Durchlass, wenn er hoechstens 1 dB unter dem Hoechstwert liegt
constexpr double kPeakingDb = 1.0; // Resonanzueberhoehung eines Tief-/Hochpasses ab 1 dB ueber dem Durchlass

// Frequenz, bei der die Kurve zwischen i und i + 1 den Pegel level kreuzt (logarithmisch interpoliert)
double crossing(const std::vector<double>& f, const std::vector<double>& db, std::size_t i, double level) {
    const double d = db[i + 1] - db[i];
    const double t = d == 0.0 ? 0.0 : std::clamp((level - db[i]) / d, 0.0, 1.0);
    return std::exp(std::log(f[i]) + t * (std::log(f[i + 1]) - std::log(f[i])));
}

// Gipfel bzw. Tal bei k genauer: Parabel durch drei Punkte ueber log f
double refineExtremum(const std::vector<double>& f, const std::vector<double>& db, std::size_t k) {
    if (k == 0 || k + 1 >= f.size()) return f[k];
    const double x0 = std::log(f[k - 1]), x1 = std::log(f[k]), x2 = std::log(f[k + 1]);
    const double y0 = db[k - 1], y1 = db[k], y2 = db[k + 1];
    const double den = (x0 - x1) * (x0 - x2) * (x1 - x2);
    if (den == 0.0) return f[k];
    const double a = (x2 * (y1 - y0) + x1 * (y0 - y2) + x0 * (y2 - y1)) / den;
    const double b = (x2 * x2 * (y0 - y1) + x1 * x1 * (y2 - y0) + x0 * x0 * (y1 - y2)) / den;
    if (a == 0.0) return f[k];
    const double x = std::clamp(-b / (2.0 * a), std::min(x0, x2), std::max(x0, x2));
    return std::exp(x);
}
} // namespace

std::vector<double> logFrequencies(double fMin, double fMax, int perDecade) {
    std::vector<double> f;
    if (!(fMin > 0.0) || !(fMax > fMin) || perDecade < 1) return f;
    const double decades = std::log10(fMax / fMin);
    const int n = std::max(2, static_cast<int>(std::lround(decades * perDecade)) + 1);
    f.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) f.push_back(fMin * std::pow(10.0, decades * i / (n - 1)));
    f.back() = fMax;
    return f;
}

FrequencyResponse analyzeResponse(const std::vector<double>& f, const std::vector<Complex>& h) {
    FrequencyResponse r;
    const std::size_t n = std::min(f.size(), h.size());
    if (n < 3) return r;
    r.db.resize(n);
    r.phaseDeg.resize(n);
    double prev = 0.0, offset = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        const double a = std::abs(h[i]);
        r.db[i] = std::isfinite(a) && a > 0.0 ? std::max(kFloorDb, 20.0 * std::log10(a)) : kFloorDb;
        double p = std::isfinite(a) && a > 0.0 ? std::arg(h[i]) * 180.0 / kPi : prev - offset;
        if (i > 0) { // stetig fortsetzen
            while (p + offset - prev > 180.0) offset -= 360.0;
            while (p + offset - prev < -180.0) offset += 360.0;
        }
        r.phaseDeg[i] = p + offset;
        prev = r.phaseDeg[i];
    }
    const auto peakIt = std::max_element(r.db.begin(), r.db.end());
    const std::size_t peak = static_cast<std::size_t>(peakIt - r.db.begin());
    r.maxDb = *peakIt;
    if (r.maxDb <= kFloorDb) return r; // keine Uebertragung
    const double level = r.maxDb - kCutDb;
    const bool lowPass = r.db.front() >= r.maxDb - kEdgeDb;
    const bool highPass = r.db.back() >= r.maxDb - kEdgeDb;

    // erste Kreuzung unter den Pegel oberhalb von from bzw. unterhalb von from
    auto above = [&](std::size_t from) {
        for (std::size_t i = from; i + 1 < n; ++i)
            if (r.db[i] >= level && r.db[i + 1] < level) return crossing(f, r.db, i, level);
        return 0.0;
    };
    auto below = [&](std::size_t from) {
        for (std::size_t i = from; i > 0; --i)
            if (r.db[i] >= level && r.db[i - 1] < level) return crossing(f, r.db, i - 1, level);
        return 0.0;
    };

    if (lowPass && highPass) {
        // flach oder Bandsperre: tiefste Stelle zwischen den Raendern
        const auto minIt = std::min_element(r.db.begin(), r.db.end());
        const std::size_t k = static_cast<std::size_t>(minIt - r.db.begin());
        if (*minIt >= level) {
            r.kind = FrequencyResponse::Kind::Flat;
            return r;
        }
        r.kind = FrequencyResponse::Kind::BandStop;
        r.resonanceHz = refineExtremum(f, r.db, k);
        for (std::size_t i = k; i > 0; --i)
            if (r.db[i - 1] >= level && r.db[i] < level) { r.cutLowHz = crossing(f, r.db, i - 1, level); break; }
        for (std::size_t i = k; i + 1 < n; ++i)
            if (r.db[i] < level && r.db[i + 1] >= level) { r.cutHighHz = crossing(f, r.db, i, level); break; }
    } else if (lowPass) {
        r.kind = FrequencyResponse::Kind::LowPass;
        r.cutHighHz = above(peak);
        if (r.maxDb >= r.db.front() + kPeakingDb) r.resonanceHz = refineExtremum(f, r.db, peak);
    } else if (highPass) {
        r.kind = FrequencyResponse::Kind::HighPass;
        r.cutLowHz = below(peak);
        if (r.maxDb >= r.db.back() + kPeakingDb) r.resonanceHz = refineExtremum(f, r.db, peak);
    } else {
        r.kind = FrequencyResponse::Kind::BandPass;
        r.resonanceHz = refineExtremum(f, r.db, peak);
        r.cutLowHz = below(peak);
        r.cutHighHz = above(peak);
    }
    if (r.cutLowHz > 0.0 && r.cutHighHz > 0.0 && r.resonanceHz > 0.0) {
        r.bandwidthHz = r.cutHighHz - r.cutLowHz;
        if (r.bandwidthHz > 0.0) r.quality = r.resonanceHz / r.bandwidthHz;
    }
    return r;
}

Status sweepResponse(Simulator& sim, std::size_t excitation, NodeId outPlus, NodeId outMinus, double fMin, double fMax,
                     int perDecade, std::vector<double>& freqsHz, std::vector<Complex>& h, FrequencyResponse& result) {
    freqsHz = logFrequencies(fMin, fMax, perDecade);
    Status st = sim.acSweep(excitation, freqsHz, outPlus, outMinus, h);
    if (st != Status::Ok) {
        result = FrequencyResponse{};
        return st;
    }
    result = analyzeResponse(freqsHz, h);
    const double step = std::pow(10.0, 1.0 / std::max(1, perDecade)); // Abstand zweier grober Punkte
    std::vector<double> fine;
    for (const double m : {result.resonanceHz, result.cutLowHz, result.cutHighHz}) {
        if (!(m > 0.0)) continue;
        const double lo = std::max(fMin, m / (2.0 * step)), hi = std::min(fMax, m * 2.0 * step);
        for (int i = 0; i <= 200; ++i) fine.push_back(lo * std::pow(hi / lo, i / 200.0));
    }
    if (fine.empty()) return Status::Ok;
    std::vector<Complex> hf;
    st = sim.acSweep(excitation, fine, outPlus, outMinus, hf);
    if (st != Status::Ok) return Status::Ok; // grobe Kurve bleibt gueltig
    std::vector<std::pair<double, Complex>> all;
    all.reserve(freqsHz.size() + fine.size());
    for (std::size_t i = 0; i < freqsHz.size(); ++i) all.emplace_back(freqsHz[i], h[i]);
    for (std::size_t i = 0; i < fine.size(); ++i) all.emplace_back(fine[i], hf[i]);
    std::sort(all.begin(), all.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    freqsHz.clear();
    h.clear();
    for (const auto& [ff, hh] : all) {
        if (!freqsHz.empty() && ff <= freqsHz.back() * (1.0 + 1e-12)) continue; // doppelte Punkte
        freqsHz.push_back(ff);
        h.push_back(hh);
    }
    result = analyzeResponse(freqsHz, h);
    return Status::Ok;
}

} // namespace volta::core
