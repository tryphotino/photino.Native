using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;

namespace PhotinoNET
{
    [StructLayout(LayoutKind.Sequential, Pack = 16, CharSet = CharSet.Auto)]
	internal struct PhotinoNativeParameters
	{
		///<summary>EITHER StartString or StartUrl Must be specified: Browser control will navigate to this HTML string when initialized.</summary>
		[MarshalAs(UnmanagedType.LPWStr)] internal string StartString;
		///<summary>EITHER StartString or StartUrl Must be specified: Browser control will navigate to this URL when initialized.</summary>
		[MarshalAs(UnmanagedType.LPWStr)] internal string StartUrl;
		///<summary>OPTIONAL: Appears on the title bar of the native window.</summary>
		[MarshalAs(UnmanagedType.LPWStr)] internal string Title;
		///<summary>WINDOWS ONLY: OPTIONAL: Path to a local file or a URL. Icon appears on the title bar of the native window (if supported).</summary>
		[MarshalAs(UnmanagedType.LPWStr)] internal string WindowIconFile;

		///<summary>OPTIONAL: If this window is created from another window.</summary>
		internal IntPtr Parent;

		///<summary>SET BY PHOTINIWINDOW CONSTRUCTOR</summary>
		[MarshalAs(UnmanagedType.FunctionPtr)] internal CppClosingDelegate ClosingHandler;
		///<summary>SET BY PHOTINIWINDOW CONSTRUCTOR</summary>
		[MarshalAs(UnmanagedType.FunctionPtr)] internal CppResizedDelegate ResizedHandler;
		///<summary>SET BY PHOTINIWINDOW CONSTRUCTOR</summary>
		[MarshalAs(UnmanagedType.FunctionPtr)] internal CppMovedDelegate MovedHandler;
		///<summary>SET BY PHOTINIWINDOW CONSTRUCTOR</summary>
		[MarshalAs(UnmanagedType.FunctionPtr)] internal CppWebMessageReceivedDelegate WebMessageReceivedHandler;

		///<summary>OPTIONAL: Names of a custom URL Schemes. Array length must be 16.</summary>
		[MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.LPWStr, SizeConst = 16)] internal string[] CustomSchemeNames;
		///<summary>SET BY PHOTINIWINDOW CONSTRUCTOR</summary>
		[MarshalAs(UnmanagedType.FunctionPtr)] internal CppWebResourceRequestedDelegate CustomSchemeHandler;


		///<summary>OPTIONAL: Initial window position in pixels. Can be overridden with UseOsDefaultLocation property.</summary>
		[MarshalAs(UnmanagedType.I4)] internal int Left;
		///<summary>OPTIONAL: Initial window position in pixels. Can be overridden with UseOsDefaultLocation property.</summary>
		[MarshalAs(UnmanagedType.I4)] internal int Top;
		///<summary>OPTIONAL: Initial window size in pixels. Can be overridden with UseOsDefaultSize property.</summary>
		[MarshalAs(UnmanagedType.I4)] internal int Width;
		///<summary>OPTIONAL: Initial window size in pixels. Can be overridden with UseOsDefaultSize property.</summary>
		[MarshalAs(UnmanagedType.I4)] internal int Height;
		///<summary>OPTIONAL: Initial zoom level of browser control. e.g.100 = 100%</summary>
		[MarshalAs(UnmanagedType.I4)] internal int Zoom;

		///<summary>OPTIONAL: If true, native window appears in centered on screen. Left and Top properties are ignored.</summary>
		[MarshalAs(UnmanagedType.I1)] internal bool CenterOnInitialize;
		///<summary>OPTIONAL: If true, window is created without a title bar or borders. This allows owner-drawn title bars and borders.</summary>
		[MarshalAs(UnmanagedType.I1)] internal bool Chromeless;
		///<summary>OPTIONAL: If true, browser control covers the entire screen. Useful for kiosks for example. Incompatible with Maximized and Minimized.</summary>
		[MarshalAs(UnmanagedType.I1)] internal bool FullScreen;
		///<summary>OPTIONAL: If true, native window is maximized to fill the screen. Incompatible with Minimized and FullScreen.</summary>
		[MarshalAs(UnmanagedType.I1)] internal bool Maximized;
		///<summary>OPTIONAL: If true, native window is minimized (hidden). Incompatible with Maximized and FullScreen.</summary>
		[MarshalAs(UnmanagedType.I1)] internal bool Minimized;
		///<summary>OPTIONAL: If true, native window cannot be resized by the user. Can still be resized by the program.</summary>
		[MarshalAs(UnmanagedType.I1)] internal bool Resizable;
		///<summary>OPTIONAL: If true, native window appears in front of other windows and cannot be hidden behind them.</summary>
		[MarshalAs(UnmanagedType.I1)] internal bool Topmost;
		///<summary>OPTIONAL: If true, overrides Top and Left parameters and lets the OS size the newly created window.</summary>
		[MarshalAs(UnmanagedType.I1)] internal bool UseOsDefaultLocation;
		///<summary>OPTIONAL: If true, overrides Height and Width parameters and lets the OS position the newly created window.</summary>
		[MarshalAs(UnmanagedType.I1)] internal bool UseOsDefaultSize;

		
		///<summary>The size is set when GetParamErrors() is called, prior to initializing the native window. It is a check to make sure the struct matches what C++ is expecting.</summary>
		[MarshalAs(UnmanagedType.I4)] internal int Size;


		///<summary>Checks the parameters to ensure they are valid before window creation. 
		///Called by PhotinoWindow prior to initializing native window.
		///</summary>
		///<returns>List of error strings</returns>
		internal List<string> GetParamErrors()
		{
			var response = new List<string>();

			if (string.IsNullOrWhiteSpace(StartUrl) && string.IsNullOrWhiteSpace(StartString))
				response.Add("An initial URL or HTML string must be supplied in StartUrl or StartString for the browser control to naviage to.");

			if (Maximized && Minimized)
				response.Add("Window cannot be both maximized and minimized on startup.");

			if (FullScreen && (Maximized || Minimized))
				response.Add("FullScreen cannot be combined with Maximized or Minimized");

			if (!string.IsNullOrWhiteSpace(WindowIconFile) && !File.Exists(WindowIconFile))
				response.Add($"WindowIconFile: {WindowIconFile} cannot be found");

			Size = Marshal.SizeOf(typeof(PhotinoNativeParameters));

			return response;
		}
	}
}
