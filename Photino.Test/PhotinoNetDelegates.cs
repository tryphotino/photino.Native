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
        public void OnLocationChanged(Point location)
        {
            LocationChangedHandler?.Invoke(this, location);
        }



        public event EventHandler<Size> SizeChangedHandler;
        public PhotinoWindow RegisterSizeChangedHandler(EventHandler<Size> handler)
        {
            SizeChangedHandler += handler;
            return this;
        }
        public void OnSizeChanged(Size size)
        {
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



        public event EventHandler WindowClosing;
        public PhotinoWindow RegisterWindowClosingHandler(EventHandler handler)
        {
            WindowClosing += handler;
            return this;
        }
        public void OnWindowClosing()
        {
            WindowClosing?.Invoke(this, null);
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
        public IDictionary<string, CustomSchemeDelegate> CustomSchemeHandlers { get; } = new Dictionary<string, CustomSchemeDelegate>();
        public delegate Stream CustomSchemeDelegate(string url, out string contentType);
    }
}
