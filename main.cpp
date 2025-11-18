// en üste ekle (Windows.h'DEN ÖNCE!)

#pragma comment(lib, "Ole32.lib")

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <wincodec.h>
#include <wrl/client.h>
#pragma comment(lib, "Windowscodecs.lib")

#include <iostream>
#include <algorithm>  // std::max, std::clamp
#include <cmath>      // std::fabs
#include <Windows.h>
#include <thread>
#include <TlHelp32.h>
#include <vector>
#include <Psapi.h>
#include <string>
#include <tchar.h>
#include <cstdio>
#include <map>
#include <unordered_map>
#include "resource.h"

#define _CRT_SECURE_NO_WARNINGS
#include <filesystem>

//imgui stuff
#include <dwmapi.h>
#include <d3d11.h>
#include <windowsx.h>
#include "./ImGui/imgui.h"
#include "./ImGui/imgui_impl_dx11.h"
#include "./ImGui/imgui_impl_win32.h"
#include "./ImGui/imgui_internal.h" // Animasyonlar için gerekli
#include "vector.h"
#include "render.h"
#include "font.h"
#define ICON_FA_CROSSHAIRS "\xEF\x81\x9B"  // U+F05B
#define ICON_FA_COG        "\xEF\x80\x93"  // U+F013
#define ICON_FA_CUBE       "\xEF\x84\xB2"  // U+F1B2
#define ICON_FA_EYE        "\xEF\x81\xAE"  // U+F06E
#define ICON_FA_IMAGE      "\xEF\x80\xBE"  // U+F03E
#define ICON_FA_SLIDERS_H  "\xEF\x87\x9E"  // U+F1DE
#define ICON_FA_BULLSEYE   "\xEF\x85\x80"  // U+F140

struct GifFrame {
    ID3D11ShaderResourceView* srv;
    int w, h;
    int delay; // ms
};

struct GifFrame;
bool LoadGifFromMemory(
    ID3D11Device* dev,
    ID3D11DeviceContext* ctx,
    const void* data,
    size_t size,
    std::vector<GifFrame>& outFrames
);

// ============================================
// HELPER FUNCTIONS
// ============================================

// Vector3 mesafe hesaplama
inline float calculate_distance(const Vector3& a, const Vector3& b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

#include <cfloat> // FLT_MAX
#include <limits>

// int width, int height
int screenWidth = GetSystemMetrics(SM_CXSCREEN);
int screenHeight = GetSystemMetrics(SM_CYSCREEN);

// global imgui menu
inline bool showHealthESP = false;
bool showNameESP = false;
bool showFlashESP = false;
bool showChinaHat = false;
bool showImGui = false;
bool drawLines = true;
bool showTeam = false;
bool enableTrue = true;
bool showBoxESP = true;   // Kutu ESP açık/kapalı
bool showHealthBarESP = true;
bool showSkeletonESP = false;
bool showCylinderESP = false;
bool showDefuseESP = false;
bool showC4ESP = false;
bool showWeaponESP = false;  // ← YENİ
bool showMoneyESP = false;   // ← YENİ
bool aimbotVisibilityCheck = true;

int gifFps = 15;
int gifFps2 = 15;
int gifFps3 = 15;
int gifFps4 = 15;


std::vector<GifFrame> gGifFrames;
int gGifIndex = 0;
DWORD gLastGifTick = 0;
bool showGifInBox = false;

std::vector<GifFrame> gGifFrames2;
int gGifIndex2 = 0;
DWORD gLastGifTick2 = 0;
bool showGifInBox2 = false;

std::vector<GifFrame> gGifFrames3;
int gGifIndex3 = 0;
DWORD gLastGifTick3 = 0;
bool showGifInBox3 = false;

std::vector<GifFrame> gGifFrames4;
int gGifIndex4 = 0;
DWORD gLastGifTick4 = 0;
bool showGifInBox4 = true;

ID3D11Device* gDevice = nullptr;
ID3D11DeviceContext* gDeviceContext = nullptr;

struct EmbeddedImage {
    int id;
    std::string name;
};

std::vector<EmbeddedImage> gEmbeddedImages;
int gSelectedEmbedded = -1;

ID3D11ShaderResourceView* gSelectedSRV = nullptr;
int gSelW = 0, gSelH = 0;

bool running = true;

float chinaHatOffset = 2.8f; // şapkayı aşağı çeker
float chinaHatRadius = 11.1f;
float chinaHatHeight = 5.6f;

bool   aimbotEnabled = false;
float  aimbotFovPx = 120.0f;   // daire yarıçapı (px)
float  aimbotSmooth = 12.0f;    // büyük = daha yumuşak
float  aimDeadZone = 0.7f;     // px altında hareket etme
bool   drawAimFov = false;

// --- RCS (Recoil Control) ayarları ---
bool  rcsEnabled = false;
int   rcsStartBullet = 1;      // kaçıncı mermiden sonra başlasın
float rcsScaleX = 1.4f;   // yatay telafi çarpanı
float rcsScaleY = 1.4f;   // dikey telafi çarpanı
float rcsDegToMouse = 0.011f; // derece->mouse hareketi katsayısı (m_yaw vb. ile değişebilir)

// Basit 2D vektör
struct FVec2 { float x, y; };

// — kutuda resim —
bool showPhotoInBox = false;
ID3D11ShaderResourceView* gPhotoSRV = nullptr;
int gPhotoW = 0, gPhotoH = 0;

// ===== BONE INDICES (IMXNOOBX VERIFIED) =====
enum class BoneIndex : int {
    Head = 6,
    Neck = 5,
    Spine_1 = 4,
    Spine_2 = 3,
    Pelvis = 0,

    // Sol kol
    LeftShoulder = 8,
    LeftElbow = 9,
    LeftHand = 10,

    // Sağ kol
    RightShoulder = 13,
    RightElbow = 14,
    RightHand = 15,

    // Sol bacak
    LeftHip = 22,
    LeftKnee = 23,
    LeftFoot = 24,

    // Sağ bacak
    RightHip = 25,
    RightKnee = 26,
    RightFoot = 27
};

// ============================================
// SKELETON SYSTEM STRUCTS & ENUMS
// ============================================

// Quaternion (rotation)
struct Quaternion_t {
    float x, y, z, w;
};

// Bone data
struct CBoneData {
    Vector3 Location;
    float Scale;
    Quaternion_t Rotation;
};

// Bone IDs
enum class BoneId : int {
    Head = 6,
    Neck = 5,
    Spine_0 = 1,
    Spine_1 = 2,
    Spine_2 = 3,
    Spine_3 = 4,
    Hip = 0,
    LeftShoulder = 8,
    LeftArm = 9,
    LeftHand = 10,
    RightShoulder = 13,
    RightArm = 14,
    RightHand = 15,
    LeftHip = 22,
    LeftKnee = 23,
    LeftFoot = 24,
    RightHip = 25,
    RightKnee = 26,
    RightFoot = 27,
};

// Bone connections (skeleton lines)
struct BoneConnection {
    BoneId from;
    BoneId to;
};

static const BoneConnection g_BoneConnections[] = {
    // Spine
    {BoneId::Hip, BoneId::Spine_0},
    {BoneId::Spine_0, BoneId::Spine_1},
    {BoneId::Spine_1, BoneId::Spine_2},
    {BoneId::Spine_2, BoneId::Spine_3},
    {BoneId::Spine_3, BoneId::Neck},
    {BoneId::Neck, BoneId::Head},

    // Left arm
    {BoneId::Spine_3, BoneId::LeftShoulder},
    {BoneId::LeftShoulder, BoneId::LeftArm},
    {BoneId::LeftArm, BoneId::LeftHand},

    // Right arm
    {BoneId::Spine_3, BoneId::RightShoulder},
    {BoneId::RightShoulder, BoneId::RightArm},
    {BoneId::RightArm, BoneId::RightHand},

    // Left leg
    {BoneId::Hip, BoneId::LeftHip},
    {BoneId::LeftHip, BoneId::LeftKnee},
    {BoneId::LeftKnee, BoneId::LeftFoot},

    // Right leg
    {BoneId::Hip, BoneId::RightHip},
    {BoneId::RightHip, BoneId::RightKnee},
    {BoneId::RightKnee, BoneId::RightFoot},
};

// Lock davranışı
bool   lockPersistsAfterKey = false;     // tuşu bıraksan da kilit kalsın
int    switchKeyVK = VK_XBUTTON1; // Mouse4 ile kilidi değiştir

// Hotkey (LMB varsayılan)
int aimbotHotkeyIndex = 1;
static const int kAimHotkeys[] = { VK_LMENU, VK_LBUTTON, VK_RBUTTON, VK_XBUTTON1, VK_XBUTTON2, VK_CAPITAL, VK_LSHIFT, VK_LCONTROL };
inline int GetAimHotkeyVK() { return kAimHotkeys[aimbotHotkeyIndex]; }

// Skeleton bone hedefleme (0=Head, 1=Neck, 2=Upper Chest, 3=Lower Chest, 4=Stomach, 5=Pelvis)
int aimBoneSelect = 0;

// Kilit durumu
static uintptr_t gLockedPawn = 0;
static int       gLockedIndex = -1;

// EMA filtre
static float emaDx = 0.f, emaDy = 0.f;

static bool LoadResourceData(WORD id, LPCWSTR typeOrNull, std::vector<unsigned char>& out) {
    HMODULE hMod = GetModuleHandleW(nullptr);

    // 1) Verilen tip (ör: L"RCDATA")
    HRSRC hRes = FindResourceW(hMod, MAKEINTRESOURCEW(id), typeOrNull);
    if (!hRes) {
        // 2) RT_RCDATA (integer) ile de dene
        hRes = FindResourceW(hMod, MAKEINTRESOURCEW(id), MAKEINTRESOURCEW(RT_RCDATA));
        if (!hRes) return false;
    }

    HGLOBAL hData = LoadResource(hMod, hRes);
    if (!hData) return false;

    DWORD size = SizeofResource(hMod, hRes);
    void* pData = LockResource(hData);
    if (!pData || size == 0) return false;

    out.resize(size);
    memcpy(out.data(), pData, size);
    return true;
}

bool LoadGifFromMemory(
    ID3D11Device* dev,
    ID3D11DeviceContext* ctx,
    const void* data,
    size_t size,
    std::vector<GifFrame>& outFrames
)
{
    using namespace Microsoft::WRL;

    outFrames.clear();
    if (!dev || !data || size < 8) return false;

    ComPtr<IWICImagingFactory> fac;
    ComPtr<IWICStream> stream;
    ComPtr<IWICBitmapDecoder> dec;

    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&fac));
    if (FAILED(hr)) return false;

    fac->CreateStream(&stream);
    stream->InitializeFromMemory((WICInProcPointer)data, (DWORD)size);

    fac->CreateDecoderFromStream(
        stream.Get(), nullptr, WICDecodeMetadataCacheOnLoad, &dec);

    UINT frameCount = 0;
    dec->GetFrameCount(&frameCount);
    if (frameCount == 0) return false;

    // FULL GIF CANVAS BOYUTU
    UINT fullW = 0, fullH = 0;
    {
        ComPtr<IWICBitmapFrameDecode> firstFrame;
        dec->GetFrame(0, &firstFrame);
        firstFrame->GetSize(&fullW, &fullH);
    }

    // Composite canvas (tam GIF görüntüsü)
    std::vector<BYTE> composite(fullW * fullH * 4, 0);

    for (UINT i = 0; i < frameCount; i++)
    {
        ComPtr<IWICBitmapFrameDecode> frame;
        dec->GetFrame(i, &frame);

        UINT w = 0, h = 0;
        frame->GetSize(&w, &h);

        ComPtr<IWICFormatConverter> conv;
        fac->CreateFormatConverter(&conv);

        conv->Initialize(
            frame.Get(),
            GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone,
            nullptr,
            0.0,
            WICBitmapPaletteTypeCustom);

        // FRAME OFFSET – GIF metadata
        int offX = 0, offY = 0;
        {
            ComPtr<IWICMetadataQueryReader> qr;
            frame->GetMetadataQueryReader(&qr);
            if (qr)
            {
                PROPVARIANT pv;
                PropVariantInit(&pv);

                if (SUCCEEDED(qr->GetMetadataByName(L"/imgdesc/Left", &pv)))
                    offX = pv.uiVal;
                PropVariantClear(&pv);

                if (SUCCEEDED(qr->GetMetadataByName(L"/imgdesc/Top", &pv)))
                    offY = pv.uiVal;
                PropVariantClear(&pv);
            }
        }

        // LOCAL FRAME PIXELS
        UINT strideLocal = w * 4;
        UINT bufLocal = strideLocal * h;
        std::vector<BYTE> local(bufLocal);

        conv->CopyPixels(nullptr, strideLocal, bufLocal, local.data());

        // DISPOSAL METHOD — FRAME'İN NASIL OVERLAY EDİLECEĞİ
        int disposal = 0;
        {
            ComPtr<IWICMetadataQueryReader> qr;
            frame->GetMetadataQueryReader(&qr);
            if (qr)
            {
                PROPVARIANT pv;
                PropVariantInit(&pv);

                if (SUCCEEDED(qr->GetMetadataByName(L"/grctlext/Disposal", &pv)))
                    disposal = pv.uiVal;

                PropVariantClear(&pv);
            }
        }

        // Eğer disposal=2 ise (background restore) → composite sıfırlanır
        if (disposal == 2)
            memset(composite.data(), 0, composite.size());

        // FRAME → FULL CANVAS ALPHA BLEND
        for (UINT y = 0; y < h; y++)
        {
            BYTE* dst = composite.data() + ((y + offY) * fullW * 4 + (offX * 4));
            BYTE* src = local.data() + (y * w * 4);

            for (UINT x = 0; x < w; x++)
            {
                BYTE a = src[3]; // alpha
                if (a == 255)
                {
                    memcpy(dst, src, 4);
                }
                else if (a > 0)
                {
                    // alpha blend
                    dst[0] = (BYTE)((src[0] * a + dst[0] * (255 - a)) / 255);
                    dst[1] = (BYTE)((src[1] * a + dst[1] * (255 - a)) / 255);
                    dst[2] = (BYTE)((src[2] * a + dst[2] * (255 - a)) / 255);
                    dst[3] = 255;
                }

                dst += 4;
                src += 4;
            }
        }

        // CREATE TEXTURE
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = fullW;
        desc.Height = fullH;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA init{};
        init.pSysMem = composite.data();
        init.SysMemPitch = fullW * 4;

        ComPtr<ID3D11Texture2D> tex;
        dev->CreateTexture2D(&desc, &init, &tex);

        ID3D11ShaderResourceView* srv = nullptr;
        dev->CreateShaderResourceView(tex.Get(), nullptr, &srv);

        // DELAY
        int delay = 80;
        {
            ComPtr<IWICMetadataQueryReader> qr;
            frame->GetMetadataQueryReader(&qr);

            PROPVARIANT pv;
            PropVariantInit(&pv);

            if (qr && SUCCEEDED(qr->GetMetadataByName(L"/grctlext/Delay", &pv)))
            {
                delay = (int)pv.uiVal * 10;
                if (delay < 20) delay = 20;
            }

            PropVariantClear(&pv);
        }

        outFrames.push_back({ srv, (int)fullW, (int)fullH, delay });
    }

    return true;
}

