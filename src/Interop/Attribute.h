#pragma once

// Interop export for the global attribute registry.
// C# P/Invoke example (simplified):
// ```csharp
// [DllImport("Phobos.dll", CallingConvention = CallingConvention.StdCall, EntryPoint = "Attribute_GetCount_Phobos")]
// public static extern int Attribute_GetCount_Phobos(out int pCount);
// ```

#include "Utilities/Macro.h"

/// <summary>
/// Gets the total number of registered attributes (global registry size).
/// Attribute indices are always in the range [0, count).
/// </summary>
/// <param name="pCount">Receives the number of registered attributes</param>
/// <returns>S_OK on success, E_POINTER if pCount is null</returns>
DEFINE_EXPORT(HRESULT, Attribute_GetCount_Phobos, int* pCount);
