// generator   : Record generator (de.toem.impulse.samples.ISingleDomainRecordGenerator) 
// inputStream : Input stream (java.io.InputStream)
// progress    : Progress control (de.toem.pattern.threading.IProgress),
// console     : Console output (de.toem.impulse.scripting.IScriptConsole)

// Init the record
generator.initRecord("Example Record", TimeBase.ns);
var a = generator.addSignal(null, "a", "", ProcessType.Discrete, SignalType.Integer, SignalDescriptor.DEFAULT);
var b = generator.addSignal(null, "b", "", ProcessType.Discrete, SignalType.Integer, SignalDescriptor.DEFAULT);
var c = generator.addSignal(null, "c", "", ProcessType.Discrete, SignalType.Integer, SignalDescriptor.DEFAULT);
var d = generator.addSignal(null, "d", "", ProcessType.Discrete, SignalType.Integer, SignalDescriptor.DEFAULT);
var wa /*:IIntegerSamplesWriter:*/ = generator.getWriter(a);
var wb /*:IIntegerSamplesWriter:*/ = generator.getWriter(b);
var wc /*:IIntegerSamplesWriter:*/ = generator.getWriter(c);
var wd /*:IIntegerSamplesWriter:*/ = generator.getWriter(d);

var current = 0;
generator.open(current);

try {
    var reader /*:java.io.BufferedReader:*/ = new BufferedReader(new InputStreamReader(inputStream));
    /*
    
		1.0;1;1;2;3
		2.0;2;3;4;;
		3.0;1;1;2;3
		4.0;2;3;4;5
    */
    var line /*:java.lang.String:*/ = reader.readLine(); // skip empty header
    while ((line = reader.readLine()) != null) {
        var splitted = line.split(";");
        var current = TimeBase.ns.parseUnits(splitted[0]+"ms");
        console.println(current+" "+line+" "+line.length());
        wa.writeInt(current, false, splitted[1]);
        wb.writeInt(current, false, splitted[2]);
        wc.writeInt(current, false, splitted[3]);
        wd.writeInt(current, false, splitted[4]);
    }
} catch (e) {
    console.println(e); // pass exception object to error handler
}


generator.close(current + 1);
