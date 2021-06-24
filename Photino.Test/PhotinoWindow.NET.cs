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
        //PRIVATE FIELDS
        ///<summary>Parameters set to Photino.Native to start a new instance of a Photino.Native window.</summary>
        private PhotinoNativeParameters _startupParameters = new PhotinoNativeParameters
        {
            Resizable = true,   //these values can't be initialized within the struct itself
            Zoom = 100,
            CustomSchemeNames = new string[16],
            TemporaryFilesPath = IsWindowsPlatform
                ? Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Photino")
                : null,
            Title = "Photino",
            UseOsDefaultLocation = true,
            UseOsDefaultSize = true,
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

        ///<summary>Windows platform only. Gets the handle of the native window. Throws exception if called from platform other than Windows.</summary>
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

        ///<summary>Gets list of information for each monitor from the native window.</summary>
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

        ///<summary>Gets Information for the primary display from the native window.</summary>
        public Monitor MainMonitor
        {
            get
            {
                if (_nativeInstance == IntPtr.Zero)
                    throw new ApplicationException("The Photino window hasn't been initialized yet.");

                return Monitors.First();
            }
        }

        ///<summary>Gets dots per inch for the primary display from the native window.</summary>
        public uint ScreenDpi
        {
            get
            {
                if (_nativeInstance == IntPtr.Zero)
                    throw new ApplicationException("The Photino window hasn't been initialized yet.");

                return Photino_GetScreenDpi(_nativeInstance);
            }
        }

        ///<summary>Gets a unique GUID to identify the native window. Not used by Photino.</summary>
        public Guid Id { get; } = Guid.NewGuid();




        //READ-WRITE PROPERTIES
        ///<summary>When true, the native window will appear without a title bar or border. The user can supply both, as well as handle dragging and resizing. Default is false. Throws exception if called after native window is initalized.</summary>
        public bool Chromeless
        {
            get
            {
                if (_nativeInstance == IntPtr.Zero)
                    return _startupParameters.Chromeless;

                throw new ApplicationException("Chromeless can only be read before the native window is instantiated.");
            }
            set
            {
                if (_nativeInstance == IntPtr.Zero)
                {
                    if (_startupParameters.Chromeless != value)
                        _startupParameters.Chromeless = value;
                }
                else
                    throw new ApplicationException("Chromeless can only be set before the native window is instantiated.");
            }
        }

        ///<summary>When true, the native window will cover the entire screen area - kiosk style. Default is false.</summary>
        public bool FullScreen
        {
            get
            {
                if (_nativeInstance == IntPtr.Zero)
                    return _startupParameters.FullScreen;

                Photino_GetSize(_nativeInstance, out int width, out int height);
                return width == MainMonitor.WorkArea.Width
                    && height == MainMonitor.WorkArea.Height;
            }
            set
            {
                if (FullScreen != value)
                {
                    if (_nativeInstance == IntPtr.Zero)
                        _startupParameters.FullScreen = value;
                    else
                        Photino_SetSize(_nativeInstance, MainMonitor.WorkArea.Width, MainMonitor.WorkArea.Height);
                }
            }
        }

        ///<summary>Gets or Sets the native window Height in pixels. Default is 0. See also UseOsDefaultSize.</summary>
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
        ///<summary>Gets or sets the icon on the native window title bar on Windows and Linux. Must be a local file, not a URL. Default is none.</summary>
        public string IconFile
        {
            get => _iconFile;
            set
            {
                if (_iconFile != value)
                {
                    if (!File.Exists(value))
                    {
                        var absolutePath = $"{System.AppContext.BaseDirectory}{value}";
                        if (!File.Exists(absolutePath))
                            throw new ArgumentException($"Icon file: {value} does not exist.");
                    }

                    _iconFile = value;


                    if (_nativeInstance == IntPtr.Zero)
                        _startupParameters.WindowIconFile = _iconFile;
                    else
                        Photino_SetIconFile(_nativeInstance, _iconFile);
                }
            }
        }

        ///<summary>Gets or sets the native window Left (X) and Top coordinates (Y) in pixels. Default is 0,0. See also UseOsDefaultLocation.</summary>
        public Point Location
        {
            get
            {
                if (_nativeInstance == IntPtr.Zero)
                    return new Point(_startupParameters.Left, _startupParameters.Top);

                Photino_GetPosition(_nativeInstance, out int left, out int top);
                return new Point(left, top);
            }
            set
            {
                if (Location.X != value.X || Location.Y != value.Y)
                {
                    if (_nativeInstance == IntPtr.Zero)
                    {
                        _startupParameters.Left = value.X;
                        _startupParameters.Top = value.Y;
                    }
                    else
                        Photino_SetPosition(_nativeInstance, value.X, value.Y);
                }
            }
        }

        ///<summary>Gets or sets the native window Left (X) coordinate in pixels. Default is 0.</summary>
        public int Left
        {
            get => Location.X;
            set
            {
                if (Location.X != value)
                    Location = new Point(value, Location.Y);
            }
        }

        ///<summary>Gets or sets whether the native window is maximized. Default is false.</summary>
        public bool Maximized
        {
            get
            {
                if (_nativeInstance == IntPtr.Zero)
                    return _startupParameters.Maximized;

                bool maximized = false;
                Photino_IsMaximized(_nativeInstance, maximized);
                return maximized;
            }
            set
            {
                if (Maximized != value)
                {
                    if (_nativeInstance == IntPtr.Zero)
                        _startupParameters.Maximized = value;
                    else
                        Photino_IsMaximized(_nativeInstance, value);
                }
            }
        }

        ///<summary>Gets or sets whether the native window is minimized (hidden). Default is false.</summary>
        public bool Minimized
        {
            get
            {
                if (_nativeInstance == IntPtr.Zero)
                    return _startupParameters.Minimized;

                bool minimized = false;
                Photino_IsMinimized(_nativeInstance, minimized);
                return minimized;
            }
            set
            {
                if (Minimized != value)
                {
                    if (_nativeInstance == IntPtr.Zero)
                        _startupParameters.Minimized = value;
                    else
                        Photino_SetMinimized(_nativeInstance, value);
                }
            }
        }

        ///<summary>Gets or sets whether the native window can be resized by the user. Default is true.</summary>
        public bool Resizable
        {
            get
            {
                if (_nativeInstance == IntPtr.Zero)
                    return _startupParameters.Resizable;

                Photino_GetResizable(_nativeInstance, out bool resizeable);
                return resizeable;
            }
            set
            {
                if (Resizable != value)
                {
                    if (_nativeInstance == IntPtr.Zero)
                        _startupParameters.FullScreen = value;
                    else
                        Photino_SetResizable(_nativeInstance, value);
                }
            }
        }

        ///<summary>Gets or sets the native window Width and Height in pixels. Default is 0,0. See also UseOsDefaultSize.</summary>
        public Size Size
        {
            get
            {
                if (_nativeInstance == IntPtr.Zero)
                    return new Size(_startupParameters.Width, _startupParameters.Height);

                Photino_GetSize(_nativeInstance, out int width, out int height);
                return new Size(width, height);
            }
            set
            {
                if (Size.Width != value.Width || Size.Height != value.Height)
                {
                    if (_nativeInstance == IntPtr.Zero)
                    {
                        _startupParameters.Height = value.Height;
                        _startupParameters.Width = value.Width;
                    }
                    else
                        Photino_SetSize(_nativeInstance, value.Width, value.Height);
                }
            }
        }

        ///<summary>Windows platform only. Gets or sets the local path to store temp files for browser control. Default is user's AppDataLocal folder. Throws exception if platform is not Windows.</summary>
        public string TemporaryFilesPath
        {
            get
            {
                return _startupParameters.TemporaryFilesPath;
            }
            set
            {
                if (_startupParameters.TemporaryFilesPath != value)
                {
                    if (_nativeInstance != IntPtr.Zero)
                        throw new ApplicationException($"{nameof(_startupParameters.TemporaryFilesPath)} cannot be changed after Photino Window is initialized");
                    _startupParameters.TemporaryFilesPath = value;
                }
            }
        }

        ///<summary>Gets or sets the native window title. Default is "Photino".</summary>
        public string Title
        {
            get
            {
                if (_nativeInstance == IntPtr.Zero)
                    return _startupParameters.Title;

                return Marshal.PtrToStringAuto(Photino_GetTitle(_nativeInstance));
            }
            set
            {
                if (Title != value)
                {
                    // Due to Linux/Gtk platform limitations, the window title has to be no more than 31 chars
                    if (value.Length > 31 && IsLinuxPlatform)
                        value = value.Substring(0, 31);

                    if (_nativeInstance == IntPtr.Zero)
                        _startupParameters.Title = value;
                    else
                        Photino_SetTitle(_nativeInstance, value);
                }
            }
        }

        ///<summary>Gets or sets the native window Top (Y) coordinate in pixels. Default is 0. See also UseOsDefaultLocation.</summary>
        public int Top
        {
            get => Location.Y;
            set
            {
                if (Location.Y != value)
                    Location = new Point(Location.X, value);
            }
        }

        ///<summary>Gets or sets whehter the native window is always at the top of the z-order. Default is false.</summary>
        public bool TopMost
        {
            get
            {
                if (_nativeInstance == IntPtr.Zero)
                    return _startupParameters.Topmost;

                Photino_GetTopmost(_nativeInstance, out bool topmost);
                return topmost;
            }
            set
            {
                if (TopMost != value)
                {
                    if (_nativeInstance == IntPtr.Zero)
                        _startupParameters.Topmost = value;
                    else
                        Photino_SetTopmost(_nativeInstance, value ? 1 : 0);
                }
            }
        }

        ///<summary>When true the native window starts up at the OS Default location. Overrides Left (X) and Top (Y) properties. Default is true. Throws Application Exception if accessed after native window initialization.</summary>
        public bool UseOsDefaultLocation
        {
            get
            {
                if (_nativeInstance == IntPtr.Zero)
                    return _startupParameters.UseOsDefaultLocation;

                throw new ApplicationException("UseOsDefaultLocation can only be read before the native window is instantiated.");
            }
            set
            {
                if (_nativeInstance == IntPtr.Zero)
                {
                    if (UseOsDefaultLocation != value)
                        _startupParameters.UseOsDefaultLocation = value;
                }
                else
                    throw new ApplicationException("UseOsDefaultLocation can only be set before the native window is instantiated.");
            }
        }

        ///<summary>When true the native window starts at the OS Default size. Overrides Height and Width properties. Default is true. Throws Application Exception if accessed after native window initialization.</summary>
        public bool UseOsDefaultSize
        {
            get
            {
                if (_nativeInstance == IntPtr.Zero)
                    return _startupParameters.UseOsDefaultSize;

                throw new ApplicationException("UseOsDefaultSize can only be read before the native window is instantiated.");
            }
            set
            {
                if (_nativeInstance == IntPtr.Zero)
                {
                    if (UseOsDefaultSize != value)
                        _startupParameters.UseOsDefaultSize = value;
                }
                else
                    throw new ApplicationException("UseOsDefaultSize can only be set before the native window is instantiated.");
            }
        }

        ///<summary>Gets or Sets the native window width in pixels. Default is 0. See also UseOsDefaultSize.</summary>
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

        ///<summary>Gets or sets the native browser control zoom. e.g. 100 = 100%  Default is 100;</summary>
        public int Zoom
        {
            get
            {
                if (_nativeInstance == IntPtr.Zero)
                    return _startupParameters.Zoom;

                Photino_GetZoom(_nativeInstance, out int zoom);
                return zoom;
            }
            set
            {
                if (Zoom != value)
                {
                    if (_nativeInstance == IntPtr.Zero)
                        _startupParameters.Zoom = value;
                    else
                       Photino_SetZoom(_nativeInstance, value);
                }
            }
        }



        ///<summary>Gets or sets the amound of logging to standard output (console window) by Photino.Native. 0 = Critical Only, 1 = Critical and Warning, 2 = Verbose, >2 = All Details. Default is 2.</summary>
        public int LogVerbosity { get; set; } = 2;



        //CONSTRUCTOR
        ///<summary>.NET class that represents a native window with a native browser control taking up the entire client area.</summary>
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
        
        ///<summary>Loads the specified file or url into the browser control. Load() or LoadString() must be called before native window is initialized.</summary>
        public PhotinoWindow Load(Uri uri)
        {
            Log($".Load({uri})");
            if (_nativeInstance == IntPtr.Zero)
                _startupParameters.StartUrl = uri.ToString();
            else
                Photino_NavigateToUrl(_nativeInstance, uri.ToString());
            return this;
        }

        ///<summary>Loads the specified file or url into the browser control. Load() or LoadString() must be called before native window is initialized.</summary>
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

        ///<summary>Loads a raw string (typically HTML) into the browser control. Load() or LoadString() must be called before native window is initialized.</summary>
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



        ///<summary>Centers the native window in the primary display. If called prior to window initialization, overrides Left (X) and Top (Y) properties. See also UseOsDefaultLocation.</summary>
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

        ///<summary>Moves the native window to the specified location on the screen in pixels using a Point.</summary>
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

        ///<summary>Moves the native window to the specified location on the screen in pixels using Left (X) and Top (Y) properties.</summary>
        public PhotinoWindow MoveTo(int left, int top, bool allowOutsideWorkArea = false)
        {
            Log($".MoveTo({left}, {top}, {allowOutsideWorkArea})");
            return MoveTo(new Point(left, top), allowOutsideWorkArea);
        }

        ///<summary>Moves the native window relative to its current location on the screen using a Point.</summary>
        public PhotinoWindow Offset(Point offset)
        {
            Log($".Offset({offset})");
            var location = Location;
            int left = location.X + offset.X;
            int top = location.Y + offset.Y;
            return MoveTo(left, top);
        }

        ///<summary>Moves the native window relative to its current location on the screen in pixels using Left (X) and Top (Y) properties.</summary>
        public PhotinoWindow Offset(int left, int top)
        {
            Log($".Offset({left}, {top})");
            return Offset(new Point(left, top));
        }



        ///<summary>When true, the native window will appear without a title bar or border. The user must then supply both, as well as handle dragging and resizing if desired. Default is false.</summary>
        public PhotinoWindow SetChromeless(bool chromeless)
        {
            Log($".SetChromeless({chromeless})");
            if (_nativeInstance != IntPtr.Zero)
                throw new ApplicationException("Chromeless setting cannot be used on an unitialized window.");

            _startupParameters.Chromeless = chromeless;
            return this;
        }

        ///<summary>When true, the native window will cover the entire screen area - kiosk style. Default is false.</summary>
        public PhotinoWindow SetFullScreen(bool fullScreen)
        {
            Log($".SetFullScreen({fullScreen})");
            FullScreen = fullScreen;
            return this;
        }

        ///<summary>Sets the native window Height in pixels. Default is 0. See also UseOsDefaultSize.</summary>
        public PhotinoWindow SetHeight(int height)
        {
            Log($".SetHeight({height})");
            Height = height;
            return this;
        }

        ///<summary>sets the icon on the native window title bar on Windows and Linux. Must be a local file, not a URL. Default is none.</summary>
        public PhotinoWindow SetIconFile(string iconFile)
        {
            Log($".SetIconFile({iconFile})");
            IconFile = iconFile;
            return this;
        }

        ///<summary>Moves the native window to a new Left (X) coordinate in pixels. Default is 0. See also UseOsDefaultLocation.</summary>
        public PhotinoWindow SetLeft(int left)
        {
            Log($".SetLeft({Left})");
            Left = left;
            return this;
        }

        ///<summary>When true, the native window can be resized by the user. Default is true.</summary>
        public PhotinoWindow SetResizable(bool resizable)
        {
            Log($".SetResizable({resizable})");
            Resizable = resizable;
            return this;
        }

        ///<summary>Sets the native window Width and Height in pixels. Default is 0,0. See also UseOsDefaultSize.</summary>
        public PhotinoWindow SetSize(Size size)
        {
            Log($".SetSize({size})");
            Size = size;
            return this;
        }

        ///<summary>Moves the native window to the new Left (X) and Top (Y) coordinates in pixels. Default is 0,0. See also UseOsDefaultLocation.</summary>
        public PhotinoWindow SetLocation(Point location)
        {
            Log($".SetLocation({location})");
            Location = location;
            return this;
        }

        ///<summary>Sets the level of logging to standard output (console window) by Photino.Native. 0 = Critical Only, 1 = Critical and Warning, 2 = Verbose, >2 = All Details. Default is 2.</summary>
        public PhotinoWindow SetLogVerbosity(int verbosity)
        {
            Log($".SetLogVerbosity({verbosity})");
            LogVerbosity = verbosity;
            return this;
        }

        ///<summary>When true, the native window is maximized. Default is false.</summary>
        public PhotinoWindow SetMaximized(bool maximized)
        {
            Log($".SetMaximized({maximized})");
            Maximized = maximized;
            return this;
        }

        ///<summary>When true, the native window is minimized (hidden). Default is false.</summary>
        public PhotinoWindow SetMinimized(bool minimized)
        {
            Log($".SetMinimized({minimized})");
            Minimized = minimized;
            return this;
        }

        ///<summary>Windows platform only. Sets the local path to store temp files for browser control. Default is user's AppDataLocal. Throws exception if called on platform other than Windows.</summary>
        public PhotinoWindow SetTemporaryFilesPath(string tempFilesPath)
        {
            Log($".SetTemporaryFilesPath({tempFilesPath})");
            TemporaryFilesPath = tempFilesPath;
            return this;
        }

        ///<summary>Sets the native window title. Default is "Photino".</summary>
        public PhotinoWindow SetTitle(string title)
        {
            Log($".SetTitle({title})");
            Title = title;
            return this;
        }

        ///<summary>Moves the native window to a new Top (Y) coordinate in pixels. Default is 0. See also UseOsDefaultLocation.</summary>
        public PhotinoWindow SetTop(int top)
        {
            Log($".SetTop({top})");
            Top = top;
            return this;
        }

        ///<summary>When true, the native window is always at the top of the z-order. Default is false.</summary>
        public PhotinoWindow SetTopMost(bool topMost)
        {
            Log($".SetTopMost({topMost})");
            TopMost = topMost;
            return this;
        }

        ///<summary>Native window Width in pixels. Default is 0. See also UseOsDefaultSize.</summary>
        public PhotinoWindow SetWidth(int width)
        {
            Log($".SetWidth({width})");
            Width = width;
            return this;
        }

        ///<summary>Sets the browser control zoom level in the native window. e.g. 100 = 100%  Default is 100.</summary>
        public PhotinoWindow SetZoom(int zoom)
        {
            Log($".SetZoom({zoom})");
            Zoom = zoom;
            return this;
        }

        ///<summary>Overrides Left (X) and Top (Y) properties and relies on the OS to position the window intially. Default is true.</summary>
        public PhotinoWindow SetUseOsDefaultLocation(bool useOsDefault)
        {
            Log($".SetUseOsDefaultLocation({useOsDefault})");
            UseOsDefaultLocation = useOsDefault;
            return this;
        }

        ///<summary>Overrides Height and Width properties and relies on the OS to determine the initial size of the window. Default is true.</summary>
        public PhotinoWindow SetUseOsDefaultSize(bool useOsDefault)
        {
            Log($".SetUseOsDefaultSize({useOsDefault})");
            UseOsDefaultSize = useOsDefault;
            return this;
        }





        //NON-FLUENT METHODS - CAN ONLY BE CALLED AFTER WINDOW IS INITIALIZED

        //ONE OF THESE 2 METHODS *MUST* BE CALLED TO CREATE THE WINDOW
        
        ///<summary>Initializes the main (first) native window and blocks until the window is closed. Also used to initialize child windows in which case it does not block. Only the main native window runs a message loop.</summary>
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
                try  //All C++ exceptions will bubble up to here.
                {
                    _nativeInstance = Photino_ctor(_startupParameters);
                }
                catch (Exception ex)
                {
                    int lastError = 0;
                    if (IsWindowsPlatform)
                        lastError = Marshal.GetLastWin32Error();

                    Log($"***\n{ex.Message}\n{ex.StackTrace}\nError #{lastError}");
                    throw new ApplicationException($"Native code exception. Error # {lastError}  See inner exception for details.", ex);
                }
                OnWindowCreated();

                if (!_messageLoopIsStarted)
                {
                    _messageLoopIsStarted = true;
                    try
                    {
                        Photino_WaitForExit(_nativeInstance);       //start the message loop. there can only be 1 message loop for all windows.
                    }
                    catch (Exception ex)
                    {
                        int lastError = 0;
                        if (IsWindowsPlatform)
                            lastError = Marshal.GetLastWin32Error();

                        Log($"***\n{ex.Message}\n{ex.StackTrace}\nError #{lastError}");
                        throw new ApplicationException($"Native code exception. Error # {lastError}  See inner exception for details.", ex);
                    }
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

        ///<summary>Closes the native window. Throws an exception if the window is not initialized.</summary>
        public void Close()
        {
            Log(".Close()");
            if (_nativeInstance == IntPtr.Zero)
                throw new ApplicationException("Close cannot be called until after the Photino window is initialized.");
            Photino_Close(_nativeInstance);
        }

        ///<summary>Opens a native alert (MessageBox) on the native window with a title and message. Throws an exception if the window is not initialized.</summary>
        public void OpenAlertWindow(string title, string message)
        {
            Log($".OpenAlertWindow({title}, {message})");
            if (_nativeInstance == IntPtr.Zero)
                throw new ApplicationException("OpenAlertWindow cannot be called until after the Photino window is initialized.");
            Photino_ShowMessage(_nativeInstance, title, message, /* MB_OK */ 0);
        }

        ///<summary>Send a message to the ative window's native browser control's JavaScript context. Throws an exception if the window is not initialized.</summary>
        public void SendWebMessage(string message)
        {
            Log($".SendWebMessage({message})");
            if (_nativeInstance == IntPtr.Zero)
                throw new ApplicationException("SendWebMessage cannot be called until after the Photino window is initialized.");
            Photino_SendWebMessage(_nativeInstance, message);
        }



        private void Log(string message)
        {
            if (LogVerbosity < 1) return;
            Console.WriteLine($"Photino.NET: \"{Title ?? "PhotinoWindow"}\"{message}");
        }
    }
}