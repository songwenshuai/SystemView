// serialPort  : Serial port (jssc.SerialPort)
// log         : Log output (java.io.OutputStream)
// out         : Output stream (java.io.OutputStream)
//   out.write(0x10);
// progress    : Progress control (de.toem.impulse.cells.ports.IPortProgress)
// console     : Console output (de.toem.impulse.scripting.IScriptConsole)

out.write(0x10);
java.lang.Thread.sleep(100);
out.write(0x15);