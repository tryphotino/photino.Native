using System;
using System.IO;

namespace PhotinoNET
{
    public partial class PhotinoWindow
    {
        private static IntPtr _hInstance = IntPtr.Zero;
        private readonly IntPtr _nativeInstance;
        private bool _wasShown = false;
        public event EventHandler<string> WebMessageReceived;

        public PhotinoWindow()
        {
            //if (_hInstance == IntPtr.Zero)
            //{
            //    _hInstance = Marshal.GetHINSTANCE(typeof(PhotinoWindow).Module);
            //    Photino_register_win32(_hInstance);
            //}

            var p = new PhotinoParameters
            {
                StartUrl = "http://google.com",
                WebMessageReceivedHandler = OnWebMessageReceived,

                CustomSchemeNames = new string[32]
            };

            //p.CustomSchemeNames[0] = "Mike";

            _nativeInstance = Photino_ctor(p);
        }

        public PhotinoWindow Show()
        {
            //Photino_Show(_nativeInstance);
            // Is used to indicate that the window was shown to the user at least once. Some
            // functionality like registering custom scheme handlers can only be executed on
            // the native window before it was shown the first time.
            _wasShown = true;
            return this;
        }

        public void WaitForClose()
        {
            Photino_WaitForExit(_nativeInstance);
        }

        public PhotinoWindow Load(Uri uri)
        {
            // Navigation only works after the window was shown once.
            if (!_wasShown) Show();
            //Photino_NavigateToUrl(_nativeInstance, uri.ToString());
            return this;
        }

        public PhotinoWindow Load(string path)
        {
            if (path.Contains("http://") || path.Contains("https://"))
                return Load(new Uri(path));

            // Open a file resource string path
            string absolutePath = Path.GetFullPath(path);

            // For bundled app it can be necessary to consider
            // the app context base directory. Check there too.
            if (File.Exists(absolutePath) == false)
            {
                absolutePath = $"{AppContext.BaseDirectory}/{path}";
                if (File.Exists(absolutePath) == false)
                {
                    Console.WriteLine($"File \"{path}\" could not be found.");
                    return this;
                }
            }

            return Load(new Uri(absolutePath, UriKind.Absolute));
        }

        public PhotinoWindow RegisterWebMessageReceivedHandler(EventHandler<string> handler)
        {
            WebMessageReceived += handler;
            return this;
        }

        private void OnWebMessageReceived(string message)
        {
            WebMessageReceived?.Invoke(this, message);
        }
    }
}