#pragma once

#include <string>
#include <vector>

namespace volta::model {

// QR-Code als Raster (third_party/qrcodegen, Fehlerkorrektur M): size x size Module, dark[y * size + x].
// false, wenn der Text nicht hineinpasst.
struct QrMatrix {
    int size = 0;
    std::vector<bool> dark;
};
bool makeQrCode(const std::string& text, QrMatrix& out);

} // namespace volta::model
