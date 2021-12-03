using System.ComponentModel;

namespace LabPIDv2.Modbus
{
    public enum RegisterTypes
    {
        Input,
        Holding
    }

    public enum DiscreteTypes
    {
        Coil,
        Input
    }

    public interface IProperty : INotifyPropertyChanged
    {
        public string Name { get; }
        public ushort Address { get; }
    }

    public abstract class PropertyBase : NotifyPropertyChangedBase, IProperty
    {
        public PropertyBase(ushort address, string name)
        {
            Address = address;
            Name = name;
        }

        public string Name { get; }
        public ushort Address { get; }
    }

    public interface IRegisterProperty<T> : IProperty
    {
        public T Value { get; }
        public RegisterTypes Type { get; }
        public ushort Length { get; }

        public void SetValue(ushort[] raw);
    }

    public interface IDiscreteProperty : IProperty
    {
        public bool Value { get; set; }
        public DiscreteTypes Type { get; }
    }
}
