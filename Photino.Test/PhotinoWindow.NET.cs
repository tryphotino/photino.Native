using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;

namespace PhotinoNET
{
    public partial class PhotinoWindow
    {
        ///<summary>Parameters set to Photino.Native to start a new instance of a Photino.Native window.</summary>
        private PhotinoNativeParameters _startupParameters = new PhotinoNativeParameters
        {
            Resizable = true,   //these values can't be initialize within the struct itself
            Height = -1,
            Width = -1,
            Zoom = 100,
            CustomSchemeNames = new string[16],
        };
        
        //Pointers to the type and instance.
        private static IntPtr _nativeType = IntPtr.Zero;
        private IntPtr _nativeInstance;

        
        //There can only be 1 message loop for all windows.
        private static bool _messageLoopIsStarted = false;


        //READ ONLY PROPERTIES
        public static bool IsWindowsPlatform => RuntimeInformation.IsOSPlatform(OSPlatform.Windows);
        public static bool IsMacOsPlatform => RuntimeInformation.IsOSPlatform(OSPlatform.OSX);
        public static bool IsLinuxPlatform => RuntimeInformation.IsOSPlatform(OSPlatform.Linux);

        public IntPtr WindowHandle
        {
            get
            {
                if (IsWindowsPlatform)
                {
                    if (_nativeInstance == IntPtr.Zero)
                        throw new ApplicationException("The Photino window is not initialized yet");

                    return Photino_getHwnd_win32(_nativeInstance);
                }
                else
                    throw new PlatformNotSupportedException($"{nameof(WindowHandle)} is only supported on Windows.");
            }
        }

        public IReadOnlyList<Monitor> Monitors
        {
            get
            {
                if (_nativeInstance == IntPtr.Zero)
                    throw new ApplicationException("The Photino window hasn't been initialized yet.");

                List<Monitor> monitors = new List<Monitor>();

                int callback(in NativeMonitor monitor)
                {
                    monitors.Add(new Monitor(monitor));
                    return 1;
                }

                Photino_GetAllMonitors(_nativeInstance, callback);

                return monitors;
            }
        }

        public Monitor MainMonitor
        {
            get
            {
                if (_nativeInstance == IntPtr.Zero)
                    throw new ApplicationException("The Photino window hasn't been initialized yet.");

                return Monitors.First();
            }
        }

        public uint ScreenDpi
        {
            get
            {
                if (_nativeInstance == IntPtr.Zero)
                    throw new ApplicationException("The Photino window hasn't been initialized yet.");

                return Photino_GetScreenDpi(_nativeInstance);
            }
        }


        private Guid _id = Guid.NewGuid();
        public Guid Id => _id;



        //READ-WRITE PROPERTIES
        private bool _fullScreen;
        public bool FullScreen
        {
            get => _fullScreen;
            set
            {
                if (_fullScreen != value)
                {
                    _fullScreen = value;
                    if (_nativeInstance == IntPtr.Zero)
                        _startupParameters.FullScreen = _fullScreen;
                    else
                        Photino_SetSize(_nativeInstance, MainMonitor.WorkArea.Width, MainMonitor.WorkArea.Height);
                }
            }
        }


        private int _height = -1;
        public int Height
        {
            get => Size.Height;
            set
            {
                var currentSize = Size;
                if (currentSize.Height != value)
                    Size = new Size(currentSize.Width, value);
            }
        }


        private string _iconFile;
        public string IconFile
        {
            get => _iconFile;
            set
            {
                if (_iconFile != value)
                {
                    _iconFile = value;

                    if (!File.Exists(_iconFile))
                    {
                        var absolutePath = $"{System.AppContext.BaseDirectory}{_iconFile}";
                        if (!File.Exists(absolutePath))
                            throw new ArgumentException($"Icon file: {_iconFile} does not exist.");
                    }
                 
                    if (_nativeInstance == IntPtr.Zero)
                        _startupParameters.WindowIconFile = _iconFile;
                    else
                        Photino_SetIconFile(_nativeInstance, _iconFile);
                }
            }
        }
        

        private bool _resizable = true;
        public bool Resizable
        {
            get => _resizable;
            set
            {
                if (_resizable != value)
                {
                    _resizable = value;
                    if (_nativeInstance == IntPtr.Zero)
                        _startupParameters.Resizable = _resizable;
                    else
                       Photino_SetResizable(_nativeInstance, _resizable ? 1 : 0);
                }
            }
        }


