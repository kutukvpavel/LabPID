using MoreLinq;
using System;
using System.Collections.Generic;
using System.Linq;

namespace LabPIDv2.Modbus
{
    public static class Extensions
    {
        public static T ConvertTo<T>(this ushort[] regs) where T : struct
        {
            try
            {
                return (T)ConverterDictionary[typeof(T)](regs.ToBytes());
            }
            catch (KeyNotFoundException)
            {
                throw new NotSupportedException("Conversion to this type is not supported.");
            }
            catch (IndexOutOfRangeException)
            {
                throw new ArgumentException("Unable to convert to byte: input sequence is empty.");
            }
        }

        public static ushort[] ConvertToRegs<T>(this T val) where T : struct
        {
            return val switch
            {
                float v => BitConverter.GetBytes(v).ToRegs(),
                ushort us => new ushort[] { us },
                short s => new ushort[] { (ushort)s },
                byte b => new ushort[] { b },
                sbyte sb => new ushort[] { (byte)sb },
                bool bb => new ushort[] { (ushort)(bb ? 1 : 0) },
                _ => throw new NotSupportedException("This type can not be converted to registers."),
            };
        }

        public static ushort[] ToRegs(this byte[] bytes)
        {
            CheckEndianess();
            if (bytes.Length % 2 != 0) throw new NotSupportedException("Data types with uneven size are not supported.");
            ushort[] res = new ushort[bytes.Length / 2];
            for (int i = 0; i < res.Length; i++)
            {
                res[i] = BitConverter.ToUInt16(bytes, i * 2);
            }
            return res;
        }
        public static byte[] ToBytes(this ushort[] regs)
        {
            CheckEndianess();
            return regs.Select(x => BitConverter.GetBytes(x)).Flatten().Cast<byte>().ToArray();
        }

        private static void CheckEndianess()
        {
            if (!BitConverter.IsLittleEndian) throw new PlatformNotSupportedException("Only little-endian platforms are supported.");
        }

        private static readonly Dictionary<Type, Func<byte[], object>> ConverterDictionary = new Dictionary<Type, Func<byte[], object>>()
        {
            { typeof(float), x => BitConverter.ToSingle(x) },
            { typeof(short),  x => BitConverter.ToInt16(x) },
            { typeof(ushort),  x => BitConverter.ToUInt16(x) },
            { typeof(byte),  x => x[0] },
            { typeof(sbyte),  x => (sbyte)x[0] },
            { typeof(bool),  x => x[0] != 0 }
        };
    }
}