static HRESULT LoadTextureFromMemory(
    ID3D11Device* device,
    ID3D11DeviceContext* /*context*/,
    const void* data, size_t size,
    ID3D11ShaderResourceView** out_srv,
    int* out_w, int* out_h)
{
    if (out_srv) *out_srv = nullptr;
    if (out_w) *out_w = 0;
    if (out_h) *out_h = 0;

    if (!device || !data || size < 8) return E_INVALIDARG;

    // PNG imzasını da doğrula (tam 8 byte)
    const unsigned char* p = static_cast<const unsigned char*>(data);
    static const unsigned char sig[8] = { 0x89,'P','N','G',0x0D,0x0A,0x1A,0x0A };
    if (memcmp(p, sig, 8) != 0) return E_FAIL;

    using Microsoft::WRL::ComPtr;
    ComPtr<IWICImagingFactory>    fac;
    ComPtr<IWICStream>            stream;
    ComPtr<IWICBitmapDecoder>     dec;
    ComPtr<IWICBitmapFrameDecode> frame;
    ComPtr<IWICFormatConverter>   conv;

    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&fac));
    if (FAILED(hr)) return hr;

    hr = fac->CreateStream(&stream);
    if (FAILED(hr)) return hr;

    hr = stream->InitializeFromMemory((WICInProcPointer)data, (DWORD)size);
    if (FAILED(hr)) return hr;

    hr = fac->CreateDecoderFromStream(stream.Get(), nullptr, WICDecodeMetadataCacheOnLoad, &dec);
    if (FAILED(hr)) return hr;

    hr = dec->GetFrame(0, &frame);
    if (FAILED(hr)) return hr;

    hr = fac->CreateFormatConverter(&conv);
    if (FAILED(hr)) return hr;

    hr = conv->Initialize(frame.Get(), GUID_WICPixelFormat32bppPBGRA,
        WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) return hr;

    UINT w = 0, h = 0;
    hr = conv->GetSize(&w, &h);
    if (FAILED(hr) || !w || !h) return FAILED(hr) ? hr : E_FAIL;

    const UINT stride = w * 4;
    const UINT bufSize = stride * h;

    std::vector<BYTE> pixels(bufSize);
    hr = conv->CopyPixels(nullptr, stride, bufSize, pixels.data());
    if (FAILED(hr)) return hr;

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = w; desc.Height = h;
    desc.MipLevels = 1; desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA init{};
    init.pSysMem = pixels.data();
    init.SysMemPitch = stride;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
    hr = device->CreateTexture2D(&desc, &init, &tex);
    if (FAILED(hr)) return hr;

    hr = device->CreateShaderResourceView(tex.Get(), nullptr, out_srv);
    if (FAILED(hr)) return hr;

    if (out_w) *out_w = (int)w;
    if (out_h) *out_h = (int)h;
    return S_OK;
}

// === Embedded Image Loader (RCDATA içinden PNG/JPG/GIF yükler) ===
bool LoadEmbeddedImage(ID3D11Device* dev, ID3D11DeviceContext* ctx,
    int id, ID3D11ShaderResourceView** out_srv, int* w, int* h)
{
    std::vector<unsigned char> data;
    if (!LoadResourceData(id, L"RCDATA", data))
        return false;

    HRESULT hr = LoadTextureFromMemory(dev, ctx, data.data(), data.size(),
        out_srv, w, h);

    return SUCCEEDED(hr);
}

// Mouse relatif delta
inline void AimMoveDelta(float dx, float dy)
{
    LONG mx = static_cast<LONG>(std::lround(dx));
    LONG my = static_cast<LONG>(std::lround(dy));
    if (mx || my) mouse_event(MOUSEEVENTF_MOVE, mx, my, 0, 0);
}

// === Overlay helpers: PID -> HWND ve boyut/konum eşitleme ===
static BOOL CALLBACK EnumWindowsProcFindByPid(HWND hWnd, LPARAM lParam) {
    DWORD wndPid = 0;
    GetWindowThreadProcessId(hWnd, &wndPid);
    if (wndPid == (DWORD)lParam) {
        // Sadece görünür ve üst seviye olsun:
        if (IsWindowVisible(hWnd) && GetWindow(hWnd, GW_OWNER) == nullptr) {
            SetLastError((DWORD)(ULONG_PTR)hWnd); // küçük hile: HWND'i döndürmek için last error'ı kullanıyoruz
            return FALSE; // durdur
        }
    }
    return TRUE; // devam
}

static HWND FindMainWindowByPid(DWORD pid) {
    SetLastError(0);
    EnumWindows(EnumWindowsProcFindByPid, (LPARAM)pid);
    HWND h = (HWND)(ULONG_PTR)GetLastError();
    return (h ? h : nullptr);
}

static void SyncOverlayToTarget(HWND overlay, HWND target) {
    if (!overlay || !target) return;

    RECT rcClient{};
    GetClientRect(target, &rcClient);

    POINT tl{ rcClient.left, rcClient.top };
    POINT br{ rcClient.right, rcClient.bottom };
    ClientToScreen(target, &tl);
    ClientToScreen(target, &br);

    const int x = tl.x;
    const int y = tl.y;
    const int w = br.x - tl.x;
    const int h = br.y - tl.y;

    // Konum + boyutu tek çağrıda uygula:
    SetWindowPos(overlay, HWND_TOPMOST, x, y, w, h, SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK window_procedure(HWND window, UINT message, WPARAM w_param, LPARAM l_param) {
    if (ImGui_ImplWin32_WndProcHandler(window, message, w_param, l_param)) {
        return 0L;
    }

    if (message == WM_DESTROY) {
        PostQuitMessage(0);
        return 0L;
    }

    switch (message)
    {
    case WM_NCHITTEST:
    {
        // menü kapalıysa tıklama oyuna geçsin (overlay click-through)
        if (!showImGui)
            return HTTRANSPARENT;

        // menü açıkken mevcut davranış kalsın
        const LONG borderWidth = GetSystemMetrics(SM_CXSIZEFRAME);
        const LONG titleBarHeight = GetSystemMetrics(SM_CYCAPTION);
        POINT cursorPos = { GET_X_LPARAM(w_param), GET_Y_LPARAM(l_param) };
        RECT windowRect; GetWindowRect(window, &windowRect);

        if (cursorPos.y >= windowRect.top && cursorPos.y < windowRect.top + titleBarHeight)
            return HTCAPTION;

        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(window, message, w_param, l_param);
}

int GetProcessIdByName(const wchar_t* processName) {
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (Process32First(snapshot, &entry) == TRUE) {
        while (Process32Next(snapshot, &entry) == TRUE) {
            if (_wcsicmp(entry.szExeFile, processName) == 0) {
                CloseHandle(snapshot);
                return entry.th32ProcessID;
            }
        }
    }
    CloseHandle(snapshot);
    return 0;
}

DWORD_PTR GetModuleBaseAddress(DWORD dwPid, const wchar_t* moduleName) {
    MODULEENTRY32 moduleEntry = { sizeof(MODULEENTRY32) };
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, dwPid);
    if (hSnapshot == INVALID_HANDLE_VALUE)
        return 0;

    if (Module32First(hSnapshot, &moduleEntry)) {
        do {
            if (!_wcsicmp(moduleEntry.szModule, moduleName)) {
                CloseHandle(hSnapshot);
                return (DWORD_PTR)moduleEntry.modBaseAddr;
            }
        } while (Module32Next(hSnapshot, &moduleEntry));
    }
    CloseHandle(hSnapshot);
    return 0;
}

namespace driver {
    namespace codes {
        // used to setup the driver
        constexpr ULONG attach =
            CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);

        // read process memory from um application 
        constexpr ULONG read =
            CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);

        // write process memory from um application
        constexpr ULONG write =
            CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
    } // namespace codes

    // shared between user mode and kernel mode
    struct Request {
        HANDLE process_id;

        PVOID target;
        PVOID buffer;

        SIZE_T size;
        SIZE_T return_size;
    };

    bool attach_to_process(HANDLE driver_handle, const DWORD pid) {
        Request r;
        r.process_id = reinterpret_cast<HANDLE>(pid);

        return DeviceIoControl(driver_handle, codes::attach, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);
    }


    template <class T>
    T read_memory(HANDLE driver_handle, const std::uintptr_t addr) {
        T temp = {};

        Request r;
        r.target = reinterpret_cast<PVOID>(addr);
        r.buffer = &temp;
        r.size = sizeof(T);

        DeviceIoControl(driver_handle, codes::read, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);

        return temp;
    }

    template <class T>
    void write_memory(HANDLE driver_handle, const std::uintptr_t addr, const T& value) {
        Request r;
        r.target = reinterpret_cast<PVOID>(addr);
        r.buffer = (PVOID)&value;
        r.size = sizeof(T);

        DeviceIoControl(driver_handle, codes::write, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);
    }
} // namespace driver

std::string read_string(HANDLE driver, uintptr_t address, size_t size = 32) {
    char buffer[32] = { 0 };

    driver::Request r;
    r.target = reinterpret_cast<PVOID>(address);
    r.buffer = &buffer;
    r.size = size;
    r.return_size = 0;

    DeviceIoControl(driver, driver::codes::read, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);

    return std::string(buffer);
}
// === Skeet-benzeri menü yardımcıları ===
static bool SideTab(const char* id, const char* text, bool selected,
    const ImVec2& size = ImVec2(75, 75)) {
    ImGui::PushID(id);
    ImGui::PushStyleColor(ImGuiCol_Button, selected ? ImVec4(0.35f, 0.33f, 0.46f, 1.0f) : ImVec4(0.20f, 0.20f, 0.25f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.40f, 0.38f, 0.52f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.33f, 0.46f, 1.0f));
    bool clicked = ImGui::Button(text, size);
    ImGui::PopStyleColor(3);
    ImGui::PopID();
    return clicked;
}

static void ColorBarTop(float w = -1.0f, float h = 2.0f) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetWindowPos();
    ImVec2 s = ImGui::GetWindowSize();
    if (w < 0.0f) w = s.x;
    dl->AddRectFilled(ImVec2(p.x, p.y), ImVec2(p.x + w, p.y + h), IM_COL32(140, 120, 180, 255));
}

namespace UI {
    // Animasyon değerleri için cache
    static std::unordered_map<std::string, float> animations;

    // Smooth animation helper
    inline float GetAnimation(const std::string& id, bool state, float speed = 0.095f) {
        auto& value = animations[id];
        float target = state ? 1.0f : 0.0f;

        if (value < target) {
            value = std::min(value + speed * ImGui::GetIO().DeltaTime * 60.f, target);
        }
        else if (value > target) {
            value = std::max(value - speed * ImGui::GetIO().DeltaTime * 60.f, target);
        }

        return value;
    }

    // Easing functions
    inline float EaseOutQuad(float t) { return t * (2.0f - t); }
    inline float EaseInOutQuad(float t) {
        return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
    }
    inline float EaseOutCubic(float t) {
        float f = t - 1.0f;
        return f * f * f + 1.0f;
    }

    // Color lerp
    inline ImVec4 LerpColor(ImVec4 a, ImVec4 b, float t) {
        return ImVec4(
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t,
            a.z + (b.z - a.z) * t,
            a.w + (b.w - a.w) * t
        );
    }

    // Renk paleti
    namespace Colors {
        inline ImVec4 Primary = ImVec4(0.26f, 0.59f, 0.98f, 1.0f);
        inline ImVec4 PrimaryHovered = ImVec4(0.36f, 0.69f, 1.0f, 1.0f);
        inline ImVec4 PrimaryActive = ImVec4(0.16f, 0.49f, 0.88f, 1.0f);

        inline ImVec4 Background = ImVec4(0.10f, 0.10f, 0.13f, 1.0f);
        inline ImVec4 BackgroundDark = ImVec4(0.06f, 0.06f, 0.08f, 1.0f);
        inline ImVec4 BackgroundLight = ImVec4(0.14f, 0.14f, 0.17f, 1.0f);

        inline ImVec4 Text = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);
        inline ImVec4 TextDark = ImVec4(0.60f, 0.60f, 0.60f, 1.0f);
        inline ImVec4 TextDisabled = ImVec4(0.40f, 0.40f, 0.40f, 1.0f);

        inline ImVec4 Accent = ImVec4(0.85f, 0.33f, 0.0f, 1.0f);
        inline ImVec4 AccentHovered = ImVec4(1.0f, 0.43f, 0.1f, 1.0f);
    }
}

// Custom Widgets
namespace Widgets {

