using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;

namespace PhotinoNET
{
    public partial class PhotinoWindow
    {
        //FLUENT EVENT HANDLER REGISTRATION
        public event EventHandler<Point> LocationChangedHandler;
        public PhotinoWindow RegisterLocationChangedHandler(EventHandler<Point> handler)
        {
            LocationChangedHandler += handler;
            return this;
        }
        public void OnLocationChanged(int left, int top)
        {
            var location = new Point(left, top);
            LocationChangedHandler?.Invoke(this, location);
        }



        public event EventHandler<Size> SizeChangedHandler;
        public PhotinoWindow RegisterSizeChangedHandler(EventHandler<Size> handler)
        {
            SizeChangedHandler += handler;
            return this;
        }
        public void OnSizeChanged(int width, int height)
        {
            var size = new Size(width, height);
            SizeChangedHandler?.Invoke(this, size);
        }



        public event EventHandler<string> WebMessageReceived;
        public PhotinoWindow RegisterWebMessageReceivedHandler(EventHandler<string> handler)
        {
            WebMessageReceived += handler;
            return this;
        }
        public void OnWebMessageReceived(string message)
        {
            WebMessageReceived?.Invoke(this, message);
        }


        public delegate bool NetClosingDelegate(object sender, EventArgs e);
        public event NetClosingDelegate WindowClosing;
        public PhotinoWindow RegisterWindowClosingHandler(NetClosingDelegate handler)
        {
            WindowClosing += handler;
            return this;
        }
        public byte OnWindowClosing()
        {
            //C++ handles bool values as a single byte, C# uses 4 bytes
            byte noClose = 0;
            var doNotClose = WindowClosing?.Invoke(this, null);
            if (doNotClose ?? false)
                noClose = 1;

            return noClose;
        }



        public event EventHandler WindowCreating;
        public PhotinoWindow RegisterWindowCreatingHandler(EventHandler handler)
        {
            WindowCreating += handler;
            return this;
        }
        public void OnWindowCreating()
        {
            WindowCreating?.Invoke(this, null);
        }



        public event EventHandler WindowCreated;
        public PhotinoWindow RegisterWindowCreatedHandler(EventHandler handler)
        {
            WindowCreated += handler;
            return this;
        }
        public void OnWindowCreated()
        {
            WindowCreated?.Invoke(this, null);
        }




        ///<summary>Custom schemes (other than 'http', 'https' and 'file') must be handled by creating the handlers manually</summary>
        public IDictionary<string, NetCustomSchemeDelegate> CustomSchemeHandlers { get; } = new Dictionary<string, NetCustomSchemeDelegate>();
        public delegate Stream NetCustomSchemeDelegate(string url, out string contentType);
    }
}