        public Size Size
        {
            get
            {
                if (_nativeInstance != IntPtr.Zero)
                    Photino_GetSize(_nativeInstance, out _width, out _height);
    
                return new Size(_width, _height);
            }
            set
            {
                // ToDo:
                // Should this be locked if _isResizable == false?
                if (_width != value.Width || _height != value.Height)
                {
                    _width = value.Width;
                    _height = value.Height;

                    if (_nativeInstance == IntPtr.Zero)
                    {
                        _startupParameters.Height = _height;
                        _startupParameters.Width = _width;
                    }
                    else
                        Photino_SetSize(_nativeInstance, _width, _height);
                }
            }
        }


        public Point Location
        {
            get
            {
                if (_nativeInstance != IntPtr.Zero)
                    Photino_GetPosition(_nativeInstance, out _left, out _top);

                return new Point(_left, _top);
            }
            set
            {
                if (_left != value.X || _top != value.Y)
                {
                    _left = value.X;
                    _top = value.Y;

                    if (_nativeInstance == IntPtr.Zero)
                    {
                        _startupParameters.Left = _left;
                        _startupParameters.Top = _top;
                    }
                    else
                        Photino_SetPosition(_nativeInstance, _left, _top);
                }
            }
        }


        private int _left;
        public int Left
        {
            get => Location.X;
            set
            {
                var currentLocation = Location;
                if (currentLocation.X != value)
                    Location = new Point(value, currentLocation.Y);
            }
        }


        private bool _maximized;
        public bool Maximized
        {
            get
            {
                if (_nativeInstance != IntPtr.Zero)
                    Photino_IsMaximized(_nativeInstance, _minimized);
                
                return _minimized;
            }
            set
            {
                if (_maximized != value)
                {
                    _maximized = value;
                    if (_nativeInstance == IntPtr.Zero)
                        _startupParameters.Maximized = _maximized;
                    else
                        Photino_IsMaximized(_nativeInstance, _maximized);
                }
            }
        }


        private bool _minimized;
        public bool Minimized
        {
            get
            {
                if (_nativeInstance != IntPtr.Zero)
                    Photino_IsMinimized(_nativeInstance, _minimized);

                return _minimized;
            }
            set
            {
                if (_minimized != value)
                {
                    _minimized = value;
                    if (_nativeInstance == IntPtr.Zero)
                        _startupParameters.Minimized = _minimized;
                    else
                        Photino_SetMinimized(_nativeInstance, _minimized);
                }
            }
        }


        private string _title;
        public string Title
        {
            get
            {
                if (_nativeInstance != IntPtr.Zero)
                    Photino_GetTitle(_nativeInstance, out _title);

                return _title;
            }
            set
            {
                if (_title != value)
                {
                    if (string.IsNullOrEmpty(value.Trim()))
                        value = "Untitled Window";

                    // Due to Linux/Gtk platform limitations, the window title has to be no more than 31 chars
                    if (value.Length > 31 && IsLinuxPlatform)
                        value = value.Substring(0, 31);

                    _title = value;

                    if (_nativeInstance == IntPtr.Zero)
                        _startupParameters.Title = _title;
                    else
                        Photino_SetTitle(_nativeInstance, _title);
                }
            }
        }


        private int _top;
        public int Top
        {
            get => Location.Y;
            set
            {
                var currentLocation = Location;
                if (currentLocation.Y != value)
                    Location = new Point(currentLocation.X, value);
            }
        }


        private bool _topMost = false;
        public bool TopMost
        {
            get => _topMost;
            set
            {
                if (_topMost != value)
                {
                    _topMost = value;
                    if (_nativeInstance == IntPtr.Zero)
                        _startupParameters.Topmost = _topMost;
                    else
                        Photino_SetTopmost(_nativeInstance, _topMost ? 1 : 0);
                }
            }
        }


        private bool _useOsDefaultLocation;
        public bool UseOsDefaultLocation
        {
            get => _useOsDefaultLocation;
            set
            {
                if (_useOsDefaultLocation != value)
                {
                    _useOsDefaultLocation = value;

                    if (_nativeInstance == IntPtr.Zero)
                        _startupParameters.UseOsDefaultLocation = _useOsDefaultLocation;
                    else
                        throw new ApplicationException("UseOsDefaultLocation can only be set before the window is instantiated.");
                }
            }
        }


