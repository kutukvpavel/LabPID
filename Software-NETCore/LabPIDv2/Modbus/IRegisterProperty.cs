using System.ComponentModel;

namespace LabPIDv2.Modbus
{
    public interface IRegisterProperty : INotifyPropertyChanged
    {
        public ushort Address { get; }
        public PropertyTypes Type { get; }
        public ushort Length { get; }

        public void SetValue(ushort[] raw);
    }
}
