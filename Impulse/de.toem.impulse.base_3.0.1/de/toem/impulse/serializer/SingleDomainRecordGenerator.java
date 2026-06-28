/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.serializer;

import de.toem.impulse.cells.record.Record;
import de.toem.impulse.samples.ISingleDomainRecordGenerator;
import de.toem.impulse.serializer.AbstractSingleDomainRecordReader;
import de.toem.toolkits.pattern.element.serializer.ParseException;
import de.toem.toolkits.pattern.threading.IProgress;
import java.io.InputStream;

public class SingleDomainRecordGenerator
extends AbstractSingleDomainRecordReader
implements ISingleDomainRecordGenerator {
    public SingleDomainRecordGenerator(Record record) {
        this.record = record;
        this.base = this.record;
    }

    @Override
    protected final void parse(IProgress progress, InputStream in) throws ParseException {
    }

    @Override
    protected final int isApplicable(String name, String contentType) {
        return 0;
    }
}

