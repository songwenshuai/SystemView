/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.serializer;

import de.toem.impulse.ImpulseLicense;
import de.toem.impulse.cells.record.Record;
import de.toem.impulse.serializer.AbstractRecordReader;
import de.toem.toolkits.pattern.element.Cover;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.ICover;
import de.toem.toolkits.pattern.element.serializer.ICellReader;
import de.toem.toolkits.pattern.element.serializer.Message;
import de.toem.toolkits.pattern.threading.IProgress;
import java.io.InputStream;

public abstract class AbstractMultiInputReader
extends AbstractRecordReader {
    private Cover cover;
    private ICell base;
    protected ICellReader current;

    public AbstractMultiInputReader() {
    }

    public AbstractMultiInputReader(String id, InputStream in) {
        super(id, in);
    }

    @Override
    public ICover read(IProgress progress, String configurationName, ICell base, int insert) {
        Record record = new Record();
        this.cover = new Cover(record);
        this.cover.setSerializer(this.id);
        if (insert == 0) {
            this.base = record;
        } else if (insert == 2) {
            this.base = base;
        } else if (insert == 1) {
            return null;
        }
        if (ImpulseLicense.isFeatureLocked("de.toem.impulse.featureDomain.serializer", "de.toem.impulse.feature.default", this.id)) {
            base.insertChild(new Message("Locked serializer", 3, "The selected serializer is locked!"), 0);
            return this.cover;
        }
        try {
            if (this.configuration == null) {
                this.configuration = this.findConfiguration(configurationName);
            }
            if (this.configuration != null) {
                this.configurationProperties = this.readConfigurationProperties(this.configuration, this.properties);
                this.cover.setConfiguration(this.configuration.getName());
            }
            this.parse(progress, this.in, this.base);
        }
        catch (Throwable e) {
            AbstractMultiInputReader.addParseErrorMessage(this.id, e, record);
        }
        return this.cover;
    }

    protected abstract void parse(IProgress var1, InputStream var2, ICell var3) throws Throwable;

    @Override
    public boolean supportsStreaming() {
        return true;
    }

    @Override
    public ICover flush() {
        ICellReader reader = this.current;
        if (reader != null) {
            reader.flush();
        }
        return this.cover;
    }

    @Override
    public int hasChanged() {
        ICellReader reader = this.current;
        return reader != null ? reader.hasChanged() : 0;
    }
}

