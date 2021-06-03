using System;

namespace PhotinoNET
{
    class Program
    {
        [STAThread]
        static void Main(string[] args)
        {
            new PhotinoWindow()
                .RegisterWebMessageReceivedHandler(NewWindowMessageDelegate)
                .Load("wwwroot/main.html")
                .WaitForClose();
        }

        static void NewWindowMessageDelegate(object sender, string message)
        {
            Console.WriteLine("About to open second window");
            new PhotinoWindow()
                .Load("wwwroot/second.html");
                //.WaitForClose();
            Console.WriteLine("The above code will never return");
        }
    }
}
