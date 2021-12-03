using System;
using System.Collections.Generic;

namespace LabPIDv2.Modbus
{
    public abstract class RegisterPropertyBase<T> : PropertyBase, IRegisterProperty<T>
    {
        public RegisterPropertyBase(ushort address, string name) : base(address, name)
        {
            try
            {
                Length = Extensions.GetLength<T>();
            }
            catch (KeyNotFoundException)
            {
                throw new NotSupportedException("This type is not supported by LabPID modbus.");
            }
            Raw = new ushort[Length];
        }

        public abstract RegisterTypes Type { get; }
        public ushort Length { get; }
        public T Value { get; protected set; }
        public ushort[] Raw { get; protected set; }

        public void SetValue(ushort[] raw)
        {
            Raw = raw;
            Value = raw.ConvertTo<T>();
            RaisePropertyChanged(nameof(Raw));
            RaisePropertyChanged(nameof(Value));
        }
    }
}
