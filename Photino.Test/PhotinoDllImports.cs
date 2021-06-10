using System;
using System.Runtime.InteropServices;

namespace PhotinoNET
{
    public partial class PhotinoWindow
    {
        const string DllName = "Photino.Native";
        //REGISTER
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)] static extern IntPtr Photino_register_win32(IntPtr hInstance);
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)] static extern IntPtr Photino_register_mac();


        //CTOR-DTOR
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Auto)] static extern IntPtr Photino_ctor(PhotinoNativeParameters parameters);
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)] static extern void Photino_dtor(IntPtr instance);


        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Auto)] static extern void Photino_AddCustomScheme(IntPtr instance, string scheme, WebResourceRequestedDelegate requestHandler);
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)] static extern void Photino_Close(IntPtr instance);


        //GET
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)] static extern IntPtr Photino_getHwnd_win32(IntPtr instance);
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)] static extern void Photino_GetAllMonitors(IntPtr instance, GetAllMonitorsDelegate callback);
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)] static extern void Photino_GetPosition(IntPtr instance, out int x, out int y);
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)] static extern uint Photino_GetScreenDpi(IntPtr instance);
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)] static extern void Photino_GetSize(IntPtr instance, out int width, out int height);
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)] static extern void Photino_GetZoom(IntPtr instance, out int zoom);
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)] static extern void Photino_IsMaximized(IntPtr instance, bool maximized);
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)] static extern void Photino_IsMinimized(IntPtr instance, bool minimized);



        //deprecated [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)] static extern void Photino_Invoke(IntPtr instance, InvokeCallback callback);


        //NAVIGATE
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Auto)] static extern void Photino_NavigateToString(IntPtr instance, string content);
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Auto)] static extern void Photino_NavigateToUrl(IntPtr instance, string url);


        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Auto)] static extern void Photino_SendWebMessage(IntPtr instance, string message);


        //SET
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)] static extern void Photino_SetMaximized(IntPtr instance, bool maximized);
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)] static extern void Photino_SetMinimized(IntPtr instance, bool minimized);
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)] static extern void Photino_SetResizable(IntPtr instance, int resizable);
        //deprecated - always set on initialization [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)] static extern void Photino_SetResizedCallback(IntPtr instance, SizeChangedDelegate callback);
        //deprecated - always set on initialization [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)] static extern void Photino_SetClosingCallback(IntPtr instance, ClosingDelegate callback);
        //deprecated - always set on initialization [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)] static extern void Photino_SetMovedCallback(IntPtr instance, LocationChangedDelegate callback);
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)] static extern void Photino_SetPosition(IntPtr instance, int x, int y);
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)] static extern void Photino_SetSize(IntPtr instance, int width, int height);
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Auto)] static extern void Photino_SetTitle(IntPtr instance, string title);
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)] static extern void Photino_SetTopmost(IntPtr instance, int topmost);
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Auto)] static extern void Photino_SetIconFile(IntPtr instance, string filename);
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)] static extern void Photino_SetZoom(IntPtr instance, int zoom);


        //deprecated- always shown on initialization  [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)] static extern void Photino_Show(IntPtr instance);


        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Auto)] static extern void Photino_ShowMessage(IntPtr instance, string title, string body, uint type);
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)] static extern void Photino_WaitForExit(IntPtr instance);
    }
}
