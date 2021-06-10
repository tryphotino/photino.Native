using System;
using System.Drawing;

namespace PhotinoNET
{
    class Program
    {
        private static bool _logEvents = true;

        private static PhotinoWindow mainWindow;

        [STAThread]
        static void Main(string[] args)
        {
            mainWindow = new PhotinoWindow()
                .SetIconFile("wwwroot/photino-logo.ico")
                .SetTitle("My Photino Window")

                //.Load(new Uri("https://google.com"))
                .Load("wwwroot/main.html")
                //.LoadRawString("<h1>Hello Photino!</h1>")

                //.SetChromeless(true)
                //.SetFullScreen(true)
                //.SetMaximized(true)
                //.SetMinimized(true)
                //.SetResizable(false)
                //.SetTopMost(true)
                //.SetUseOsDefaultLocation(true)
                //.SetUseOsDefaultSize(true)
                .SetZoom(150)

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
        }


        //These are the event handlers I'm hooking up
        private static void WindowLocationChanged(object sender, Point location)
        {
            if (_logEvents)
                Console.WriteLine($" Window Location Changed To Left: {location.X}  Top: {location.Y}");
        }

        private static void WindowSizeChanged(object sender, Size size)
        {
            if (_logEvents)
                Console.WriteLine($" Window Size Changed To Height: {size.Height}  Width: {size.Width}");
        }

        private static void MessageReceivedFromWindow(object sender, string message)
        {
            if (_logEvents)
                Console.WriteLine($" New Message Recieved From Window: {message}");

            if (string.Compare(message, "random-window", true) == 0)
            {
                var x = new PhotinoWindow()
                    .RegisterWebMessageReceivedHandler(MessageReceivedFromWindow)
                    .Load("wwwroot/child.html");

                x.CreateChildWindow();
            }
            else if (string.Compare(message, "zoom-in", true) == 0)
            {
                mainWindow.Zoom += 5;
                Console.WriteLine($"Zoom in to: {mainWindow.Zoom}");
            }
            else if (string.Compare(message, "zoom-out", true) == 0)
            {
                mainWindow.Zoom -= 5;
                Console.WriteLine($"Zoom out to: {mainWindow.Zoom}");
            }
            else
            {
                throw new Exception($"WTF? Unknown message '{message}'");
            }

        }

        private static void WindowIsClosing(object sender, EventArgs e)
        {
            if (_logEvents)
                Console.WriteLine($" Window Is Closing");
        }

        private static void WindowCreated(object sender, EventArgs e)
        {
            if (_logEvents)
                Console.WriteLine($" Window was Created");
        }

        private static void WindowCreating(object sender, EventArgs e)
        {
            if (_logEvents)
                Console.WriteLine($" Window Creating");
        }
    }
}
