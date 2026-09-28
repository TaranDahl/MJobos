#pragma once

#include <AbstractClass.h>

// Interop exports for attribute queries on any AbstractClass-derived object.
// C# P/Invoke examples (simplified):
// ```csharp
// [DllImport("Phobos.dll", CallingConvention = CallingConvention.StdCall, EntryPoint = "Abstract_GetAttributeIndices_Phobos")]
// public static extern int Abstract_GetAttributeIndices_Phobos(IntPtr pThis, int[] pOutIndices, int maxCount, out int pCount);
//
// [DllImport("Phobos.dll", CallingConvention = CallingConvention.StdCall, EntryPoint = "Abstract_GetAttributeNames_Phobos")]
// public static extern int Abstract_GetAttributeNames_Phobos(IntPtr pThis, IntPtr[] pOutNames, int maxCount, out int pCount);
// ```
// Two-phase usage: call with the output buffer = IntPtr.Zero to query the required
// count via pCount, then call again with a buffer of at least that size.

#include "Utilities/Macro.h"

/// <summary>
/// Gets the attribute indices assigned to an object (copied from its type at initialization).
/// On success, pCount receives the total number of attributes; up to maxCount indices are
/// written to pOutIndices when it is not null.
/// </summary>
/// <param name="pThis">Pointer to the AbstractClass instance</param>
/// <param name="pOutIndices">Buffer receiving the attribute indices, or nullptr to only query the count</param>
/// <param name="maxCount">Capacity of pOutIndices in elements, ignored when pOutIndices is nullptr</param>
/// <param name="pCount">Receives the total number of attributes</param>
/// <returns>S_OK on success, E_POINTER if pThis or pCount is null, E_FAIL if the object has no extension</returns>
DEFINE_EXPORT(HRESULT, Abstract_GetAttributeIndices_Phobos, AbstractClass* pThis, int* pOutIndices, int maxCount, int* pCount);

/// <summary>
/// Gets the names of the attributes assigned to an object.
/// The returned name pointers stay valid until the attribute registry is cleared (rule INI reload).
/// On success, pCount receives the total number of attributes; up to maxCount name pointers are
/// written to pOutNames when it is not null.
/// </summary>
/// <param name="pThis">Pointer to the AbstractClass instance</param>
/// <param name="pOutNames">Buffer receiving the attribute name pointers, or nullptr to only query the count</param>
/// <param name="maxCount">Capacity of pOutNames in elements, ignored when pOutNames is nullptr</param>
/// <param name="pCount">Receives the total number of attributes</param>
/// <returns>S_OK on success, E_POINTER if pThis or pCount is null, E_FAIL if the object has no extension</returns>
DEFINE_EXPORT(HRESULT, Abstract_GetAttributeNames_Phobos, AbstractClass* pThis, const char** pOutNames, int maxCount, int* pCount);
