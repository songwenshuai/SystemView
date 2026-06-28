/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.record;

import de.toem.impulse.cells.record.RecordContent;
import de.toem.impulse.provider.ISignalContext;
import de.toem.toolkits.pattern.element.CellAnnotation;
import de.toem.toolkits.pattern.element.IElement;

@CellAnnotation(type="record", dynamicChildren={"record.signal", "record.signalProxy", "record.scope", "serializer.message", "configuration.record"})
public class Record
extends RecordContent
implements ISignalContext {
    public static final String TYPE = "record";

    @Override
    public IElement getRecord() {
        return this.getElement();
    }
}

