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
            Resizable = true, 
            Height = -1,
            Width = -1,
            Left = -1,
            Top = -1,
        };
        
        //Pointers to the type and instance.
        private static IntPtr _nativeType = IntPtr.Zero;
        private IntPtr _nativeInstance;


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


        private int _left = -1;
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
            get => _title;
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


        private int _top = -1;
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




        ///<summary>0 = Critical Only, 1 = Critical and Warning, 2 = Verbose, >2 = All Details</summary>
        public int LogVerbosity { get; set; }



        //CONSTRUCTOR
        public PhotinoWindow()
        {
            //This only has to be done once
            if (_nativeType == IntPtr.Zero)
            {
                _nativeType = Marshal.GetHINSTANCE(typeof(PhotinoWindow).Module);
                Photino_register_win32(_nativeType);
            }

            //Wire up handlers from C++ to C#
            _startupParameters.ClosingHandler = (ClosingDelegate)OnWindowClosing;
            _startupParameters.ResizedHandler = (ResizedDelegate)OnSizeChanged;
            _startupParameters.MovedHandler = (MovedDelegate)OnLocationChanged;
            _startupParameters.WebMessageReceivedHandler = (WebMessageReceivedDelegate)OnWebMessageReceived;
        }



        //FLUENT METHODS FOR INITIALIZING STARTUP PARAMETERS FOR NEW WINDOWS

        //CAN ALSO BE CALLED AFTER INITIALIZATION TO SET VALUES

        //ONE OF THESE 3 METHODS *MUST* BE CALLED PRIOR TO CALLING WAITFORCLOSE() OR CREATECHILDWINDOW()
        ///<summary>Loads the specified file or url into the browser control</summary>
        public PhotinoWindow Load(Uri uri)
        {
            if (LogVerbosity > 1)
                Console.WriteLine($"Executing: \"{Title ?? "PhotinoWindow"}\".Load(Uri uri)");

            if (_nativeInstance == IntPtr.Zero)
                _startupParameters.StartUrl = uri.ToString();
            else
                Photino_NavigateToUrl(_nativeInstance, uri.ToString());

            return this;
        }

        ///<summary>Loads the specified file or url into the browser control</summary>
        public PhotinoWindow Load(string path)
        {
            if (LogVerbosity > 1)
                Console.WriteLine($"Executing: \"{Title ?? "PhotinoWindow"}\".Load(string path)");

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
                    Console.WriteLine($"File \"{path}\" could not be found.");
                    return this;
                }
            }

            return Load(new Uri(absolutePath, UriKind.Absolute));
        }

        ///<summary>Loads a raw string (typically HTML) into the browser control</summary>
        public PhotinoWindow LoadRawString(string content)
        {
            if (LogVerbosity > 1)
                Console.WriteLine($"Executing: \"{Title ?? "PhotinoWindow"}\".LoadRawString(string content)");

            if (_nativeInstance == IntPtr.Zero)
                _startupParameters.StartString = content;
            else
                Photino_NavigateToString(_nativeInstance, content);

            return this;
        }


        ///<summary>Centers the window in the main monitor. If called prior to window initialization, overrides Left and Top properties.</summary>
        public PhotinoWindow Center()
        {
            if (LogVerbosity > 1)
                Console.WriteLine($"Executing: \"{Title ?? "PhotinoWindow"}\".Center()");

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
            if (LogVerbosity > 1)
                Console.WriteLine($"Executing: \"{Title ?? "PhotinoWindow"}\".MoveTo(Point location, bool allowOutsideWorkArea)");

            if (LogVerbosity > 2)
            {
                Console.WriteLine($"Current location: {Location}");
                Console.WriteLine($"New location: {location}");
            }

            // If the window is outside of the work area,
            // recalculate the position and continue.
            if (allowOutsideWorkArea == false)
            {
                int horizontalWindowEdge = location.X + Width;
                int verticalWindowEdge = location.Y + Height;

                int horizontalWorkAreaEdge;
                int verticalWorkAreaEdge;

                if (_nativeInstance == IntPtr.Zero)
                {
                    horizontalWorkAreaEdge = Console.LargestWindowWidth;
                    verticalWorkAreaEdge = Console.LargestWindowHeight;
                }
                else
                {
                    horizontalWorkAreaEdge = MainMonitor.WorkArea.Width;
                    verticalWorkAreaEdge = MainMonitor.WorkArea.Height;
                }

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
            if (LogVerbosity > 1)
                Console.WriteLine($"Executing: \"{Title ?? "PhotinoWindow"}\".MoveTo(int left, int top, bool allowOutsideWorkArea)");

            return MoveTo(new Point(left, top), allowOutsideWorkArea);
        }

        ///<summary>Moves the window relative to its current location on the screen using a Point</summary>
        public PhotinoWindow Offset(Point offset)
        {
            if (LogVerbosity > 1)
                Console.WriteLine($"Executing: \"{Title ?? "PhotinoWindow"}\".Offset(Point offset)");

            var location = Location;

            int left = location.X + offset.X;
            int top = location.Y + offset.Y;

            return MoveTo(left, top);
        }

        ///<summary>Moves the window relative to its current location on the screen using left and top coordinates</summary>
        public PhotinoWindow Offset(int left, int top)
        {
            if (LogVerbosity > 1)
                Console.WriteLine($"Executing: \"{Title ?? "PhotinoWindow"}\".Offset(int left, int top)");

            return Offset(new Point(left, top));
        }
        ///<summary>Centers the window on the main monitor work area.</summary>




        public PhotinoWindow SetChromeless(bool chromeless)
        {
            if (LogVerbosity > 1)
                Console.WriteLine($"Executing: \"{Title ?? "PhotinoWindow"}\".SetChromeless(bool chromeless)");

            if (_nativeInstance != IntPtr.Zero)
                throw new ApplicationException("Chromeless setting cannot be used on an unitialized window.");

            _startupParameters.Chromeless = chromeless;

            return this;
        }

        public PhotinoWindow SetFullScreen(bool fullScreen)
        {
            if (LogVerbosity > 1)
                Console.WriteLine($"Executing: \"{Title ?? "PhotinoWindow"}\".SetFullScreen(bool fullScreen)");

            FullScreen = fullScreen;

            return this;
        }

        ///<summary>In Pixels. Leave at -1 to initialize window to OS default size.</summary>
        public PhotinoWindow SetHeight(int height)
        {
            if (LogVerbosity > 1)
                Console.WriteLine($"Executing: \"{Title ?? "PhotinoWindow"}\".SetHeight(int height)");

            Height = height;

            return this;
        }

        ///<summary>Must be a local file, not a URL.</summary>
        public PhotinoWindow SetIconFile(string iconFile)
        {
            if (LogVerbosity > 1)
                Console.WriteLine($"Executing: \"{Title ?? "PhotinoWindow"}\".SetIconFile(string iconFile)");

            IconFile = iconFile;

            return this;
        }

        ///<summary>In Pixels. Leave at -1 to initialize window to OS default position.</summary>
        public PhotinoWindow SetLeft(int left)
        {
            if (LogVerbosity > 1)
                Console.WriteLine($"Executing: \"{Title ?? "PhotinoWindow"}\".SetLeft(int Left)");

            Left = left;

            return this;
        }

        public PhotinoWindow SetResizable(bool resizable)
        {
            if (LogVerbosity > 1)
                Console.WriteLine($"Executing: \"{Title ?? "PhotinoWindow"}\".SetIsTopMost(bool topMost)");

            Resizable = resizable;

            return this;
        }

        ///<summary>In Pixels. Leave at -1, -1 to initialize window to OS default size.</summary>
        public PhotinoWindow SetSize(Size size)
        {
            if (LogVerbosity > 1)
                Console.WriteLine($"Executing: \"{Title ?? "PhotinoWindow"}\".SetSize(Size size)");

            Size = size;

            return this;
        }

        ///<summary>In Pixels. Leave at -1, -1 to initialize window to OS default position.</summary>
        public PhotinoWindow SetLocation(Point location)
        {
            if (LogVerbosity > 1)
                Console.WriteLine($"Executing: \"{Title ?? "PhotinoWindow"}\".SetLocation(Point location)");

            Location = location;

            return this;
        }

        public PhotinoWindow SetMaximized(bool maximized)
        {
            if (LogVerbosity > 1)
                Console.WriteLine($"Executing: \"{Title ?? "PhotinoWindow"}\".SetMaximized(bool maximized)");

            Maximized = maximized;

            return this;
        }

        public PhotinoWindow SetMinimized(bool minimized)
        {
            if (LogVerbosity > 1)
                Console.WriteLine($"Executing: \"{Title ?? "PhotinoWindow"}\".SetMinimized(bool minimized)");

            Minimized = minimized;

            return this;
        }

        public PhotinoWindow SetTitle(string title)
        {
            if (LogVerbosity > 1)
                Console.WriteLine($"Executing: \"{Title ?? "PhotinoWindow"}\".SetTitle(string title)");

            Title = title;

            return this;
        }

        ///<summary>In Pixels. Leave at -1 to initialize window to OS default position.</summary>
        public PhotinoWindow SetTop(int top)
        {
            if (LogVerbosity > 1)
                Console.WriteLine($"Executing: \"{Title ?? "PhotinoWindow"}\".SetTop(int Top)");

            Top = top;

            return this;
        }

        public PhotinoWindow SetTopMost(bool topMost)
        {
            if (LogVerbosity > 1)
                Console.WriteLine($"Executing: \"{Title ?? "PhotinoWindow"}\".SetIsTopMost(bool topMost)");

            TopMost = topMost;

            return this;
        }

        ///<summary>In Pixels. Leave at -1 to initialize window to OS default size.</summary>
        public PhotinoWindow SetWidth(int width)
        {
            if (LogVerbosity > 1)
                Console.WriteLine($"Executing: \"{Title ?? "PhotinoWindow"}\".SetWidth(int width)");

            Width = width;

            return this;
        }




        //NON-FLUENT METHODS - CAN ONLY BE CALLED AFTER WINDOW IS INITIALIZED

        //ONE OF THESE 2 METHODS *MUST* BE CALLED TO CREATE THE WINDOW
        ///<summary>Creates the main (first) window. All other windows must be created with CreateChildWindow(). 
        ///This is the only Window that runs a message loop.</summary>
        public void WaitForClose()
        {
            if (_nativeInstance != IntPtr.Zero) 
                throw new ArgumentException("Photino Window has already been started.");

            var errors = _startupParameters.GetParamErrors();
            if (errors.Count == 0)
            {
                _nativeInstance = Photino_ctor(_startupParameters);
                Photino_WaitForExit(_nativeInstance);
            }
            else
            {
                var formattedErrors = "\n";
                foreach (var error in errors)
                    formattedErrors += error + "\n";

                throw new ArgumentException($"Startup Parameters Are Not Valid: {formattedErrors}");
            }
        }
        ///<summary>Creates a child window. The main window must be created first by calling WaitForClose()</summary>
        public void CreateChildWindow()
        {
            if (_nativeInstance != IntPtr.Zero) 
                throw new ArgumentException("Photino Child Window has already been started.");

            var errors = _startupParameters.GetParamErrors();
            if (errors.Count == 0)
            {
                _nativeInstance = Photino_ctor(_startupParameters);
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

        ///<summary>Opens a native alert window with a title and message</summary>
        public void OpenAlertWindow(string title, string message)
        {
            if (LogVerbosity > 1)
                Console.WriteLine($"Executing: \"{Title ?? "PhotinoWindow"}\".OpenAlertWindow(string title, string message)");

            // Bug:
            // Closing the message shown with the OpenAlertWindow
            // method closes the sender window as well.
            Photino_ShowMessage(_nativeInstance, title, message, /* MB_OK */ 0);
        }

        ///<summary>Send a message to the window's JavaScript context</summary>
        public void SendWebMessage(string message)
        {
            if (LogVerbosity > 1)
                Console.WriteLine($"Executing: \"{Title ?? "PhotinoWindow"}\".SendWebMessage(string message)");

            Photino_SendWebMessage(_nativeInstance, message);
        }
    }
}