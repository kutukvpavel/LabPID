using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using NModbus;
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

        public async Task ReadRegisterProperty<T>(IRegisterProperty<T> p)
        {
            try
            {
                ushort[] res = p.Type switch
                {
                    RegisterTypes.Input => await _modbusMaster.ReadInputRegistersAsync(SlaveAddress, p.Address, p.Length),
                    RegisterTypes.Holding => await _modbusMaster.ReadHoldingRegistersAsync(SlaveAddress, p.Address, p.Length),
                    _ => throw new ArgumentException("Invalid register type.")
                };
                p.SetValue(res);
            }
            catch (SlaveException ex)
            {
                ModbusExceptionHelper(ex);
            }
        }
        public async Task ReadDiscreteProperty(IDiscreteProperty p)
        {
            try
            {
                p.Value = p.Type switch
                {
                    DiscreteTypes.Coil => (await _modbusMaster.ReadCoilsAsync(SlaveAddress, p.Address, 1))[0],
                    DiscreteTypes.Input => (await _modbusMaster.ReadInputsAsync(SlaveAddress, p.Address, 1))[0],
                    _ => throw new ArgumentException("Invalid discrete property type.")
                };
            }
            catch (SlaveException ex)
            {
                ModbusExceptionHelper(ex);
            }
        }
        public async Task SetRegisterProperty<T>(IRegisterProperty<T> p, T newValue)
        {
            try
            {
                switch (p.Type)
                {
                    case RegisterTypes.Holding:
                        await _modbusMaster.WriteMultipleRegistersAsync(SlaveAddress, p.Address, newValue.ConvertToRegs());
                        break;
                    default:
                        throw new ArgumentException("Invalid register type for writing to.");
                }
            }
            catch (SlaveException ex)
            {
                ModbusExceptionHelper(ex);
            }
            await ReadRegisterProperty(p);
        }
        public async Task SetDiscreteProperty(IDiscreteProperty p, bool newValue)
        {
            try
            {
                switch (p.Type)
                {
                    case DiscreteTypes.Coil:
                        await _modbusMaster.WriteSingleCoilAsync(SlaveAddress, p.Address, newValue);
                        break;
                    default:
                        throw new ArgumentException("Invalid discrete property type for writing to.");
                }
            }
            catch (SlaveException ex)
            {
                ModbusExceptionHelper(ex);
            }
            await ReadDiscreteProperty(p);
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

        private void ModbusExceptionHelper(SlaveException ex)
        {
            //TODO
            throw ex;
        }
    }
}
