using System;
using System.Drawing;
using System.IO;
using System.Text;

namespace PhotinoNET
{
    class Program
    {
        private static bool _logEvents = true;
        private static int _windowNumber = 1;

        private static PhotinoWindow mainWindow;

        [STAThread]
        static void Main(string[] args)
        {
            try
            {
                mainWindow = new PhotinoWindow()
                    .SetIconFile("wwwroot/photino-logo.ico")
                    .SetTitle($"My Photino Window {_windowNumber++}")

                    //.Load(new Uri("https://google.com"))
                    .Load("wwwroot/main.html")
                    //.LoadRawString("<h1>Hello Photino!</h1>")

                    //.SetChromeless(true)
                    //.SetFullScreen(true)
                    //.SetMaximized(true)
                    //.SetMinimized(true)
                    //.SetResizable(false)
                    //.SetTopMost(true)
                    .SetUseOsDefaultLocation(true)
                    //.SetUseOsDefaultSize(true)
                    //.SetZoom(150)

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

                    .RegisterWindowCreatingHandler(WindowCreating)
                    .RegisterWindowCreatedHandler(WindowCreated)
                    .RegisterLocationChangedHandler(WindowLocationChanged)
                    .RegisterSizeChangedHandler(WindowSizeChanged)
                    .RegisterWebMessageReceivedHandler(MessageReceivedFromWindow)
                    .RegisterWindowClosingHandler(WindowIsClosing)

                    //.SetTemporaryFilesPath(@"C:\Temp")

                    .SetLogVerbosity(_logEvents ? 2 : 0);

                mainWindow.RegisterCustomSchemeHandler("app", AppCustomSchemeUsed);

                mainWindow.WaitForClose();

                mainWindow.Center(); //will never happen - this is blocking.
            }
            catch (Exception ex)
            {
                Log(null, ex.Message);
                Console.ReadKey();
            }
        }


        //These are the event handlers I'm hooking up
        private static Stream AppCustomSchemeUsed(string scheme, out string contentType)
        {
            Log(null, $"Custom scheme '{scheme}' was used.");

            contentType = "text/javascript";
            return new MemoryStream(Encoding.UTF8.GetBytes(@"
                                (() =>{
                                    window.setTimeout(() => {
                                        alert(`🎉 Dynamically inserted JavaScript.`);
                                    }, 1000);
                                })();
                            "));
        }

        private static void MessageReceivedFromWindow(object sender, string message)
        {
            Log(sender, $"MessageRecievedFromWindow Callback Fired.");

            var currentWindow = sender as PhotinoWindow;
            if (string.Compare(message, "random-window", true) == 0)
            {
                var x = new PhotinoWindow()
                    .SetTitle($"Child Window {_windowNumber++}")
                    .Load("wwwroot/child.html")

                    .SetUseOsDefaultLocation(true)
                    .SetHeight(600)
                    .SetWidth(800)

                    .RegisterWindowCreatingHandler(WindowCreating)
                    .RegisterWindowCreatedHandler(WindowCreated)
                    .RegisterLocationChangedHandler(WindowLocationChanged)
                    .RegisterSizeChangedHandler(WindowSizeChanged)
                    .RegisterWebMessageReceivedHandler(MessageReceivedFromWindow)
                    .RegisterWindowClosingHandler(WindowIsClosing)

                    .SetTemporaryFilesPath(currentWindow.TemporaryFilesPath)
                    .SetLogVerbosity(_logEvents ? 2 : 0);

                x.WaitForClose();

                //x.Center();           //WaitForClose() is non-blocking for child windows
                //x.SetHeight(800);
                //x.Close();
            }
            else if (string.Compare(message, "zoom-in", true) == 0)
            {
                if (currentWindow != null)
                    currentWindow.Zoom += 5;
            }
            else if (string.Compare(message, "zoom-out", true) == 0)
            {
                if (currentWindow != null)
                    currentWindow.Zoom -= 5;
            }
            else if (string.Compare(message, "close", true) == 0)
            {
                if (currentWindow != null)
                    currentWindow.Close();
            }
            else
                throw new Exception($"WTF? Unknown message '{message}'");
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

        private static bool WindowIsClosing(object sender, EventArgs e)
        {
            Log(sender, "WindowIsClosing Callback Fired.");
            return false;   //return true to block closing of the window
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
