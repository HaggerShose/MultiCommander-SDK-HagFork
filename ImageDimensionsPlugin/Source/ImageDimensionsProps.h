#pragma once

#include "PluginWinConfig.h"

#include "..\..\MultiCommander\SDK\ExtensionInfo.h"
#include "..\..\MultiCommander\SDK\IFileProperties.h"

MCNSBEGIN

class ImageDimensionsFileProperties : public IFileProperties {
public:
  ImageDimensionsFileProperties();
  ~ImageDimensionsFileProperties();

  static bool GetExtensionInfo(DLLExtensionInfo *pInfo);

  char *Get_ModuleID() override;
  long PreStartInit(IMultiAppInterface *pAppInterface) override;

  bool Open(IFileItem *pParentFileItem) override;
  bool Open(const WCHAR *szParentPath) override;
  bool Close() override;

  bool GetDisplayValue(IFileItem *pFileItem, WCHAR *propData, WORD nLen,
                       WORD PropertyId, const volatile bool *pAbort) override;
  bool GetPropStr(IFileItem *pFileItem, WCHAR *propData, WORD nLen,
                  WORD PropertyId, const volatile bool *pAbort) override;
  bool GetPropNum(IFileItem *pFileItem, INT64 *propData, WORD PropertyId,
                  const volatile bool *pAbort) override;
  bool GetPropDouble(IFileItem *pFileItem, double *propData, WORD PropertyId,
                     const volatile bool *pAbort) override;
  bool SetProp(IFileItem *pFileItem, WORD PropertyId,
               const BYTE *propData) override;

private:
  static char m_GuidString[34];
};

MCNSEND
