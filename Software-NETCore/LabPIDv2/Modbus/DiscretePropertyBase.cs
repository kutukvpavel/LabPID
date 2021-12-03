using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;

namespace LabPIDv2.Modbus
{
    public abstract class DiscretePropertyBase : PropertyBase, IDiscreteProperty
    {
        public DiscretePropertyBase(ushort address, string name) : base(address, name)
        {

        }

        public abstract DiscreteTypes Type { get; }

        protected bool _value;
        public bool Value
        {
            get => _value;
            set
            {
                _value = value;
                RaisePropertyChanged();
            }
        }
    }
}
