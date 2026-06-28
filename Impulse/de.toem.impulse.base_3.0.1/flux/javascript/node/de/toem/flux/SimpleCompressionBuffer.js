"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
var de;
(function (de) {
    var toem;
    (function (toem) {
        var flux;
        (function (flux) {
            class SimpleCompressionBuffer extends de.toem.flux.Flx.SimpleBuffer {
                constructor(size, mode, output) {
                    super(size);
                    if (this.output === undefined)
                        this.output = null;
                }
                /**
                 *
                 * @return {number}
                 */
                flush() {
                    let data = this.data();
                    let start = this.startPos();
                    let end = this.endPos();
                    this.clear();
                    return de.toem.flux.Flx.OK;
                }
                /**
                 *
                 * @return {number}
                 */
                close() {
                    return de.toem.flux.Flx.OK;
                }
            }
            flux.SimpleCompressionBuffer = SimpleCompressionBuffer;
            SimpleCompressionBuffer["__class"] = "de.toem.flux.SimpleCompressionBuffer";
        })(flux = toem.flux || (toem.flux = {}));
    })(toem = de.toem || (de.toem = {}));
})(de || (de = {}));
