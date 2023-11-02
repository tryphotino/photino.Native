using System;
using System.Drawing;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;

namespace PhotinoNET
{
    class Program
    {
        private static readonly bool _logEvents = true;
        private static int _windowNumber = 1;

        private static PhotinoWindow mainWindow;

        [STAThread]
        static void Main(string[] args)
        {
            try
            {
                FluentStyle();
                //PropertyInitStyle();
            }
            catch (Exception ex)
            {
                Log(null, ex.Message);
                Console.ReadKey();
            }
        }

        private static void FluentStyle()
        {
            var iconFile = PhotinoWindow.IsWindowsPlatform
                ? "wwwroot/photino-logo.ico"
                : "wwwroot/photino-logo.png";

            string browserInit = string.Empty;

            if (PhotinoWindow.IsWindowsPlatform)
            {
                //Windows example for WebView2
                browserInit = "--disable-web-security --hide-scrollbars ";
            }
            else if (PhotinoWindow.IsMacOsPlatform)
            {
                //Mac example for Webkit on Cocoa
                browserInit = JsonSerializer.Serialize(new
                {
                    standardFontFamily = "Helvetica Neue",
                    defaultFontSize = 22
                });
            }
            else if (PhotinoWindow.IsLinuxPlatform)
            {
                //Linux example for Webkit2Gtk
                browserInit = JsonSerializer.Serialize(new
                {
                    set_enable_encrypted_media = true,
                    //set_default_font_size = 48,
                    //set_enable_developer_extras = true,
                    set_default_font_family = "monospace"
                });
            }

            mainWindow = new PhotinoWindow()
                //.Load(new Uri("https://google.com"))
                //.Load("https://duckduckgo.com/?t=ffab&q=user+agent+&ia=answer")
                .Load("wwwroot/main.html")
                //.Load("wwwroot/index.html")
                //.LoadRawString("<h1>Hello Photino!</h1>")

                //Window settings
                .SetIconFile(iconFile)
                .SetTitle($"My Photino Window {_windowNumber++}")
                //.SetChromeless(true)
                //.SetFullScreen(true)
                //.SetMaximized(true)
                //.SetMaxSize(640, 480)
                //.SetMinimized(true)
                //.SetMinSize(320, 240)
                //.SetResizable(false)
                //.SetTopMost(true)
                //.SetUseOsDefaultLocation(false)
                .SetUseOsDefaultSize(false)
                //.Center()
                //.SetSize(new Size(800, 600))
                .SetHeight(600)
                .SetWidth(800)
                //.SetLocation(new Point(50, 50))
                //.SetTop(50)
                //.SetLeft(50)
                //.MoveTo(new Point(10, 10))
                //.MoveTo(20, 20)
                //.Offset(new Point(150, 150))
                //.Offset(250, 250)

                //Browser settings
                //.SetContextMenuEnabled(false)
                //.SetDevToolsEnabled(false)
                .SetGrantBrowserPermissions(true)
                //.SetZoom(150)

                //Browser startup flags
                .SetBrowserControlInitParameters(browserInit)
                //.SetUserAgent("Custom Photino User Agent")
                //.SetMediaAutoplayEnabled(true)
                //.SetFileSystemAccessEnabled(true)
                //.SetWebSecurityEnabled(true)
                //.SetJavascriptClipboardAccessEnabled(true)
                //.SetMediaStreamEnabled(true)
                //.SetSmoothScrollingEnabled(true)
                //.SetTemporaryFilesPath(@"C:\Temp")

                .RegisterCustomSchemeHandler("app", AppCustomSchemeUsed)

                .RegisterWindowCreatingHandler(WindowCreating)
                .RegisterWindowCreatedHandler(WindowCreated)
                .RegisterLocationChangedHandler(WindowLocationChanged)
                .RegisterSizeChangedHandler(WindowSizeChanged)
                .RegisterWebMessageReceivedHandler(MessageReceivedFromWindow)
                .RegisterWindowClosingHandler(WindowIsClosing)
                .RegisterFocusInHandler(WindowFocusIn)
                .RegisterFocusOutHandler(WindowFocusOut)

                .SetLogVerbosity(_logEvents ? 2 : 0);

            mainWindow.WaitForClose();

            mainWindow.Center(); //will never happen - this is blocking.
        }