        private bool _useOsDefaultSize;
        public bool UseOsDefaultSize
        {
            get => _useOsDefaultSize;
            set
            {
                if (_useOsDefaultSize != value)
                {
                    _useOsDefaultSize = value;

                    if (_nativeInstance == IntPtr.Zero)
                        _startupParameters.UseOsDefaultSize = _useOsDefaultSize;
                    else
                        throw new ApplicationException("UseOsDefaultSize can only be set before the window is instantiated.");
                }
            }
        }


        private int _width = -1;
        public int Width
        {
            get => Size.Width;
            set
            {
                var currentSize = Size;
                if (currentSize.Width != value)
                    Size = new Size(value, currentSize.Height);
            }
        }


        private int _zoom = 100;
        public int Zoom
        {
            get
            {
                if (_nativeInstance != IntPtr.Zero)
                    Photino_GetZoom(_nativeInstance, out _zoom);

                return _zoom;
            }
            set
            {
                if (_zoom != value)
                {
                    _zoom = value;

                    if (_nativeInstance == IntPtr.Zero)
                        _startupParameters.Zoom = _zoom;
                    else
                       Photino_SetZoom(_nativeInstance, _zoom);
                }
            }
        }



        ///<summary>0 = Critical Only, 1 = Critical and Warning, 2 = Verbose, >2 = All Details</summary>
        public int LogVerbosity { get; set; } = 2;



        //CONSTRUCTOR
        public PhotinoWindow()
        {
            if (IsWindowsPlatform)
            {
                //This only has to be done once
                if (_nativeType == IntPtr.Zero)
                {
                    _nativeType = Marshal.GetHINSTANCE(typeof(PhotinoWindow).Module);
                    Photino_register_win32(_nativeType);
                }
            }
            else if (IsMacOsPlatform)
            {
                Photino_register_mac();
            }

            //Wire up handlers from C++ to C#
            _startupParameters.ClosingHandler = OnWindowClosing;
            _startupParameters.ResizedHandler = OnSizeChanged;
            _startupParameters.MovedHandler = OnLocationChanged;
            _startupParameters.WebMessageReceivedHandler = OnWebMessageReceived;
            _startupParameters.CustomSchemeHandler = OnCustomScheme;
        }



        //FLUENT METHODS FOR INITIALIZING STARTUP PARAMETERS FOR NEW WINDOWS

        //CAN ALSO BE CALLED AFTER INITIALIZATION TO SET VALUES

        //ONE OF THESE 3 METHODS *MUST* BE CALLED PRIOR TO CALLING WAITFORCLOSE() OR CREATECHILDWINDOW()
        ///<summary>Loads the specified file or url into the browser control</summary>
        public PhotinoWindow Load(Uri uri)
        {
            Log($".Load({uri})");
            if (_nativeInstance == IntPtr.Zero)
                _startupParameters.StartUrl = uri.ToString();
            else
                Photino_NavigateToUrl(_nativeInstance, uri.ToString());
            return this;
        }

        ///<summary>Loads the specified file or url into the browser control</summary>
        public PhotinoWindow Load(string path)
        {
            Log($".Load({path})");

            // ––––––––––––––––––––––
            // SECURITY RISK!
            // This needs validation!
            // ––––––––––––––––––––––
            // Open a web URL string path
            if (path.Contains("http://") || path.Contains("https://"))
                return Load(new Uri(path));

            // Open a file resource string path
            string absolutePath = Path.GetFullPath(path);

            // For bundled app it can be necessary to consider
            // the app context base directory. Check there too.
            if (File.Exists(absolutePath) == false)
            {
                absolutePath = $"{System.AppContext.BaseDirectory}/{path}";

                if (File.Exists(absolutePath) == false)
                {
                    Log($" ** File \"{path}\" could not be found.");
                    return this;
                }
            }

            return Load(new Uri(absolutePath, UriKind.Absolute));
        }

        ///<summary>Loads a raw string (typically HTML) into the browser control</summary>
        public PhotinoWindow LoadRawString(string content)
        {
            var shortContent = content.Length > 50 ? content.Substring(0, 50) + "..." : content;
            Log($".LoadRawString({shortContent})");
            if (_nativeInstance == IntPtr.Zero)
                _startupParameters.StartString = content;
            else
                Photino_NavigateToString(_nativeInstance, content);
            return this;
        }


        ///<summary>Centers the window in the main monitor. If called prior to window initialization, overrides Left and Top properties.</summary>
        public PhotinoWindow Center()
        {
            Log(".Center()");
            if (_nativeInstance == IntPtr.Zero)
            {
                _startupParameters.CenterOnInitialize = true;
                return this;
            }
            Size workAreaSize= MainMonitor.WorkArea.Size;
            var centeredPosition = new Point(
                ((workAreaSize.Width / 2) - (Width / 2)),
                ((workAreaSize.Height / 2) - (Height / 2))
            );
            return SetLocation(centeredPosition);
        }

