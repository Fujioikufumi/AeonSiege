#pragma once
#include <vector>
#include <wincodec.h>

/// <summary>
/// WIC で画像を読み込み、指定フォーマットの生ピクセルデータを取得する。(8bit)
/// 高さマップ（8bit Gray）やフィールドマップ（32bit RGBA）などで共通利用する。
/// </summary>
/// <param name="fullPath">解決済みの画像ファイルパス（SearchFilePathW 済みを渡す）</param>
/// <param name="targetFormat">WIC ピクセルフォーマット（例: GUID_WICPixelFormat8bppGray, GUID_WICPixelFormat32bppRGBA）</param>
/// <param name="outWidth">出力: 画像幅</param>
/// <param name="outHeight">出力: 画像高さ</param>
/// <param name="outPixels">出力: 生ピクセルデータ。サイズは width*height*bytesPerPixel（Gray=1, RGBA=4）</param>
/// <returns>成功時 true、失敗時 false </returns>
bool LoadImageWithWIC(
	const wchar_t* fullPath,
	REFGUID targetFormat,
	UINT& outWidth,
	UINT& outHeight,
	std::vector<uint8_t>& outPixels
);

/// <summary>
/// WIC で画像を読み込み、16bit Gray フォーマットの生ピクセルデータを取得する。
/// </summary>
bool LoadImageWithWIC16(
	const wchar_t* fullPath,
	UINT& outWidth,
	UINT& outHeight,
	std::vector<uint16_t>& outPixels);