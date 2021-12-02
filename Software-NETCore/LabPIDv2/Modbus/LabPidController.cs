using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using NModbus;
using NModbus.Data;
using NModbus.Device;
using NModbus.Message;
using NModbus.Utility;
using NModbus.IO;
using NModbus.SerialPortStream;
using RJCP.IO.Ports;

namespace LabPIDv2.Modbus
{
    public sealed class LabPidController : INotifyPropertyChanged, IDisposable
    {
        public event PropertyChangedEventHandler PropertyChanged;

        public LabPidController(byte slaveAddress, string portName, int baudRate = 9600)
        {
            SlaveAddress = slaveAddress;
            _port = new SerialPortStream(portName, baudRate);
            _modbusMaster = new ModbusFactory().CreateRtuMaster(new SerialPortStreamAdapter(_port));
        }

        public byte SlaveAddress { get; }
        public bool IsDisposed { get; private set; } = false;

        public async void ReadProperty<T>(IRegisterProperty<T> p)
        {
            if (p.Type == PropertyTypes.Coil)
            {

            }
            else
            {
                ushort[] res = p.Type switch
                {
                    PropertyTypes.Input => await _modbusMaster.ReadInputRegistersAsync(SlaveAddress, p.Address, p.Length),
                    PropertyTypes.Holding => await _modbusMaster.ReadHoldingRegistersAsync(SlaveAddress, p.Address, p.Length),
                    _ => throw new ArgumentException("Invalid register type.")
                };
                p.SetValue(res);
            }
        }
        public void SetProperty<T>(IRegisterProperty<T> p)
        {

        }

        public void Dispose()
        {
            if (IsDisposed) return;
            try
            {
                _modbusMaster.Dispose();
            }
            catch (ObjectDisposedException)
            { }
            finally
            {
                IsDisposed = true;
            }
        }

        //Private

        private IModbusSerialMaster _modbusMaster;
        private SerialPortStream _port;
    }
}
