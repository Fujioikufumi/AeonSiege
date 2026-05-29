#include "ImageLoader.h"
#include "Logger.h"
#include <wincodec.h>
#include <ComPtr.h>


bool LoadImageWithWIC(
	const wchar_t* fullPath,
	REFGUID targetFormat,
	UINT& outWidth,
	UINT& outHeight,
	std::vector<uint8_t>& outPixels)
{
	outPixels.clear();
	outWidth = 0;
	outHeight = 0;

	// COM の初期化（マルチスレッド）。既に初期化済みなら S_FALSE が返る
	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	(void)hr;

	ComPtr<IWICImagingFactory> pWICFactory;
	hr = CoCreateInstance(
		CLSID_WICImagingFactory,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&pWICFactory));
	if (FAILED(hr))
	{
		ELOG("Error : CoCreateInstance IWICImagingFactory failed.");
		return false;
	}

	ComPtr<IWICBitmapDecoder> pDecoder;
	hr = pWICFactory->CreateDecoderFromFilename(
		fullPath,
		nullptr,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		&pDecoder);
	if (FAILED(hr))
	{
		ELOG("Error : CreateDecoderFromFilename failed. path = %ls", fullPath);
		return false;
	}

	ComPtr<IWICBitmapFrameDecode> pFrame;
	hr = pDecoder->GetFrame(0, &pFrame);
	if (FAILED(hr))
	{
		ELOG("Error : GetFrame failed.");
		return false;
	}

	UINT width, height;
	hr = pFrame->GetSize(&width, &height);
	if (FAILED(hr))
	{
		ELOG("Error : GetSize failed.");
		return false;
	}

	ComPtr<IWICFormatConverter> pConverter;
	hr = pWICFactory->CreateFormatConverter(&pConverter);
	if (FAILED(hr))
	{
		ELOG("Error : CreateFormatConverter failed.");
		return false;
	}

	hr = pConverter->Initialize(
		pFrame.Get(),
		targetFormat,
		WICBitmapDitherTypeNone,
		nullptr,
		0.0,
		WICBitmapPaletteTypeCustom);
	if (FAILED(hr))
	{
		ELOG("Error : FormatConverter Initialize failed.");
		return false;
	}

	// フォーマットに応じたストライドとバッファサイズ（Gray=1, RGBA=4）
	UINT bytesPerPixel = 1u;
	if (targetFormat == GUID_WICPixelFormat32bppRGBA)
	{
		bytesPerPixel = 4u;
	}
	else if (targetFormat == GUID_WICPixelFormat16bppGray)
	{
		bytesPerPixel = 2u;
	}
	UINT stride = width * bytesPerPixel;
	size_t pixelCount = static_cast<size_t>(width) * height * bytesPerPixel;

	std::vector<uint8_t> pixels(pixelCount);
	hr = pConverter->CopyPixels(nullptr, stride, static_cast<UINT>(pixels.size()), pixels.data());
	if (FAILED(hr))
	{
		ELOG("Error : CopyPixels failed.");
		return false;
	}

	outWidth = width;
	outHeight = height;
	outPixels = std::move(pixels);
	return true;
}

bool LoadImageWithWIC16(
	const wchar_t* fullPath,
	UINT& outWidth,
	UINT& outHeight,
	std::vector<uint16_t>& outPixels)
{
	outPixels.clear();
	outWidth = 0;
	outHeight = 0;

	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	(void)hr;

	ComPtr<IWICImagingFactory> pWICFactory;
	hr = CoCreateInstance(
		CLSID_WICImagingFactory,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&pWICFactory));
	if (FAILED(hr))
	{
		ELOG("Error : CoCreateInstance IWICImagingFactory failed.");
		return false;
	}

	ComPtr<IWICBitmapDecoder> pDecoder;
	hr = pWICFactory->CreateDecoderFromFilename(
		fullPath,
		nullptr,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		&pDecoder);
	if (FAILED(hr))
	{
		ELOG("Error : CreateDecoderFromFilename failed. path = %ls", fullPath);
		return false;
	}

	ComPtr<IWICBitmapFrameDecode> pFrame;
	hr = pDecoder->GetFrame(0, &pFrame);
	if (FAILED(hr))
	{
		ELOG("Error : GetFrame failed.");
		return false;
	}

	UINT width = 0, height = 0;
	hr = pFrame->GetSize(&width, &height);
	if (FAILED(hr))
	{
		ELOG("Error : GetSize failed.");
		return false;
	}

	ComPtr<IWICFormatConverter> pConverter;
	hr = pWICFactory->CreateFormatConverter(&pConverter);
	if (FAILED(hr))
	{
		ELOG("Error : CreateFormatConverter failed.");
		return false;
	}

	hr = pConverter->Initialize(
		pFrame.Get(),
		GUID_WICPixelFormat16bppGray, // 16bit Grayに変換
		WICBitmapDitherTypeNone,
		nullptr,
		0.0,
		WICBitmapPaletteTypeCustom);
	if (FAILED(hr))
	{
		ELOG("Error : FormatConverter Initialize failed.");
		return false;
	}

	// 16bpp Gray: 2 bytes / pixel
	const UINT bytesPerPixel = 2u;
	const UINT stride = width * bytesPerPixel;
	const size_t byteCount = static_cast<size_t>(width) * height * bytesPerPixel;

	std::vector<uint8_t> bytes(byteCount);
	hr = pConverter->CopyPixels(nullptr, stride, static_cast<UINT>(bytes.size()), bytes.data());
	if (FAILED(hr))
	{
		ELOG("Error : CopyPixels failed.");
		return false;
	}

	outWidth = width;
	outHeight = height;
	outPixels.resize(static_cast<size_t>(width) * height);

	// little-endianでuint16に復元
	for (size_t i = 0; i < outPixels.size(); ++i)
	{
		const uint16_t lo = bytes[i * 2 + 0];
		const uint16_t hi = bytes[i * 2 + 1];
		outPixels[i] = static_cast<uint16_t>(lo | (hi << 8));
	}

	return true;
}