    // Animated Tab
    bool AnimatedTab(const char* label, bool selected, const ImVec2& size = ImVec2(0, 0)) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);
        const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

        ImVec2 pos = window->DC.CursorPos;
        ImVec2 size_arg = size;
        if (size_arg.x <= 0.0f) size_arg.x = label_size.x + style.FramePadding.x * 2.0f;
        if (size_arg.y <= 0.0f) size_arg.y = label_size.y + style.FramePadding.y * 2.0f;

        const ImRect bb(pos, ImVec2(pos.x + size_arg.x, pos.y + size_arg.y));
        ImGui::ItemSize(size_arg, style.FramePadding.y);
        if (!ImGui::ItemAdd(bb, id)) return false;

        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

        // Animation
        std::string anim_id = std::string("tab_") + label;
        float anim = UI::GetAnimation(anim_id, selected || hovered, 0.12f);
        float select_anim = UI::GetAnimation(anim_id + "_select", selected, 0.08f);

        // Colors
        ImVec4 bg_color = UI::LerpColor(
            UI::Colors::BackgroundLight,
            UI::Colors::Primary,
            select_anim
        );

        ImVec4 text_color = UI::LerpColor(
            UI::Colors::TextDark,
            UI::Colors::Text,
            UI::EaseOutQuad(anim)
        );

        // Render
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // Background
        draw_list->AddRectFilled(
            bb.Min,
            bb.Max,
            ImGui::GetColorU32(bg_color),
            4.0f
        );

        // Hover effect
        if (hovered && !selected) {
            ImVec4 hover_col = UI::Colors::Primary;
            hover_col.w = anim * 0.5f;
            draw_list->AddRect(
                bb.Min,
                bb.Max,
                ImGui::GetColorU32(hover_col),
                4.0f
            );
        }

        // Selected indicator
        if (selected) {
            ImVec2 indicator_min = ImVec2(bb.Min.x, bb.Max.y - 3.0f);
            ImVec2 indicator_max = ImVec2(bb.Max.x, bb.Max.y);
            draw_list->AddRectFilled(
                indicator_min,
                indicator_max,
                ImGui::GetColorU32(UI::Colors::Accent),
                2.0f
            );
        }

        // Text
        ImVec2 text_pos = ImVec2(
            bb.Min.x + (bb.Max.x - bb.Min.x - label_size.x) * 0.5f,
            bb.Min.y + (bb.Max.y - bb.Min.y - label_size.y) * 0.5f
        );
        draw_list->AddText(text_pos, ImGui::GetColorU32(text_color), label);

        return pressed;
    }

    // Custom Checkbox
    bool AnimatedCheckbox(const char* label, bool* v) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);
        const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

        const float square_sz = ImGui::GetFrameHeight();
        const ImVec2 pos = window->DC.CursorPos;
        const ImRect total_bb(pos, ImVec2(pos.x + square_sz + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), pos.y + square_sz));
        ImGui::ItemSize(total_bb, style.FramePadding.y);
        if (!ImGui::ItemAdd(total_bb, id)) return false;

        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
        if (pressed) {
            *v = !(*v);
            ImGui::MarkItemEdited(id);
        }

        // Animation
        std::string anim_id = std::string("check_") + label;
        float check_anim = UI::GetAnimation(anim_id, *v, 0.15f);
        float hover_anim = UI::GetAnimation(anim_id + "_hover", hovered, 0.2f);

        const ImRect check_bb(pos, ImVec2(pos.x + square_sz, pos.y + square_sz));

        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // Background
        ImVec4 bg_color = UI::LerpColor(
            UI::Colors::BackgroundDark,
            UI::Colors::Primary,
            UI::EaseOutCubic(check_anim)
        );

        draw_list->AddRectFilled(
            check_bb.Min,
            check_bb.Max,
            ImGui::GetColorU32(bg_color),
            3.0f
        );

        // Border
        ImVec4 border_color = UI::LerpColor(
            UI::Colors::BackgroundLight,
            UI::Colors::Primary,
            hover_anim
        );

        draw_list->AddRect(
            check_bb.Min,
            check_bb.Max,
            ImGui::GetColorU32(border_color),
            3.0f,
            0,
            1.5f
        );

        // Checkmark
        if (check_anim > 0.0f) {
            const float pad = ImMax(1.0f, (float)(int)(square_sz / 6.0f));
            const float sz = square_sz - pad * 2.0f;
            const float thickness = ImMax(1.0f, square_sz / 5.0f);

            ImVec2 center = ImVec2(check_bb.Min.x + square_sz * 0.5f, check_bb.Min.y + square_sz * 0.5f);

            // Animated checkmark drawing
            float check_sz = sz * UI::EaseOutCubic(check_anim);

            ImVec2 p1 = ImVec2(center.x - check_sz * 0.3f, center.y);
            ImVec2 p2 = ImVec2(center.x - check_sz * 0.1f, center.y + check_sz * 0.3f);
            ImVec2 p3 = ImVec2(center.x + check_sz * 0.4f, center.y - check_sz * 0.4f);

            draw_list->AddLine(p1, p2, ImGui::GetColorU32(UI::Colors::Text), thickness);
            draw_list->AddLine(p2, p3, ImGui::GetColorU32(UI::Colors::Text), thickness);
        }

        // Label
        if (label_size.x > 0.0f) {
            ImVec2 label_pos = ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x, check_bb.Min.y + style.FramePadding.y);
            draw_list->AddText(label_pos, ImGui::GetColorU32(UI::Colors::Text), label);
        }

        return pressed;
    }

    // Custom Slider
    bool AnimatedSlider(const char* label, float* v, float v_min, float v_max, const char* format = "%.0f") {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);

        const float w = ImGui::CalcItemWidth();
        const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
        const ImRect frame_bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + w, window->DC.CursorPos.y + label_size.y + style.FramePadding.y * 2.0f));
        const ImRect total_bb(frame_bb.Min, ImVec2(frame_bb.Max.x + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), frame_bb.Max.y));

        ImGui::ItemSize(total_bb, style.FramePadding.y);
        if (!ImGui::ItemAdd(total_bb, id)) return false;

        //const bool hovered = ImGui::ItemHoverable(frame_bb, id, ImGuiItemFlags_None);
        //const bool hovered = ImGui::ItemHoverable(frame_bb, id);
        const bool hovered = ImGui::IsItemHovered();
        bool temp_input_is_active = ImGui::TempInputIsActive(id);
        if (!temp_input_is_active) {
            const bool clicked = (hovered && g.IO.MouseClicked[0]);
            if (clicked) {
                ImGui::SetActiveID(id, window);
                ImGui::SetFocusID(id, window);
                ImGui::FocusWindow(window);
            }
        }

        // Animation
        std::string anim_id = std::string("slider_") + label;
        float hover_anim = UI::GetAnimation(anim_id, hovered || ImGui::IsItemActive(), 0.15f);

        // Slider logic
        ImRect grab_bb;
        const bool value_changed = ImGui::SliderBehavior(frame_bb, id, ImGuiDataType_Float, v, &v_min, &v_max, format, ImGuiSliderFlags_None, &grab_bb);
        if (value_changed) ImGui::MarkItemEdited(id);

        // Render
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // Background track
        const float track_height = 4.0f;
        ImVec2 track_min = ImVec2(frame_bb.Min.x, frame_bb.Min.y + (frame_bb.Max.y - frame_bb.Min.y - track_height) * 0.5f);
        ImVec2 track_max = ImVec2(frame_bb.Max.x, track_min.y + track_height);

        draw_list->AddRectFilled(
            track_min,
            track_max,
            ImGui::GetColorU32(UI::Colors::BackgroundDark),
            2.0f
        );

        // Filled track
        float fraction = (*v - v_min) / (v_max - v_min);
        ImVec2 fill_max = ImVec2(track_min.x + (track_max.x - track_min.x) * fraction, track_max.y);

        ImVec4 fill_color = UI::LerpColor(
            UI::Colors::Primary,
            UI::Colors::PrimaryHovered,
            hover_anim
        );

        draw_list->AddRectFilled(
            track_min,
            fill_max,
            ImGui::GetColorU32(fill_color),
            2.0f
        );

        // Grab
        const float grab_radius = 8.0f + hover_anim * 2.0f;
        ImVec2 grab_center = ImVec2(fill_max.x, (track_min.y + track_max.y) * 0.5f);

        draw_list->AddCircleFilled(
            grab_center,
            grab_radius,
            ImGui::GetColorU32(UI::Colors::Text),
            16
        );

        // Grab border
        draw_list->AddCircle(
            grab_center,
            grab_radius + 1.0f,
            ImGui::GetColorU32(fill_color),
            16,
            2.0f
        );

        // Value text
        char value_buf[64];
        const char* value_buf_end = value_buf + ImGui::DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), ImGuiDataType_Float, v, format);
        ImVec2 value_size = ImGui::CalcTextSize(value_buf, value_buf_end);
        ImVec2 value_pos = ImVec2(frame_bb.Max.x - value_size.x - 5.0f, frame_bb.Min.y);
        draw_list->AddText(value_pos, ImGui::GetColorU32(UI::Colors::TextDark), value_buf, value_buf_end);

        // Label
        if (label_size.x > 0.0f) {
            ImVec2 label_pos = ImVec2(frame_bb.Min.x, frame_bb.Min.y);
            draw_list->AddText(label_pos, ImGui::GetColorU32(UI::Colors::Text), label);
        }

        return value_changed;
    }

    // ✅ INT SLIDER VERSIYONU (GIF FPS İÇİN)
    bool AnimatedSliderInt(const char* label, int* v, int v_min, int v_max, const char* format = "%d") {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);

        const float w = ImGui::CalcItemWidth();
        const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
        const ImRect frame_bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + w, window->DC.CursorPos.y + label_size.y + style.FramePadding.y * 2.0f));
        const ImRect total_bb(frame_bb.Min, ImVec2(frame_bb.Max.x + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), frame_bb.Max.y));

        ImGui::ItemSize(total_bb, style.FramePadding.y);
        if (!ImGui::ItemAdd(total_bb, id)) return false;

        // Güvenli hover detection
        bool hovered = ImGui::IsMouseHoveringRect(frame_bb.Min, frame_bb.Max);

        bool temp_input_is_active = ImGui::TempInputIsActive(id);
        if (!temp_input_is_active) {
            const bool clicked = (hovered && g.IO.MouseClicked[0]);
            if (clicked) {
                ImGui::SetActiveID(id, window);
                ImGui::SetFocusID(id, window);
                ImGui::FocusWindow(window);
            }
        }

        // Animation
        std::string anim_id = std::string("slider_") + label;
        float hover_anim = UI::GetAnimation(anim_id, hovered || ImGui::IsItemActive(), 0.15f);

        // Slider logic (INT VERSION)
        ImRect grab_bb;
        const bool value_changed = ImGui::SliderBehavior(frame_bb, id, ImGuiDataType_S32, v, &v_min, &v_max, format, ImGuiSliderFlags_None, &grab_bb);
        if (value_changed) ImGui::MarkItemEdited(id);

        // Render
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // Background track
        const float track_height = 4.0f;
        ImVec2 track_min = ImVec2(frame_bb.Min.x, frame_bb.Min.y + (frame_bb.Max.y - frame_bb.Min.y - track_height) * 0.5f);
        ImVec2 track_max = ImVec2(frame_bb.Max.x, track_min.y + track_height);

        draw_list->AddRectFilled(
            track_min,
            track_max,
            ImGui::GetColorU32(UI::Colors::BackgroundDark),
            2.0f
        );

        // Filled track
        float fraction = (float)(*v - v_min) / (float)(v_max - v_min);
        ImVec2 fill_max = ImVec2(track_min.x + (track_max.x - track_min.x) * fraction, track_max.y);

        ImVec4 fill_color = UI::LerpColor(
            UI::Colors::Primary,
            UI::Colors::PrimaryHovered,
            hover_anim
        );

        draw_list->AddRectFilled(
            track_min,
            fill_max,
            ImGui::GetColorU32(fill_color),
            2.0f
        );

        // Grab
        const float grab_radius = 8.0f + hover_anim * 2.0f;
        ImVec2 grab_center = ImVec2(fill_max.x, (track_min.y + track_max.y) * 0.5f);

        draw_list->AddCircleFilled(
            grab_center,
            grab_radius,
            ImGui::GetColorU32(UI::Colors::Text),
            16
        );

        // Grab border
        draw_list->AddCircle(
            grab_center,
            grab_radius + 1.0f,
            ImGui::GetColorU32(fill_color),
            16,
            2.0f
        );

        // ===== LABEL (SOL TARAF) =====
        if (label_size.x > 0.0f) {
            ImVec2 label_pos = ImVec2(frame_bb.Min.x, frame_bb.Min.y + style.FramePadding.y);
            draw_list->AddText(label_pos, ImGui::GetColorU32(UI::Colors::Text), label);
        }

        // ===== VALUE (SAĞ TARAF - KUTULU) =====
        char value_buf[64];
        const char* value_buf_end = value_buf + ImGui::DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), ImGuiDataType_S32, v, format);
        ImVec2 value_size = ImGui::CalcTextSize(value_buf, value_buf_end);
        ImVec2 value_pos = ImVec2(frame_bb.Max.x - value_size.x - 8.0f, frame_bb.Min.y + style.FramePadding.y);

        // Value background box
        ImVec2 value_bg_min = ImVec2(value_pos.x - 4.0f, value_pos.y - 2.0f);
        ImVec2 value_bg_max = ImVec2(value_pos.x + value_size.x + 4.0f, value_pos.y + value_size.y + 2.0f);
        draw_list->AddRectFilled(value_bg_min, value_bg_max, ImGui::GetColorU32(UI::Colors::BackgroundDark), 3.0f);
        draw_list->AddRect(value_bg_min, value_bg_max, ImGui::GetColorU32(fill_color), 3.0f, 0, 1.0f);

        // Value text
        draw_list->AddText(value_pos, ImGui::GetColorU32(UI::Colors::Primary), value_buf, value_buf_end);

        // ===== HOVER TOOLTIP (GRAB ÜZERİNDE) =====
        if (hovered || ImGui::IsItemActive()) {
            ImGui::SetTooltip("%d FPS", *v);
        }

        return value_changed;
    }

    // Combo box with animation
    bool AnimatedCombo(const char* label, int* current_item, const char* items_separated_by_zeros, int popup_max_height_in_items = -1) {
        ImGuiContext& g = *GImGui;

        std::string anim_id = std::string("combo_") + label;
        bool is_open = ImGui::IsPopupOpen(label);
        float open_anim = UI::GetAnimation(anim_id, is_open, 0.12f);

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetColorU32(UI::LerpColor(UI::Colors::BackgroundLight, UI::Colors::Primary, open_anim)));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetColorU32(UI::Colors::PrimaryHovered));

        bool changed = ImGui::Combo(label, current_item, items_separated_by_zeros, popup_max_height_in_items);

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar();

        return changed;
    }

    // Group box with animation
    bool BeginGroupBox(const char* label, ImVec2 size = ImVec2(0, 0)) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;

        ImVec2 pos = window->DC.CursorPos;
        ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

        if (size.x <= 0.0f) size.x = ImGui::GetContentRegionAvail().x;
        if (size.y <= 0.0f) size.y = 300.0f;

        // Header
        ImRect header_bb(pos, ImVec2(pos.x + size.x, pos.y + label_size.y + style.FramePadding.y * 2.0f));

        ImDrawList* draw_list = window->DrawList;

        // Background with gradient
        draw_list->AddRectFilledMultiColor(
            header_bb.Min,
            header_bb.Max,
            ImGui::GetColorU32(UI::Colors::BackgroundLight),
            ImGui::GetColorU32(UI::Colors::BackgroundLight),
            ImGui::GetColorU32(UI::Colors::Background),
            ImGui::GetColorU32(UI::Colors::Background)
        );

        // Title text
        ImVec2 text_pos = ImVec2(header_bb.Min.x + style.FramePadding.x, header_bb.Min.y + style.FramePadding.y);
        draw_list->AddText(text_pos, ImGui::GetColorU32(UI::Colors::Text), label);

        // Accent line
        draw_list->AddLine(
            ImVec2(header_bb.Min.x, header_bb.Max.y),
            ImVec2(header_bb.Max.x, header_bb.Max.y),
            ImGui::GetColorU32(UI::Colors::Primary),
            2.0f
        );

        // Content area
        ImVec2 content_pos = ImVec2(pos.x, header_bb.Max.y);
        ImVec2 content_size = ImVec2(size.x, size.y - (header_bb.Max.y - pos.y));

        ImGui::SetCursorScreenPos(ImVec2(content_pos.x + style.FramePadding.x, content_pos.y + style.FramePadding.y));

        ImGui::PushID(label);
        ImGui::BeginGroup();

        return true;
    }

    void EndGroupBox() {
        ImGui::EndGroup();
        ImGui::PopID();
        ImGui::Dummy(ImVec2(0, 5)); // Spacing
    }
}

// ============================================
// RAGNAREK-Style Menu Implementation
// ============================================

