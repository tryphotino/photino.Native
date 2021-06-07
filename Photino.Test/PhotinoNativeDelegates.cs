using System;
using System.IO;
using System.Runtime.InteropServices;

namespace PhotinoNET
{
    //These are for the callbacks from C++ to C#.

    // Here we use auto charset instead of forcing UTF-8.
    // Thus the native code for Windows will be much more simple.
    // Auto charset is UTF-16 on Windows and UTF-8 on Unix(.NET Core 3.0 and later and Mono).
    // As we target .NET Standard 2.1, we assume it runs on .NET Core 3.0 and later.
    // We should specify using auto charset because the default value is ANSI.
    [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Auto)] public delegate void ClosingDelegate();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Auto)] public delegate void ResizedDelegate(int width, int height);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Auto)] public delegate void MovedDelegate(int x, int y);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Auto)] public delegate void GetAllMonitorsDelegate(in NativeMonitor monitor);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Auto)] public delegate void WebMessageReceivedDelegate(string message);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Auto)] public delegate IntPtr WebResourceRequestedDelegate(string url, out int outNumBytes, out string outContentType);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Auto)] public delegate Stream CustomSchemeDelegate(string url, out string contentType);

    //[UnmanagedFunctionPointer(CallingConvention.Cdecl)] delegate void InvokeCallback();
}