        private static void PropertyInitStyle()
        {
            var iconFile = RuntimeInformation.IsOSPlatform(OSPlatform.Windows)
                ? "wwwroot/photino-logo.ico"
                : "wwwroot/photino-logo.png";

            var browserInit = RuntimeInformation.IsOSPlatform(OSPlatform.Windows)
                ? "--disable-web-security --hide-scrollbars "           //Windows example for WebView2
                : RuntimeInformation.IsOSPlatform(OSPlatform.Windows)
                    ? "{ 'set_enable_encrypted_media': true }"          //Linux example for Webkit2Gtk
                    : "{ 'setLegacyEncryptedMediaAPIEnabled': true }";  //Mac example for Webkit on Cocoa

            mainWindow = new PhotinoWindow
            {
                //StartUrl = "https://google.com",
                //StartUrl = "https://duckduckgo.com/?t=ffab&q=user+agent+&ia=answer",
                StartUrl = "wwwroot/main.html",
                //StartString = "<h1>Hello Photino!</h1>",

                //Window settings
                IconFile = iconFile,
                Title = $"My Photino Window {_windowNumber++}",
                //Chromeless = true,
                //FullScreen = true,
                //Maximized = true,
                MaxWidth = 640,
                MaxHeight = 480,
                //MaxSize = new Point(640, 480),
                //Minimized = true,
                MinWidth = 320,
                MinHeight = 240,
                //MinSize = new Point(320, 240),
                //Resizable = false,
                //TopMost = true,
                UseOsDefaultLocation = false,
                UseOsDefaultSize = false,
                Centered = true,
                Size = new Size(800, 600),
                Height = 600,
                Width = 800,
                Location = new Point(50, 50),
                Top = 50,
                Left = 50,

                //Browser settings
                ContextMenuEnabled = false,
                DevToolsEnabled = false,
                GrantBrowserPermissions = false,
                Zoom = 150,

                //Browser startup flags
                BrowserControlInitParameters = browserInit,
                UserAgent = "Custom Photino User Agent",
                MediaAutoplayEnabled = true,
                FileSystemAccessEnabled = true,
                WebSecurityEnabled = true,
                JavascriptClipboardAccessEnabled = true,
                MediaStreamEnabled = true,
                SmoothScrollingEnabled = true,
                //TemporaryFilesPath = @"C:\Temp",

                WindowCreatingHandler = WindowCreating,
                WindowCreatedHandler = WindowCreated,
                WindowLocationChangedHandler = WindowLocationChanged,
                WindowSizeChangedHandler = WindowSizeChanged,
                WindowMaximizedHandler = WindowMaximized,
                WindowRestoredHandler = WindowRestored,
                WindowMinimizedHandler = WindowMinimized,
                WebMessageReceivedHandler = MessageReceivedFromWindow,
                WindowClosingHandler = WindowIsClosing,
                WindowFocusInHandler = WindowFocusIn,
                WindowFocusOutHandler = WindowFocusOut,

                LogVerbosity = _logEvents ? 2 : 0,
            };

            //Can this be done with a property? 
            mainWindow.RegisterCustomSchemeHandler("app", AppCustomSchemeUsed);

            mainWindow.WaitForClose();

            Console.WriteLine("Done Blocking!");
        }



        //These are the event handlers I'm hooking up
        private static Stream AppCustomSchemeUsed(object sender, string scheme, string url, out string contentType)
        {
            Log(sender, $"Custom scheme '{scheme}' was used.");
            var currentWindow = sender as PhotinoWindow;

            contentType = "text/javascript";

            var js =
@"
(() =>{
    window.setTimeout(() => {
        const title = document.getElementById('Title');
        const lineage = document.getElementById('Lineage');
        title.innerHTML = "

            + $"'{currentWindow.Title}';" + "\n"

            + $"        lineage.innerHTML = `PhotinoWindow Id: {currentWindow.Id} <br>`;" + "\n";

            //show lineage of this window
            var p = currentWindow.Parent;
            while (p != null)
            {
                js += $"        lineage.innerHTML += `Parent Id: {p.Id} <br>`;" + "\n";
                p = p.Parent;
            }

            js +=
@"        alert(`🎉 Dynamically inserted JavaScript.`);
    }, 1000);
})();
";

            return new MemoryStream(Encoding.UTF8.GetBytes(js));
        }

