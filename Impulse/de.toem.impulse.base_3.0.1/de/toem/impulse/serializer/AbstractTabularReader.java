/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.serializer;

import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.domain.AmpsBase;
import de.toem.impulse.domain.DomainBase;
import de.toem.impulse.domain.FloatBase;
import de.toem.impulse.domain.FrequencyBase;
import de.toem.impulse.domain.TimeBase;
import de.toem.impulse.domain.VoltsBase;
import de.toem.impulse.i18n.I18n;
import de.toem.impulse.samples.IFloatSamplesWriter;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.serializer.AbstractSingleDomainRecordReader;
import de.toem.toolkits.pattern.element.ICover;
import de.toem.toolkits.pattern.element.serializer.ParseException;
import de.toem.toolkits.pattern.properties.IPropertyModel;
import de.toem.toolkits.pattern.threading.IProgress;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.math.BigDecimal;

public class AbstractTabularReader
extends AbstractSingleDomainRecordReader {
    private static final String SEPERATOR = "\\s+";
    private int linesProcessed;
    private Signal[] signals;
    private BigDecimal quantum;
    private long current;
    int changed;

    public AbstractTabularReader() {
    }

    public AbstractTabularReader(String id, InputStream in) {
        super(id, in);
    }

    public static IPropertyModel getPropertyModel() {
        return AbstractTabularReader.getDefaultPropertyModel().add("domainBase", "auto", "Domain Base", null, null);
    }

    public static IPropertyModel getPropertyModel(Class sz) {
        return AbstractTabularReader.getPropertyModel();
    }

    @Override
    protected int isApplicable(String name, String contentType) {
        return 10;
    }

    @Override
    protected int isApplicable(byte[] buffer) {
        int off = 0;
        if (buffer[0] == 35 || buffer[0] == 33 || buffer[0] == 32 || buffer[0] == 37) {
            off = 1;
        }
        if (new String(buffer, off, 4).toLowerCase().equals("time") || new String(buffer, off, 5).toLowerCase().equals("volts") || new String(buffer, off, 9).toLowerCase().equals("frequency")) {
            return 1;
        }
        return -1;
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     * Unable to fully structure code
     * Could not resolve type clashes
     */
    @Override
    protected void parse(IProgress progress, InputStream in) throws ParseException {
        block26: {
            reader = new BufferedReader(new InputStreamReader(in));
            try {
                domainClass /* !! */  = FloatBase.class;
                line = reader.readLine();
                if (line == null) {
                    throw new ParseException(I18n.Serializer_EmptyFile);
                }
                splitted = line.trim().split("\\s+");
                if (splitted.length < 2 || splitted[0] == null) {
                    throw new ParseException(I18n.Serializer_NoValidHeader);
                }
                initial = splitted[0].toLowerCase();
                domainClass /* !! */  = initial.startsWith("#time") != false || initial.startsWith("!time") != false || initial.startsWith("%time") != false || initial.startsWith("time") != false ? TimeBase.class : (initial.startsWith("#frequency") != false || initial.startsWith("!frequency") != false || initial.startsWith("%frequency") != false || initial.startsWith("frequency") != false ? FrequencyBase.class : (initial.startsWith("#volts") != false || initial.startsWith("!volts") != false || initial.startsWith("%volts") != false || initial.startsWith("volts") != false ? VoltsBase.class : (initial.startsWith("#amps") != false || initial.startsWith("!amps") != false || initial.startsWith("%amps") != false || initial.startsWith("amps") != false ? AmpsBase.class : FloatBase.class)));
                domainBase = DomainBase.parse(this.properties.get("domainBase"));
                line1 = reader.readLine();
                line2 = reader.readLine();
                if (line1 == null || line2 == null) {
                    throw new ParseException(I18n.Serializer_Must3Lines);
                }
                t1 = Double.parseDouble(line1.trim().split("\\s+")[0]);
                t2 = Double.parseDouble(line2.trim().split("\\s+")[0]);
                diff = t2 - t1;
                if (domainBase == DomainBase.Unknown) {
                    domainBase = DomainBase.bestFit(domainClass /* !! */ , diff);
                }
                this.quantum = domainBase.getQuantum();
                this.initRecord("Tabular File", domainBase);
                this.signals = new Signal[splitted.length - 1];
                n = 1;
                while (n < splitted.length) {
                    this.signals[n - 1] = this.addSignal(this.base, splitted[n], null, ISamples.ProcessType.Discrete, ISamples.SignalType.Float, ISamples.SignalDescriptor.Float64);
                    ++n;
                }
                this.changed = 4;
                this.waitStreaming(progress);
                var17_17 = this;
                synchronized (var17_17) {
                    this.linesProcessed = 1;
                    this.current = DomainBase.toUnits(t1, this.quantum);
                    this.open(this.current);
                    this.parse(line1);
                    this.parse(line2);
                    this.changed = this.changed > 2 ? this.changed : 2;
                    // MONITOREXIT @DISABLED, blocks:[0, 1, 2, 8] lbl42 : MonitorExitStatement: MONITOREXIT : var17_17
                    if (true) ** GOTO lbl48
                }
                do {
                    this.parse(line);
lbl48:
                    // 2 sources

                } while ((line = reader.readLine()) != null && (progress == null || !progress.isCanceled()));
                var17_17 = this;
                synchronized (var17_17) {
                    this.close(this.current + 1L);
                    this.changed = 0;
                }
            }
            catch (ParseException e) {
                throw e;
            }
            catch (IOException v2) {
                try {
                    reader.close();
                }
                catch (IOException v3) {}
                break block26;
            }
            catch (Throwable e) {
                try {
                    throw new ParseException(this.linesProcessed, e.getMessage(), e);
                }
                catch (Throwable var18_18) {
                    try {
                        reader.close();
                    }
                    catch (IOException v4) {}
                    throw var18_18;
                }
            }
            try {
                reader.close();
            }
            catch (IOException v5) {}
        }
    }

    @Override
    public boolean supportsStreaming() {
        return true;
    }

    @Override
    public synchronized ICover flush() {
        if (this.changed != 0) {
            this.changed = 0;
            return super.doFlush(this.current);
        }
        return null;
    }

    @Override
    public int hasChanged() {
        return this.changed;
    }

    private synchronized void parse(String line) throws ParseException {
        if (line.trim().isEmpty()) {
            return;
        }
        String[] splitted = line.trim().split(SEPERATOR);
        if (splitted.length < this.signals.length + 1) {
            throw new ParseException(this.linesProcessed, I18n.Serializer_InvalidNoColumns);
        }
        this.current = new BigDecimal(splitted[0]).divide(this.quantum).setScale(0, 6).longValue();
        int n = 0;
        while (n < this.signals.length) {
            ((IFloatSamplesWriter)this.getWriter(this.signals[n])).write(this.current, Double.parseDouble(splitted[n + 1]));
            ++n;
        }
        this.changed = this.changed > 3 ? this.changed : 3;
    }
}

