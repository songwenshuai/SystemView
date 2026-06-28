/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.serializer;

import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.flux.AbstractFluxHandler;
import de.toem.impulse.flux.FluxParser;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.base.PackedSamples;
import de.toem.impulse.serializer.AbstractFluxConverterRecordReader;
import de.toem.impulse.serializer.BinaryParseBuffer;
import de.toem.impulse.values.StructMember;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.serializer.ParseException;
import de.toem.toolkits.pattern.pageable.BytesPageable;
import de.toem.toolkits.pattern.pageable.IPageGenerator;
import de.toem.toolkits.pattern.pageable.Pageable;
import de.toem.toolkits.pattern.properties.IPropertyModel;
import de.toem.toolkits.pattern.threading.Actives;
import de.toem.toolkits.pattern.threading.IExecutable;
import de.toem.toolkits.pattern.threading.IExecution;
import de.toem.toolkits.pattern.threading.IProgress;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;

public abstract class AbstractFluxDatabaseRecordReader
extends AbstractFluxConverterRecordReader {
    public static final int MAX_SIGNALS_TO_FILL = 32;
    public static final int TIMEOUT_DEFAULT = 5000;
    public static final boolean LOAD_ON_REQUEST_DEFAULT = true;
    public static final int FLX_CONTROL_DB_SCHEME = 256;
    public static final int FLX_CONTROL_DB_O_REQ_SCHEME = 256;
    public static final int FLX_CONTROL_DB_I_REQ_ITEMS = 257;
    public static final int FLX_CONTROL_DB_I_REQ_TRACE = 258;
    private int timeout = 5000;
    private boolean loadOnRequest;
    private String domain;
    private long start;
    private long end;
    private long rate;
    private int version;
    private int maxTraceItems;
    private LinkedList<ICell> prepared = new LinkedList();
    private boolean errror;
    private IExecution watchdog;
    private IExecutable killInput = new IExecutable(){

        @Override
        public void execute(IProgress progress) {
            try {
                AbstractFluxDatabaseRecordReader.this.p.destroyForcibly();
            }
            catch (Throwable throwable) {}
        }
    };

    public static IPropertyModel getPropertyModel(Class sz) {
        return AbstractFluxDatabaseRecordReader.getPropertyModel();
    }

    public static IPropertyModel getPropertyModel() {
        return AbstractFluxDatabaseRecordReader.getDefaultPropertyModel().add("timeout", 5000, "Timeout", null).add("loadOnRequest", true, "Load on request", null);
    }

    public AbstractFluxDatabaseRecordReader() {
    }

    public AbstractFluxDatabaseRecordReader(String id, InputStream in) {
        super(id, in);
    }

    @Override
    protected boolean prepare(IProgress progress, InputStream in) throws ParseException {
        if (!super.prepare(progress, in)) {
            return false;
        }
        this.handler = new Handler();
        ((Handler)this.handler).writeSchemeRequest();
        ((Handler)this.handler).writeItemRequest();
        if (this.properties != null) {
            this.timeout = Utils.parseInt(this.properties.get("timeout"), 5000);
            this.loadOnRequest = Utils.parseBoolean(this.properties.get("loadOnRequest"), true);
        }
        this.killProcessAfterParse = !this.loadOnRequest;
        return true;
    }

    @Override
    protected void parse(IProgress progress, InputStream in) throws ParseException {
        this.watchdog = Actives.watch(this.killInput, this.timeout);
        super.parse(progress, in);
        if (this.watchdog.hasTimeout()) {
            throw new ParseException("Timeout occured. Please check if the serializers native part is installed and configured correctly (Preferences->impulse->Native Extensions->" + this.getFluxName().toUpperCase() + "Native)");
        }
        this.watchdog.finish();
        if (!this.loadOnRequest) {
            for (ICell cell : this.base.getTribe(false, Signal.class)) {
                if (cell.getData("SERIALIZER") != this) continue;
                this.prepared.add(cell);
            }
            while (this.fillSignals(null, this.prepared, Integer.MAX_VALUE, true)) {
            }
        }
    }

    protected void prepareSignal(Signal signal) {
        if (!this.prepared.contains(signal)) {
            this.prepared.add(signal);
        }
    }

    protected boolean fillSignal(Signal signal) {
        return this.fillSignals(signal, this.prepared, 32, true);
    }

    protected boolean fillSignals(Signal signal, LinkedList<ICell> optional, int maxSignals, boolean useWatchdog) {
        int sidx;
        if (this.errror) {
            return false;
        }
        ArrayList<ICell> signals = new ArrayList<ICell>();
        int traceItems = 0;
        if (signal != null) {
            signals.add(signal);
            if ((traceItems += this.parser.getCurrentTrace().getNoOfItems(signal)) > this.maxTraceItems) {
                return false;
            }
        }
        if ((sidx = optional.indexOf(signal)) == -1) {
            sidx = 0;
        }
        while (!optional.isEmpty()) {
            ICell toBeAdd;
            if (sidx >= optional.size()) {
                sidx = optional.size() - 1;
            }
            if (signals.contains(toBeAdd = optional.get(sidx))) {
                optional.remove(sidx);
                continue;
            }
            int ti = this.parser.getCurrentTrace().getNoOfItems(toBeAdd);
            if (traceItems + ti > this.maxTraceItems || signals.size() >= maxSignals) break;
            optional.remove(sidx);
            traceItems += ti;
            signals.add(toBeAdd);
        }
        if (signals.isEmpty()) {
            return false;
        }
        FluxParser.Trace trace = this.parser.getCurrentTrace();
        if (trace == null) {
            return false;
        }
        ArrayList<Integer> itemIds = new ArrayList<Integer>();
        if (this.parser.getCurrentTrace().open(-1, signals, this.domain, this.start, this.rate, itemIds, null)) {
            block22: {
                ((Handler)this.handler).writeTraceRequest(itemIds);
                try {
                    this.watchdog = useWatchdog ? Actives.watch(this.killInput, this.timeout) : null;
                    try {
                        AbstractFluxDatabaseRecordReader.super.parse(null, null);
                    }
                    catch (Throwable throwable) {
                        if (this.watchdog != null) {
                            if (this.watchdog.hasTimeout()) {
                                try {
                                    this.addParseErrorMessage(new ParseException("Timeout occured. Please check if the serializer is configured correctly (Preferences->impulse->Native Extensions->" + this.getFluxName().toUpperCase() + ")"));
                                    this.base.getRoot().getElement().fireElementResetted();
                                }
                                catch (Throwable throwable2) {}
                                this.errror = true;
                            } else {
                                this.watchdog.finish();
                            }
                        }
                        throw throwable;
                    }
                    if (this.watchdog == null) break block22;
                    if (this.watchdog.hasTimeout()) {
                        try {
                            this.addParseErrorMessage(new ParseException("Timeout occured. Please check if the serializer is configured correctly (Preferences->impulse->Native Extensions->" + this.getFluxName().toUpperCase() + ")"));
                            this.base.getRoot().getElement().fireElementResetted();
                        }
                        catch (Throwable throwable) {}
                        this.errror = true;
                        break block22;
                    }
                    this.watchdog.finish();
                }
                catch (Throwable e) {
                    this.addParseErrorMessage(e);
                    this.errror = true;
                    return false;
                }
            }
            trace.close(-1, signals, this.end, null);
        }
        return true;
    }

    @Override
    public boolean supportsStreaming() {
        return false;
    }

    class Handler
    extends AbstractFluxHandler {
        private boolean finished;
        private int messageId;

        Handler() {
        }

        public void writeSchemeRequest() {
            this.finished = false;
            FluxParser.writeControlEntry(AbstractFluxDatabaseRecordReader.this.pout, true, 256, ++this.messageId, null);
        }

        public void writeItemRequest() {
            this.finished = false;
            StructMember[] structMemberArray = new StructMember[]{new StructMember(0, 2, 1), new StructMember(1, 1, "signalName"), new StructMember(2, 2, 2)};
            FluxParser.writeControlEntry(AbstractFluxDatabaseRecordReader.this.pout, true, 257, ++this.messageId, null);
        }

        public void writeTraceRequest(List<Integer> itemIds) {
            this.finished = false;
            int pos = 0;
            byte[] buffer = new byte[itemIds.size() * 5];
            for (Integer i : itemIds) {
                pos += PackedSamples.plusWrite(buffer, pos, i);
            }
            byte[] itemIdBytes = new byte[pos];
            System.arraycopy(buffer, 0, itemIdBytes, 0, pos);
            StructMember[] members = new StructMember[]{new StructMember(0, 6, itemIdBytes)};
            FluxParser.writeControlEntry(AbstractFluxDatabaseRecordReader.this.pout, true, 258, ++this.messageId, members);
        }

        @Override
        public boolean handleControl(IProgress p, FluxParser.Trace trace, boolean request, int controlId, int messageId, StructMember[] members, BinaryParseBuffer b) {
            if (!request && controlId == 256) {
                if (members != null) {
                    StructMember[] structMemberArray = members;
                    int n = members.length;
                    int n2 = 0;
                    while (n2 < n) {
                        StructMember m = structMemberArray[n2];
                        if (m.getId() == 0) {
                            AbstractFluxDatabaseRecordReader.this.version = m.getIntValue();
                        } else if (m.getId() == 1) {
                            AbstractFluxDatabaseRecordReader.this.maxTraceItems = m.getIntValue();
                        }
                        ++n2;
                    }
                }
            } else if (!request && controlId == 257 && messageId == this.messageId) {
                if (AbstractFluxDatabaseRecordReader.this.version <= 0 || AbstractFluxDatabaseRecordReader.this.maxTraceItems <= 0) {
                    b.setError("Unknown Version");
                } else {
                    AbstractFluxDatabaseRecordReader.this.parser.getCurrentTrace().applyDefinitions();
                    if (AbstractFluxDatabaseRecordReader.this.loadOnRequest) {
                        for (ICell cell : AbstractFluxDatabaseRecordReader.this.base.getTribe(false, Signal.class)) {
                            Signal signal;
                            if (cell.getData("SERIALIZER") != AbstractFluxDatabaseRecordReader.this || (signal = (Signal)cell) == null) continue;
                            signal.samples = new BytesPageable(new IPageGenerator(){

                                /*
                                 * WARNING - Removed try catching itself - possible behaviour change.
                                 */
                                @Override
                                public void fill(Pageable<?> pageable) {
                                    AbstractFluxDatabaseRecordReader abstractFluxDatabaseRecordReader = AbstractFluxDatabaseRecordReader.this;
                                    synchronized (abstractFluxDatabaseRecordReader) {
                                        if (pageable.needsGeneration()) {
                                            AbstractFluxDatabaseRecordReader.this.fillSignal(signal);
                                        }
                                    }
                                }

                                /*
                                 * WARNING - Removed try catching itself - possible behaviour change.
                                 */
                                @Override
                                public void prepare(Pageable<?> pageable) {
                                    AbstractFluxDatabaseRecordReader abstractFluxDatabaseRecordReader = AbstractFluxDatabaseRecordReader.this;
                                    synchronized (abstractFluxDatabaseRecordReader) {
                                        if (pageable.needsGeneration()) {
                                            AbstractFluxDatabaseRecordReader.this.prepareSignal(signal);
                                        }
                                    }
                                }
                            });
                            signal.domainBase = AbstractFluxDatabaseRecordReader.this.domain;
                            signal.start = AbstractFluxDatabaseRecordReader.this.start;
                            signal.end = AbstractFluxDatabaseRecordReader.this.end;
                            signal.rate = AbstractFluxDatabaseRecordReader.this.rate;
                            signal.processType = (AbstractFluxDatabaseRecordReader.this.rate > 0L ? ISamples.ProcessType.Continuous : ISamples.ProcessType.Discrete).toString();
                        }
                    }
                }
                this.finished = true;
            } else if (!request && controlId == 258 && messageId == this.messageId) {
                this.finished = true;
            }
            return true;
        }

        @Override
        public boolean handleOpen(IProgress p, FluxParser.Trace trace, int itemId, String domain, long start, long rate, BinaryParseBuffer b) {
            AbstractFluxDatabaseRecordReader.this.domain = domain;
            AbstractFluxDatabaseRecordReader.this.start = start;
            AbstractFluxDatabaseRecordReader.this.rate = rate;
            return false;
        }

        @Override
        public boolean handleClose(IProgress p, FluxParser.Trace currentTrace, int itemId, long end, BinaryParseBuffer b) {
            AbstractFluxDatabaseRecordReader.this.end = end;
            return false;
        }

        @Override
        public boolean handleData(IProgress p, FluxParser.Trace trace, int itemId, long delta, int size, BinaryParseBuffer b) {
            return false;
        }

        @Override
        public boolean isFinished() {
            return this.finished;
        }

        @Override
        public String adjustItemName(boolean isScope, String name) {
            name = name.replaceAll("\\s+\\[", "[");
            return name;
        }
    }
}

