/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.serializer;

import de.toem.impulse.cells.record.Record;
import de.toem.impulse.samples.IRecordGenerator;
import de.toem.impulse.serializer.AbstractParsingRecordReader;
import de.toem.toolkits.pattern.element.serializer.ParseException;
import de.toem.toolkits.pattern.threading.IProgress;
import java.io.InputStream;

public class RecordGenerator
extends AbstractParsingRecordReader
implements IRecordGenerator {
    public RecordGenerator(Record record) {
        this.record = record;
        this.base = this.record;
    }

    @Override
    protected final int isApplicable(String name, String contentType) {
        return 0;
    }

    @Override
    protected final void parse(IProgress progress, InputStream in) throws ParseException {
    }
}

