/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.serializer.base;

import de.toem.impulse.cells.record.Record;
import de.toem.impulse.samples.base.RecordComparator;
import de.toem.impulse.serializer.AbstractMultiElementReader;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.ICover;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.element.serializer.ParseException;
import de.toem.toolkits.pattern.threading.IProgress;
import de.toem.toolkits.pattern.threading.PartialProgress;
import java.io.InputStream;

public class DiffReader
extends AbstractMultiElementReader {
    public DiffReader() {
    }

    public DiffReader(String id, InputStream in) {
        super(id, in);
    }

    @Override
    protected int isApplicable(String name, String contentType) {
        return -1;
    }

    @Override
    protected void parse(IProgress progress, InputStream in, ICell base) throws Throwable {
        Record b;
        super.parse(progress, in, base);
        if (this.references.size() != 2) {
            throw new ParseException("Need 2 inputs");
        }
        if (!(base instanceof Record)) {
            throw new ParseException("Result need to be record");
        }
        ICover ca = this.read((IProgress)new PartialProgress(progress, 0.33333334f), "Load", (IElement)this.elements.get(this.references.get(0)), null);
        ICover cb = this.read((IProgress)new PartialProgress(progress, 0.33333334f), "Load", (IElement)this.elements.get(this.references.get(1)), null);
        Record a = ca.getCell() instanceof Record ? (Record)ca.getCell() : null;
        Record record = b = cb.getCell() instanceof Record ? (Record)cb.getCell() : null;
        if (a == null || b == null) {
            throw new ParseException("Could not read inputs");
        }
        RecordComparator.createDiff(new PartialProgress(progress, 0.33333334f), a, b, (Record)base);
    }
}

