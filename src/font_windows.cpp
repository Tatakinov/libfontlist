#include "font_windows.hpp"
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#include <combaseapi.h>
#include <dwrite_3.h>
#include <wrl/client.h>

#include <chrono>
#include <cstring>
#include <iostream>

#define CHK_HR(x, msg) do {  \
    if (auto result = x; result != S_OK) {\
        std::cerr << std::hex << result << std::endl; \
        throw std::runtime_error(msg); \
    } \
} while(false)

namespace fontlist {

template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

auto to_string(std::wstring wstr) {
    int len = WideCharToMultiByte(
        CP_ACP, WC_COMPOSITECHECK, wstr.c_str(), wstr.length(),
        nullptr, 0, nullptr, nullptr);

    std::string str;
    str.resize(len);
    int name_multibyte_len = WideCharToMultiByte(
        CP_ACP, WC_COMPOSITECHECK, wstr.c_str(), wstr.length(),
        str.data(), str.size(), nullptr, nullptr);

    return str;
}

static std::string get_font_name(IDWriteFontFamily *family, std::wstring locale) {
    ComPtr<IDWriteLocalizedStrings> names;
    CHK_HR(
        family->GetFamilyNames(names.GetAddressOf()),
        "failed to run IDWriteFontFamily::GetFamilyNames");

    UINT32 nameIndex;
    BOOL exists;
    CHK_HR(
        names->FindLocaleName(locale.c_str(), &nameIndex, &exists),
        "failed to run IDWriteLocalizedStrings::FindLocaleName");
    if (!exists)
        nameIndex = 0;

    UINT32 name_length;
    std::wstring name;
    CHK_HR(
        names->GetStringLength(nameIndex, &name_length),
        "failed to run IDWriteLocalizedStrings::GetStringLength");
    name.resize(name_length + 1);

    CHK_HR(
        names->GetString(nameIndex, name.data(), std::size(name)),
        "failed to run IDWriteLocalizedStrings::GetString");
    name.pop_back();

    return to_string(name);
}

static std::filesystem::path get_font_path(IDWriteFontFile *pFontFile) {
    const void *fontFileReferenceKey = nullptr;
    UINT32 fontFileReferenceKeySize = 0;
    CHK_HR(pFontFile->GetReferenceKey(&fontFileReferenceKey, &fontFileReferenceKeySize),
           "fontlist: failed to run IDWriteFontFile::GetReferenceKey()");

    ComPtr<IDWriteFontFileLoader> pLoader;
    CHK_HR(pFontFile->GetLoader(&pLoader),
           "fontlist: failed to run IDWriteFontFile::GetLoader()");

    ComPtr<IDWriteLocalFontFileLoader> pLocalLoader;
    CHK_HR(pLoader.As(&pLocalLoader),
           "fontlist: failed to cast IDWriteFontFileLoader to IDWriteLocalFontFileLoader");

    UINT32 filePathLength = 0;
    CHK_HR(pLocalLoader->GetFilePathLengthFromKey(fontFileReferenceKey, fontFileReferenceKeySize, &filePathLength),
           "fontlist: failed to run IDWriteLocalFontFileLoader::GetFilePathLengthFromKey()");

    std::wstring filePath(filePathLength + 1, L'\0');
    CHK_HR(pLocalLoader->GetFilePathFromKey(fontFileReferenceKey, fontFileReferenceKeySize, filePath.data(), filePathLength + 1),
           "fontlist: failed to run IDWriteLocalFontFileLoader::GetFilePathFromKey()");
    filePath.pop_back();

    return filePath;
}

static fontstyle trans_style(DWRITE_FONT_STYLE win_style) {
    switch (win_style) {
    case DWRITE_FONT_STYLE_NORMAL:
        return fontstyle::normal;
    case DWRITE_FONT_STYLE_ITALIC:
        return fontstyle::italic;
    case DWRITE_FONT_STYLE_OBLIQUE:
        return fontstyle::oblique;
    default:
        return fontstyle::normal;
    }
}

std::vector<fontfamily> enumerate_font_win32_dwrite() {
    ComPtr<IDWriteFactory2> factory;
    ComPtr<IDWriteFontCollection> font_collection;

    CHK_HR(
        DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory2),
            reinterpret_cast<IUnknown **>(factory.GetAddressOf())),
        "fontlist: failed to run DWriteCreateFactory()");

    CHK_HR(
        factory->GetSystemFontCollection(font_collection.GetAddressOf()),
        "fontlist: failed to run IDWriteFactory::GetSystemFontCollection()");

    std::wstring locale;
    {
        wchar_t locale_name[LOCALE_NAME_MAX_LENGTH];

        if (GetUserDefaultLocaleName(locale_name, std::size(locale_name)) == 0)
            throw std::runtime_error("fontlist: failed to run GetUserDefaultLocaleName()");

        locale = locale_name;
    }

    const auto count = font_collection->GetFontFamilyCount();
    std::vector<fontfamily> font_array(count);

    for (size_t i = 0; i < count; i++) {
        auto &fontfamily = font_array[i];

        ComPtr<IDWriteFontFamily> family;
        CHK_HR(
            font_collection->GetFontFamily(i, family.GetAddressOf()),
            "failed to run IDWriteFactory::GetFontFamily()");

        fontfamily.name = get_font_name(family.Get(), locale);

        auto font_count = family->GetFontCount();
        fontfamily.fonts.resize(font_count);

        for (size_t j = 0; j < font_count; j++) {
            ComPtr<IDWriteFont> pFont;
            family->GetFont(j, &pFont);

            DWRITE_FONT_STYLE win_style = pFont->GetStyle();
            DWRITE_FONT_WEIGHT weight = pFont->GetWeight();
            DWRITE_FONT_STRETCH stretch = pFont->GetStretch();
            
            auto &font = fontfamily.fonts[j];
            font.style = trans_style(win_style);
            font.weight = weight;

            ComPtr<IDWriteFontFace> pFontFace;
            CHK_HR(pFont->CreateFontFace(&pFontFace),
                   "fontlist: failed to run IDWriteFont::CreateFontFace()");

            UINT32 fileCount = 0;
            pFontFace->GetFiles(&fileCount, nullptr);

            std::vector<IDWriteFontFile *> fontFiles(fileCount);
            pFontFace->GetFiles(&fileCount, fontFiles.data());

            font.file = get_font_path(fontFiles[0]);
            // for (UINT32 k = 0; k < fileCount; ++k) {
            //     font.file = get_font_path(fontFiles[k]);
            // }
        }
    }

    return font_array;
}

