#include "PluginWinConfig.h"

#include <stdio.h>

// Skip FilePropertiesPlugin.h (heavy IAppInterface.h + duplicate ExtensionInfo
// include paths break clangd).
#include "..\..\MultiCommander\SDK\IAppInterface.h"
#include "..\..\MultiCommander\SDK\IFileItem.h"
#include "..\..\MultiCommander\SDK\IFilePropertiesManager.h"
#include "..\..\MultiCommander\SDK\SDKVersion.h"

#include "ImageDimensionsProps.h"

#include "ImageDimensionsReader.h"

#include <wchar.h>

using namespace MCNS;

namespace {
constexpr WORD kPropImageDimensionsStub = 10;

static wchar_t LowerW(wchar_t c) {
  if (c >= L'A' && c <= L'Z')
    return static_cast<wchar_t>(c + (L'a' - L'A'));
  return c;
}

static bool PathEndsWithI(const wchar_t *path, const wchar_t *suffix) {
  if (path == nullptr || suffix == nullptr)
    return false;
  const size_t n = wcslen(path);
  const size_t m = wcslen(suffix);
  if (n < m)
    return false;
  const wchar_t *p = path + (n - m);
  for (size_t i = 0; i < m; ++i) {
    if (LowerW(p[i]) != LowerW(suffix[i]))
      return false;
  }
  return true;
}

static bool PathHasKnownImageExtension(const wchar_t *path) {
  static const wchar_t *const kExts[] = {L".jpg", L".jpeg", L".png", L".gif",
                                         L".bmp", L".webp", L".tif", L".tiff",
                                         L".dng", L".jxl"};
  for (const wchar_t *ext : kExts) {
    if (PathEndsWithI(path, ext))
      return true;
  }
  return false;
}
} // namespace

// Unique extension GUID (registry format without braces/dashes)
char ImageDimensionsFileProperties::m_GuidString[34] =
    "7E2F4C9A1B8D4E6F90A3B5C7D1E2F4A8";

ImageDimensionsFileProperties::ImageDimensionsFileProperties() = default;

ImageDimensionsFileProperties::~ImageDimensionsFileProperties() = default;

bool ImageDimensionsFileProperties::GetExtensionInfo(DLLExtensionInfo *pInfo) {
  if (pInfo == nullptr)
    return false;

  ZeroMemory(pInfo, sizeof(DLLExtensionInfo));

  wcsncpy(pInfo->wsName, L"Image Dimensions (common formats)", 100);
  pInfo->wsName[99] = L'\0';
  wcsncpy(pInfo->wsPublisher, L"MultiCommander-SDK", 100);
  pInfo->wsPublisher[99] = L'\0';
  wcsncpy(pInfo->wsURL, L"https://multicommander.com", 100);
  pInfo->wsURL[99] = L'\0';
  wcsncpy(pInfo->wsDesc,
          L"Adds a Dimensions column (JPEG, PNG, GIF, WebP, BMP, TIFF/DNG, "
          L"JXL; header/chunk parse; no HEIF/AVIF).",
          160);
  pInfo->wsDesc[159] = L'\0';
  wcsncpy(pInfo->wsBaseName, L"ImageDimensions", 100);
  pInfo->wsBaseName[99] = L'\0';

  strncpy(pInfo->strVersion, "1.0.0.0", 10);
  strncpy(pInfo->strGuid, m_GuidString, 34);

  // Do not require ImageDimensions_lang_*.xml / ImageDimensions.xml (none
  // shipped with this stub).
#ifdef _UNICODE
  pInfo->dwFlags =
      EXT_TYPE_PROP | EXT_PREINIT | EXT_OS_UNICODE | EXT_NOLANGFILE;
#else
  pInfo->dwFlags = EXT_TYPE_PROP | EXT_PREINIT | EXT_OS_ANSI | EXT_NOLANGFILE;
#endif

  pInfo->dwInitOrder = 2010;
  pInfo->dwInterfaceVersion = MULTI_INTERFACE_VERSION;
  return true;
}

