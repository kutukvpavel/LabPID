using MoreLinq;
using System;
using System.Collections.Generic;
using System.Linq;

namespace LabPIDv2.Modbus
{
    public static class Extensions
    {
        public static T ConvertTo<T>(this ushort[] regs)
        {
            try
            {
                if (ArrayCheck(typeof(T), regs, out ushort len))
                {
                    Type elementType = typeof(T).GetElementType();
                    var res = Array.CreateInstance(elementType, regs.Length / len);
                    var bytes = regs.ToBytes();
                    for (int i = 0; i < res.Length; i++)
                    {
                        res.SetValue((T)ConverterDictionary[elementType](bytes, i * len), i); 
                    }
                    return (T)(object)res;
                }
                else
                {
                    return (T)ConverterDictionary[typeof(T)](regs.ToBytes(), 0);
                }
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

        public static ushort[] ConvertToRegs<T>(this T val)
        {
            if (ArrayCheck(typeof(T), null, out ushort len))
            {
                Array a = (Array)(object)val;
                ushort[] res = new ushort[a.Length * len];
                for (int i = 0; i < a.Length; i++)
                {
                    ToRegsHelper(a.GetValue(i)).CopyTo(res, i * len);
                }
                return res;
            }
            else
            {
                return ToRegsHelper(val);
            }
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
        public static byte[] ToBytes(this ushort[] regs, ushort startIndex = 0)
        {
            CheckEndianess();
            return regs.Select(x => BitConverter.GetBytes(x)).Flatten().Cast<byte>().Skip(startIndex).ToArray();
        }
        public static ushort GetLength<T>(this T val)
        {
            return GetLengthHelper(val.GetType());
        }
        public static ushort GetLength<T>()
        {
            return GetLengthHelper(typeof(T));
        }


        private static ushort[] ToRegsHelper(object val)
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
        private static bool ArrayCheck(Type t, ushort[] regs, out ushort len)
        {
            len = LengthDictionary[t];
            bool a = t.IsArray;
            if (((regs?.Length ?? 0) % len != 0) && a)
                throw new InvalidOperationException("Non-uniform data length is not supported for array-properties.");
            return a;
        }
        private static ushort GetLengthHelper(Type t)
        {
            try
            {
                return LengthDictionary[t];
            }
            catch (KeyNotFoundException)
            {
                throw new NotSupportedException("This type is not supported for GetLength() method.");
            }
        }
        private static void CheckEndianess()
        {
            if (!BitConverter.IsLittleEndian) throw new PlatformNotSupportedException("Only little-endian platforms are supported.");
        }

        private static readonly Dictionary<Type, Func<byte[], int, object>> ConverterDictionary 
            = new Dictionary<Type, Func<byte[], int, object>>()
        {
            { typeof(float), (x, i) => BitConverter.ToSingle(x, i) },
            { typeof(short),  (x, i) => BitConverter.ToInt16(x, i) },
            { typeof(ushort),  (x, i) => BitConverter.ToUInt16(x, i) },
            { typeof(byte),  (x, i) => x[i] },
            { typeof(sbyte),  (x, i) => (sbyte)x[i] },
            { typeof(bool),  (x, i) => x[i] != 0 }
        };

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
    }
}
