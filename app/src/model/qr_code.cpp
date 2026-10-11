#include "qr_code.hpp"

#include "qrcodegen.hpp"

namespace volta::model {

bool makeQrCode(const std::string& text, QrMatrix& out) {
    try {
        const auto qr = qrcodegen::QrCode::encodeText(text.c_str(), qrcodegen::QrCode::Ecc::MEDIUM);
        out.size = qr.getSize();
        out.dark.assign(static_cast<std::size_t>(out.size * out.size), false);
        for (int y = 0; y < out.size; ++y)
            for (int x = 0; x < out.size; ++x) out.dark[static_cast<std::size_t>(y * out.size + x)] = qr.getModule(x, y);
        return true;
    } catch (const std::exception&) {
        out = {};
        return false;
    }
}

} // namespace volta::model
