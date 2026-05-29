#pragma once
//-----------------------------------------------------------------------------
//	Includes
//-----------------------------------------------------------------------------
#include <string>

std::string EncodeWideToUtf8(const std::wstring& wide);
std::wstring EncodeUtf8ToWide(const std::string& utf8);
std::wstring DecodeUtf8OrAcpToWide(const char* text);