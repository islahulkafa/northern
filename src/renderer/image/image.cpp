#include "renderer/image/image.hpp"

#include <atomic>
#include <filesystem>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <objbase.h>
#include <wincodec.h>
#include <windows.h>
#endif

namespace northern::renderer {

namespace {

constexpr uint32_t kMaxImageDimension = 16384;
std::atomic<uint32_t> sNextImageId{1};

#ifdef _WIN32

template <typename T>
class ComPtr {
public:
    ComPtr() noexcept = default;
    ~ComPtr() { reset(); }

    ComPtr(const ComPtr&) = delete;
    ComPtr& operator=(const ComPtr&) = delete;

    ComPtr(ComPtr&& other) noexcept : ptr_(other.ptr_) { other.ptr_ = nullptr; }
    ComPtr& operator=(ComPtr&& other) noexcept {
        if (this != &other) {
            reset();
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    T** put() noexcept {
        reset();
        return &ptr_;
    }

    T* get() const noexcept { return ptr_; }
    T* operator->() const noexcept { return ptr_; }
    explicit operator bool() const noexcept { return ptr_ != nullptr; }

    void reset() noexcept {
        if (ptr_) {
            ptr_->Release();
            ptr_ = nullptr;
        }
    }

private:
    T* ptr_{nullptr};
};

class ComScope {
public:
    ComScope() noexcept {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        shouldUninit_ = (hr == S_OK || hr == S_FALSE);
    }

    ~ComScope() {
        if (shouldUninit_) {
            CoUninitialize();
        }
    }

    ComScope(const ComScope&) = delete;
    ComScope& operator=(const ComScope&) = delete;

private:
    bool shouldUninit_{false};
};

core::Result<std::vector<uint8_t>, core::Error>
extractRgbaFromWicDecoder(IWICImagingFactory* factory, IWICBitmapDecoder* decoder, uint32_t& outW,
                          uint32_t& outH) {
    ComPtr<IWICBitmapFrameDecode> frame;
    HRESULT hr = decoder->GetFrame(0, frame.put());
    if (FAILED(hr) || !frame) {
        return core::Error(core::ErrorCode::InvalidArgument, "Failed to decode image frame");
    }

    UINT w = 0;
    UINT h = 0;
    hr = frame->GetSize(&w, &h);
    if (FAILED(hr) || w == 0 || h == 0 || w > kMaxImageDimension || h > kMaxImageDimension) {
        return core::Error(core::ErrorCode::InvalidArgument, "Invalid or unsupported image size");
    }

    ComPtr<IWICFormatConverter> converter;
    hr = factory->CreateFormatConverter(converter.put());
    if (FAILED(hr) || !converter) {
        return core::Error(core::ErrorCode::InternalError, "Failed to create WIC format converter");
    }

    hr = converter->Initialize(frame.get(), GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone,
                               nullptr, 0.0, WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) {
        return core::Error(core::ErrorCode::InvalidArgument,
                           "Failed to convert image to 32bpp RGBA");
    }

    UINT stride = w * 4u;
    size_t totalBytes = static_cast<size_t>(stride) * static_cast<size_t>(h);
    std::vector<uint8_t> pixels(totalBytes);

    hr = converter->CopyPixels(nullptr, stride, static_cast<UINT>(totalBytes), pixels.data());
    if (FAILED(hr)) {
        return core::Error(core::ErrorCode::InternalError, "Failed to copy decoded image pixels");
    }

    outW = static_cast<uint32_t>(w);
    outH = static_cast<uint32_t>(h);
    return pixels;
}

std::wstring utf8ToWide(std::string_view utf8) {
    if (utf8.empty()) {
        return {};
    }
    int len =
        MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    if (len <= 0) {
        return {};
    }
    std::wstring wide(static_cast<size_t>(len), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), wide.data(), len);
    return wide;
}

#endif

} // namespace

struct Image::Impl {
    uint32_t id{0};
    uint32_t width{0};
    uint32_t height{0};
    std::vector<uint8_t> rgbaPixels;
};

Image::Image(std::shared_ptr<const Impl> impl) noexcept : impl_(std::move(impl)) {}

Image::~Image() = default;
Image::Image(Image&&) noexcept = default;
Image& Image::operator=(Image&&) noexcept = default;

core::Result<std::shared_ptr<Image>, core::Error>
Image::fromPixels(uint32_t width, uint32_t height, std::span<const uint8_t> rgbaPixels) {
    if (width == 0 || height == 0 || width > kMaxImageDimension || height > kMaxImageDimension) {
        return core::Error(core::ErrorCode::InvalidArgument, "Invalid image dimensions");
    }
    size_t expectedBytes = static_cast<size_t>(width) * static_cast<size_t>(height) * 4u;
    if (rgbaPixels.size() < expectedBytes) {
        return core::Error(core::ErrorCode::InvalidArgument,
                           "RGBA pixel buffer is smaller than width * height * 4");
    }

    auto impl = std::make_shared<Impl>();
    impl->id = sNextImageId.fetch_add(1, std::memory_order_relaxed);
    impl->width = width;
    impl->height = height;
    impl->rgbaPixels.assign(rgbaPixels.begin(),
                            rgbaPixels.begin() + static_cast<ptrdiff_t>(expectedBytes));
    return std::shared_ptr<Image>(new Image(std::move(impl)));
}

core::Result<std::shared_ptr<Image>, core::Error> Image::fromFile(std::string_view filePath) {
    if (filePath.empty()) {
        return core::Error(core::ErrorCode::InvalidArgument, "Image file path cannot be empty");
    }
    std::filesystem::path fsPath(filePath);
    std::error_code ec;
    if (!std::filesystem::exists(fsPath, ec)) {
        return core::Error(core::ErrorCode::NotFound,
                           "Image file not found: " + std::string(filePath));
    }

#ifdef _WIN32
    ComScope comScope;
    ComPtr<IWICImagingFactory> factory;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(factory.put()));
    if (FAILED(hr) || !factory) {
        return core::Error(core::ErrorCode::InternalError, "Failed to create WIC imaging factory");
    }

