#include "Photino.Windows.DarkMode.h"

#include <mutex>

using RtlGetNtVersionNumbers_f = void(WINAPI *)(LPDWORD, LPDWORD, LPDWORD);

using SetWindowCompositionAttribute_f =
    HRESULT(WINAPI *)(HWND, WINDOWCOMPOSITIONATTRIBDATA *);

using ShouldAppsUseDarkMode_f = BOOLEAN(WINAPI *)();

using AllowDarkModeForWindow_f = BOOLEAN(WINAPI *)(HWND, BOOLEAN);

using RefreshImmersiveColorPolicyState_f = void(WINAPI *)();

using IsDarkModeAllowedForWindow_f = BOOLEAN(WINAPI *)(HWND);

using GetIsImmersiveColorUsingHighContrast_f =
    BOOLEAN(WINAPI *)(IMMERSIVE_HC_CACHE_MODE);

using SetPreferredAppMode_f = PreferredAppMode(WINAPI *)(PreferredAppMode);

static SetWindowCompositionAttribute_f SetWindowCompositionAttribute = nullptr;
static ShouldAppsUseDarkMode_f ShouldAppsUseDarkMode = nullptr;
static AllowDarkModeForWindow_f AllowDarkModeForWindow = nullptr;
static RefreshImmersiveColorPolicyState_f RefreshImmersiveColorPolicyState =
    nullptr;
static IsDarkModeAllowedForWindow_f IsDarkModeAllowedForWindow = nullptr;
static GetIsImmersiveColorUsingHighContrast_f
    GetIsImmersiveColorUsingHighContrast = nullptr;
static SetPreferredAppMode_f SetPreferredAppMode = nullptr;

static constexpr DWORD WIN10_MINIMUM_BUILD_DARK_MODE = 18362;

static std::once_flag flag_init_dark_mode_support;

static void EnableDarkModeForApp() noexcept {
    if (SetPreferredAppMode != nullptr) {
        SetPreferredAppMode(AllowDark);
    }
}

[[nodiscard]] static DWORD GetBuildNumber() noexcept {
    auto RtlGetNtVersionNumbers =
        reinterpret_cast<RtlGetNtVersionNumbers_f>(GetProcAddress(
            GetModuleHandleW(L"ntdll.dll"), "RtlGetNtVersionNumbers"));

    if (RtlGetNtVersionNumbers == nullptr) {
        return 0;
    }

    DWORD major = 0;
    DWORD minor = 0;
    DWORD build = 0;
    RtlGetNtVersionNumbers(&major, &minor, &build);
    build &= ~0xF0000000;
    return build;
}

[[nodiscard]] static bool IsHighContrast() noexcept {
    HIGHCONTRASTW high_contrast;
    high_contrast.cbSize = sizeof(high_contrast);
    if (SystemParametersInfoW(SPI_GETHIGHCONTRAST,
                              sizeof(high_contrast),
                              &high_contrast,
                              FALSE) == TRUE) {
        return (high_contrast.dwFlags & HCF_HIGHCONTRASTON) > 0;
    }
    return false;
}

static void InitDarkModeSupportOnce() noexcept {
    const auto build_number = GetBuildNumber();

    if (build_number < WIN10_MINIMUM_BUILD_DARK_MODE) {
        return;
    }

    HMODULE uxtheme =
        LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);

    if (uxtheme == nullptr) {
        return;
    }

    RefreshImmersiveColorPolicyState =
        reinterpret_cast<RefreshImmersiveColorPolicyState_f>(
            GetProcAddress(uxtheme, MAKEINTRESOURCEA(104)));

    GetIsImmersiveColorUsingHighContrast =
        reinterpret_cast<GetIsImmersiveColorUsingHighContrast_f>(
            GetProcAddress(uxtheme, MAKEINTRESOURCEA(106)));

    ShouldAppsUseDarkMode = reinterpret_cast<ShouldAppsUseDarkMode_f>(
        GetProcAddress(uxtheme, MAKEINTRESOURCEA(132)));

    AllowDarkModeForWindow = reinterpret_cast<AllowDarkModeForWindow_f>(
        GetProcAddress(uxtheme, MAKEINTRESOURCEA(133)));

    SetPreferredAppMode = reinterpret_cast<SetPreferredAppMode_f>(
        GetProcAddress(uxtheme, MAKEINTRESOURCEA(135)));

    IsDarkModeAllowedForWindow =
        reinterpret_cast<IsDarkModeAllowedForWindow_f>(
            GetProcAddress(uxtheme, MAKEINTRESOURCEA(137)));

    SetWindowCompositionAttribute =
        reinterpret_cast<SetWindowCompositionAttribute_f>(GetProcAddress(
            GetModuleHandleW(L"user32.dll"), "SetWindowCompositionAttribute"));

    if (RefreshImmersiveColorPolicyState != nullptr &&
        ShouldAppsUseDarkMode != nullptr &&
        AllowDarkModeForWindow != nullptr && SetPreferredAppMode != nullptr &&
        IsDarkModeAllowedForWindow != nullptr) {
        EnableDarkModeForApp();
        RefreshImmersiveColorPolicyState();
    }
}

void InitDarkModeSupport() noexcept {
    std::call_once(flag_init_dark_mode_support, InitDarkModeSupportOnce);
}

bool IsDarkModeEnabled() noexcept {
    if (ShouldAppsUseDarkMode == nullptr) {
        return false;
    }
    return (ShouldAppsUseDarkMode() == TRUE) && !IsHighContrast();
}

void EnableDarkMode(HWND hwnd, bool enable) noexcept {
    if (AllowDarkModeForWindow == nullptr) {
        return;
    }
    AllowDarkModeForWindow(hwnd, enable ? TRUE : FALSE);
}

void RefreshNonClientArea(HWND hwnd) noexcept {
    if (IsDarkModeAllowedForWindow == nullptr ||
        ShouldAppsUseDarkMode == nullptr) {
        return;
    }

    BOOL dark = FALSE;
    if (IsDarkModeAllowedForWindow(hwnd) == TRUE &&
        ShouldAppsUseDarkMode() == TRUE && !IsHighContrast()) {
        dark = TRUE;
    }

    if (SetWindowCompositionAttribute != nullptr) {
        WINDOWCOMPOSITIONATTRIBDATA data = {
            WCA_USEDARKMODECOLORS, &dark, sizeof(dark)};
        SetWindowCompositionAttribute(hwnd, &data);
    }
}

bool IsColorSchemeChange(LPARAM l_param) noexcept {
    bool return_value = false;
    if (l_param > 0 && CompareStringOrdinal(reinterpret_cast<LPCWCH>(l_param),
                                            -1,
                                            L"ImmersiveColorSet",
                                            -1,
                                            TRUE) == CSTR_EQUAL) {
        RefreshImmersiveColorPolicyState();
        return_value = true;
    }

    if (GetIsImmersiveColorUsingHighContrast != nullptr) {
        GetIsImmersiveColorUsingHighContrast(IHCM_REFRESH);
    }
    return return_value;
}
