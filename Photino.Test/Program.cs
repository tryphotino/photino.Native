using System;
using System.Drawing;

namespace PhotinoNET
{
    class Program
    {
        private static bool _logEvents = false;

        [STAThread]
        static void Main(string[] args)
        {
            new PhotinoWindow()
                .SetHeight(600)
                .SetIconFile("wwwroot/myicon.png")
                .SetLeft(50)
                .SetResizable(true)
                .SetSize(new Size(800, 600))
                .SetLocation(new Point(50, 50))
                .SetMaximized(false)
                .SetMinimized(false)
                .SetTitle("My Photino Window")
                .SetTop(50)
                .SetTopMost(false)
                .SetWidth(800)
                .Load(new Uri("wwwroot/main.html"))
                .Load("wwwroot/main.html")
                .LoadRawString("<h1>Hello Photino!</h1>")
                .MoveTo(new Point(10, 10))
                .MoveTo(20, 20)
                .Offset(new Point(15, 15))
                .Offset(15, 15)
                .Center()

                .RegisterLocationChangedHandler(WindowLocationChanged)
                .RegisterSizeChangedHandler(WindowSizeChanged)
                .RegisterWebMessageReceivedHandler(MessageReceivedFromWindow)
                .RegisterWindowClosingHandler(WindowIsClosing)
                .RegisterWindowCreatedHandler(WindowCreated)
                .RegisterWindowCreatingHandler(WindowCreating)
                
                .WaitForClose();
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
            
            new PhotinoWindow()
                .RegisterWebMessageReceivedHandler(MessageReceivedFromWindow)
                .Load("wwwroot/second.html")
                .CreateChildWindow();
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
