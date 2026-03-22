// Precompiled header: NotUsing (see vcxproj). Include only what this
// translation unit uses.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "..\..\MultiCommander\SDK\ExtensionInfo.h"
#include "..\..\MultiCommander\SDK\IFileProperties.h"
#include "ImageDimensionsProps.h"

MCNSBEGIN
class IPluginInterface;
MCNSEND

#ifdef _MANAGED
#pragma managed(push, off)
#endif

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD ul_reason_for_call,
                      LPVOID /*lpReserved*/) {
  switch (ul_reason_for_call) {
  case DLL_PROCESS_ATTACH:
  case DLL_THREAD_ATTACH:
  case DLL_THREAD_DETACH:
  case DLL_PROCESS_DETACH:
    break;
  }
  return TRUE;
}

extern "C" PVOID APIENTRY Create(int nID) {
  if (nID == 0) {
    MCNS::ImageDimensionsFileProperties *p =
        new MCNS::ImageDimensionsFileProperties();
    return static_cast<MCNS::IFileProperties *>(p);
  }
  return nullptr;
}

// Same signature as MCAppExtensionSample: host passes the same pointer returned
// by Create.
extern "C" bool APIENTRY Delete(MCNS::IPluginInterface *pModule, int nID) {
  if (pModule == nullptr)
    return false;

  if (nID == 0) {
    auto *p = reinterpret_cast<MCNS::ImageDimensionsFileProperties *>(pModule);
    delete p;
    return true;
  }

  return false;
}

extern "C" bool APIENTRY GetExtensionInfo(int nID,
                                          MCNS::DLLExtensionInfo *pInfo) {
  if (nID == 0)
    return MCNS::ImageDimensionsFileProperties::GetExtensionInfo(pInfo);
  return false;
}

#ifdef _MANAGED
#pragma managed(pop)
#endif