        private static void MessageReceivedFromWindow(object sender, string message)
        {
            Log(sender, $"MessageReceivedFromWindow Callback Fired.");

            var currentWindow = sender as PhotinoWindow;
            if (string.Compare(message, "child-window", true) == 0)
            {
                var iconFile = RuntimeInformation.IsOSPlatform(OSPlatform.Windows)
                    ? "wwwroot/photino-logo.ico"
                    : "wwwroot/photino-logo.png";

                var x = new PhotinoWindow(currentWindow)
                    .SetTitle($"Child Window {_windowNumber++}")
                    //.SetIconFile(iconFile)
                    .Load("wwwroot/main.html")

                    .SetUseOsDefaultLocation(true)
                    .SetHeight(600)
                    .SetWidth(800)

                    .SetGrantBrowserPermissions(false)

                    .RegisterWindowCreatingHandler(WindowCreating)
                    .RegisterWindowCreatedHandler(WindowCreated)
                    .RegisterLocationChangedHandler(WindowLocationChanged)
                    .RegisterSizeChangedHandler(WindowSizeChanged)
                    .RegisterWebMessageReceivedHandler(MessageReceivedFromWindow)
                    .RegisterWindowClosingHandler(WindowIsClosing)

                    .RegisterCustomSchemeHandler("app", AppCustomSchemeUsed)

                    .SetTemporaryFilesPath(currentWindow.TemporaryFilesPath)
                    .SetLogVerbosity(_logEvents ? 2 : 0);

                x.WaitForClose();

                //x.Center();           //WaitForClose() is non-blocking for child windows
                //x.SetHeight(800);
                //x.Close();
            }
            else if (string.Compare(message, "zoom-in", true) == 0)
            {
                currentWindow.Zoom += 5;
                Log(sender, $"Zoom: {currentWindow.Zoom}");
            }
            else if (string.Compare(message, "zoom-out", true) == 0)
            {
                currentWindow.Zoom -= 5;
                Log(sender, $"Zoom: {currentWindow.Zoom}");
            }
            else if (string.Compare(message, "center", true) == 0)
            {
                currentWindow.Center();
            }
            else if (string.Compare(message, "close", true) == 0)
            {
                currentWindow.Close();
            }
            else if (string.Compare(message, "clearbrowserautofill", true) == 0)
            {
                currentWindow.ClearBrowserAutoFill();
            }
            else if (string.Compare(message, "minimize", true) == 0)
            {
                currentWindow.SetMinimized(!currentWindow.Minimized);
            }
            else if (string.Compare(message, "maximize", true) == 0)
            {
                currentWindow.SetMaximized(!currentWindow.Maximized);
            }
            else if (string.Compare(message, "setcontextmenuenabled", true) == 0)
            {
                currentWindow.SetContextMenuEnabled(!currentWindow.ContextMenuEnabled);
            }
            else if (string.Compare(message, "setdevtoolsenabled", true) == 0)
            {
                currentWindow.SetDevToolsEnabled(!currentWindow.DevToolsEnabled);
            }
            else if (string.Compare(message, "setgrantbrowserpermissions", true) == 0)
            {
                currentWindow.SetGrantBrowserPermissions(!currentWindow.GrantBrowserPermissions);
            }
            else if (string.Compare(message, "seticonfile", true) == 0)
            {
                var iconFile = RuntimeInformation.IsOSPlatform(OSPlatform.Windows)
                    ? "wwwroot/photino-logo.ico"
                    : "wwwroot/photino-logo.png";

                currentWindow.SetIconFile(iconFile);
            }
            else if (string.Compare(message, "setposition", true) == 0)
            {
                currentWindow.SetLeft(currentWindow.Left + 5);
                currentWindow.SetTop(currentWindow.Top + 5);
            }
            else if (string.Compare(message, "setresizable", true) == 0)
            {
                currentWindow.SetResizable(!currentWindow.Resizable);
            }
            else if (string.Compare(message, "setsize-up", true) == 0)
            {
                currentWindow.SetHeight(currentWindow.Height + 5);
                currentWindow.SetWidth(currentWindow.Width + 5);
            }
            else if (string.Compare(message, "setsize-down", true) == 0)
            {
                currentWindow.SetHeight(currentWindow.Height - 5);
                currentWindow.SetWidth(currentWindow.Width - 5);
            }
            else if (string.Compare(message, "settitle", true) == 0)
            {
                currentWindow.SetTitle(currentWindow.Title + "*");
            }
            else if (string.Compare(message, "settopmost", true) == 0)
            {
                currentWindow.SetTopMost(!currentWindow.Topmost);
            }
            else if (string.Compare(message, "setfullscreen", true) == 0)
            {
                currentWindow.SetFullScreen(!currentWindow.FullScreen);
            }
            else if (string.Compare(message, "showproperties", true) == 0)
            {
                var properties = GetPropertiesDisplay(currentWindow);
                currentWindow.ShowMessage("Settings", properties);
            }
            else if (string.Compare(message, "sendWebMessage", true) == 0)
            {
                currentWindow.SendWebMessage("web message");
            }
            else if (string.Compare(message, "setMinSize", true) == 0)
            {
                currentWindow.SetMinSize(320, 240);
            }
            else if (string.Compare(message, "setMaxSize", true) == 0)
            {
                currentWindow.SetMaxSize(640, 480);
            }
            else if (string.Compare(message, "toastNotification", true) == 0)
            {
                currentWindow.SendNotification("Toast Title", " Toast message!");
            }
            else if (string.Compare(message, "showOpenFile", true) == 0)
            {
                var results = currentWindow.ShowOpenFile(filters: new[]{
                    ("All files", new [] {"*.*"}),
                    ("Text files", new [] {"*.txt"}),
                    ("Image files", new [] {"*.png", "*.jpg", "*.jpeg"}),
                    ("PDF files", new [] {"*.pdf"}),
                    ("CSharp files", new [] { "*.cs" })
                });
                if (results.Length > 0)
                    currentWindow.ShowMessage("Open File", string.Join(Environment.NewLine, results));
                else
                    currentWindow.ShowMessage("Open File", "No file chosen", icon: PhotinoDialogIcon.Error);
            }
            else if (string.Compare(message, "showOpenFolder", true) == 0)
            {
                var results = currentWindow.ShowOpenFolder(multiSelect: true);
                if (results.Length > 0)
                    currentWindow.ShowMessage("Open Folder", string.Join(Environment.NewLine, results));
                else
                    currentWindow.ShowMessage("Open Folder", "No folder chosen", icon: PhotinoDialogIcon.Error);
            }
            else if (string.Compare(message, "showSaveFile", true) == 0)
            {
                var result = currentWindow.ShowSaveFile();
                if (result != null)
                    currentWindow.ShowMessage("Save File", result);
                else
                    currentWindow.ShowMessage("Save File", "File not saved", icon: PhotinoDialogIcon.Error);
            }
            else if (string.Compare(message, "showMessage", true) == 0)
            {
                var result = currentWindow.ShowMessage("Title", "Testing...");
            }
            else
                throw new Exception($"Unknown message '{message}'");
        }