char *ImageDimensionsFileProperties::Get_ModuleID() { return m_GuidString; }

long ImageDimensionsFileProperties::PreStartInit(
    IMultiAppInterface *pAppInterface) {
  IFilePropertiesManager *pPropMan =
      (IFilePropertiesManager *)pAppInterface->QueryInterface(ZOBJ_PROPMANGER,
                                                              0);

  if (pPropMan) {
    pPropMan->Init(m_GuidString);

    static const WCHAR kCategory[] = L"ImageDimensions";
    static const WCHAR kDisplay[] = L"Dimensions";

    FilePropData fpd;
    ZeroMemory(&fpd, sizeof(FilePropData));
    fpd.PropertyId = kPropImageDimensionsStub;
    fpd.szPropName = L"ImageDimensions";
    fpd.szDisplayName = kDisplay;
    // Explicit column title (some hosts cache a legacy title if this was null).
    fpd.szColumnName = kDisplay;
    fpd.szCategoryName = kCategory;
    fpd.szDescription = nullptr;
    // Keep sync: FILEPROP_ASYNC races with F5/reload (IFileItem freed while
    // worker still runs / first touches a dead item -> AV).
    fpd.dwOptions =
        FILEPROP_STRING | FILEPROP_CUSTOMIZABLE | FILEPROP_ONLY_FILES;
    fpd.IdealWidth = 100;
    fpd.Align = DT_LEFT;

    pPropMan->RegisterProperty(&fpd);

    pAppInterface->ReleaseInterface((ZHANDLE)pPropMan, ZOBJ_PROPMANGER);
  }

  return 0;
}

bool ImageDimensionsFileProperties::Open(IFileItem * /*pParentFileItem*/) {
  return false;
}

bool ImageDimensionsFileProperties::Open(const WCHAR * /*szParentPath*/) {
  return false;
}

bool ImageDimensionsFileProperties::Close() { return true; }

bool ImageDimensionsFileProperties::GetDisplayValue(
    IFileItem * /*pFileItem*/, WCHAR * /*propData*/, WORD /*nLen*/,
    WORD /*PropertyId*/, const volatile bool * /*pAbort*/) {
  return false;
}

bool ImageDimensionsFileProperties::GetPropStr(IFileItem *pFileItem,
                                               WCHAR *propData, WORD nLen,
                                               WORD PropertyId,
                                               const volatile bool *pAbort) {
  if (PropertyId != kPropImageDimensionsStub || propData == nullptr ||
      nLen == 0)
    return false;

  if (pAbort && *pAbort)
    return false;

  if (pFileItem == nullptr || pFileItem->isFolder())
    return false;

  WCHAR path[_MC_MAXPATH_];
  path[0] = L'\0';
  pFileItem->Get_FullPath(path,
                          static_cast<DWORD>(sizeof(path) / sizeof(path[0])));

  if (pAbort && *pAbort)
    return false;
  if (path[0] == L'\0')
    return false;

  if (!PathHasKnownImageExtension(path))
    return false;

  unsigned w = 0;
  unsigned h = 0;
  if (!TryReadImageDimensions(path, w, h, pAbort))
    return false;

  _snwprintf_s(propData, nLen, _TRUNCATE, L"%u x %u", w, h);
  return true;
}

bool ImageDimensionsFileProperties::GetPropNum(
    IFileItem * /*pFileItem*/, INT64 * /*propData*/, WORD /*PropertyId*/,
    const volatile bool * /*pAbort*/) {
  return false;
}

bool ImageDimensionsFileProperties::GetPropDouble(
    IFileItem * /*pFileItem*/, double * /*propData*/, WORD /*PropertyId*/,
    const volatile bool * /*pAbort*/) {
  return false;
}

bool ImageDimensionsFileProperties::SetProp(IFileItem * /*pFileItem*/,
                                            WORD /*PropertyId*/,
                                            const BYTE * /*propData*/) {
  return false;
}
