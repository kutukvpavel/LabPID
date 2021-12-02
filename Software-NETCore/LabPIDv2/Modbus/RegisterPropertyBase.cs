using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Text;

namespace LabPIDv2.Modbus
{
    public enum PropertyTypes
    {
        Input,
        Holding,
        Coil
    }

    public abstract class RegisterPropertyBase<T> : IRegisterProperty where T : struct
    {
        public event PropertyChangedEventHandler PropertyChanged;

        public RegisterPropertyBase(ushort address, string name)
        {
            Address = address;
            Name = name;
            try
            {
                Length = LengthDictionary[typeof(T)];
            }
            catch (KeyNotFoundException)
            {
                throw new NotSupportedException("This type is not supported by LabPID modbus.");
            }
            Raw = new ushort[Length];
        }

        public string Name { get; }
        public ushort Address { get; }
        public abstract PropertyTypes Type { get; }
        public ushort Length { get; }
        public T Value { get; private set; }
        public ushort[] Raw { get; private set; }

        public void SetValue(ushort[] raw)
        {
            Raw = raw;
            Value = raw.ConvertTo<T>();
            RaisePropertyChanged(nameof(Raw));
            RaisePropertyChanged(nameof(Value));
        }

        protected void RaisePropertyChanged([CallerMemberName] string name = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }

        #region Static

        /// <summary>
        /// Length in modbus registers (16-bit words). Supports: float, short, byte (bool), including signed types.
        /// </summary>
        private static readonly Dictionary<Type, ushort> LengthDictionary = new Dictionary<Type, ushort>()
        {
            { typeof(float), 2 },
            { typeof(short), 1 },
            { typeof(ushort), 1 },
            { typeof(byte), 1 },
            { typeof(sbyte), 1 },
            { typeof(bool), 1 }
        };

        #endregion
    }
}
