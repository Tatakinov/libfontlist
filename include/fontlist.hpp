#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#ifndef FONTLIST_HPP
#define FONTLIST_HPP

namespace fontlist {

enum class fontstyle {
    normal,
    italic,
    oblique,
};

inline std::string to_string(fontstyle style) {
    switch (style) {
    case fontstyle::normal:
        return "normal";
    case fontstyle::italic:
        return "italic";
    case fontstyle::oblique:
        return "oblique";
    }
    return "unknown style";
}

struct font {
    fontstyle style;
    int weight;
    double size;
    std::filesystem::path file;
};

struct fontfamily {
    std::string name;
    std::vector<font> fonts;
};

std::vector<fontfamily> enumerate_font();
fontfamily get_default_font();

} // namespace fontlist

#endif