    std::wstring widePath = utf8ToWide(filePath);
    if (widePath.empty()) {
        return core::Error(core::ErrorCode::InvalidArgument, "Invalid UTF-8 image file path");
    }

    ComPtr<IWICBitmapDecoder> decoder;
    hr = factory->CreateDecoderFromFilename(widePath.c_str(), nullptr, GENERIC_READ,
                                            WICDecodeMetadataCacheOnLoad, decoder.put());
    if (FAILED(hr) || !decoder) {
        return core::Error(core::ErrorCode::InvalidArgument,
                           "Failed to open or decode image file: " + std::string(filePath));
    }

    uint32_t w = 0;
    uint32_t h = 0;
    auto pixRes = extractRgbaFromWicDecoder(factory.get(), decoder.get(), w, h);
    if (pixRes.isErr()) {
        return pixRes.error();
    }

    auto impl = std::make_shared<Impl>();
    impl->id = sNextImageId.fetch_add(1, std::memory_order_relaxed);
    impl->width = w;
    impl->height = h;
    impl->rgbaPixels = std::move(pixRes.value());
    return std::shared_ptr<Image>(new Image(std::move(impl)));
#else
    return core::Error(core::ErrorCode::NotSupported,
                       "Image::fromFile is not implemented on this platform");
#endif
}

core::Result<std::shared_ptr<Image>, core::Error>
Image::fromMemory(std::span<const uint8_t> encodedBytes) {
    if (encodedBytes.empty()) {
        return core::Error(core::ErrorCode::InvalidArgument, "Encoded image buffer is empty");
    }

#ifdef _WIN32
    ComScope comScope;
    ComPtr<IWICImagingFactory> factory;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(factory.put()));
    if (FAILED(hr) || !factory) {
        return core::Error(core::ErrorCode::InternalError, "Failed to create WIC imaging factory");
    }

    ComPtr<IWICStream> stream;
    hr = factory->CreateStream(stream.put());
    if (FAILED(hr) || !stream) {
        return core::Error(core::ErrorCode::InternalError, "Failed to create WIC stream");
    }

    hr = stream->InitializeFromMemory(const_cast<BYTE*>(encodedBytes.data()),
                                      static_cast<DWORD>(encodedBytes.size()));
    if (FAILED(hr)) {
        return core::Error(core::ErrorCode::InvalidArgument,
                           "Failed to initialize WIC stream from memory");
    }

    ComPtr<IWICBitmapDecoder> decoder;
    hr = factory->CreateDecoderFromStream(stream.get(), nullptr, WICDecodeMetadataCacheOnLoad,
                                          decoder.put());
    if (FAILED(hr) || !decoder) {
        return core::Error(core::ErrorCode::InvalidArgument,
                           "Failed to decode image from memory buffer");
    }

    uint32_t w = 0;
    uint32_t h = 0;
    auto pixRes = extractRgbaFromWicDecoder(factory.get(), decoder.get(), w, h);
    if (pixRes.isErr()) {
        return pixRes.error();
    }

    auto impl = std::make_shared<Impl>();
    impl->id = sNextImageId.fetch_add(1, std::memory_order_relaxed);
    impl->width = w;
    impl->height = h;
    impl->rgbaPixels = std::move(pixRes.value());
    return std::shared_ptr<Image>(new Image(std::move(impl)));
#else
    return core::Error(core::ErrorCode::NotSupported,
                       "Image::fromMemory is not implemented on this platform");
#endif
}

uint32_t Image::id() const noexcept {
    return impl_ ? impl_->id : 0;
}

uint32_t Image::width() const noexcept {
    return impl_ ? impl_->width : 0;
}

uint32_t Image::height() const noexcept {
    return impl_ ? impl_->height : 0;
}

core::Size Image::size() const noexcept {
    return impl_ ? core::Size{static_cast<float>(impl_->width), static_cast<float>(impl_->height)}
                 : core::Size{0.0f, 0.0f};
}

float Image::aspectRatio() const noexcept {
    return (impl_ && impl_->height > 0)
               ? static_cast<float>(impl_->width) / static_cast<float>(impl_->height)
               : 0.0f;
}

std::span<const uint8_t> Image::pixels() const noexcept {
    return impl_ ? std::span<const uint8_t>(impl_->rgbaPixels) : std::span<const uint8_t>{};
}

size_t Image::byteSize() const noexcept {
    return impl_ ? impl_->rgbaPixels.size() : 0;
}

bool Image::isValid() const noexcept {
    return impl_ != nullptr && impl_->width > 0 && impl_->height > 0 && !impl_->rgbaPixels.empty();
}

const std::shared_ptr<const Image::Impl>& Image::sharedImpl() const noexcept {
    return impl_;
}

uint32_t Image::implId(const Impl& impl) noexcept {
    return impl.id;
}

uint32_t Image::implWidth(const Impl& impl) noexcept {
    return impl.width;
}

uint32_t Image::implHeight(const Impl& impl) noexcept {
    return impl.height;
}

std::span<const uint8_t> Image::implPixels(const Impl& impl) noexcept {
    return impl.rgbaPixels;
}

} // namespace northern::renderer
