#include <iostream>

#include "fontlist.hpp"

int main() {
    try {
        std::cout << "System fonts enumerating..." << std::endl;

        auto fontfamilies = fontlist::enumerate_font();

        for (const auto &fontfamily : fontfamilies) {
            std::cout << fontfamily.name << std::endl;
            for (const auto &font : fontfamily.fonts) {
                std::cout << "  " << font.file << " weight:" << font.weight << " style:" << fontlist::to_string(font.style) << std::endl;
            }
        }
        auto ff = fontlist::get_default_font();
        std::cout << ff.name << std::endl;
        for (const auto &font : ff.fonts) {
            std::cout << "  " << font.file << " weight:" << font.weight << " style:" << fontlist::to_string(font.style) << std::endl;
        }
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}
