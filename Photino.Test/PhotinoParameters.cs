using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;

namespace PhotinoNET
{
    [StructLayout(LayoutKind.Sequential, Pack = 16)]
	public struct PhotinoParameters
	{
		///<summary>EITHER StartString or StartUrl Must be specified: Browser control will navigate to this HTML string when initialized.</summary>
		[MarshalAs(UnmanagedType.LPWStr)] public string StartString;
		///<summary>EITHER StartString or StartUrl Must be specified: Browser control will navigate to this URL when initialized.</summary>
		[MarshalAs(UnmanagedType.LPWStr)] public string StartUrl;
		///<summary>OPTIONAL: Appears on the title bar of the native window.</summary>
		[MarshalAs(UnmanagedType.LPWStr)] public string Title;
		///<summary>WINDOWS ONLY: OPTIONAL: Path to a local file or a URL. Icon appears on the title bar of the native window (if supported).</summary>
		[MarshalAs(UnmanagedType.LPWStr)] public string WindowIconFile;

		///<summary>OPTIONAL: If this window is created from another window.</summary>
		public IntPtr Parent;

		///<summary>OPTIONAL: </summary>
		public ClosingDelegate ClosingHandler;      //EventHandler?
		///<summary>OPTIONAL: </summary>
		public ResizedDelegate ResizedHandler;
		///<summary>OPTIONAL: </summary>
		public MovedDelegate MovedHandler;
		///<summary>OPTIONAL: </summary>
		public GetAllMonitorsDelegate GetAllMonitorsHandler;
		///<summary>OPTIONAL: </summary>
		public WebMessageReceivedDelegate WebMessageReceivedHandler;
		///<summary>OPTIONAL: </summary>
		public WebResourceRequestedDelegate WebResourceRequestedHandler;

		///<summary>OPTIONAL: Name of a custom URL Scheme. Must have a matching entry in CustomSchemeHandlers array. Array length must be 32.</summary>
		[MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.LPWStr, SizeConst = 32)] public string[] CustomSchemeNames;
		///<summary>OPTIONAL: A custom URL Scheme handler. Must have a matching entry in CustomSchemeNamess array. Array length must be 32.</summary>
		[MarshalAs(UnmanagedType.ByValArray, SizeConst = 32)] public WebResourceRequestedDelegate[] CustomSchemeHandlers;

		///<summary>OPTIONAL: Initial window position in pixels.</summary>
		public int Left;
		///<summary>OPTIONAL: Initial window position in pixels.</summary>
		public int Top;
		///<summary>OPTIONAL: Initial window size in pixels.</summary>
		public int Width;
		///<summary>OPTIONAL: Initial window size in pixels.</summary>
		public int Height;

		///<summary>OPTIONAL: If true, window is created without a title bar or borders. This allows owner-drawn title bars and borders.</summary>
		public bool Chromeless;
		///<summary>OPTIONAL: If true, browser control covers the entire screen. Useful for kiosks for example. Incompatible with Maximized and Minimized.</summary>
		public bool FullScreen;
		///<summary>OPTIONAL: If true, native window is maximized to fill the screen. Incompatible with Minimized and FullScreen.</summary>
		public bool Maximized;
		///<summary>OPTIONAL: If true, native window is minimized (hidden). Incompatible with Maximized and FullScreen.</summary>
		public bool Minimized;
		///<summary>OPTIONAL: If true, native window cannot be resized by the user. Can still be resized by the program.</summary>
		public bool Resizable;
		///<summary>OPTIONAL: If true, native window appears in front of other windows and cannot be hidden behind them.</summary>
		public bool Topmost;

		///<summary>Checks the parameters to ensure they are valid before window creation. 
		///Called by PhotinoWindow prior to initializing native window.
		///</summary>
		///<returns>List of error strings</returns>
		public List<string> GetParamErrors()
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

			return response;
		}
	}
}
