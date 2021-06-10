using System;
using System.IO;
using System.Runtime.InteropServices;

namespace PhotinoNET
{
    //These are for the callbacks from C++ to C#.

    //These are wired up automatically in the PhotinoWindow (.NET) constructor.
    [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Auto)] public delegate void ClosingDelegate();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Auto)] public delegate void ResizedDelegate(int width, int height);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Auto)] public delegate void MovedDelegate(int x, int y);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Auto)] public delegate void WebMessageReceivedDelegate(string message);

    //These are sent in during the request
    [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Auto)] public delegate int GetAllMonitorsDelegate(in NativeMonitor monitor);
    //[UnmanagedFunctionPointer(CallingConvention.Cdecl)] delegate void InvokeCallback();

    //These are for custom resource handlers
    [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Auto)] public delegate Stream CustomSchemeDelegate(string url, out string contentType);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Auto)] public delegate IntPtr WebResourceRequestedDelegate(string url, out int outNumBytes, out string outContentType);
}
