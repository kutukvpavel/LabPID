using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;

namespace LabPIDv2.Models
{
    public class InputRegisterProperty<T> : Modbus.RegisterPropertyBase<T>
    {
        public InputRegisterProperty(ushort address, string name) : base(address, name)
        {

        }

        public override Modbus.RegisterTypes Type => Modbus.RegisterTypes.Input;
    }

    public class HoldingRegisterProperty<T> : Modbus.RegisterPropertyBase<T>
    {
        public HoldingRegisterProperty(ushort address, string name) : base(address, name)
        {

        }

        public override Modbus.RegisterTypes Type => Modbus.RegisterTypes.Holding;
    }

    public class CoilProperty : Modbus.DiscretePropertyBase
    {
        public CoilProperty(ushort address, string name) : base(address, name)
        {

        }

        public override Modbus.DiscreteTypes Type => Modbus.DiscreteTypes.Coil;
    }

    public class DiscreteInputProperty : Modbus.DiscretePropertyBase
    {
        public DiscreteInputProperty(ushort address, string name) : base(address, name)
        {

        }

        public override Modbus.DiscreteTypes Type => Modbus.DiscreteTypes.Input;
    }
}
