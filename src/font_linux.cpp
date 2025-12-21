#include "font_linux.hpp"
#include <stdexcept>

#ifdef __linux__
#include <fontconfig/fontconfig.h>
#include <map>

namespace fontlist {

fontstyle trans_slant(int slant) {
    switch (slant) {
    case FC_SLANT_ROMAN:
        return fontstyle::normal;
    case FC_SLANT_ITALIC:
        return fontstyle::italic;
    case FC_SLANT_OBLIQUE:
        return fontstyle::oblique;
    default:
        return fontstyle::normal;
    }
}

int trans_weight(int fontconfig_weight) {
    struct WeightMapping {
        int fcWeight;
        int cssWeight;
    };

    static const std::vector<WeightMapping> weightTable = {
        {FC_WEIGHT_THIN, 100},       // Thin
        {FC_WEIGHT_EXTRALIGHT, 200}, // Extra Light
        {FC_WEIGHT_LIGHT, 300},      // Light
        {FC_WEIGHT_REGULAR, 400},    // Regular
        {FC_WEIGHT_MEDIUM, 500},     // Medium
        {FC_WEIGHT_SEMIBOLD, 600},   // Semi Bold
        {FC_WEIGHT_BOLD, 700},       // Bold
        {FC_WEIGHT_EXTRABOLD, 800},  // Extra Bold
        {FC_WEIGHT_BLACK, 900}       // Black
    };

    int closestWeight = weightTable[0].cssWeight;
    int minDifference = std::abs(fontconfig_weight - weightTable[0].fcWeight);

    for (const auto &entry : weightTable) {
        int diff = std::abs(fontconfig_weight - entry.fcWeight);
        if (diff < minDifference) {
            minDifference = diff;
            closestWeight = entry.cssWeight;
        }
    }

    return closestWeight;
}

std::vector<fontfamily> enumerate_font_linux_fontconfig() {
    std::vector<fontfamily> font_array;

    auto config = FcInitLoadConfigAndFonts();
    auto pat = FcPatternCreate();
    auto os = FcObjectSetBuild(FC_FAMILY, FC_FILE, FC_SIZE, FC_SLANT, FC_WEIGHT, (char *)0);
    FcFontSet *fs = FcFontList(config, pat, os);

    std::map<std::string, fontfamily> fontfamily_map;

    for (int i = 0; fs && i < fs->nfont; ++i) {
        FcPattern *font = fs->fonts[i];
        FcChar8 *file, *family;
        int slant, weight;
        double size;
        if (FcPatternGetString(font, FC_FAMILY, 0, &family) == FcResultMatch &&
            FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch &&
            FcPatternGetDouble(font, FC_SIZE, 0, &size) == FcResultMatch &&
            FcPatternGetInteger(font, FC_SLANT, 0, &slant) == FcResultMatch &&
            FcPatternGetInteger(font, FC_WEIGHT, 0, &weight) == FcResultMatch) {

            auto family_str = std::string(reinterpret_cast<const char *>(family));
            auto file_str = std::filesystem::path(reinterpret_cast<const char *>(file));
            fontfamily_map[family_str].name = family_str;
            fontfamily_map[family_str].fonts.push_back(fontlist::font{
                .style = trans_slant(slant),
                .weight = trans_weight(weight),
                .file = file_str,
            });
        }
    }
    if (fs)
        FcFontSetDestroy(fs);

    for (auto &[name, family] : fontfamily_map) {
        font_array.push_back(std::move(family));
    }

    return font_array;
}

fontfamily get_default_font_linux_fontconfig() {
    auto config = FcInitLoadConfigAndFonts();
    auto pat = FcPatternCreate();
    FcConfigSubstitute(config, pat, FcMatchPattern);
    FcDefaultSubstitute(pat);
    FcResult result;
    auto match = FcFontMatch(config, pat, &result);
    FcPatternDestroy(pat);
    if (result == FcResultNoMatch) {
        throw std::runtime_error("fontlist: default font not found");
    }
    FcChar8 *file, *family;
    int slant, weight;
    double size;
    fontfamily ff;
    if (FcPatternGetString(match, FC_FAMILY, 0, &family) == FcResultMatch &&
        FcPatternGetString(match, FC_FILE, 0, &file) == FcResultMatch &&
        FcPatternGetDouble(match, FC_SIZE, 0, &size) == FcResultMatch &&
        FcPatternGetInteger(match, FC_SLANT, 0, &slant) == FcResultMatch &&
        FcPatternGetInteger(match, FC_WEIGHT, 0, &weight) == FcResultMatch) {

        auto family_str = std::string(reinterpret_cast<const char *>(family));
        auto file_str = std::filesystem::path(reinterpret_cast<const char *>(file));
        ff.name = family_str;
        ff.fonts.push_back(font{
            .style = trans_slant(slant),
            .weight = trans_weight(weight),
            .size = size,
            .file = file_str,
        });
    }
    else {
        FcPatternDestroy(match);
        throw std::runtime_error("fontlist: default font not found");
    }
    FcPatternDestroy(match);
    return ff;
}

} // namespace fontlist
#endif
