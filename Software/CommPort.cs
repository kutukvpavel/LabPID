using System;
using System.IO;
using System.IO.Ports;
using System.Collections;
using System.Threading;

namespace LabPID
{
	// Copied from project "Termie" almost "as-is".


	/// <summary> CommPort class creates a singleton instance
	/// of SerialPort (System.IO.Ports) </summary>
	/// <remarks> When ready, you open the port.
	///   <code>
	///   CommPort com = CommPort.Instance;
	///   com.StatusChanged += OnStatusChanged;
	///   com.DataReceived += OnDataReceived;
	///   com.Open();
	///   </code>
	///   Notice that delegates are used to handle status and data events.
	///   When settings are changed, you close and reopen the port.
	///   <code>
	///   CommPort com = CommPort.Instance;
	///   com.Close();
	///   com.PortName = "COM4";
	///   com.Open();
	///   </code>
	/// </remarks>
	public sealed class CommPort
	{
		SerialPort _serialPort;
		Thread _readThread;
		volatile bool _keepReading;

		//begin Singleton pattern
		static readonly CommPort instance = new CommPort();

		// Explicit static constructor to tell C# compiler
		// not to mark type as beforefieldinit
		static CommPort()
		{
		}

		CommPort()
		{
			_serialPort = new SerialPort();
			_readThread = null;
			_keepReading = false;
		}

		public void Dtr(bool Use)                   // Modified part.
		{
			_serialPort.DtrEnable = Use; 
		}

		public static CommPort Instance
		{
			get
			{
				return instance;
			}
		}
		//end Singleton pattern

		//begin Observer pattern
		public delegate void EventHandler(string param);
		public EventHandler StatusChanged;
		public EventHandler DataReceived;
		//end Observer pattern

		private void StartReading()
		{
			if (!_keepReading)
			{
				_keepReading = true;
				_readThread = new Thread(ReadPort);
				_readThread.Start();
			}
		}

		private void StopReading()
		{
			if (_keepReading)
			{
				_keepReading = false;
				_readThread.Join();	//block until exits
				_readThread = null;
			}
		}

		/// <summary> Get the data and pass it on. </summary>
		private void ReadPort()
		{
			while (_keepReading)
			{
				if (_serialPort.IsOpen)
				{
					byte[] readBuffer = new byte[_serialPort.ReadBufferSize + 1];
					try
					{
						// If there are bytes available on the serial port,
						// Read returns up to "count" bytes, but will not block (wait)
						// for the remaining bytes. If there are no bytes available
						// on the serial port, Read will block until at least one byte
						// is available on the port, up until the ReadTimeout milliseconds
						// have elapsed, at which time a TimeoutException will be thrown.
						int count = _serialPort.Read(readBuffer, 0, _serialPort.ReadBufferSize);
						String SerialIn = System.Text.Encoding.ASCII.GetString(readBuffer,0,count);
						DataReceived(SerialIn);
					}
					catch (TimeoutException) { }
					catch (Exception) { }
				}
				else
				{
					TimeSpan waitTime = new TimeSpan(0, 0, 0, 0, 50);
					Thread.Sleep(waitTime);
				}
			}
		}

		/// <summary> Open the serial port with current settings. </summary>
		public void Open(string PortName, int BaudRate, Parity Parity, int DataBits, StopBits StopBits, Handshake Handshake)
		{
			Close();

			try
			{
				_serialPort.PortName = PortName;
				_serialPort.BaudRate = BaudRate;
				_serialPort.Parity = Parity;
				_serialPort.DataBits = DataBits;
				_serialPort.StopBits = StopBits;
				_serialPort.Handshake = Handshake;

				// Set the read/write timeouts
				_serialPort.ReadTimeout = 50;
				_serialPort.WriteTimeout = 50;

				_serialPort.Open();
				StartReading();
			}
			catch (IOException)
			{
				StatusChanged(String.Format("{0} does not exist", PortName));
			}
			catch (UnauthorizedAccessException)
			{
				StatusChanged(String.Format("{0} already in use", PortName));
			}
			catch (Exception ex)
			{
				StatusChanged(String.Format("{0}", ex.ToString()));
			}

			// Update the status
			if (_serialPort.IsOpen)
			{
				string p = _serialPort.Parity.ToString().Substring(0, 1);   //First char
				string h = _serialPort.Handshake.ToString();
				if (_serialPort.Handshake == Handshake.None)
					h = "no handshake"; // more descriptive than "None"

				StatusChanged(String.Format("{0}: {1} bps, {2}{3}{4}, {5}",
					_serialPort.PortName, _serialPort.BaudRate,
					_serialPort.DataBits, p, (int)_serialPort.StopBits, h));
			}
			else
			{
				StatusChanged(String.Format("{0} already in use", PortName));
			}
		}

		/// <summary> Close the serial port. </summary>
		public void Close()
		{
			StopReading();
			_serialPort.Close();
			StatusChanged("Connection closed.");
		}

		/// <summary> Get the status of the serial port. </summary>
		public bool IsOpen
		{
			get
			{
				return _serialPort.IsOpen;
			}
		}

		/// <summary> Get a list of the available ports. Already opened ports
		/// are not returend. </summary>
		public string[] GetAvailablePorts()
		{
			return SerialPort.GetPortNames();
		}

		/// <summary>Send data to the serial port after appending line ending. </summary>
		/// <param name="data">An string containing the data to send. </param>
		public void Send(string data, bool lne = true)
		{
			if (IsOpen)
			{
                if (lne) data += "\r\n";
				_serialPort.Write(data);
			}
		}
	}
}