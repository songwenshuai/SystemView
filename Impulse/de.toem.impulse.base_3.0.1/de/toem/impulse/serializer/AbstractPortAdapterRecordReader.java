/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.serializer;

import de.toem.impulse.cells.ports.IPortProgress;
import de.toem.impulse.serializer.AbstractSingleDomainRecordReader;
import de.toem.toolkits.pattern.element.serializer.ParseException;
import de.toem.toolkits.pattern.threading.IProgress;
import java.io.InputStream;

public abstract class AbstractPortAdapterRecordReader
extends AbstractSingleDomainRecordReader {
    @Override
    protected final int isApplicable(String name, String contentType) {
        return 0;
    }

    @Override
    protected final int isApplicable(byte[] buffer) {
        return 0;
    }

    @Override
    protected final void parse(IProgress progress, InputStream in) throws ParseException {
        if (progress instanceof IPortProgress) {
            this.process((IPortProgress)progress);
        }
    }

    protected abstract void process(IPortProgress var1);
}

