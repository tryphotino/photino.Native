using System;
using System.Drawing;

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
                .RegisterWindowClosingHandler(WindowIsClosing);
                
            mainWindow.WaitForClose();

            mainWindow.Center(); //will never happen - this is blocking.
        }


        //These are the event handlers I'm hooking up
        private static void MessageReceivedFromWindow(object sender, string message)
        {
            if (_logEvents)
            {
                var currentWindow = sender as PhotinoWindow;
                var windowTitle = currentWindow == null ? string.Empty : currentWindow.Title;
                Console.WriteLine($" Window '{windowTitle}' Received New Message: {message}");

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
                        .RegisterWindowClosingHandler(WindowIsClosing);

                    x.WaitForClose();

                    Console.WriteLine($"****** Window Title: {x.Title}");

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
        }



        private static void WindowCreating(object sender, EventArgs e)
        {
            if (_logEvents)
            {
                var currentWindow = sender as PhotinoWindow;
                var windowTitle = currentWindow == null ? string.Empty : currentWindow.Title;
                Console.WriteLine($" Window '{windowTitle}' Creating");
            }
        }

        private static void WindowCreated(object sender, EventArgs e)
        {
            if (_logEvents)
            {
                var currentWindow = sender as PhotinoWindow;
                var windowTitle = currentWindow == null ? string.Empty : currentWindow.Title;
                Console.WriteLine($" Window '{windowTitle}' was Created");
            }
        }

        private static void WindowLocationChanged(object sender, Point location)
        {
            if (_logEvents)
            {
                var currentWindow = sender as PhotinoWindow;
                var windowTitle = currentWindow == null ? string.Empty : currentWindow.Title;
                Console.WriteLine($" Window '{windowTitle}' Location Changed To Left: {location.X}  Top: {location.Y}");
            }
        }

        private static void WindowSizeChanged(object sender, Size size)
        {
            if (_logEvents)
            {
                var currentWindow = sender as PhotinoWindow;
                var windowTitle = currentWindow == null ? string.Empty : currentWindow.Title;
                Console.WriteLine($" Window '{windowTitle}' Size Changed To Height: {size.Height}  Width: {size.Width}");
            }
        }

        private static bool WindowIsClosing(object sender, EventArgs e)
        {
            if (_logEvents)
            {
                var currentWindow = sender as PhotinoWindow;
                var windowTitle = currentWindow == null ? string.Empty : currentWindow.Title;
                Console.WriteLine($" Window '{windowTitle}' Is Closing");
            }

            return false;   //return true to block closing of the window
        }
    }
}
