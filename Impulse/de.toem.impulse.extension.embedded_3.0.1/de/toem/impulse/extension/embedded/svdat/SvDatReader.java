/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.extension.embedded.svdat;

import de.toem.impulse.cells.serializer.ReaderConfiguration;
import de.toem.impulse.extension.embedded.svdat.SvDatConfiguration;
import de.toem.impulse.extension.embedded.svdat.SvDatParser2;
import de.toem.impulse.serializer.AbstractSingleDomainRecordReader;
import de.toem.impulse.serializer.BinaryParseBuffer;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.ICover;
import de.toem.toolkits.pattern.element.serializer.ParseException;
import de.toem.toolkits.pattern.systemLog.SystemLog;
import de.toem.toolkits.pattern.threading.IProgress;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;

public class SvDatReader
extends AbstractSingleDomainRecordReader {
    SvDatParser2 parser;
    int version;

    public SvDatReader() {
    }

    public SvDatReader(String id, InputStream in) {
        super(id, in);
    }

    public static List<ReaderConfiguration> getInitialConfigurations() {
        ArrayList<ReaderConfiguration> list = new ArrayList<ReaderConfiguration>();
        SvDatConfiguration configuration = new SvDatConfiguration();
        configuration.setName("Any");
        list.add(configuration);
        return list;
    }

    @Override
    protected int isApplicable(String name, String contentType) {
        return 80;
    }

    @Override
    protected int isApplicable(byte[] buffer) {
        if (new String(buffer, 0, buffer.length).contains("SEGGER SystemView")) {
            return 1;
        }
        return -1;
    }

    @Override
    public boolean supportsStreaming() {
        return true;
    }

    @Override
    public synchronized ICover flush() {
        if (this.parser != null && this.parser.hasChanged() != 0) {
            this.parser.resetChanged();
            return super.doFlush(this.parser.getCurrent());
        }
        return null;
    }

    @Override
    public int hasChanged() {
        return this.parser != null ? this.parser.hasChanged() : 0;
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    @Override
    protected void parse(IProgress progress, InputStream in) throws ParseException {
        block25: {
            SvDatConfiguration selected = null;
            if (this.configuration instanceof SvDatConfiguration) {
                selected = (SvDatConfiguration)this.configuration;
            }
            if (selected == null) {
                throw new ParseException(-1, "Configuration required");
            }
            BinaryParseBuffer b = new BinaryParseBuffer(524288);
            this.parser = new SvDatParser2(this, selected);
            try {
                try {
                    Utils.millies();
                    while (!(!b.fill(in) || b.isError() || progress != null && progress.isCanceled())) {
                        SvDatReader svDatReader = this;
                        synchronized (svDatReader) {
                            do {
                                b = b.begin();
                                this.parser.parseEntry(b);
                            } while ((b = b.end()).isOk());
                            if (progress != null) {
                                double d = 1.0 * (double)b.used() / (double)in.available();
                                progress.done(d);
                            }
                            b.clean();
                        }
                    }
                }
                catch (Throwable e) {
                    SystemLog.log(e);
                    SvDatReader svDatReader = this;
                    synchronized (svDatReader) {
                        this.parser.finalizeParse();
                        if (b.isError()) {
                            ParseException e2 = new ParseException(b.getErrorText());
                            e2.position = b.used();
                            throw e2;
                        }
                        break block25;
                    }
                }
            }
            catch (Throwable throwable) {
                SvDatReader svDatReader = this;
                synchronized (svDatReader) {
                    this.parser.finalizeParse();
                    if (b.isError()) {
                        ParseException e = new ParseException(b.getErrorText());
                        e.position = b.used();
                        throw e;
                    }
                }
                throw throwable;
            }
            SvDatReader svDatReader = this;
            synchronized (svDatReader) {
                this.parser.finalizeParse();
                if (b.isError()) {
                    ParseException e = new ParseException(b.getErrorText());
                    e.position = b.used();
                    throw e;
                }
            }
        }
    }
}

