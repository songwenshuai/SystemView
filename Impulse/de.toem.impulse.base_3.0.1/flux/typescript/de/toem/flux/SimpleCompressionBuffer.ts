import * as fs from "fs";
namespace de.toem.flux {
    export class SimpleCompressionBuffer extends de.toem.flux.Flx.SimpleBuffer {
        output : any;

        public constructor(size : number, mode : number, output : de.toem.flux.Flx.Buffer) {
            super(size);
            if(this.output===undefined) this.output = null;
        }

        /**
         * 
         * @return {number}
         */
        public flush() : number {
            let data : number[] = this.data();
            let start : number = this.startPos();
            let end : number = this.endPos();
            this.clear();
            return de.toem.flux.Flx.OK;
        }

        /**
         * 
         * @return {number}
         */
        public close() : number {
            return de.toem.flux.Flx.OK;
        }
    }
    SimpleCompressionBuffer["__class"] = "de.toem.flux.SimpleCompressionBuffer";

}