static void RenderRAGNAREKMenu(
    int& r, int& g, int& b,
    int& t_r, int& t_g, int& t_b,
    int& ch_r, int& ch_g, int& ch_b
)
{
    ImGui::SetNextWindowSize(ImVec2(750.f, 550.f), ImGuiCond_Once);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::GetColorU32(UI::Colors::Background));
    //ImGui::PushStyleColor(ImGuiCol_Border, ImGui::GetColorU32(UI::Colors::Primary, 0.5f));
    ImVec4 border_col = UI::Colors::Primary;
    border_col.w = 0.5f;
    ImGui::PushStyleColor(ImGuiCol_Border, border_col);

    if (!ImGui::Begin("MONIX | CS2 External", nullptr,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);
        ImGui::End();
        return;
    }

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetWindowPos();
    ImVec2 s = ImGui::GetWindowSize();

    // Gradient header
    draw_list->AddRectFilledMultiColor(
        p,
        ImVec2(p.x + s.x, p.y + 60),
        ImGui::GetColorU32(ImVec4(0.15f, 0.15f, 0.20f, 1.0f)),
        ImGui::GetColorU32(ImVec4(0.15f, 0.15f, 0.20f, 1.0f)),
        ImGui::GetColorU32(ImVec4(0.10f, 0.10f, 0.13f, 1.0f)),
        ImGui::GetColorU32(ImVec4(0.10f, 0.10f, 0.13f, 1.0f))
    );

    // Top accent line
    draw_list->AddRectFilledMultiColor(
        p,
        ImVec2(p.x + s.x, p.y + 3),
        ImGui::GetColorU32(UI::Colors::Primary),
        ImGui::GetColorU32(UI::Colors::Accent),
        ImGui::GetColorU32(UI::Colors::Accent),
        ImGui::GetColorU32(UI::Colors::Primary)
    );

    // Title
    ImGui::SetCursorPos(ImVec2(20, 30));
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // Varsayılan font
    ImGui::TextColored(UI::Colors::Text, "MONIX");
    ImGui::PopFont();

    ImGui::SameLine();
    ImGui::SetCursorPosY(30);
    ImGui::TextColored(UI::Colors::TextDark, "CS2 External");

    // Tabs
    static int current_tab = 0;

    ImGui::SetCursorPos(ImVec2(10, 70));
    ImGui::BeginChild("TabBar", ImVec2(s.x - 20, 50), false, ImGuiWindowFlags_NoScrollbar);
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 0));

        if (Widgets::AnimatedTab(ICON_FA_CROSSHAIRS "AIMBOT", current_tab == 0, ImVec2(175, 50)))
            current_tab = 0;
        ImGui::SameLine();

        if (Widgets::AnimatedTab(ICON_FA_EYE "VISUALS", current_tab == 1, ImVec2(175, 50)))
            current_tab = 1;
        ImGui::SameLine();

        if (Widgets::AnimatedTab(ICON_FA_SLIDERS_H "MISC", current_tab == 2, ImVec2(175, 50)))
            current_tab = 2;
        ImGui::SameLine();

        if (Widgets::AnimatedTab(ICON_FA_COG "CONFIG", current_tab == 3, ImVec2(175, 50)))
            current_tab = 3;

        ImGui::PopStyleVar();
    }
    ImGui::EndChild();

    // Content area
    ImGui::SetCursorPos(ImVec2(10, 130));
    ImGui::BeginChild("Content", ImVec2(s.x - 20, s.y - 140), false);
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));

        switch (current_tab)
        {
        case 0: // AIMBOT
        {
            ImGui::Columns(2, nullptr, false);
            ImGui::SetColumnWidth(0, (s.x - 40) * 0.5f);

            // Left column
            if (Widgets::BeginGroupBox("Aimbot Settings", ImVec2(-1, 300)))
            {
                Widgets::AnimatedCheckbox("Enable Aimbot", &aimbotEnabled);
                ImGui::Spacing();

                Widgets::AnimatedSlider("FOV", &aimbotFovPx, 20.f, 400.f, "%.0f px");
                Widgets::AnimatedSlider("Smooth", &aimbotSmooth, 1.f, 30.f, "%.1f");
                Widgets::AnimatedSlider("Dead Zone", &aimDeadZone, 0.f, 5.f, "%.1f px");

                ImGui::Spacing();
                Widgets::AnimatedCheckbox("Draw FOV Circle", &drawAimFov);

                ImGui::Spacing();
                static const char* hotkeys = "ALT\0LMB\0RMB\0XBUTTON1\0XBUTTON2\0CAPS\0LSHIFT\0LCTRL\0\0";
                Widgets::AnimatedCombo("Hotkey", &aimbotHotkeyIndex, hotkeys);

                static const char* bones = "Head\0Neck\0Upper Chest\0Lower Chest\0Stomach\0Pelvis\0\0";
                Widgets::AnimatedCombo("Target Bone", &aimBoneSelect, bones);
            }
            Widgets::EndGroupBox();

            if (Widgets::BeginGroupBox("Advanced", ImVec2(-1, 150)))
            {
                Widgets::AnimatedCheckbox("Lock After Release", &lockPersistsAfterKey);
                ImGui::Spacing();

                Widgets::AnimatedCheckbox("Visibility Check", &aimbotVisibilityCheck); // ← YENİ
                ImGui::Spacing();

                ImGui::TextColored(UI::Colors::TextDark, "Switch Key: XBUTTON1");
            }
            Widgets::EndGroupBox();

            // Right column
            ImGui::NextColumn();

            if (Widgets::BeginGroupBox("Colors", ImVec2(-1, 200)))
            {
                ImVec4 enemy_col = ImVec4(r / 255.f, g / 255.f, b / 255.f, 1.f);
                ImGui::ColorEdit3("Enemy Color", (float*)&enemy_col, ImGuiColorEditFlags_NoInputs);
                r = (int)(enemy_col.x * 255);
                g = (int)(enemy_col.y * 255);
                b = (int)(enemy_col.z * 255);

                ImGui::Spacing();

                ImVec4 team_col = ImVec4(t_r / 255.f, t_g / 255.f, t_b / 255.f, 1.f);
                ImGui::ColorEdit3("Team Color", (float*)&team_col, ImGuiColorEditFlags_NoInputs);
                t_r = (int)(team_col.x * 255);
                t_g = (int)(team_col.y * 255);
                t_b = (int)(team_col.z * 255);
            }
            Widgets::EndGroupBox();

            if (Widgets::BeginGroupBox("Quick ESP", ImVec2(-1, 250)))
            {
                Widgets::AnimatedCheckbox("Show Team", &showTeam);
                Widgets::AnimatedCheckbox("ESP Lines", &drawLines);
                Widgets::AnimatedCheckbox("Box ESP", &showBoxESP);
                Widgets::AnimatedCheckbox("Name ESP", &showNameESP);
                Widgets::AnimatedCheckbox("Health Text", &showHealthESP);
                Widgets::AnimatedCheckbox("Health Bar", &showHealthBarESP);
                Widgets::AnimatedCheckbox("Flash Warning", &showFlashESP);
            }
            Widgets::EndGroupBox();

            ImGui::Columns(1);
            break;
        }

        case 1: // VISUALS
        {
            ImGui::Columns(2, nullptr, false);
            ImGui::SetColumnWidth(0, (s.x - 40) * 0.5f);

            if (Widgets::BeginGroupBox("Player ESP", ImVec2(-1, 280)))
            {
                Widgets::AnimatedCheckbox("Enable ESP", &enableTrue);
                ImGui::Spacing();

                Widgets::AnimatedCheckbox("Show Team", &showTeam);
                Widgets::AnimatedCheckbox("Box ESP", &showBoxESP);
                Widgets::AnimatedCheckbox("Snap Lines", &drawLines);
                Widgets::AnimatedCheckbox("Name Tag", &showNameESP);
                Widgets::AnimatedCheckbox("Health Text", &showHealthESP);
                Widgets::AnimatedCheckbox("Health Bar", &showHealthBarESP);
                Widgets::AnimatedCheckbox("Defuse Warning", &showDefuseESP);
                Widgets::AnimatedCheckbox("C4 Location", &showC4ESP);
                Widgets::AnimatedCheckbox("Weapon Name", &showWeaponESP);     // ← YENİ
                Widgets::AnimatedCheckbox("Money Amount", &showMoneyESP);     // ← YENİ
                Widgets::AnimatedCheckbox("Flash Indicator", &showFlashESP);
                Widgets::AnimatedCheckbox("Skeleton ESP", &showSkeletonESP);
                Widgets::AnimatedCheckbox("3D Cylinder ESP", &showCylinderESP);
            }
            Widgets::EndGroupBox();

            if (Widgets::BeginGroupBox("China Hat", ImVec2(-1, 180)))  // ← Yükseklik 150→180
            {
                Widgets::AnimatedCheckbox("Enable", &showChinaHat);
                ImGui::Spacing();

                Widgets::AnimatedSlider("Height Offset", &chinaHatOffset, -50.f, 50.f, "%.1f");
                Widgets::AnimatedSlider("Radius", &chinaHatRadius, 5.f, 100.f, "%.1f");
                Widgets::AnimatedSlider("Hat Height", &chinaHatHeight, 5.f, 50.f, "%.1f");

                ImGui::Spacing();

                ImVec4 hat_col = ImVec4(ch_r / 255.f, ch_g / 255.f, ch_b / 255.f, 1.f);
                ImGui::ColorEdit3("Hat Color", (float*)&hat_col, ImGuiColorEditFlags_NoInputs);
                ch_r = (int)(hat_col.x * 255);
                ch_g = (int)(hat_col.y * 255);
                ch_b = (int)(hat_col.z * 255);
            }
            Widgets::EndGroupBox();

            ImGui::NextColumn();

            if (Widgets::BeginGroupBox("Image ESP", ImVec2(-1, 350)))
            {
                Widgets::AnimatedCheckbox("Show Enemy PNG", &showPhotoInBox);
                ImGui::Spacing();

                Widgets::AnimatedCheckbox("Hitler GIF", &showGifInBox);
                if (showGifInBox) {
                    Widgets::AnimatedSliderInt("FPS##1", &gifFps, 1, 60);
                    //ImGui::SliderInt("FPS##1", &gifFps, 1, 60);
                }

                ImGui::Spacing();
                Widgets::AnimatedCheckbox("Femboy GIF", &showGifInBox2);
                if (showGifInBox2) {
                    Widgets::AnimatedSliderInt("FPS##2", &gifFps2, 1, 60);
                }

                ImGui::Spacing();
                Widgets::AnimatedCheckbox("Hunnypaint GIF", &showGifInBox3);
                if (showGifInBox3) {
                    Widgets::AnimatedSliderInt("FPS##3", &gifFps3, 1, 60);
                }

                ImGui::Spacing();
                Widgets::AnimatedCheckbox("Roles GIF", &showGifInBox4);
                if (showGifInBox4) {
                    Widgets::AnimatedSliderInt("FPS##4", &gifFps4, 1, 60);
                }
            }
            Widgets::EndGroupBox();

            ImGui::Columns(1);
            break;
        }

        case 2: // MISC
        {
            ImGui::Columns(2, nullptr, false);
            ImGui::SetColumnWidth(0, (s.x - 40) * 0.5f);

            if (Widgets::BeginGroupBox("Recoil Control (RCS)", ImVec2(-1, 250)))
            {
                Widgets::AnimatedCheckbox("Enable RCS", &rcsEnabled);
                ImGui::Spacing();

                Widgets::AnimatedSlider("Start Bullet", (float*)&rcsStartBullet, 0.f, 10.f, "%.0f");
                Widgets::AnimatedSlider("Scale X", &rcsScaleX, 0.f, 3.f, "%.2f");
                Widgets::AnimatedSlider("Scale Y", &rcsScaleY, 0.f, 3.f, "%.2f");
                Widgets::AnimatedSlider("Sensitivity", &rcsDegToMouse, 0.005f, 0.030f, "%.3f");
            }
            Widgets::EndGroupBox();

            ImGui::NextColumn();

            if (Widgets::BeginGroupBox("Information", ImVec2(-1, 250)))
            {
                ImGui::TextColored(UI::Colors::Primary, "Hotkeys");
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::BulletText("INSERT - Toggle Menu");
                ImGui::BulletText("DELETE - Exit");
                ImGui::Spacing();
                ImGui::Spacing();

                ImGui::TextColored(UI::Colors::Primary, "Status");
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
                ImGui::Text("Aimbot: %s", aimbotEnabled ? "ACTIVE" : "INACTIVE");
                ImGui::Text("ESP: %s", enableTrue ? "ACTIVE" : "INACTIVE");
            }
            Widgets::EndGroupBox();

            ImGui::Columns(1);
            break;
        }

        case 3: // CONFIG
        {
            ImGui::SetCursorPosX((s.x - 300) * 0.5f);
            ImGui::SetCursorPosY(100);

            ImGui::BeginChild("ConfigArea", ImVec2(300, 300), true);
            {
                ImGui::TextColored(UI::Colors::Primary, "Configuration");
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::Button("Save Config", ImVec2(-1, 40))) {
                    // Save logic here
                }

                ImGui::Spacing();

                if (ImGui::Button("Load Config", ImVec2(-1, 40))) {
                    // Load logic here
                }

                ImGui::Spacing();
                ImGui::Spacing();

                //ImGui::TextColored(UI::Colors::TextDark, "Made with " ICON_FA_CROSSHAIRS " by RAGNAREK");
                ImGui::TextColored(UI::Colors::TextDark, "Made with");
                ImGui::SameLine(0, 2);
                ImGui::TextColored(UI::Colors::Primary, ICON_FA_CROSSHAIRS);
                ImGui::SameLine(0, 2);
                ImGui::TextColored(UI::Colors::TextDark, "by MONIX");
            }
            ImGui::EndChild();
            break;
        }
        }

        ImGui::PopStyleVar();
    }
    ImGui::EndChild();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
    ImGui::End();
}

// offsets

// offsets.hpp
constexpr std::ptrdiff_t dwEntityList = 0x1D11CF8;
constexpr std::ptrdiff_t dwViewMatrix = 0x1E303D0;
constexpr std::ptrdiff_t dwLocalPlayerPawn = 0x1BECF38;

// client.dll.hpp
constexpr std::ptrdiff_t m_vOldOrigin = 0x15A0; // Vector
constexpr std::ptrdiff_t m_iTeamNum = 0x3EB; // uint8
constexpr std::ptrdiff_t m_lifeState = 0x354; // uint8
constexpr std::ptrdiff_t m_hPlayerPawn = 0x8FC; // CHandle<C_CSPlayerPawn>
constexpr std::ptrdiff_t m_vecViewOffset = 0xD80; // CNetworkViewOffsetVector
constexpr std::ptrdiff_t m_iHealth = 0x34C; // int32
constexpr std::ptrdiff_t m_pObserverServices = 0x1408;
constexpr std::ptrdiff_t m_hObserverTarget = 0x44;
constexpr std::ptrdiff_t m_iObserverMode = 0x40;
constexpr std::ptrdiff_t m_iszPlayerName = 0x6E8;
constexpr std::ptrdiff_t m_sSanitizedPlayerName = 0x850;
constexpr std::ptrdiff_t m_flFlashOverlayAlpha = 0x1604;
constexpr std::ptrdiff_t m_pGameSceneNode = 0x330; // TODO: kendi dump'ından
constexpr std::ptrdiff_t m_aimPunchAngle = 0x16E4; // Vec2 (float x,y)
constexpr std::ptrdiff_t m_iShotsFired = 0x272C; // int
// ===== YENİ EKLEMELER: Skeleton için =====
constexpr std::ptrdiff_t m_modelState = 0x190;    // CSkeletonInstance->m_modelState
constexpr std::ptrdiff_t m_boneArray = 0x210;      // modelState içindeki bone array offset
constexpr std::ptrdiff_t m_bIsDefusing = 0x271A; // bool
constexpr std::ptrdiff_t m_nBombSite = 0x1164; // int32
constexpr std::ptrdiff_t m_szName = 0x720; // CGlobalSymbol
constexpr std::ptrdiff_t m_iAccount = 0x40; // int32
constexpr std::ptrdiff_t m_pInGameMoneyServices = 0x7F8; // CCSPlayerController_InGameMoneyServices*
constexpr std::ptrdiff_t m_pClippingWeapon = 0x3DE0; // C_CSWeaponBase*
constexpr std::ptrdiff_t m_bSpottedByMask = 0xC; // uint32[2]
//constexpr std::ptrdiff_t m_bBombPlanted = 0x9A9; // bool
//constexpr std::ptrdiff_t m_bBombPlanted = 0x1FBB; // bool

constexpr std::ptrdiff_t dwPlantedC4 = 0x1E34BE8;
constexpr std::ptrdiff_t m_vecAbsOrigin = 0xD0; // VectorWS

// ============================================
// IMXNOOBX TARZI %100 ÇALIŞAN SKELETON ESP
// ============================================

// ===== IMXNOOBX EXACT METHOD =====
inline bool GetBonePosition(HANDLE driver, uintptr_t pawn, int boneIndex, Vector3& out) {
    if (!pawn) return false;

    // 1️⃣ Pawn → GameSceneNode (pointer)
    uintptr_t gameSceneNode = driver::read_memory<uintptr_t>(driver, pawn + m_pGameSceneNode);
    if (!gameSceneNode) return false;

    // 2️⃣ GameSceneNode + 0x210 → BoneArray (pointer)
    // IMXNOOBX'te hardcoded 0x210 kullanılmış
    uintptr_t boneArray = driver::read_memory<uintptr_t>(driver, gameSceneNode + m_boneArray);
    if (!boneArray) return false;

    // 3️⃣ BoneArray + (index * 32) → Bone Vector3
    out = driver::read_memory<Vector3>(driver, boneArray + (boneIndex * 0x20));

    return true;
}

// ===== IMXNOOBX TARZI SKELETON ÇİZİMİ =====
inline void DrawSkeleton(HANDLE driver, uintptr_t pawn, const view_matrix_t& vm, const RGB& color) {
    ImDrawList* dl = ImGui::GetBackgroundDrawList();

    // Renk ayarı
    ImU32 col = IM_COL32(color.R, color.G, color.B, 255);

    // Tüm bone'ları oku
    Vector3 bones[28] = {};
    bool valid = true;

    for (int i = 0; i < 28; ++i) {
        if (!GetBonePosition(driver, pawn, i, bones[i])) {
            valid = false;
            break;
        }
    }

    if (!valid) return;

    // Çizgi çizme helper
    auto drawLine = [&](int from, int to) {
        Vector3 screen1 = bones[from].WTS(vm);
        Vector3 screen2 = bones[to].WTS(vm);

        // Ekran dışı kontrolü
        if (screen1.z < 0.01f || screen2.z < 0.01f) return;

        dl->AddLine(
            ImVec2(screen1.x, screen1.y),
            ImVec2(screen2.x, screen2.y),
            col,
            2.0f // kalınlık
        );
        };

    // ===== OMURGA =====
    drawLine(6, 5);  // Head → Neck
    drawLine(5, 4);  // Neck → Spine1
    drawLine(4, 3);  // Spine1 → Spine2
    drawLine(3, 0);  // Spine2 → Pelvis

    // ===== SOL KOL =====
    drawLine(4, 8);   // Spine1 → L.Shoulder
    drawLine(8, 9);   // L.Shoulder → L.Elbow
    drawLine(9, 10);  // L.Elbow → L.Hand

    // ===== SAĞ KOL =====
    drawLine(4, 13);  // Spine1 → R.Shoulder
    drawLine(13, 14); // R.Shoulder → R.Elbow
    drawLine(14, 15); // R.Elbow → R.Hand

    // ===== SOL BACAK =====
    drawLine(0, 22);  // Pelvis → L.Hip
    drawLine(22, 23); // L.Hip → L.Knee
    drawLine(23, 24); // L.Knee → L.Foot

    // ===== SAĞ BACAK =====
    drawLine(0, 25);  // Pelvis → R.Hip
    drawLine(25, 26); // R.Hip → R.Knee
    drawLine(26, 27); // R.Knee → R.Foot
}

// ============================================
// 3D CYLINDER + SPHERE ESP SYSTEM (IMPROVED)
// ============================================