fontfamily get_default_font_win32_dwrite() {
    std::wstring locale;
    {
        wchar_t locale_name[LOCALE_NAME_MAX_LENGTH];

        if (GetUserDefaultLocaleName(locale_name, std::size(locale_name)) == 0)
            throw std::runtime_error("fontlist: failed to run GetUserDefaultLocaleName()");

        locale = locale_name;
    }
    NONCLIENTMETRICS m;
    const auto size = sizeof(NONCLIENTMETRICS);
    memset(&m, 0x00, size);
    m.cbSize = size;
    if (!SystemParametersInfo(SPI_GETNONCLIENTMETRICS, size, &m, 0)) {
        std::cerr << "SystemparametersInfo: " << std::hex << GetLastError() << std::endl;
    }
    std::wcout << m.lfMenuFont.lfFaceName << std::endl;
    ComPtr<IDWriteFactory2> factory;
    CHK_HR(
        DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory2),
            reinterpret_cast<IUnknown **>(factory.GetAddressOf())),
        "fontlist: failed to run DWriteCreateFactory()");
    ComPtr<IDWriteGdiInterop> gdi_interop;
    CHK_HR(
        factory->GetGdiInterop(gdi_interop.GetAddressOf()),
        "fontlist: failed to run IDWriteFactory::GetGdiInterop()");
    ComPtr<IDWriteFont> pFont;
    CHK_HR(gdi_interop->CreateFontFromLOGFONT(&m.lfMenuFont, pFont.GetAddressOf()),
           "fontlist: failed to run IDWriteGdiInterop::CreateFontFromLOGFONT()");

    ComPtr<IDWriteFontFamily> family;
    CHK_HR(
        pFont->GetFontFamily(family.GetAddressOf()),
        "failed to run IDWriteFactory::GetFontFamily()");

    fontfamily fontfamily;
    fontfamily.name = get_font_name(family.Get(), locale);

    DWRITE_FONT_STYLE win_style = pFont->GetStyle();
    DWRITE_FONT_WEIGHT weight = pFont->GetWeight();
    DWRITE_FONT_STRETCH stretch = pFont->GetStretch();

    font font;
    font.style = trans_style(win_style);
    font.weight = weight;

    ComPtr<IDWriteFontFace> pFontFace;
    CHK_HR(pFont->CreateFontFace(&pFontFace),
           "fontlist: failed to run IDWriteFont::CreateFontFace()");

    UINT32 fileCount = 0;
    pFontFace->GetFiles(&fileCount, nullptr);

    std::vector<IDWriteFontFile *> fontFiles(fileCount);
    pFontFace->GetFiles(&fileCount, fontFiles.data());

    font.file = get_font_path(fontFiles[0]);
    fontfamily.fonts.push_back(font);
    return fontfamily;
}

} // namespace fontlist
#endif
