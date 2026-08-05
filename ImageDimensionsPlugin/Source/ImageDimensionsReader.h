#pragma once

bool TryReadImageDimensions(const wchar_t *path, unsigned &outW, unsigned &outH,
                            const volatile bool *pAbort);