        private static void WindowCreating(object sender, EventArgs e)
        {
            Log(sender, "WindowCreating Callback Fired.");
        }

        private static void WindowCreated(object sender, EventArgs e)
        {
            Log(sender, "WindowCreated Callback Fired.");
        }

        private static void WindowLocationChanged(object sender, Point location)
        {
            Log(sender, $"WindowLocationChanged Callback Fired.  Left: {location.X}  Top: {location.Y}");
        }

        private static void WindowSizeChanged(object sender, Size size)
        {
            Log(sender, $"WindowSizeChanged Callback Fired.  Height: {size.Height}  Width: {size.Width}");
        }

        private static void WindowMaximized(object sender, EventArgs e)
        {
            Log(sender, $"{nameof(WindowMaximized)} Callback Fired.");
        }

        private static void WindowRestored(object sender, EventArgs e)
        {
            Log(sender, $"{nameof(WindowRestored)} Callback Fired.");
        }

        private static void WindowMinimized(object sender, EventArgs e)
        {
            Log(sender, $"{nameof(WindowMinimized)} Callback Fired.");
        }

        private static bool WindowIsClosing(object sender, EventArgs e)
        {
            Log(sender, "WindowIsClosing Callback Fired.");
            return false;   //return true to block closing of the window
        }

        private static void WindowFocusIn(object sender, EventArgs e)
        {
            Log(sender, "WindowFocusIn Callback Fired.");
        }

        private static void WindowFocusOut(object sender, EventArgs e)
        {
            Log(sender, "WindowFocusOut Callback Fired.");
        }




        private static string GetPropertiesDisplay(PhotinoWindow currentWindow)
        {
            var sb = new StringBuilder();
            sb.AppendLine($"Title: {currentWindow.Title}");
            sb.AppendLine($"Zoom: {currentWindow.Zoom}");
            sb.AppendLine();
            sb.AppendLine($"ContextMenuEnabled: {currentWindow.ContextMenuEnabled}");
            sb.AppendLine($"DevToolsEnabled: {currentWindow.DevToolsEnabled}");
            sb.AppendLine($"GrantBrowserPermissions: {currentWindow.GrantBrowserPermissions}");
            sb.AppendLine();
            sb.AppendLine($"Top: {currentWindow.Top}");
            sb.AppendLine($"Left: {currentWindow.Left}");
            sb.AppendLine($"Height: {currentWindow.Height}");
            sb.AppendLine($"Width: {currentWindow.Width}");
            sb.AppendLine();
            sb.AppendLine($"Resizable: {currentWindow.Resizable}");
            sb.AppendLine($"Screen DPI: {currentWindow.ScreenDpi}");
            sb.AppendLine($"Topmost: {currentWindow.Topmost}");
            sb.AppendLine($"Maximized: {currentWindow.Maximized}");
            sb.AppendLine($"Minimized: {currentWindow.Minimized}");

            return sb.ToString();
        }

        private static void Log(object sender, string message)
        {
            if (!_logEvents) return;
            var currentWindow = sender as PhotinoWindow;
            var windowTitle = currentWindow == null ? string.Empty : currentWindow.Title;
            Console.WriteLine($"-Client App: \"{windowTitle ?? "title?"}\" {message}");
        }
    }
}