// ===== KÜRE ÇİZİMİ (KAFA İÇİN) =====
inline void DrawBoneSphere(
    const Vector3& center,
    float radius,
    const view_matrix_t& vm,
    const RGB& color,
    int segments = 8
)
{
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    ImU32 col = IM_COL32(color.R, color.G, color.B, 200);
    ImU32 colDark = IM_COL32(color.R / 2, color.G / 2, color.B / 2, 140);

    // Latitude circles (yatay daireler)
    for (int lat = 0; lat < segments / 2; lat++) {
        float latAngle = (3.14159265f / segments) * (lat + 1);
        float r = radius * std::sin(latAngle);
        float z = radius * std::cos(latAngle);

        std::vector<Vector3> circleScreen(segments);
        bool anyValid = false;

        for (int i = 0; i < segments; i++) {
            float angle = (2.0f * 3.14159265f / segments) * i;

            Vector3 point = {
                center.x + r * std::cos(angle),
                center.y + r * std::sin(angle),
                center.z + z
            };

            circleScreen[i] = point.WTS(vm);
            if (circleScreen[i].z > 0.01f) anyValid = true;
        }

        if (anyValid) {
            for (int i = 0; i < segments; i++) {
                int next = (i + 1) % segments;
                Vector3 p1 = circleScreen[i];
                Vector3 p2 = circleScreen[next];

                if (p1.z > 0.01f && p2.z > 0.01f) {
                    dl->AddLine(ImVec2(p1.x, p1.y), ImVec2(p2.x, p2.y), colDark, 1.5f);
                }
            }
        }
    }

    // Longitude circles (dikey daireler)
    for (int lon = 0; lon < segments / 2; lon++) {
        float lonAngle = (2.0f * 3.14159265f / segments) * lon;

        std::vector<Vector3> meridianScreen(segments);
        bool anyValid = false;

        for (int i = 0; i < segments; i++) {
            float latAngle = (3.14159265f / segments) * i;

            Vector3 point = {
                center.x + radius * std::sin(latAngle) * std::cos(lonAngle),
                center.y + radius * std::sin(latAngle) * std::sin(lonAngle),
                center.z + radius * std::cos(latAngle)
            };

            meridianScreen[i] = point.WTS(vm);
            if (meridianScreen[i].z > 0.01f) anyValid = true;
        }

        if (anyValid) {
            for (int i = 0; i < segments - 1; i++) {
                Vector3 p1 = meridianScreen[i];
                Vector3 p2 = meridianScreen[i + 1];

                if (p1.z > 0.01f && p2.z > 0.01f) {
                    dl->AddLine(ImVec2(p1.x, p1.y), ImVec2(p2.x, p2.y), col, 1.8f);
                }
            }
        }
    }
}

// ===== SİLİNDİR ÇİZİMİ (VÜCUt PARÇALARI İÇİN) =====
inline void DrawBoneCylinder(
    const Vector3& start,
    const Vector3& end,
    float radius,
    const view_matrix_t& vm,
    const RGB& color,
    int segments = 8
)
{
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    ImU32 col = IM_COL32(color.R, color.G, color.B, 180);
    ImU32 colDark = IM_COL32(color.R / 2, color.G / 2, color.B / 2, 120);

    // Silindir ekseni
    Vector3 axis = { end.x - start.x, end.y - start.y, end.z - start.z };
    float length = std::sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);

    if (length < 0.1f) return;

    // Normalize
    axis.x /= length;
    axis.y /= length;
    axis.z /= length;

    // Perpendicular vektörler (silindir dairesi için)
    Vector3 perp1, perp2;

    if (std::abs(axis.z) < 0.9f) {
        perp1 = { -axis.y, axis.x, 0 };
    }
    else {
        perp1 = { 1, 0, 0 };
    }

    // Normalize perp1
    float pLen = std::sqrt(perp1.x * perp1.x + perp1.y * perp1.y + perp1.z * perp1.z);
    if (pLen > 0.001f) {
        perp1.x /= pLen;
        perp1.y /= pLen;
        perp1.z /= pLen;
    }

    // Cross product: axis × perp1 = perp2
    perp2 = {
        axis.y * perp1.z - axis.z * perp1.y,
        axis.z * perp1.x - axis.x * perp1.z,
        axis.x * perp1.y - axis.y * perp1.x
    };

    // Başlangıç ve bitiş dairelerindeki noktalar
    std::vector<Vector3> startCircle(segments);
    std::vector<Vector3> endCircle(segments);
    std::vector<Vector3> startScreen(segments);
    std::vector<Vector3> endScreen(segments);

    bool anyValid = false;

    for (int i = 0; i < segments; i++) {
        float angle = (2.0f * 3.14159265f / segments) * i;
        float cosA = std::cos(angle) * radius;
        float sinA = std::sin(angle) * radius;

        // 3D daire noktaları
        startCircle[i] = {
            start.x + perp1.x * cosA + perp2.x * sinA,
            start.y + perp1.y * cosA + perp2.y * sinA,
            start.z + perp1.z * cosA + perp2.z * sinA
        };

        endCircle[i] = {
            end.x + perp1.x * cosA + perp2.x * sinA,
            end.y + perp1.y * cosA + perp2.y * sinA,
            end.z + perp1.z * cosA + perp2.z * sinA
        };

        // World to Screen
        startScreen[i] = startCircle[i].WTS(vm);
        endScreen[i] = endCircle[i].WTS(vm);

        if (startScreen[i].z > 0.01f || endScreen[i].z > 0.01f) {
            anyValid = true;
        }
    }

    if (!anyValid) return;

    // ===== SİLİNDİR ÇİZİMİ =====

    // 1) KENAR ÇİZGİLERİ (silindirin yan yüzeyi)
    for (int i = 0; i < segments; i++) {
        Vector3 s = startScreen[i];
        Vector3 e = endScreen[i];

        if (s.z > 0.01f && e.z > 0.01f) {
            dl->AddLine(ImVec2(s.x, s.y), ImVec2(e.x, e.y), col, 2.0f);
        }
    }

    // 2) BAŞLANGIÇ DAİRESİ
    for (int i = 0; i < segments; i++) {
        int next = (i + 1) % segments;
        Vector3 s1 = startScreen[i];
        Vector3 s2 = startScreen[next];

        if (s1.z > 0.01f && s2.z > 0.01f) {
            dl->AddLine(ImVec2(s1.x, s1.y), ImVec2(s2.x, s2.y), colDark, 1.5f);
        }
    }

    // 3) BİTİŞ DAİRESİ
    for (int i = 0; i < segments; i++) {
        int next = (i + 1) % segments;
        Vector3 e1 = endScreen[i];
        Vector3 e2 = endScreen[next];

        if (e1.z > 0.01f && e2.z > 0.01f) {
            dl->AddLine(ImVec2(e1.x, e1.y), ImVec2(e2.x, e2.y), colDark, 1.5f);
        }
    }
}

// ===== SİLİNDİRLİ + KÜRE KAFALΙ SKELETON ESP =====
inline void DrawCylinderSkeleton(HANDLE driver, uintptr_t pawn, const view_matrix_t& vm, const RGB& color)
{
    // Tüm bone'ları oku
    Vector3 bones[28] = {};
    bool valid = true;

    for (int i = 0; i < 28; ++i) {
        if (!GetBonePosition(driver, pawn, i, bones[i])) {
            valid = false;
            break;
        }
    }

    if (!valid) return;

    // ===== 1) KAFA KÜRE =====
    Vector3 headPos = bones[6];
    DrawBoneSphere(headPos, 8.5f, vm, color, 10); // Daha büyük radius + daha fazla segment

    // ===== 2) VÜCUt SİLİNDİRLERİ (KALINLAŞTIRILMIŞ) =====
    struct BoneCylinder {
        int from, to;
        float radius;
    };

    static const BoneCylinder cylinders[] = {
        // OMURGA (kalın → ince) - 2x büyütme
        {6, 5, 3.6f},   // Head → Neck (eski: 1.8f)
        {5, 4, 5.0f},   // Neck → Spine1 (eski: 2.5f)
        {4, 3, 6.0f},   // Spine1 → Spine2 (eski: 3.0f)
        {3, 0, 7.0f},   // Spine2 → Pelvis (eski: 3.5f)

        // SOL KOL - 2x büyütme
        {4, 8, 4.4f},   // Spine1 → L.Shoulder (eski: 2.2f)
        {8, 9, 3.6f},   // L.Shoulder → L.Elbow (eski: 1.8f)
        {9, 10, 2.8f},  // L.Elbow → L.Hand (eski: 1.4f)

        // SAĞ KOL - 2x büyütme
        {4, 13, 4.4f},  // Spine1 → R.Shoulder (eski: 2.2f)
        {13, 14, 3.6f}, // R.Shoulder → R.Elbow (eski: 1.8f)
        {14, 15, 2.8f}, // R.Elbow → R.Hand (eski: 1.4f)

        // SOL BACAK - 2x büyütme
        {0, 22, 7.0f},  // Pelvis → L.Hip (eski: 3.5f)
        {22, 23, 6.0f}, // L.Hip → L.Knee (eski: 3.0f)
        {23, 24, 4.4f}, // L.Knee → L.Foot (eski: 2.2f)

        // SAĞ BACAK - 2x büyütme
        {0, 25, 7.0f},  // Pelvis → R.Hip (eski: 3.5f)
        {25, 26, 6.0f}, // R.Hip → R.Knee (eski: 3.0f)
        {26, 27, 4.4f}, // R.Knee → R.Foot (eski: 2.2f)
    };

    for (const auto& cyl : cylinders) {
        DrawBoneCylinder(bones[cyl.from], bones[cyl.to], cyl.radius, vm, color, 10);
    }
}

// --- SKELETON BONE HEDEFLEME ---
inline bool GetTargetPoint_Bone(HANDLE driver, uintptr_t pawn, int sel, Vector3& out)
{
    int targetBone = 6; // Varsayılan: Head

    switch (sel) {
    case 0:  targetBone = 6;  break; // Head
    case 1:  targetBone = 5;  break; // Neck
    case 2:  targetBone = 4;  break; // Upper Chest (Spine_1)
    case 3:  targetBone = 3;  break; // Lower Chest (Spine_2)
    case 4:  targetBone = 2;  break; // Stomach (Spine_0)
    case 5:  targetBone = 0;  break; // Pelvis
    default: targetBone = 6;  break; // Fallback
    }

    return GetBonePosition(driver, pawn, targetBone, out);
}

static HRESULT LoadTextureFromFile(
    ID3D11Device* device,
    ID3D11DeviceContext* context,
    const wchar_t* filename,
    ID3D11ShaderResourceView** out_srv,
    int* out_w,
    int* out_h)
{
    if (!device || !context || !filename || !out_srv)
        return E_INVALIDARG;

    *out_srv = nullptr;
    if (out_w) *out_w = 0;
    if (out_h) *out_h = 0;

    // COM başlat
    HRESULT co = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    const bool doUninit = (co == S_OK || co == S_FALSE);

    HRESULT hr = E_FAIL;

    {   // ComPtr, vector vb. bu scope’ta; çıkışta otomatik Release olur
        using Microsoft::WRL::ComPtr;

        ComPtr<IWICImagingFactory>    factory;
        ComPtr<IWICBitmapDecoder>     decoder;
        ComPtr<IWICBitmapFrameDecode> frame;
        ComPtr<IWICFormatConverter>   converter;

        do {
            hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(&factory));
            if (FAILED(hr)) break;

            hr = factory->CreateDecoderFromFilename(filename, nullptr, GENERIC_READ,
                WICDecodeMetadataCacheOnLoad, &decoder);
            if (FAILED(hr)) break;

            hr = decoder->GetFrame(0, &frame);
            if (FAILED(hr)) break;

            hr = factory->CreateFormatConverter(&converter);
            if (FAILED(hr)) break;

            hr = converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppPBGRA,
                WICBitmapDitherTypeNone, nullptr, 0.0,
                WICBitmapPaletteTypeCustom);
            if (FAILED(hr)) break;

            UINT w = 0, h = 0;
            hr = converter->GetSize(&w, &h);
            if (FAILED(hr)) break;

            const UINT stride = w * 4;
            const UINT bufSize = stride * h;

            std::vector<BYTE> pixels(bufSize);
            hr = converter->CopyPixels(nullptr, stride, bufSize, pixels.data());
            if (FAILED(hr)) break;

            D3D11_TEXTURE2D_DESC desc{};
            desc.Width = w;
            desc.Height = h;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // PBGRA ile uyumlu
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

            D3D11_SUBRESOURCE_DATA init{};
            init.pSysMem = pixels.data();
            init.SysMemPitch = stride;

            ComPtr<ID3D11Texture2D> tex;
            hr = device->CreateTexture2D(&desc, &init, tex.GetAddressOf());
            if (FAILED(hr)) break;

            hr = device->CreateShaderResourceView(tex.Get(), nullptr, out_srv);
            if (FAILED(hr)) break;

            if (out_w) *out_w = static_cast<int>(w);
            if (out_h) *out_h = static_cast<int>(h);

        } while (false);
    } // <- Burada ComPtr’lar scope dışına çıkar (Release edilir)

    if (doUninit) CoUninitialize();
    return hr;
}

