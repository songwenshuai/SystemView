/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.record;

import de.toem.impulse.cells.record.RecordContent;
import de.toem.toolkits.pattern.element.CellAnnotation;

@CellAnnotation(type="record.scope", dynamicChildren={"record.signal", "record.signalProxy", "record.scope", "serializer.message"}, properties={"imageExtension"})
public class Scope
extends RecordContent {
    public static final String TYPE = "record.scope";
    public String domainType;

    public String imageExtension() {
        if (this.diff == 1) {
            return "-mod";
        }
        if (this.diff == 2) {
            return "-add";
        }
        if (this.diff == 3) {
            return "-del";
        }
        return "";
    }
}