        ///<summary>Moves the window to the specified location on the screen using a Point</summary>
        public PhotinoWindow MoveTo(Point location, bool allowOutsideWorkArea = false)
        {
            Log($".MoveTo({location}, {allowOutsideWorkArea})");

            if (LogVerbosity > 2)
            {
                Log($"  Current location: {Location}");
                Log($"  New location: {location}");
            }

            // If the window is outside of the work area,
            // recalculate the position and continue.
            //When window isn't initialized yet, cannot determine screen size.
            if (allowOutsideWorkArea == false && _nativeInstance != IntPtr.Zero)
            {
                int horizontalWindowEdge = location.X + Width;
                int verticalWindowEdge = location.Y + Height;

                int horizontalWorkAreaEdge = MainMonitor.WorkArea.Width;
                int verticalWorkAreaEdge = MainMonitor.WorkArea.Height;

                bool isOutsideHorizontalWorkArea = horizontalWindowEdge > horizontalWorkAreaEdge;
                bool isOutsideVerticalWorkArea = verticalWindowEdge > verticalWorkAreaEdge;

                var locationInsideWorkArea = new Point(
                    isOutsideHorizontalWorkArea ? horizontalWorkAreaEdge - Width : location.X,
                    isOutsideVerticalWorkArea ? verticalWorkAreaEdge - Height : location.Y
                );

                location = locationInsideWorkArea;
            }

            // Bug:
            // For some reason the vertical position is not handled correctly.
            // Whenever a positive value is set, the window appears at the
            // very bottom of the screen and the only visible thing is the
            // application window title bar. As a workaround we make a 
            // negative value out of the vertical position to "pull" the window up.
            // Note: 
            // This behavior seems to be a macOS thing. In the Photino.Native
            // project files it is commented to be expected behavior for macOS.
            // There is some code trying to mitigate this problem but it might
            // not work as expected. Further investigation is necessary.
            if (IsMacOsPlatform)
            {
                var workArea = MainMonitor.WorkArea.Size;
                location.Y = location.Y >= 0
                    ? location.Y - workArea.Height
                    : location.Y;
            }

            Location = location;

            return this;
        }

        ///<summary>Moves the window to the specified location on the screen using left and right properties</summary>
        public PhotinoWindow MoveTo(int left, int top, bool allowOutsideWorkArea = false)
        {
            Log($".MoveTo({left}, {top}, {allowOutsideWorkArea})");
            return MoveTo(new Point(left, top), allowOutsideWorkArea);
        }

        ///<summary>Moves the window relative to its current location on the screen using a Point</summary>
        public PhotinoWindow Offset(Point offset)
        {
            Log($".Offset({offset})");
            var location = Location;
            int left = location.X + offset.X;
            int top = location.Y + offset.Y;
            return MoveTo(left, top);
        }

        ///<summary>Moves the window relative to its current location on the screen using left and top coordinates</summary>
        public PhotinoWindow Offset(int left, int top)
        {
            Log($".Offset({left}, {top})");
            return Offset(new Point(left, top));
        }




        public PhotinoWindow SetChromeless(bool chromeless)
        {
            Log($".SetChromeless({chromeless})");
            if (_nativeInstance != IntPtr.Zero)
                throw new ApplicationException("Chromeless setting cannot be used on an unitialized window.");

            _startupParameters.Chromeless = chromeless;
            return this;
        }

        public PhotinoWindow SetFullScreen(bool fullScreen)
        {
            Log($".SetFullScreen({fullScreen})");
            FullScreen = fullScreen;
            return this;
        }

        ///<summary>In Pixels. Leave at -1 to initialize window to OS default size.</summary>
        public PhotinoWindow SetHeight(int height)
        {
            Log($".SetHeight({height})");
            Height = height;
            return this;
        }

        ///<summary>Must be a local file, not a URL.</summary>
        public PhotinoWindow SetIconFile(string iconFile)
        {
            Log($".SetIconFile({iconFile})");
            IconFile = iconFile;
            return this;
        }

        public PhotinoWindow SetLeft(int left)
        {
            Log($".SetLeft({Left})");
            Left = left;
            return this;
        }

        public PhotinoWindow SetResizable(bool resizable)
        {
            Log($".SetResizable({resizable})");
            Resizable = resizable;
            return this;
        }

