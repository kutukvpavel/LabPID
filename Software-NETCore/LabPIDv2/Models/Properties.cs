using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;

namespace LabPIDv2.Models
{
    public class InputRegisterProperty<T> : Modbus.RegisterPropertyBase<T> where T : struct
    {
        public InputRegisterProperty(ushort address, string name) : base(address, name)
        {

        }

        public override Modbus.PropertyTypes Type => Modbus.PropertyTypes.Input;
    }

    public class HoldingRegisterProperty<T> : Modbus.RegisterPropertyBase<T> where T : struct
    {
        public HoldingRegisterProperty(ushort address, string name) : base(address, name)
        {

        }

        public override Modbus.PropertyTypes Type => Modbus.PropertyTypes.Holding;
    }

    
}
