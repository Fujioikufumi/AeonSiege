#include "EncodingUtils.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vector>

//--------------------------------------------------------------
// 	    ワイド文字列をUTF-8にエンコードする
//--------------------------------------------------------------
std::string EncodeWideToUtf8(const std::wstring& wide)
{
    if (wide.empty()) return std::string();

    const int bytes = ::WideCharToMultiByte(
        CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);

    if (bytes <= 0) return std::string();

    std::vector<char> buffer((size_t)bytes);
    ::WideCharToMultiByte(
        CP_UTF8, 0, wide.c_str(), -1, buffer.data(), bytes, nullptr, nullptr);

    return std::string(buffer.data());
}

//--------------------------------------------------------------
// 	    マルチバイト文字列をワイド文字列にデコードする
//--------------------------------------------------------------
static std::wstring DecodeMultiByteToWide(UINT codePage, DWORD flags, const char* text)
{
    const int wchars = ::MultiByteToWideChar(codePage, flags, text, -1, nullptr, 0);
    if (wchars <= 0) return std::wstring();

    std::vector<wchar_t> buffer((size_t)wchars);
    ::MultiByteToWideChar(codePage, flags, text, -1, buffer.data(), wchars);
    return std::wstring(buffer.data());
}

//--------------------------------------------------------------
// 	    マルチバイト文字列をワイド文字列にデコードする（UTF-8優先、失敗したらACP）
//--------------------------------------------------------------
std::wstring DecodeUtf8OrAcpToWide(const char* text)
{
    if (!text || text[0] == '\0') return std::wstring();

    // UTF-8として不正なバイト列なら失敗させたいので MB_ERR_INVALID_CHARS を使う
    // （失敗したら ACP にフォールバックする）
    std::wstring w = DecodeMultiByteToWide(CP_UTF8, MB_ERR_INVALID_CHARS, text);
    if (!w.empty()) return w;

    return DecodeMultiByteToWide(CP_ACP, 0, text);
}

std::wstring EncodeUtf8ToWide(const std::string& utf8)
{
	return DecodeMultiByteToWide(CP_UTF8, 0, utf8.c_str());
}