        ///<summary>In Pixels. Leave at -1, -1 to initialize window to OS default size.</summary>
        public PhotinoWindow SetSize(Size size)
        {
            Log($".SetSize({size})");
            Size = size;
            return this;
        }

        public PhotinoWindow SetLocation(Point location)
        {
            Log($".SetLocation({location})");
            Location = location;
            return this;
        }

        public PhotinoWindow SetMaximized(bool maximized)
        {
            Log($".SetMaximized({maximized})");
            Maximized = maximized;
            return this;
        }

        public PhotinoWindow SetMinimized(bool minimized)
        {
            Log($".SetMinimized({minimized})");
            Minimized = minimized;
            return this;
        }

        public PhotinoWindow SetTitle(string title)
        {
            Log($".SetTitle({title})");
            Title = title;
            return this;
        }

        public PhotinoWindow SetTop(int top)
        {
            Log($".SetTop({top})");
            Top = top;
            return this;
        }

        public PhotinoWindow SetTopMost(bool topMost)
        {
            Log($".SetTopMost({topMost})");
            TopMost = topMost;
            return this;
        }

        ///<summary>In Pixels. Leave at -1 to initialize window to OS default size.</summary>
        public PhotinoWindow SetWidth(int width)
        {
            Log($".SetWidth({width})");
            Width = width;
            return this;
        }

        ///<summary>Browser control zoom level. e.g. 100 = 100%</summary>
        public PhotinoWindow SetZoom(int zoom)
        {
            Log($".SetZoom({zoom})");
            Zoom = zoom;
            return this;
        }

        ///<summary>Overrides Left and Top properties and relise on the OS to position the window.</summary>
        public PhotinoWindow SetUseOsDefaultLocation(bool useOsDefault)
        {
            Log($".SetUseOsDefaultLocation({useOsDefault})");
            UseOsDefaultLocation = useOsDefault;
            return this;
        }

        ///<summary>Overrides Height and Width properties and relise on the OS to determine the initial size of the window.</summary>
        public PhotinoWindow SetUseOsDefaultSize(bool useOsDefault)
        {
            Log($".SetUseOsDefaultSize({useOsDefault})");
            UseOsDefaultSize = useOsDefault;
            return this;
        }





        //NON-FLUENT METHODS - CAN ONLY BE CALLED AFTER WINDOW IS INITIALIZED

        //ONE OF THESE 2 METHODS *MUST* BE CALLED TO CREATE THE WINDOW
        ///<summary>Creates the main (first) window. All other windows must be created with CreateChildWindow(). 
        ///This is the only Window that runs a message loop.</summary>
        public void WaitForClose()
        {
            //fill in the fixed size array of custom scheme names
            var i = 0;
            foreach (var name in CustomSchemeNames.Take(16))
            {
                _startupParameters.CustomSchemeNames[i] = name;
                i++;
            }

            var errors = _startupParameters.GetParamErrors();
            if (errors.Count == 0)
            {
                OnWindowCreating();
                _nativeInstance = Photino_ctor(_startupParameters);
                OnWindowCreated();

                if (!_messageLoopIsStarted)
                {
                    _messageLoopIsStarted = true;
                    Photino_WaitForExit(_nativeInstance);       //start the message loop. there can only be 1 message loop for all windows.
                }
            }
            else
            {
                var formattedErrors = "\n";
                foreach (var error in errors)
                    formattedErrors += error + "\n";

                throw new ArgumentException($"Startup Parameters Are Not Valid: {formattedErrors}");
            }
        }



        //WHAT ABOUT RESTORE() ? for windows it's just unsettng minimized or maximized

        public void Close()
        {
            Log(".Close()");
            Photino_Close(_nativeInstance);
        }

        ///<summary>Opens a native alert window with a title and message</summary>
        public void OpenAlertWindow(string title, string message)
        {
            Log($".OpenAlertWindow({title}, {message})");

            // Bug:
            // Closing the message shown with the OpenAlertWindow
            // method closes the sender window as well.
            Photino_ShowMessage(_nativeInstance, title, message, /* MB_OK */ 0);
        }

        ///<summary>Send a message to the window's JavaScript context</summary>
        public void SendWebMessage(string message)
        {
            Log($".SendWebMessage({message})");
            Photino_SendWebMessage(_nativeInstance, message);
        }



        private void Log(string message)
        {
            if (LogVerbosity < 1) return;
            Console.WriteLine($"Photino.NET: \"{Title ?? "PhotinoWindow"}\"{message}");
        }
    }
}