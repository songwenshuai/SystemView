/* Generated from Java with JSweet 3.0.0-RC3 - http://www.jsweet.org */
namespace de.toem.flux.examples {
    export class Example05 {
        static MAX_ITEM_ID : number = 20;

        static MAX_ENTRY_SIZE : number = 4096;

        public static example() {
            let current : number = 0;
            let eVal : number = 0;
            let tVal : string = null;
            let fVal : number = 0.0;
            let buffer : de.toem.flux.Flx.Buffer = ;
            let trace : de.toem.flux.Flx.Trace = new de.toem.flux.Flx.Trace(0, Example05.MAX_ITEM_ID, Example05.MAX_ENTRY_SIZE, false, buffer);
            if (trace != null){
                trace.addHead("example", "flux example");
                trace.addScope(1, 0, "Struct", "Scope Description");
                trace.addSignal(2, 1, "Simple Struct", "desc", de.toem.flux.Flx.TYPE_STRUCT, null);
                let members : de.toem.flux.Flx.MemberValue[] = trace.createMembers(4);
                members[0] = trace.createMember(0, "m0", de.toem.flux.Flx.STRUCT_TYPE_GLOBAL_ENUM, null);
                members[1] = trace.createMember(1, "m1", de.toem.flux.Flx.STRUCT_TYPE_INTEGER, "default<df=Hex>");
                members[2] = trace.createMember(2, "m2", de.toem.flux.Flx.STRUCT_TYPE_FLOAT, null);
                members[3] = trace.createMember(3, "m3", de.toem.flux.Flx.STRUCT_TYPE_TEXT, null);
                trace.open(0, "ns", 0, 0);
                trace.writeMemberDefs(2, members);
                trace.writeEnumDef(2, de.toem.flux.Flx.ENUM_GLOBAL, "Yes", 1);
                trace.writeEnumDef(2, de.toem.flux.Flx.ENUM_GLOBAL, "No", 0);
                for(let n : number = 0; n < 50000; n++) {{
                    current = n * 10;
                    eVal = n & 1;
                    fVal = (n / 100.0);
                    tVal = "val: " + /* valueOf */new String(n % 100).toString();
                    members[0].setValue(eVal);
                    members[1].setValue(n);
                    members[2].setValue(fVal);
                    members[3].setValue(tVal);
                    trace.writeMembersAt(2, 0, current, false, members);
                };}
                trace.close(0, current + 10);
            }
            trace.flush();
            buffer.close();
        }
    }
    Example05["__class"] = "de.toem.flux.examples.Example05";

}

