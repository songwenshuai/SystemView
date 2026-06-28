/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.record;

import de.toem.impulse.cells.record.AbstractSignal;
import de.toem.impulse.cells.record.Record;
import de.toem.impulse.cells.record.Signal;
import de.toem.toolkits.pattern.element.CellAnnotation;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.Link;

@CellAnnotation(type="record.signalProxy", properties={"imageExtension"})
public class SignalProxy
extends AbstractSignal {
    public static final String TYPE = "record.signalProxy";
    public Link signal;

    @Override
    public Signal getSignal() {
        ICell record = this;
        ICell signal = null;
        while (record != null && !(record instanceof Record)) {
            record = record.getParent();
        }
        if (record != null) {
            signal = record.getCellByLink(this.signal);
        }
        return signal instanceof Signal ? (Signal)signal : null;
    }

    public String imageExtension() {
        Signal signal = this.getSignal(null);
        if (signal != null) {
            return this.diff != 0 ? signal.imageExtension(this.diff) : signal.imageExtension();
        }
        return null;
    }
}

