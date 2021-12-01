using NamedPipeWrapper;
using Newtonsoft.Json;
using System;

namespace LabPID
{
    public class NamedPipeService : IDisposable
    {
        private class DataPacket
        {
            public DataPacket(float t, float s, GpioDescriptor g)
            {
                Temperature = t;
                Setpoint = s;
                Gpio = g;
            }

            public float Temperature { get; set; }
            public float Setpoint { get; set; }
            public GpioDescriptor Gpio { get; set; }

            public string GetJson()
            {
                return JsonConvert.SerializeObject(this);
            }
        }

        public const string PipeName = "LabPID_Profile_Broadcast";
        public static NamedPipeService Instance { get; } = new NamedPipeService();

        private readonly NamedPipeServer<string> _Server = new NamedPipeServer<string>(PipeName);

        private bool _Disposed = false;

        private NamedPipeService()
        {
            _Server.Start();
        }

        ~NamedPipeService()
        {
            Dispose();
        }

        public void Broadcast(float currentTemp, float setpoint, GpioDescriptor gpio)
        {
            _Server.PushMessage(new DataPacket(currentTemp, setpoint, gpio).GetJson());
        }

        public void Dispose()
        {
            if (!_Disposed)
            {
                _Server.Stop();
                _Disposed = true;
            }
        }
    }
}
