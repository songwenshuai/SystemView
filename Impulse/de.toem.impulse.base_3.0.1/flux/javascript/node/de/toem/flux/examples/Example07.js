/* Generated from Java with JSweet 3.0.0-RC3 - http://www.jsweet.org */
var de;
(function (de) {
    var toem;
    (function (toem) {
        var flux;
        (function (flux) {
            var examples;
            (function (examples) {
                class Example07 {
                    static example() {
                        let current = 0;
                        let buffer = ;
                        let trace = new de.toem.flux.Flx.Trace(0, Example07.MAX_ITEM_ID, Example07.MAX_ENTRY_SIZE, false, buffer);
                        if (trace != null) {
                            trace.addHead("example", "flux example");
                            trace.addScope(1, 0, "Logics", "Scope Description");
                            trace.addSignal(2, 1, "bit", "a bit", de.toem.flux.Flx.TYPE_LOGIC, null);
                            trace.addSignal(3, 1, "vector", "16 bits", de.toem.flux.Flx.TYPE_LOGIC, "default<bits=16>");
                            trace.addScatteredSignal(4, 1, "scattered", null, de.toem.flux.Flx.TYPE_LOGIC, null, 0, 1);
                            trace.addScatteredSignal(5, 1, "scattered", null, de.toem.flux.Flx.TYPE_LOGIC, null, 2, 5);
                            trace.open(0, "ns", 0, 0);
                            for (let n = 0; n < 50000; n++) {
                                {
                                    current = n * 10;
                                    let odd = ((n & 1) !== 0);
                                    trace.writeLogicTextAt(2, 0, current, false, de.toem.flux.Flx.STATE_0_BITS, (odd ? "1" : "0"), 1);
                                    trace.writeLogicTextAt(3, 0, 0, true, de.toem.flux.Flx.STATE_0_BITS, (odd ? "0011x1" : "111uuu"), 6);
                                    trace.writeLogicTextAt(4, 0, 0, true, de.toem.flux.Flx.STATE_0_BITS, (odd ? "uu" : "0u"), 2);
                                    trace.writeLogicTextAt(5, 0, 0, true, de.toem.flux.Flx.STATE_0_BITS, (odd ? "11x1" : "1100"), 4);
                                    let states = [de.toem.flux.Flx.STATE_1_BITS, de.toem.flux.Flx.STATE_1_BITS, de.toem.flux.Flx.STATE_X_BITS, de.toem.flux.Flx.STATE_X_BITS];
                                    trace.writeLogicStatesAt(3, 0, 5, true, de.toem.flux.Flx.STATE_U_BITS, states, 4);
                                }
                                ;
                            }
                            trace.close(0, current + 10);
                        }
                        trace.flush();
                        buffer.close();
                    }
                }
                Example07.MAX_ITEM_ID = 20;
                Example07.MAX_ENTRY_SIZE = 4096;
                examples.Example07 = Example07;
                Example07["__class"] = "de.toem.flux.examples.Example07";
            })(examples = flux.examples || (flux.examples = {}));
        })(flux = toem.flux || (toem.flux = {}));
    })(toem = de.toem || (de.toem = {}));
})(de || (de = {}));
