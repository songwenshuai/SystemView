// socket      : Socket object (java.net.DatagramSocket)
// feed        : Feed output (java.io.OutputStream)
// log         : Log output (java.io.OutputStream)
// progress    : Progress control (de.toem.impulse.cells.ports.IPortProgress),
// console     : Console output (de.toem.impulse.scripting.IScriptConsole)

// example for manual feed
var ByteArray = Java.type("byte[]");
var bytes = new ByteArray(0x10000);
var packet /*:DatagramPacket:*/ = new DatagramPacket(bytes, 0x10000);
while (true) { 
    socket.receive(packet);
    if (packet.getLength() > 0) {
    
        console.println( "written"+packet.getLength());
        feed.write(packet.getData(), 0, packet.getLength());
    }
}