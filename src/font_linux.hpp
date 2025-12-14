#include <fontlist.hpp>
#include <vector>

namespace fontlist {

std::vector<fontlist::fontfamily> enumerate_font_linux_fontconfig();
fontfamily get_default_font_linux_fontconfig();

}
