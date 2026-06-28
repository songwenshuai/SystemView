/* Generated from Java with JSweet 3.0.0-RC3 - http://www.jsweet.org */
var de;
(function (de) {
    var toem;
    (function (toem) {
        var flux;
        (function (flux) {
            var examples;
            (function (examples) {
                class Example01 {
                    static example() {
                        let current = 0;
                        let iVal = 0;
                        let fVal = 0.0;
                        let buffer = ;
                        let trace = new de.toem.flux.Flx.Trace(0, Example01.MAX_ITEM_ID, Example01.MAX_ENTRY_SIZE, false, buffer);
                        if (trace != null) {
                            trace.addHead("example", "flux example");
                            trace.addSignal(1, 0, "integer", "an integer", de.toem.flux.Flx.TYPE_INTEGER, null);
                            trace.addSignal(2, 0, "float", "a float", de.toem.flux.Flx.TYPE_FLOAT, null);
                            trace.open(0, "ns", 0, 0);
                            for (let n = 0; n < 50000; n++) {
                                {
                                    current = n * 10;
                                    iVal = n % 444;
                                    trace.writeIntAt(1, 0, current, false, iVal);
                                    fVal = (n / 1000.0);
                                    trace.writeFloatAt(2, 0, 0, true, fVal);
                                }
                                ;
                            }
                            trace.close(0, current + 10);
                        }
                        trace.flush();
                        buffer.close();
                    }
                }
                Example01.MAX_ITEM_ID = 2;
                Example01.MAX_ENTRY_SIZE = 4096;
                examples.Example01 = Example01;
                Example01["__class"] = "de.toem.flux.examples.Example01";
            })(examples = flux.examples || (flux.examples = {}));
        })(flux = toem.flux || (toem.flux = {}));
    })(toem = de.toem || (de.toem = {}));
})(de || (de = {}));