INT APIENTRY WinMain(HINSTANCE instance, HINSTANCE, PSTR, INT cmd_show) {

    HRESULT hrCo = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hrCo) && hrCo != RPC_E_CHANGED_MODE) {
        wchar_t buf[64];
        swprintf(buf, 64, L"COM init failed: 0x%08X", (unsigned)hrCo);
        MessageBoxW(nullptr, buf, L"COM Error", MB_ICONERROR);
        return 1;
    }
    const bool needCoUninit = SUCCEEDED(hrCo) && hrCo != RPC_E_CHANGED_MODE;

    const wchar_t* targetModuleName = L"cs2.exe";

    int pid = GetProcessIdByName(targetModuleName);
    if (pid == 0) {
        MessageBoxW(NULL, L"Process not found", L"Error", MB_OK | MB_ICONERROR);
        return 1; // failure
    }

    // create driver handle
    const HANDLE driver = CreateFile(L"\\\\.\\km", GENERIC_READ, 0, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    if (driver == INVALID_HANDLE_VALUE) {
        MessageBoxW(NULL, L"SÜRÜCÜYÜ YÜKLESENE YARRAĞIM", L"MAL", MB_OK | MB_ICONERROR);
        return 1; // failure
    }

    if (driver::attach_to_process(driver, pid) == true) {
        if (const std::uintptr_t client = GetModuleBaseAddress(pid, L"client.dll"); client != 0) {
            MessageBoxW(NULL, L"SİKİŞ BAŞLASIN.", L"İNTİKAL EDİLDİ", MB_OK | MB_ICONINFORMATION);

            HWND target = FindMainWindowByPid(pid); // <--- YENİ
            if (!target) {
                MessageBoxW(NULL, L"CS2 ana penceresi bulunamadı.", L"Hata", MB_OK | MB_ICONERROR);
                return 1;
            }

            // create window
            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(WNDCLASSEXW);
            wc.style = CS_HREDRAW | CS_VREDRAW;
            wc.lpfnWndProc = window_procedure;
            wc.hInstance = instance;
            wc.lpszClassName = L"^^";

            RegisterClassExW(&wc);

            // --- OVERLAY: ToolWindow + Layered + Transparent + TopMost
            const HWND overlay = CreateWindowExW(
                WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT,
                wc.lpszClassName,
                L"^^",
                WS_POPUP,
                0, 0, 100, 100, // boyutu şimdi önemli değil; birazdan SyncOverlayToTarget ile ayarlayacağız
                nullptr, nullptr, wc.hInstance, nullptr
            );

            SetLayeredWindowAttributes(overlay, RGB(0, 0, 0), 0, LWA_COLORKEY);

            // --- ÖNEMLİ: İlk hizalama
            SyncOverlayToTarget(overlay, target);
            ShowWindow(overlay, cmd_show);
            UpdateWindow(overlay);

            DXGI_SWAP_CHAIN_DESC sd{};
            sd.BufferDesc.RefreshRate.Numerator = 75U; // fps
            sd.BufferDesc.RefreshRate.Denominator = 1U;  // Numerator değil!
            //sd.BufferDesc.RefreshRate.Numerator = 1U;
            sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            sd.SampleDesc.Count = 1U;
            sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            sd.BufferCount = 2U;
            sd.OutputWindow = overlay;
            sd.Windowed = TRUE;
            sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
            sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

            constexpr D3D_FEATURE_LEVEL levels[2]{
                D3D_FEATURE_LEVEL_11_0,
                D3D_FEATURE_LEVEL_10_0,
            };

            //ID3D11Device* device{ nullptr };
            //ID3D11DeviceContext* device_context{ nullptr };
            IDXGISwapChain* swap_chain{ nullptr };
            ID3D11RenderTargetView* render_target_view{ nullptr };
            D3D_FEATURE_LEVEL level{};

            // create device
            D3D11CreateDeviceAndSwapChain(
                nullptr,
                D3D_DRIVER_TYPE_HARDWARE,
                nullptr,
                0U,
                levels,
                2U,
                D3D11_SDK_VERSION,
                &sd,
                &swap_chain,
                &gDevice,
                &level,
                &gDeviceContext
            );

            ID3D11Texture2D* back_buffer{ nullptr };
            swap_chain->GetBuffer(0U, IID_PPV_ARGS(&back_buffer));

            if (back_buffer) {
                gDevice->CreateRenderTargetView(back_buffer, nullptr, &render_target_view);
                back_buffer->Release();
            }
            else
                return 1;

            // Yenisi: kaynaktan çek
            auto HResultHex = [](HRESULT hr) {
                wchar_t buf[32]; swprintf(buf, 32, L"0x%08X", (unsigned)hr); return std::wstring(buf);
                };

            std::vector<unsigned char> png;

            if (!LoadResourceData(IDR_ENEMY_PNG, L"RCDATA", png)) {
                showPhotoInBox = false;
                MessageBoxW(nullptr, L"IDR_ENEMY_PNG bulunamadı (RCDATA/RT_RCDATA).", L"PNG Load Error", MB_ICONERROR);
            }
            else if (png.size() < 8 || memcmp(png.data(), "\x89PNG\x0D\x0A\x1A\x0A", 8) != 0) {
                showPhotoInBox = false;
                MessageBoxW(nullptr, L"PNG verisi bozuk (signature).", L"PNG Load Error", MB_ICONERROR);
            }
            else if (!gDevice) {
                showPhotoInBox = false;
                MessageBoxW(nullptr, L"D3D11 device yok (CreateDevice başarısız?).", L"D3D Error", MB_ICONERROR);
            }
            else {
                HRESULT texHr = LoadTextureFromMemory(gDevice, gDeviceContext,
                    png.data(), png.size(), &gPhotoSRV, &gPhotoW, &gPhotoH);
                if (FAILED(texHr)) {
                    showPhotoInBox = false;
                    std::wstring msg = L"LoadTextureFromMemory() Hata: " + HResultHex(texHr);
                    MessageBoxW(nullptr, msg.c_str(), L"WIC/D3D Decode Error", MB_ICONERROR);
                }
            }


            // === GIF YÜKLEME ===
            std::vector<unsigned char> gif;
            if (LoadResourceData(IDR_HITLER, L"RCDATA", gif))
            {
                LoadGifFromMemory(gDevice, gDeviceContext, gif.data(), gif.size(), gGifFrames);
            }
            // === GIF #2 YÜKLE ===
            std::vector<unsigned char> gif2;
            if (LoadResourceData(IDR_FEMBOY, L"RCDATA", gif2))
            {
                LoadGifFromMemory(gDevice, gDeviceContext, gif2.data(), gif2.size(), gGifFrames2);
            }
            // === GIF #3 YÜKLE ===
            std::vector<unsigned char> gif3;
            if (LoadResourceData(IDR_HUNNYPAINT, L"RCDATA", gif3))
            {
                LoadGifFromMemory(gDevice, gDeviceContext, gif3.data(), gif3.size(), gGifFrames3);
            }
            // === GIF #4 YÜKLE ===
            std::vector<unsigned char> gif4;
            if (LoadResourceData(IDR_ROLE, L"RCDATA", gif4))
            {
                LoadGifFromMemory(gDevice, gDeviceContext, gif4.data(), gif4.size(), gGifFrames4);
            }
            // === PROGRAM AÇILIR AÇILMAZ PNG YÜKLE ===
            {
                std::vector<unsigned char> pngData;

                if (!LoadResourceData(IDR_ENEMY_PNG, L"RCDATA", pngData))
                {
                    MessageBoxA(NULL, "IDR_ENEMY_PNG yüklenemedi!", "PNG ERROR", MB_OK | MB_ICONERROR);
                }
                else
                {
                    HRESULT hr = LoadTextureFromMemory(
                        gDevice,
                        gDeviceContext,
                        pngData.data(),
                        pngData.size(),
                        &gPhotoSRV,
                        &gPhotoW,
                        &gPhotoH
                    );

                    if (FAILED(hr))
                    {
                        MessageBoxA(NULL, "LoadTextureFromMemory PNG HATASI!", "PNG ERROR", MB_OK | MB_ICONERROR);
                    }
                }
            }
            // === EXE içindeki tüm RCDATA resimlerini otomatik tara ===
            {
                HMODULE hMod = GetModuleHandleW(NULL);

                for (int id = 1; id <= 5000; id++)
                {
                    HRSRC hRes = FindResourceW(hMod, MAKEINTRESOURCE(id), RT_RCDATA);
                    if (!hRes) continue;

                    DWORD size = SizeofResource(hMod, hRes);
                    if (size < 16) continue;

                    char buf[32];
                    sprintf_s(buf, "Image_%d", id);

                    gEmbeddedImages.push_back({ id, buf });
                }

                if (gEmbeddedImages.empty())
                {
                    MessageBoxA(NULL, "Gömülü resource bulunamadı!", "UYARI", MB_OK);
                }
            }

            ShowWindow(overlay, cmd_show);
            UpdateWindow(overlay);

            // ============================================
            // RAGNAREK-Style ImGui Initialization
            // ============================================

            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.IniFilename = nullptr; // .ini dosyası oluşturmasın
            io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

            // Font Configuration
            ImFontConfig font_cfg;
            font_cfg.OversampleH = 2;
            font_cfg.OversampleV = 2;
            font_cfg.PixelSnapH = true;
            font_cfg.FontDataOwnedByAtlas = false;

            // Main Font (16px)
            ImFont* main_font = io.Fonts->AddFontFromMemoryTTF(
                (void*)rawData,
                sizeof(rawData),
                16.0f,
                &font_cfg
            );

            // Icon Font (FontAwesome) - MERGE MODE
            std::vector<unsigned char> faData;
            if (LoadResourceData(IDR_FA_SOLID_TTF, L"RCDATA", faData))
            {
                void* fa_buf = IM_ALLOC(faData.size());
                memcpy(fa_buf, faData.data(), faData.size());

                ImFontConfig icon_cfg = font_cfg;
                icon_cfg.MergeMode = true;
                icon_cfg.GlyphMinAdvanceX = 16.0f;
                icon_cfg.GlyphOffset = ImVec2(0, 2);

                static const ImWchar icon_ranges[] = { 0xF000, 0xF8FF, 0 };

                io.Fonts->AddFontFromMemoryTTF(
                    fa_buf,
                    (int)faData.size(),
                    16.0f,
                    &icon_cfg,
                    icon_ranges
                );
            }

            io.Fonts->Build();

            ImGui_ImplWin32_Init(overlay);
            ImGui_ImplDX11_Init(gDevice, gDeviceContext);

            // ============================================
            // RAGNAREK Style Theme
            // ============================================

            ImGuiStyle& style = ImGui::GetStyle();

            // --- Rounding ---
            style.WindowRounding = 8.0f;
            style.ChildRounding = 4.0f;
            style.FrameRounding = 4.0f;
            style.PopupRounding = 4.0f;
            style.ScrollbarRounding = 4.0f;
            style.GrabRounding = 4.0f;
            style.TabRounding = 4.0f;

            // --- Padding & Spacing ---
            style.WindowPadding = ImVec2(10, 10);
            style.FramePadding = ImVec2(8, 4);
            style.ItemSpacing = ImVec2(8, 6);
            style.ItemInnerSpacing = ImVec2(6, 4);
            style.IndentSpacing = 20.0f;
            style.ScrollbarSize = 12.0f;
            style.GrabMinSize = 8.0f;

            // --- Borders ---
            style.WindowBorderSize = 1.0f;
            style.ChildBorderSize = 1.0f;
            style.PopupBorderSize = 1.0f;
            style.FrameBorderSize = 0.0f;

            // --- Colors (RAGNAREK Theme) ---
            ImVec4* colors = style.Colors;

            colors[ImGuiCol_WindowBg] = UI::Colors::Background;
            colors[ImGuiCol_ChildBg] = UI::Colors::BackgroundDark;
            colors[ImGuiCol_PopupBg] = UI::Colors::Background;
            colors[ImGuiCol_Border] = ImVec4(0.3f, 0.3f, 0.35f, 0.5f);

            colors[ImGuiCol_FrameBg] = UI::Colors::BackgroundLight;
            colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.20f, 0.25f, 1.0f);
            colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.25f, 0.30f, 1.0f);

            colors[ImGuiCol_TitleBg] = UI::Colors::BackgroundDark;
            colors[ImGuiCol_TitleBgActive] = UI::Colors::Background;
            colors[ImGuiCol_TitleBgCollapsed] = UI::Colors::BackgroundDark;

            colors[ImGuiCol_ScrollbarBg] = UI::Colors::BackgroundDark;
            colors[ImGuiCol_ScrollbarGrab] = UI::Colors::BackgroundLight;
            colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.25f, 0.25f, 0.30f, 1.0f);
            colors[ImGuiCol_ScrollbarGrabActive] = UI::Colors::Primary;

            colors[ImGuiCol_CheckMark] = UI::Colors::Primary;
            colors[ImGuiCol_SliderGrab] = UI::Colors::Primary;
            colors[ImGuiCol_SliderGrabActive] = UI::Colors::PrimaryActive;

            colors[ImGuiCol_Button] = UI::Colors::BackgroundLight;
            colors[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.20f, 0.25f, 1.0f);
            colors[ImGuiCol_ButtonActive] = UI::Colors::Primary;

            colors[ImGuiCol_Header] = UI::Colors::BackgroundLight;
            colors[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.20f, 0.25f, 1.0f);
            colors[ImGuiCol_HeaderActive] = UI::Colors::Primary;

            colors[ImGuiCol_Separator] = ImVec4(0.3f, 0.3f, 0.35f, 0.5f);
            colors[ImGuiCol_SeparatorHovered] = UI::Colors::Primary;
            colors[ImGuiCol_SeparatorActive] = UI::Colors::PrimaryActive;

            colors[ImGuiCol_Tab] = UI::Colors::BackgroundLight;
            colors[ImGuiCol_TabHovered] = UI::Colors::PrimaryHovered;
            colors[ImGuiCol_TabActive] = UI::Colors::Primary;
            colors[ImGuiCol_TabUnfocused] = UI::Colors::BackgroundDark;
            colors[ImGuiCol_TabUnfocusedActive] = UI::Colors::BackgroundLight;

            colors[ImGuiCol_Text] = UI::Colors::Text;
            colors[ImGuiCol_TextDisabled] = UI::Colors::TextDisabled;
            colors[ImGuiCol_TextSelectedBg] = ImVec4(
                UI::Colors::Primary.x,
                UI::Colors::Primary.y,
                UI::Colors::Primary.z,
                0.35f
            );

            // ============================================
            // End of ImGui Setup
            // ============================================

            ImFontConfig font;
            font.FontDataOwnedByAtlas = false;

            //io.Fonts->AddFontFromMemoryTTF((void*)rawData, sizeof(rawData), 17.0f, &font);

            bool running = true;

            while (running)
            {
                MSG msg;
                while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);

                    if (msg.message == WM_QUIT)
                    {
                        running = false;
                    }
                }
                if (!running)
                    break;

                SyncOverlayToTarget(overlay, target); // her frame hizala

                bool focusedToGame = (GetForegroundWindow() == target);
                bool allowFrame = focusedToGame || showImGui; // menü açıkken odak şartı yok

                if (!allowFrame) {
                    Sleep(5);
                    continue;
                }

                // offsets
                uintptr_t localPlayer = driver::read_memory<uintptr_t>(driver, client + dwLocalPlayerPawn);
                Vector3 Localorgin = driver::read_memory<Vector3>(driver, localPlayer + m_vOldOrigin);
                view_matrix_t view_matrix = driver::read_memory<view_matrix_t>(driver, client + dwViewMatrix);
                uintptr_t entity_list = driver::read_memory<uintptr_t>(driver, client + dwEntityList);
                int localTeam = driver::read_memory<int>(driver, localPlayer + m_iTeamNum);

                ImGui_ImplDX11_NewFrame();
                ImGui_ImplWin32_NewFrame();
                ImGui::NewFrame();
                //ImGui::GetIO().MouseDrawCursor = showImGui;

                const bool home_pressed = GetAsyncKeyState(VK_INSERT);
                const bool del_pressed = GetAsyncKeyState(VK_DELETE);
                if (del_pressed)
                {
                    running = false; // ana döngüden çık
                }
                if (home_pressed)
                {
                    showImGui = !showImGui;

                    LONG_PTR exStyle = GetWindowLongPtr(overlay, GWL_EXSTYLE);
                    if (showImGui) {
                        exStyle &= ~WS_EX_TRANSPARENT; // tıklanabilir yap
                    }
                    else {
                        exStyle |= WS_EX_TRANSPARENT;  // oyuna tıklamayı geçir
                    }
                    SetWindowLongPtr(overlay, GWL_EXSTYLE, exStyle);

                    // stil değişikliğini uygulat
                    SetWindowPos(overlay, HWND_TOPMOST, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

                    if (showImGui) {
                        SetForegroundWindow(overlay);
                        SetFocus(overlay);
                    }
                    else {
                        SetForegroundWindow(target);
                    }

                    Sleep(200); // ufak debounce
                }

                // RGB enemy
                static int r = 255;
                static int g = 0;
                static int b = 255;

                // RGB team
                static int t_r = 0;
                static int t_g = 160;
                static int t_b = 255;

                // Çin Şapkası rengi (varsayılan: sarı)
                static int ch_r = 255;
                static int ch_g = 255;
                static int ch_b = 0;

                if (showImGui)
                {
                    // Eski: RenderSkeetLikeMenu(r, g, b, t_r, t_g, t_b, ch_r, ch_g, ch_b);
                    // Yeni:
                    RenderRAGNAREKMenu(r, g, b, t_r, t_g, t_b, ch_r, ch_g, ch_b);
                }

                RGB enemy = { r, g, b };
                RGB team = { t_r, t_g, t_b };

                // Aimbot toplayıcılar
                bool  hasTarget = false;
                float bestDist = FLT_MAX;
                float bestDx = 0.f, bestDy = 0.f;
                uintptr_t bestPawn = 0;
                int bestIndex = -1;

                RECT orc{};
                GetClientRect(overlay, &orc);
                int curW = orc.right - orc.left;
                int curH = orc.bottom - orc.top;

                const float cx = screenWidth * 0.5f;
                const float cy = screenHeight * 0.5f;

                // switch tuşuna basıldıysa kilidi bırak
                if (GetAsyncKeyState(switchKeyVK) & 1) {
                    gLockedPawn = 0;
                    gLockedIndex = -1;
                }

                for (int playerIndex = 1; playerIndex < 32; ++playerIndex) {
                    uintptr_t listentry = driver::read_memory<uintptr_t>(driver, entity_list + (8 * (playerIndex & 0x7FFF) >> 9) + 16);

                    if (!listentry)
                        continue;

                    uintptr_t player = driver::read_memory<uintptr_t>(driver, listentry + 112 * (playerIndex & 0x1FF));

                    if (!player)
                        continue;

                    int playerTeam = driver::read_memory<int>(driver, player + m_iTeamNum);

                    if (!showTeam) {
                        if (playerTeam == localTeam)
                            continue;
                    }

                    uint32_t playerPawn = driver::read_memory<uint32_t>(driver, player + m_hPlayerPawn);

                    uintptr_t listentry2 = driver::read_memory<uintptr_t>(driver, entity_list + 0x8 * ((playerPawn & 0x7FFF) >> 9) + 16);

                    if (!listentry2)
                        continue;

                    uintptr_t pCSPlayerPawn = driver::read_memory<uintptr_t>(driver, listentry2 + 112 * (playerPawn & 0x1FF));

                    if (!pCSPlayerPawn)
                        continue;

                    int health = driver::read_memory<int>(driver, pCSPlayerPawn + m_iHealth);

                    if (health <= 0 || health > 100)
                        continue;

                    if (pCSPlayerPawn == localPlayer)
                        continue;

                    // ===== SİLAH VE PARA OKUMA =====
                    std::string weaponName = "";
                    int playerMoney = 0;

                    if (showWeaponESP)
                    {
                        // 1️⃣ ClippingWeapon pointer'ını al
                        uintptr_t clippingWeapon = driver::read_memory<uintptr_t>(driver, pCSPlayerPawn + m_pClippingWeapon);

                        if (clippingWeapon)
                        {
                            // 2️⃣ İki seviye pointer takibi
                            uintptr_t firstLevel = driver::read_memory<uintptr_t>(driver, clippingWeapon + 0x10);

                            if (firstLevel)
                            {
                                uintptr_t weaponData = driver::read_memory<uintptr_t>(driver, firstLevel + 0x20);

                                if (weaponData)
                                {
                                    // 3️⃣ Silah adını oku (string)
                                    char weaponBuffer[64] = { 0 };

                                    driver::Request r;
                                    r.target = reinterpret_cast<PVOID>(weaponData);
                                    r.buffer = weaponBuffer;
                                    r.size = sizeof(weaponBuffer);
                                    r.return_size = 0;

                                    DeviceIoControl(driver, driver::codes::read, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);

                                    weaponName = std::string(weaponBuffer);

                                    // 4️⃣ "weapon_" prefix'ini kaldır
                                    if (weaponName.compare(0, 7, "weapon_") == 0) {
                                        weaponName = weaponName.substr(7);
                                    }

                                    // 5️⃣ Geçersiz kontrol
                                    if (weaponName.empty() || weaponName.length() > 30) {
                                        weaponName = "";
                                    }
                                }
                            }
                        }
                    }

                    if (showMoneyESP)
                    {
                        // 1️⃣ InGameMoneyServices pointer'ını al
                        uintptr_t moneyServices = driver::read_memory<uintptr_t>(driver, player + m_pInGameMoneyServices);

                        if (moneyServices)
                        {
                            // 2️⃣ Para miktarını oku
                            playerMoney = driver::read_memory<int>(driver, moneyServices + m_iAccount);

                            // 3️⃣ Geçerlilik kontrolü
                            if (playerMoney < 0 || playerMoney > 65535) {
                                playerMoney = 0;
                            }
                        }
                    }

                    Vector3 orgian = driver::read_memory<Vector3>(driver, pCSPlayerPawn + m_vOldOrigin);
                    Vector3 head = { orgian.x, orgian.y, orgian.z + 75.f };

                    Vector3 screenPos = orgian.WTS(view_matrix);
                    Vector3 screenHead = head.WTS(view_matrix);

                    float height = screenPos.y - screenHead.y;
                    float width = height / 2.4f;

                    int rectBottomX = screenHead.x;
                    int rectBottomY = screenHead.y + height;

                    int bottomCenterX = screenWidth / 2;
                    int bottomCenterY = screenHeight;

                    if (screenHead.x - width / 2 >= 0 &&
                        screenHead.x + width / 2 <= screenWidth &&
                        screenHead.y >= 0 &&
                        screenHead.y + height <= screenHeight &&
                        screenHead.z > 0 &&
                        enableTrue == true) {
                        if (showNameESP) {
                            std::string player_name = read_string(driver, player + m_iszPlayerName, 32);

                            if (!player_name.empty() && isascii(player_name[0])) {
                                ImDrawList* drawList = ImGui::GetBackgroundDrawList();

                                float text_width = ImGui::CalcTextSize(player_name.c_str()).x;
                                ImVec2 textPos = ImVec2(screenHead.x - (text_width / 2), screenHead.y - 15.0f);

                                drawList->AddText(textPos, ImColor(255, 255, 255), player_name.c_str());
                            }
                        }
                        // FOV çemberini her frame 1 kez çiz
                        if (drawAimFov) {
                            ImGui::GetBackgroundDrawList()->AddCircle(
                                ImVec2(cx, cy),
                                aimbotFovPx,
                                IM_COL32(255, 255, 255, 40),
                                64,
                                1.0f
                            );
                        }
                        if (showHealthESP && health > 0 && health <= 100) {
                            std::string healthText = std::to_string(health) + " HP";

                            ImDrawList* drawList = ImGui::GetBackgroundDrawList();
                            float text_width = ImGui::CalcTextSize(healthText.c_str()).x;
                            ImVec2 textPos = ImVec2(screenHead.x - (text_width / 2), screenHead.y + 12); // ismin hemen altı

                            float r = 255.0f - (health * 2.55f);
                            float g = health * 2.55f;
                            ImColor healthColor = ImColor(ImVec4(r / 255.0f, g / 255.0f, 0.0f, 1.0f));

                            drawList->AddText(textPos, healthColor, healthText.c_str());
                        }
                        if (showHealthBarESP && health > 0 && health <= 100)
                        {
                            ImDrawList* drawList = ImGui::GetBackgroundDrawList();

                            // ESP kutusu boyutları
                            float boxHeight = height;
                            float boxWidth = width;

                            // Sağ tarafta çizilsin (istersen sol yapabilirsin)
                            float barX = screenHead.x + (boxWidth / 2) + 5.0f; // kutunun hemen sağı
                            float barY = screenHead.y;
                            float barWidth = 4.0f;

                            // Maksimum yükseklik (tam HP)
                            float fullHeight = boxHeight;

                            // HP oranı (0.0 - 1.0)
                            float hpPercent = std::clamp(health / 100.0f, 0.0f, 1.0f);

                            // Aktif (doluluk) yüksekliği
                            float hpHeight = fullHeight * hpPercent;

                            // Doluluk alt noktadan yukarıya çizilecek
                            float filledY = barY + (fullHeight - hpHeight);

                            // Renk geçişi (yeşil → kırmızı)
                            float r = 255.0f - (health * 2.55f);
                            float g = (health * 2.55f);
                            ImU32 hpColor = IM_COL32((int)r, (int)g, 0, 255);

                            // Arka plan (gri)
                            drawList->AddRectFilled(
                                ImVec2(barX, barY),
                                ImVec2(barX + barWidth, barY + fullHeight),
                                IM_COL32(40, 40, 40, 180)
                            );

                            // HP doluluğu (renkli)
                            drawList->AddRectFilled(
                                ImVec2(barX, filledY),
                                ImVec2(barX + barWidth, barY + fullHeight),
                                hpColor
                            );

                            // Kenarlık (isteğe bağlı)
                            drawList->AddRect(
                                ImVec2(barX, barY),
                                ImVec2(barX + barWidth, barY + fullHeight),
                                IM_COL32(0, 0, 0, 255)
                            );
                        }
                        if (playerTeam == localTeam) {
                            if (drawLines) {
                                Render::DrawLine(bottomCenterX, bottomCenterY, rectBottomX, rectBottomY, team, 1.5f);
                            }
                            if (showBoxESP) {
                                Render::DrawRect(
                                    screenHead.x - width / 2,
                                    screenHead.y,
                                    width,
                                    height,
                                    team,
                                    1.5
                                );
                            }
                        }
                        else {
                            if (drawLines) {
                                Render::DrawLine(bottomCenterX, bottomCenterY, rectBottomX, rectBottomY, enemy, 1.5f);
                            }
                            if (showBoxESP) {
                                Render::DrawRect(
                                    screenHead.x - width / 2,
                                    screenHead.y,
                                    width,
                                    height,
                                    enemy,
                                    1.5
                                );
                            }
                        }
                        if (showChinaHat) {
                            // Skeleton head bone pozisyonunu al (bone index 6)
                            Vector3 headBone;
                            if (GetBonePosition(driver, pCSPlayerPawn, 6, headBone)) {
                                ImDrawList* drawList = ImGui::GetBackgroundDrawList();
                                ImU32 hatColor = IM_COL32(ch_r, ch_g, ch_b, 255);

                                float h = chinaHatHeight;
                                //float h = 15.0f;  // Şapka yüksekliği
                                float radius = chinaHatRadius;
                                int segments = 32;  // Daha yuvarlak çember için artırıldı

                                // Şapkanın tepe noktası (apex)
                                Vector3 apex3D = { headBone.x, headBone.y, headBone.z + h + chinaHatOffset };
                                Vector3 apex2D = apex3D.WTS(view_matrix);

                                // Şapkanın tabanındaki noktaları sakla (çember için)
                                std::vector<ImVec2> basePoints;
                                basePoints.reserve(segments);

                                if (apex2D.z > 0.f) {
                                    // 1) Şapka çizgileri (tepe -> taban)
                                    for (int i = 0; i < segments; i++) {
                                        float angle = (2.0f * 3.14159265f / segments) * i;
                                        float x = headBone.x + cosf(angle) * radius;
                                        float y = headBone.y + sinf(angle) * radius;
                                        float z = headBone.z + chinaHatOffset;  // Taban seviyesi

                                        Vector3 base3D = { x, y, z };
                                        Vector3 base2D = base3D.WTS(view_matrix);

                                        if (base2D.z > 0.f) {
                                            // Tepeden tabana ışınsal çizgi
                                            drawList->AddLine(
                                                ImVec2(apex2D.x, apex2D.y),
                                                ImVec2(base2D.x, base2D.y),
                                                hatColor,
                                                1.5f
                                            );

                                            // Taban noktasını kaydet
                                            basePoints.push_back(ImVec2(base2D.x, base2D.y));
                                        }
                                    }
                                    // 2) Şapka tabanında tam çap çember çiz
                                    if (basePoints.size() >= 3) {
                                        for (size_t i = 0; i < basePoints.size(); i++) {
                                            size_t next = (i + 1) % basePoints.size();
                                            drawList->AddLine(
                                                basePoints[i],
                                                basePoints[next],
                                                hatColor,
                                                2.0f  // Çember daha belirgin olsun
                                            );
                                        }
                                    }
                                }
                            }
                        }
                        // ===== SİLAH YAZISI =====
                        if (showWeaponESP && !weaponName.empty())
                        {
                            // Silah adını düzenle (büyük harf yap)
                            std::transform(weaponName.begin(), weaponName.end(), weaponName.begin(), ::toupper);

                            Render::DrawText(
                                screenHead.x + (width / 2) + 8,
                                screenHead.y + 45, // Flash/Defuse'un altı
                                RGB{ 150, 200, 255 }, // Açık mavi
                                weaponName.c_str()
                            );
                        }

                        // ===== PARA YAZISI =====
                        if (showMoneyESP && playerMoney > 0)
                        {
                            char moneyText[32];
                            sprintf_s(moneyText, "$%d", playerMoney);

                            Render::DrawText(
                                screenHead.x + (width / 2) + 8,
                                screenHead.y + (showWeaponESP ? 60 : 45), // Silah varsa daha aşağıda
                                RGB{ 100, 255, 100 }, // Yeşil (para rengi)
                                moneyText
                            );
                        }
                        // ===== TAM SKELETON ESP =====
                        if (showSkeletonESP) {
                            RGB skeletonColor = (playerTeam == localTeam) ? team : enemy;
                            DrawSkeleton(driver, pCSPlayerPawn, view_matrix, skeletonColor);
                        }
                        // ===== YENİ: 3D SİLİNDİR ESP =====
                        if (showCylinderESP) {
                            RGB cylinderColor = (playerTeam == localTeam) ? team : enemy;
                            DrawCylinderSkeleton(driver, pCSPlayerPawn, view_matrix, cylinderColor);
                        }

                        // ===== LOCAL PLAYER CONTROLLER INDEX'İNİ BUL (DÜZELTİLMİŞ) =====
                        static int localControllerIndex = -1;
                        static int frameCounter = 0;
                        frameCounter++;

                        if (localPlayer && (localControllerIndex == -1 || frameCounter >= 120))
                        {
                            frameCounter = 0;

                            for (int i = 0; i < 64; ++i)  // ← 1 yerine 0'dan başla
                            {
                                uintptr_t listentry = driver::read_memory<uintptr_t>(driver, entity_list + (8 * (i & 0x7FFF) >> 9) + 16);
                                if (!listentry) continue;

                                uintptr_t controller = driver::read_memory<uintptr_t>(driver, listentry + 112 * (i & 0x1FF));
                                if (!controller) continue;

                                uint32_t pawnHandle = driver::read_memory<uint32_t>(driver, controller + m_hPlayerPawn);
                                if (pawnHandle == 0 || pawnHandle == 0xFFFFFFFF) continue;

                                uintptr_t listentry2 = driver::read_memory<uintptr_t>(driver, entity_list + 0x8 * ((pawnHandle & 0x7FFF) >> 9) + 16);
                                if (!listentry2) continue;

                                uintptr_t pawn = driver::read_memory<uintptr_t>(driver, listentry2 + 112 * (pawnHandle & 0x1FF));

                                if (pawn == localPlayer)
                                {
                                    localControllerIndex = i;

                                    // Console'a yazdır (debug için)
                                    char debugMsg[64];
                                    sprintf_s(debugMsg, "Local Controller Index Found: %d\n", i);
                                    OutputDebugStringA(debugMsg);

                                    break;
                                }
                            }
                        }
                        // ===== VİSİBİLİTY CHECK (YENİ EKLEME) =====
                        if (aimbotVisibilityCheck && localControllerIndex != -1)
                        {
                            // 1️⃣ Target oyuncunun spotted mask'ını oku
                            uint64_t targetSpottedMask = driver::read_memory<uint64_t>(driver, pCSPlayerPawn + m_bSpottedByMask);

                            // 2️⃣ Local player bu oyuncuyu görüyor mu?
                            bool isVisibleByLocal = (targetSpottedMask & (1ULL << localControllerIndex)) != 0;

                            // 3️⃣ VEYA local player'ın mask'ında target var mı? (karşılıklı kontrol)
                            uint64_t localSpottedMask = driver::read_memory<uint64_t>(driver, localPlayer + m_bSpottedByMask);
                            bool localSeesTarget = (localSpottedMask & (1ULL << playerIndex)) != 0;

                            // 4️⃣ İkisinden biri true ise görünüyor demektir
                            if (!isVisibleByLocal && !localSeesTarget)
                            {
                                continue; // DUVARIN ARKASINDA - ATLA!
                            }
                        }
                        // ===== AİMBOT HEDEF SEÇME (DEBUG VERSIYONU) =====
                        Vector3 aimWorld;
                        if (GetTargetPoint_Bone(driver, pCSPlayerPawn, aimBoneSelect, aimWorld))
                        {
                            Vector3 aim2D = aimWorld.WTS(view_matrix);
                            if (aim2D.z > 0.f)
                            {
                                float dx = aim2D.x - cx;
                                float dy = aim2D.y - cy;
                                float dist = std::sqrt(dx * dx + dy * dy);

                                bool okTarget = (showTeam || (playerTeam != localTeam));

                                if (okTarget)
                                {
                                    bool isVisible = true; // Varsayılan: görünür

                                    // ===== VİSİBİLİTY CHECK (DEBUG) =====
                                    if (aimbotVisibilityCheck && aimbotEnabled && localControllerIndex != -1)
                                    {
                                        uint64_t targetSpottedMask = driver::read_memory<uint64_t>(driver, pCSPlayerPawn + m_bSpottedByMask);
                                        uint64_t localSpottedMask = driver::read_memory<uint64_t>(driver, localPlayer + m_bSpottedByMask);

                                        bool isVisibleByLocal = (targetSpottedMask & (1ULL << localControllerIndex)) != 0;
                                        bool localSeesTarget = (localSpottedMask & (1ULL << playerIndex)) != 0;

                                        isVisible = (isVisibleByLocal || localSeesTarget);

                                        // ===== DEBUG - SADECE FOV İÇİNDEKİ İLK HEDEF İÇİN =====
                                        static bool debugPrinted = false;
                                        if (!debugPrinted && dist <= aimbotFovPx)
                                        {
                                            char msg[512];
                                            sprintf_s(msg,
                                                "[VISIBILITY DEBUG]\n"
                                                "  Player Index: %d\n"
                                                "  Local Controller Index: %d\n"
                                                "  Target SpottedMask: 0x%016llX\n"
                                                "  Local SpottedMask: 0x%016llX\n"
                                                "  Bit (1 << %d): 0x%016llX\n"
                                                "  isVisibleByLocal: %s\n"
                                                "  localSeesTarget: %s\n"
                                                "  FINAL isVisible: %s\n"
                                                "  Distance: %.1f (FOV: %.1f)\n",
                                                playerIndex,
                                                localControllerIndex,
                                                targetSpottedMask,
                                                localSpottedMask,
                                                localControllerIndex,
                                                (1ULL << localControllerIndex),
                                                isVisibleByLocal ? "TRUE" : "FALSE",
                                                localSeesTarget ? "TRUE" : "FALSE",
                                                isVisible ? "TRUE" : "FALSE",
                                                dist,
                                                aimbotFovPx
                                            );
                                            OutputDebugStringA(msg);
                                            debugPrinted = true; // 1 kez yazdır
                                        }
                                    }

                                    // Sadece görünür hedefleri işle
                                    if (isVisible)
                                    {
                                        if (gLockedPawn == 0)
                                        {
                                            if (dist <= aimbotFovPx && dist < bestDist)
                                            {
                                                bestDist = dist;
                                                bestDx = dx;
                                                bestDy = dy;
                                                bestPawn = pCSPlayerPawn;
                                                bestIndex = playerIndex;
                                                hasTarget = true;
                                            }
                                        }
                                        else
                                        {
                                            if (pCSPlayerPawn == gLockedPawn)
                                            {
                                                bestDist = dist;
                                                bestDx = dx;
                                                bestDy = dy;
                                                bestPawn = pCSPlayerPawn;
                                                bestIndex = playerIndex;
                                                hasTarget = true;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        // --- Aimbot ateşleme (loop bitti, 1 kez çalışır) ---
                        bool triggerDown = (GetAsyncKeyState(GetAimHotkeyVK()) & 0x8000) != 0;
                        bool shouldRun = (!showImGui && aimbotEnabled) && (triggerDown || (lockPersistsAfterKey && gLockedPawn));

                        // switch tuşuna basıldıysa kilidi bırak
                        if (GetAsyncKeyState(switchKeyVK) & 1) {
                            gLockedPawn = 0;
                            gLockedIndex = -1;
                            emaDx = emaDy = 0.f;
                        }

                        if (shouldRun) {
                            // kilit yoksa ve aday varsa al
                            if (gLockedPawn == 0 && hasTarget) {
                                gLockedPawn = bestPawn;
                                gLockedIndex = bestIndex;
                            }
                            // kilitliyse: hedef yaşıyor mu?
                            if (gLockedPawn) {
                                int hp = driver::read_memory<int>(driver, gLockedPawn + m_iHealth);
                                if (hp <= 0 || hp > 100) {
                                    gLockedPawn = 0;
                                    gLockedIndex = -1;
                                    emaDx = emaDy = 0.f;
                                }
                            }
                            // hareket
                            if (gLockedPawn && hasTarget) {
                                float alpha = std::clamp(0.18f * (10.f / aimbotSmooth), 0.05f, 0.35f);
                                emaDx += (bestDx - emaDx) * alpha;
                                emaDy += (bestDy - emaDy) * alpha;

                                if (std::fabs(emaDx) < aimDeadZone) emaDx = 0.f;
                                if (std::fabs(emaDy) < aimDeadZone) emaDy = 0.f;

                                float maxStep = (std::max)(2.0f, aimbotFovPx * 0.15f);
                                emaDx = std::clamp(emaDx, -maxStep, maxStep);
                                emaDy = std::clamp(emaDy, -maxStep, maxStep);

                                float div = (std::max)(aimbotSmooth, 1.0f);
                                AimMoveDelta(emaDx / div, emaDy / div);
                            }
                        }

                        // Tuşu bırakınca kilidi düşürmek istiyorsan (lockPersistsAfterKey=false)
                        if (!triggerDown && !lockPersistsAfterKey) {
                            gLockedPawn = 0;
                            gLockedIndex = -1;
                            emaDx = emaDy = 0.f;
                        }
                        int flashAlpha = driver::read_memory<int>(driver, pCSPlayerPawn + m_flFlashOverlayAlpha); // FLASH ALPHA
                        if (showFlashESP && flashAlpha > 100) {
                            Render::DrawText(
                                screenHead.x + (width / 2) + 5,
                                screenHead.y + 60,
                                RGB{ 255, 255, 255 },
                                "FLASHED"
                            );
                        }
                        // ===== DEFUSE ESP =====
                        if (showDefuseESP) {
                            bool isDefusing = driver::read_memory<bool>(driver, pCSPlayerPawn + m_bIsDefusing);

                            if (isDefusing) {
                                Render::DrawText(
                                    screenHead.x + (width / 2) + 5,
                                    screenHead.y + 75,  // Flash'ın 15px altı
                                    RGB{ 255, 50, 50 }, // Kırmızı renk
                                    "DEFUSING"
                                );
                            }
                        }
                        // ===== C4 PLANTED ESP (DÜZELTİLMİŞ) =====
                        if (showC4ESP && enableTrue)
                        {
                            // 1️⃣ Global planted C4 pointer'ını oku
                            uintptr_t plantedC4Ptr = driver::read_memory<uintptr_t>(driver, client + dwPlantedC4);

                            if (plantedC4Ptr)
                            {
                                // 2️⃣ Planted C4 entity'sini oku
                                uintptr_t plantedC4 = driver::read_memory<uintptr_t>(driver, plantedC4Ptr);

                                if (plantedC4)
                                {
                                    // 3️⃣ GameSceneNode'u al
                                    uintptr_t c4Node = driver::read_memory<uintptr_t>(driver, plantedC4 + m_pGameSceneNode);

                                    if (c4Node)
                                    {
                                        // 4️⃣ Absolute origin'i al (m_vecAbsOrigin kullan, m_vOldOrigin DEĞİL!)
                                        Vector3 c4Origin = driver::read_memory<Vector3>(driver, c4Node + m_vecAbsOrigin);
                                        Vector3 c4ScreenPos = c4Origin.WTS(view_matrix);

                                        if (c4ScreenPos.z >= 0.01f)
                                        {
                                            // 5️⃣ Mesafe hesapla
                                            float c4Distance = calculate_distance(Localorgin, c4Origin);
                                            float c4RoundedDistance = std::round(c4Distance / 500.f);

                                            // 6️⃣ Dinamik kutu boyutu
                                            float height = std::max(15.0f, 35.0f - c4RoundedDistance * 2);
                                            float width = height * 1.4f;

                                            ImDrawList* dl = ImGui::GetBackgroundDrawList();

                                            // ===== KIRMIZI DOLU KUTU =====
                                            dl->AddRectFilled(
                                                ImVec2(c4ScreenPos.x - (width / 2), c4ScreenPos.y - (height / 2)),
                                                ImVec2(c4ScreenPos.x + (width / 2), c4ScreenPos.y + (height / 2)),
                                                IM_COL32(200, 0, 0, 180), // Koyu kırmızı, yarı saydam
                                                4.0f // rounded corners
                                            );

                                            // ===== SARI KENАРLIK =====
                                            dl->AddRect(
                                                ImVec2(c4ScreenPos.x - (width / 2), c4ScreenPos.y - (height / 2)),
                                                ImVec2(c4ScreenPos.x + (width / 2), c4ScreenPos.y + (height / 2)),
                                                IM_COL32(255, 255, 0, 255), // Parlak sarı
                                                4.0f,
                                                0,
                                                2.5f // kalın kenarlık
                                            );

                                            // ===== "C4" YAZISI =====
                                            Render::DrawText(
                                                c4ScreenPos.x + (width / 2) + 8,
                                                c4ScreenPos.y - (height / 2),
                                                RGB{ 255, 255, 0 }, // Sarı
                                                "C4"
                                            );

                                            // ===== MESAFE YAZISI =====
                                            char distText[32];
                                            sprintf_s(distText, "%.1fm", c4Distance / 100.0f); // unit → metre

                                            Render::DrawText(
                                                c4ScreenPos.x + (width / 2) + 8,
                                                c4ScreenPos.y - (height / 2) + 15,
                                                RGB{ 255, 255, 255 }, // Beyaz
                                                distText
                                            );
                                        }
                                    }
                                }
                            }
                        }
                        if (showPhotoInBox && gPhotoSRV)
                        {
                            ImDrawList* dl = ImGui::GetBackgroundDrawList();

                            float boxX = screenHead.x - width / 2.0f;
                            float boxY = screenHead.y;
                            float boxW = width;
                            float boxH = height;

                            // PNG GERÇEK BOYUTLARI ← Burası kritik!
                            float imgAspect = (float)gPhotoW / (float)gPhotoH;
                            float boxAspect = boxW / boxH;

                            float dstW = boxW, dstH = boxH;
                            float offX = 0, offY = 0;

                            if (imgAspect > boxAspect)
                            {
                                dstH = boxW / imgAspect;
                                offY = (boxH - dstH) * 0.5f;
                            }
                            else
                            {
                                dstW = boxH * imgAspect;
                                offX = (boxW - dstW) * 0.5f;
                            }

                            dl->AddImage(
                                (ImTextureID)gPhotoSRV,
                                ImVec2(boxX + offX, boxY + offY),
                                ImVec2(boxX + offX + dstW, boxY + dstH)
                            );
                        }
                        // === GIF ÇİZİM BLOĞU ===
                        if (showGifInBox && !gGifFrames.empty())
                        {
                            ImDrawList* dl = ImGui::GetBackgroundDrawList();

                            // PNG kutusunun boyutlarını yeniden hesapla
                            float boxX = screenHead.x - width / 2.0f;
                            float boxY = screenHead.y;
                            float boxW = width;
                            float boxH = height;

                            // Frame değişimi
                            DWORD tick = GetTickCount();
                            GifFrame& f = gGifFrames[gGifIndex];

                            int frameDelayMs = 1000 / std::max(gifFps, 1); // FPS → ms

                            if (tick - gLastGifTick >= (DWORD)frameDelayMs)
                            {
                                gGifIndex = (gGifIndex + 1) % gGifFrames.size();
                                gLastGifTick = tick;
                            }
                            GifFrame& cur = gGifFrames[gGifIndex];

                            // Boyut ayarı
                            float imgAspect = (float)cur.w / (float)cur.h;
                            float boxAspect = boxW / boxH;

                            float dstW = boxW;
                            float dstH = boxH;
                            float offX = 0;
                            float offY = 0;

                            if (imgAspect > boxAspect)
                            {
                                dstH = boxW / imgAspect;
                                offY = (boxH - dstH) * 0.5f;
                            }
                            else
                            {
                                dstW = boxH * imgAspect;
                                offX = (boxW - dstW) * 0.5f;
                            }

                            dl->AddImage(
                                (ImTextureID)cur.srv,
                                ImVec2(boxX + offX, boxY + offY),
                                ImVec2(boxX + offX + dstW, boxY + offY + dstH)
                            );
                        }
                        // === GIF #2 ÇİZİMİ ===
                        if (showGifInBox2 && !gGifFrames2.empty())
                        {
                            ImDrawList* dl = ImGui::GetBackgroundDrawList();

                            float boxX = screenHead.x - width / 2.0f;
                            float boxY = screenHead.y;
                            float boxW = width;
                            float boxH = height;

                            DWORD tick = GetTickCount();
                            GifFrame& f2 = gGifFrames2[gGifIndex2];
                            int frameDelayMs2 = 1000 / std::max(gifFps2, 1);

                            if (tick - gLastGifTick2 >= (DWORD)frameDelayMs2)
                            {
                                gGifIndex2 = (gGifIndex2 + 1) % gGifFrames2.size();
                                gLastGifTick2 = tick;
                            }

                            GifFrame& cur2 = gGifFrames2[gGifIndex2];

                            float imgAspect = (float)cur2.w / (float)cur2.h;
                            float boxAspect = boxW / boxH;

                            float dstW = boxW;
                            float dstH = boxH;
                            float offX = 0;
                            float offY = 0;

                            if (imgAspect > boxAspect)
                            {
                                dstH = boxW / imgAspect;
                                offY = (boxH - dstH) * 0.5f;
                            }
                            else
                            {
                                dstW = boxH * imgAspect;
                                offX = (boxW - dstW) * 0.5f;
                            }

                            dl->AddImage(
                                (ImTextureID)cur2.srv,
                                ImVec2(boxX + offX, boxY + offY),
                                ImVec2(boxX + offX + dstW, boxY + dstH)
                            );
                        }
                        // === GIF #3 ÇİZİMİ ===
                        if (showGifInBox3 && !gGifFrames3.empty())
                        {
                            ImDrawList* dl = ImGui::GetBackgroundDrawList();

                            float boxX = screenHead.x - width / 2.0f;
                            float boxY = screenHead.y;
                            float boxW = width;
                            float boxH = height;

                            DWORD tick = GetTickCount();
                            GifFrame& f3 = gGifFrames3[gGifIndex3];
                            int frameDelayMs3 = 1000 / std::max(gifFps3, 1);

                            if (tick - gLastGifTick3 >= (DWORD)frameDelayMs3)
                            {
                                gGifIndex3 = (gGifIndex3 + 1) % gGifFrames3.size();
                                gLastGifTick3 = tick;
                            }

                            GifFrame& cur3 = gGifFrames3[gGifIndex3];

                            float imgAspect = (float)cur3.w / (float)cur3.h;
                            float boxAspect = boxW / boxH;

                            float dstW = boxW;
                            float dstH = boxH;
                            float offX = 0;
                            float offY = 0;

                            if (imgAspect > boxAspect)
                            {
                                dstH = boxW / imgAspect;
                                offY = (boxH - dstH) * 0.5f;
                            }
                            else
                            {
                                dstW = boxH * imgAspect;
                                offX = (boxW - dstW) * 0.5f;
                            }

                            dl->AddImage(
                                (ImTextureID)cur3.srv,
                                ImVec2(boxX + offX, boxY + offY),
                                ImVec2(boxX + offX + dstW, boxY + dstH)
                            );
                        }
                        // === GIF #4 ÇİZİMİ ===
                        if (showGifInBox4 && !gGifFrames4.empty())
                        {
                            ImDrawList* dl = ImGui::GetBackgroundDrawList();

                            float boxX = screenHead.x - width / 2.0f;
                            float boxY = screenHead.y;
                            float boxW = width;
                            float boxH = height;

                            DWORD tick = GetTickCount();
                            GifFrame& f4 = gGifFrames4[gGifIndex4];
                            int frameDelayMs4 = 1000 / std::max(gifFps4, 1);

                            if (tick - gLastGifTick4 >= (DWORD)frameDelayMs4)
                            {
                                gGifIndex4 = (gGifIndex4 + 1) % gGifFrames4.size();
                                gLastGifTick4 = tick;
                            }

                            GifFrame& cur4 = gGifFrames4[gGifIndex4];

                            float imgAspect = (float)cur4.w / (float)cur4.h;
                            float boxAspect = boxW / boxH;

                            float dstW = boxW;
                            float dstH = boxH;
                            float offX = 0;
                            float offY = 0;

                            if (imgAspect > boxAspect)
                            {
                                dstH = boxW / imgAspect;
                                offY = (boxH - dstH) * 0.5f;
                            }
                            else
                            {
                                dstW = boxH * imgAspect;
                                offX = (boxW - dstW) * 0.5f;
                            }

                            dl->AddImage(
                                (ImTextureID)cur4.srv,
                                ImVec2(boxX + offX, boxY + offY),
                                ImVec2(boxX + offX + dstW, boxY + dstH)
                            );
                        }
                    }
                }
                // ===== RCS (Recoil Control) - for döngüsünden hemen sonra, her frame 1 kez =====
                if (rcsEnabled && localPlayer) {
                    static FVec2 oldPunch{ 0.f, 0.f };

                    int shots = driver::read_memory<int>(driver, localPlayer + m_iShotsFired);

                    if (shots > rcsStartBullet && (GetAsyncKeyState(VK_LBUTTON) & 0x8000)) {
                        FVec2 punch = driver::read_memory<FVec2>(driver, localPlayer + m_aimPunchAngle);

                        // CS2'de gelen açı genelde punch*2.0f ile telafi edilir
                        const float newPitch = punch.x * 2.0f; // X = pitch (yukarı-aşağı)
                        const float newYaw = punch.y * 2.0f; // Y = yaw   (sağ-sol)

                        // DOĞRU delta: yeni - eski
                        const float dPitchDeg = newPitch - oldPunch.x;
                        const float dYawDeg = newYaw - oldPunch.y;

                        // Derece -> mouse count (katsayıyı menüden ayarlıyorsun: rcsDegToMouse)
                        const float moveX = (dYawDeg / rcsDegToMouse) * rcsScaleX;  // +X sağa
                        const float moveY = (-dPitchDeg / rcsDegToMouse) * rcsScaleY;  // - işareti: pozitif pitch'i AŞAĞI iter

                        AimMoveDelta(moveX, moveY);

                        // Drift olmaması için eski değerleri güncelle
                        oldPunch.x = newPitch;
                        oldPunch.y = newYaw;
                    }
                    else {
                        oldPunch = { 0.f, 0.f };
                    }
                }

                ImGui::Render();
                //float color[4]{ 0,0,0,0 };
                float color[4]{ 0.f, 0.f, 0.f, 0.f };
                gDeviceContext->OMSetRenderTargets(1U, &render_target_view, nullptr);
                gDeviceContext->ClearRenderTargetView(render_target_view, color);

                ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

                swap_chain->Present(0U, 0U);
            }

            // exiting
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();  // ← Bu satırı ekleyin
            // exiting (shutdown) kısmında:
            //ImGui_ImplDX11_Shutdown();

            ImGui::DestroyContext();

            if (swap_chain) {
                swap_chain->Release();
            }

            if (gDeviceContext) {
                gDeviceContext->Release();
            }

            if (gDevice) {
                gDevice->Release();
            }

            if (render_target_view)
            {
                render_target_view->Release();
            }
            if (gPhotoSRV) { gPhotoSRV->Release(); gPhotoSRV = nullptr; }

            DestroyWindow(overlay);
            UnregisterClassW(wc.lpszClassName, wc.hInstance);
        }
    }

    // close handle to driver
    CloseHandle(driver);

    return 0; // successful execution
}
