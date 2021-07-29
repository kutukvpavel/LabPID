using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using NamedPipeWrapper;
using NamedPipeWrapper.IO;

namespace LabPID
{
    public class NamedPipeService
    {
        public const string PipeName = "LabPID_Profile_Broadcast";
        public static NamedPipeService Instance { get; } = new NamedPipeService();

        private readonly NamedPipeServer<string> _Server = new NamedPipeServer<string>(PipeName);

        private NamedPipeService()
        {
            _Server.Start();
        }

        ~NamedPipeService()
        {
            _Server.Stop();
        }

        public string TemperatureFormat { get; set; } = "T{0:F2}";
        public string CustomFormat { get; set; } = "C{0}";

        public void BroadcastTemperature(float t)
        {
            _Server.PushMessage(string.Format(TemperatureFormat, t));
        }

        public void BroadcastCustomCommand(string s)
        {
            _Server.PushMessage(string.Format(CustomFormat, s));
        }
    }
}
