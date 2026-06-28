/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.serializer;

import de.toem.impulse.ImpulseLicense;
import de.toem.impulse.cells.ports.IPortProgress;
import de.toem.impulse.cells.record.AbstractSignal;
import de.toem.impulse.cells.record.Record;
import de.toem.impulse.cells.record.Scope;
import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.cells.record.SignalProxy;
import de.toem.impulse.cells.view.FolderConfiguration;
import de.toem.impulse.cells.view.FolderConfigurationInstancer;
import de.toem.impulse.cells.view.PlotConfiguration;
import de.toem.impulse.cells.view.PlotConfigurationInstancer;
import de.toem.impulse.cells.view.PlotConfigurationTemplate;
import de.toem.impulse.cells.view.ViewConfiguration;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.samples.IRecordGenerator;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesReader;
import de.toem.impulse.samples.ISamplesWriter;
import de.toem.impulse.samples.base.PackedSamples;
import de.toem.impulse.samples.writer.DivergingSamplesWriter;
import de.toem.impulse.samples.writer.IConvergingSamplesWriter;
import de.toem.impulse.samples.writer.IDivergingSamplesWriter;
import de.toem.impulse.serializer.AbstractRecordReader;
import de.toem.impulse.serializer.IParsingRecordReader;
import de.toem.toolkits.pattern.element.Cover;
import de.toem.toolkits.pattern.element.Elements;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.ICover;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.element.serializer.Message;
import de.toem.toolkits.pattern.element.serializer.ParseException;
import de.toem.toolkits.pattern.pageable.BytesPageable;
import de.toem.toolkits.pattern.pageable.IPageGenerator;
import de.toem.toolkits.pattern.pageable.PageTable;
import de.toem.toolkits.pattern.pageable.Pageable;
import de.toem.toolkits.pattern.properties.PropertyModel;
import de.toem.toolkits.pattern.threading.IProgress;
import java.io.IOException;
import java.io.InputStream;
import java.io.PushbackInputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.locks.ReentrantLock;
import java.util.zip.GZIPInputStream;

public abstract class AbstractParsingRecordReader
extends AbstractRecordReader
implements IRecordGenerator,
IParsingRecordReader {
    protected static final String CURRENT = "CURRENT";
    protected ICover cover;
    protected Record record;
    protected ICell base;
    protected List<ISamplesWriter> writers = new ArrayList<ISamplesWriter>();
    private Long current;
    private int changed;
    private ReentrantLock lock;

    public AbstractParsingRecordReader() {
    }

    public AbstractParsingRecordReader(String id, InputStream in) {
        super(id, in);
    }

    public static PropertyModel getDefaultPropertyModel() {
        return new PropertyModel();
    }

    public boolean isGzipStream(byte[] bytes) {
        int head = bytes[0] & 0xFF | bytes[1] << 8 & 0xFF00;
        return 35615 == head;
    }

    public InputStream decompressStream(InputStream in) throws IOException {
        PushbackInputStream pb = new PushbackInputStream(in, 2);
        byte[] head = new byte[2];
        int len = pb.read(head);
        pb.unread(head, 0, len);
        if (this.isGzipStream(head)) {
            return new GZIPInputStream(pb);
        }
        return pb;
    }

    /*
     * Unable to fully structure code
     */
    @Override
    public ICover read(IProgress progress, String configurationName, ICell base, int insert) {
        block23: {
            block22: {
                this.record = new Record();
                this.cover = new Cover(this.record);
                this.cover.setSerializer(this.id);
                if (insert == 0) {
                    this.base = this.record;
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
                    try {
                        if (this.configuration == null) {
                            this.configuration = this.findConfiguration(configurationName);
                        }
                        if (this.configuration != null) {
                            this.readConfigurationProperties(this.configuration, this.properties);
                            this.cover.setConfiguration(this.configuration.getName());
                        }
                        try {
                            if (this.prepare(progress, this.in)) {
                                this.parse(progress, this.in);
                            }
                            this.postpare(progress);
                        }
                        catch (ParseException e) {
                            this.addParseErrorMessage(e);
                        }
                        for (ICell cell : this.base.getTribe(false, Signal.class)) {
                            if (cell.getData("SERIALIZER") != this || (writer = (ISamplesWriter)(signal = (Signal)cell).getData("WRITER")) == null) continue;
                            writer.apply(signal);
                        }
                        break block22;
                    }
                    catch (Throwable e) {
                        this.addParseErrorMessage(e);
                        PageTable.setBoost(false);
                        ** for (cell : this.base.getTribe((boolean)true))
                    }
                }
                catch (Throwable var9_17) {
                    PageTable.setBoost(false);
                    ** for (cell : this.base.getTribe((boolean)true))
                }
lbl-1000:
                // 1 sources

                {
                    cell.setData("SERIALIZER", null);
                    cell.setData("WRITER", null);
                    continue;
                }
lbl43:
                // 1 sources

                if (this.lock != null) {
                    this.unlock();
                }
                break block23;
lbl-1000:
                // 1 sources

                {
                    cell.setData("SERIALIZER", null);
                    cell.setData("WRITER", null);
                    continue;
                }
lbl52:
                // 1 sources

                if (this.lock != null) {
                    this.unlock();
                }
                throw var9_17;
            }
            PageTable.setBoost(false);
            for (ICell cell : this.base.getTribe(true)) {
                cell.setData("SERIALIZER", null);
                cell.setData("WRITER", null);
            }
            if (this.lock != null) {
                this.unlock();
            }
        }
        return this.cover;
    }

    protected boolean prepare(IProgress progress, InputStream in) throws ParseException {
        return true;
    }

    protected void postpare(IProgress progress) throws ParseException {
    }

    protected abstract void parse(IProgress var1, InputStream var2) throws ParseException;

    public void addParseErrorMessage(Throwable e) {
        AbstractParsingRecordReader.addParseErrorMessage(this.id, e, this.base);
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     * Enabled aggressive block sorting
     * Enabled unnecessary exception pruning
     * Enabled aggressive exception aggregation
     */
    protected void waitStreaming(IProgress progress) {
        if (!(progress instanceof IPortProgress)) return;
        IProgress iProgress = progress;
        synchronized (iProgress) {
            while (!((IPortProgress)progress).isStreaming()) {
                if (progress.isCanceled()) {
                    return;
                }
                try {
                    progress.wait(250L);
                }
                catch (InterruptedException interruptedException) {}
            }
            return;
        }
    }

    @Override
    public boolean supportsStreaming() {
        return true;
    }

    @Override
    public ICover flush() {
        try {
            this.lock();
            if (this.changed != 0) {
                this.changed = 0;
                ICover iCover = this.current != null ? this.doFlush(this.current) : this.doFlush();
                return iCover;
            }
            return null;
        }
        finally {
            this.unlock();
        }
    }

    @Override
    public int hasChanged() {
        return this.changed;
    }

    private void lock() {
        if (this.lock == null) {
            this.lock = new ReentrantLock();
            this.lock.lock();
        }
        this.lock.lock();
    }

    private void unlock() {
        if (this.lock == null) {
            this.lock = new ReentrantLock();
            this.lock.lock();
        }
        this.lock.unlock();
    }

    @Override
    public void changed(int changed, long units) {
        if (this.current == null || units > this.current) {
            this.current = units;
        }
        this.changed |= changed;
        this.unlock();
        this.lock();
    }

    @Override
    public void changed(int changed) {
        this.changed |= changed;
        this.unlock();
        this.lock();
    }

    @Override
    public long current() {
        return this.current != null ? this.current : 0L;
    }

    protected ICover doFlush(long units) {
        for (ICell cell : this.base.getTribe(false, Signal.class)) {
            if (cell.getData("SERIALIZER") != this) continue;
            Signal signal = (Signal)cell;
            ISamplesWriter writer = (ISamplesWriter)cell.getData("WRITER");
            writer.flush(units);
            writer.apply(signal);
        }
        return this.cover;
    }

    protected ICover doFlush() {
        for (ICell cell : this.base.getTribe(false, Signal.class)) {
            if (cell.getData("SERIALIZER") != this) continue;
            Signal signal = (Signal)cell;
            ISamplesWriter writer = (ISamplesWriter)cell.getData("WRITER");
            Long current = (Long)signal.getData(CURRENT);
            if (current != null) {
                writer.flush(current);
                writer.apply(signal);
                continue;
            }
            writer.flush();
            writer.apply(signal);
        }
        return this.cover;
    }

    @Override
    public void initRecord(String name) {
        this.record.setName(name);
    }

    @Override
    public ICell getBase() {
        return this.base;
    }

    @Override
    public Scope addScope(ICell container, String name) {
        if (container == null) {
            container = this.base;
        }
        Scope child = new Scope();
        container.addChild(child);
        child.setName(container.uniqueChildName(name));
        return child;
    }

    @Override
    public Scope addScope(ICell container, String name, String description) {
        if (container == null) {
            container = this.base;
        }
        Scope child = new Scope();
        container.addChild(child);
        child.setName(container.uniqueChildName(name));
        child.description = description != null ? description.intern() : null;
        return child;
    }

    @Override
    public Signal addSignal(ICell container, String name, String description, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase) {
        return this.addSignal(container, name, description, processType, signalType, signalDescriptor, domainBase, true);
    }

    @Override
    public Signal addSignal(ICell container, String name, String description, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase, boolean createWriter) {
        if (container == null) {
            container = this.base;
        }
        Signal signal = new Signal();
        signal.setData("SERIALIZER", this);
        container.addChild(signal);
        signal.setName(container.uniqueChildName(name));
        signal.description = description != null ? description.intern() : null;
        signal.signalType = signalType.toString();
        signal.processType = processType.toString();
        signal.signalDescriptor = signalDescriptor != null ? signalDescriptor.toString().intern() : null;
        String string = signal.domainBase = domainBase != null ? domainBase.toString() : null;
        if (createWriter) {
            ISamplesWriter writer = PackedSamples.createWriter(signal.getPath(), signal.getName(), processType, signalType, signalDescriptor, domainBase, false);
            this.writers.add(writer);
            signal.setData("WRITER", writer);
        }
        return signal;
    }

    @Override
    public SignalProxy addSignalProxy(ICell container, String name, String description, Signal signal) {
        if (container == null) {
            container = this.base;
        }
        SignalProxy proxy = new SignalProxy();
        container.addChild(proxy);
        proxy.setName(container.uniqueChildName(name));
        proxy.description = description != null ? description.intern() : null;
        proxy.signal = signal != null ? signal.getLink(this.record) : null;
        return proxy;
    }

    @Override
    public ISamplesWriter createWriter(Signal signal) {
        ISamplesWriter writer = PackedSamples.createWriter(signal, false);
        this.writers.add(writer);
        signal.setData("WRITER", writer);
        return writer;
    }

    @Override
    public ISamplesWriter createWriter(Signal signal, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase) {
        ISamplesWriter writer = PackedSamples.createWriter(signal.getPath(), signal.getName(), processType, signalType, signalDescriptor, domainBase, false);
        this.writers.add(writer);
        signal.setData("WRITER", writer);
        return writer;
    }

    @Override
    public IConvergingSamplesWriter createConvergingWriter(Signal signal, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase) {
        IConvergingSamplesWriter writer = (IConvergingSamplesWriter)PackedSamples.createWriter(signal.getPath(), signal.getName(), processType, signalType, signalDescriptor, domainBase, true);
        this.writers.add(writer);
        signal.setData("WRITER", writer);
        return writer;
    }

    @Override
    public IDivergingSamplesWriter createDivergingWriter(ISamplesWriter initialTarget) {
        DivergingSamplesWriter writer = initialTarget != null ? new DivergingSamplesWriter(initialTarget) : new DivergingSamplesWriter();
        return writer;
    }

    @Override
    public ISamplesWriter getWriter(Signal signal) {
        return (ISamplesWriter)signal.getData("WRITER");
    }

    @Override
    public void createWriteHandler(final Signal signal, final IRecordGenerator.IWriteHandler handler) {
        signal.samples = new BytesPageable(new IPageGenerator(){

            @Override
            public void fill(Pageable<?> pageable) {
                if (pageable.needsGeneration()) {
                    handler.handle(0, signal, signal != null ? signal.samples : null);
                }
            }

            @Override
            public void prepare(Pageable<?> pageable) {
                if (pageable.needsGeneration()) {
                    handler.handle(1, signal, signal != null ? signal.samples : null);
                }
            }
        });
    }

    @Override
    public List<ICell> getAllSignals() {
        ArrayList<ICell> list = new ArrayList<ICell>();
        for (ICell cell : this.base.getTribe(false, Signal.class)) {
            if (cell.getData("SERIALIZER") != this) continue;
            list.add(cell);
        }
        return list;
    }

    @Override
    public Signal getSignal(String path) {
        ICell cell = this.base.getCell(path);
        if (cell instanceof Signal) {
            return (Signal)cell;
        }
        return null;
    }

    @Override
    public Scope getScope(String path) {
        ICell cell = this.base.getCell(path);
        if (cell instanceof Scope) {
            return (Scope)cell;
        }
        return null;
    }

    @Override
    public void apply() {
        for (ICell cell : this.base.getTribe(false, Signal.class)) {
            if (cell.getData("SERIALIZER") != this) continue;
            Signal signal = (Signal)cell;
            ISamplesWriter writer = (ISamplesWriter)cell.getData("WRITER");
            writer.apply(signal);
        }
    }

    @Override
    public ISamplesReader createReader(Signal signal) {
        return PackedSamples.createReader(signal);
    }

    @Override
    public ViewConfiguration addViewConfiguration(String name, String description) {
        ViewConfiguration config = new ViewConfiguration();
        config.setName(name);
        config.description = description != null ? description.intern() : null;
        this.base.addChild(config);
        return config;
    }

    @Override
    public ViewConfiguration addViewConfiguration(ViewConfiguration config) {
        this.base.addChild(config);
        return config;
    }

    @Override
    public ViewConfiguration addViewConfigurations(byte[] wallet) {
        IElement element = Elements.getElement(wallet);
        if (element.isBound() && element.hasCell()) {
            List<ViewConfiguration> configs = element.getCell().getChildren(ViewConfiguration.class);
            for (ICell iCell : configs) {
                this.base.addChild(iCell.clone());
            }
        }
        return null;
    }

    @Override
    public FolderConfiguration addFolderConfiguration(ICell container, String name, String description) {
        FolderConfiguration folder = new FolderConfiguration();
        folder.setName(name);
        folder.description = description;
        container.addChild(folder);
        return folder;
    }

    @Override
    public FolderConfiguration addFolderConfiguration(ICell container, Scope scope) {
        List<PlotConfigurationTemplate> autoTemplates = PlotConfigurationInstancer.getAutoTemplates();
        FolderConfiguration folder = (FolderConfiguration)FolderConfigurationInstancer.createCellFromSource(scope, null, autoTemplates);
        if (folder != null) {
            container.addChild(folder);
        }
        return folder;
    }

    @Override
    public PlotConfiguration addPlotConfiguration(ICell container, String name, String description) {
        PlotConfiguration samples = new PlotConfiguration();
        samples.setName(name);
        samples.description = description;
        container.addChild(samples);
        return samples;
    }

    @Override
    public PlotConfiguration addPlotConfiguration(ICell container, AbstractSignal source) {
        ArrayList<ICell> childCells = new ArrayList<ICell>();
        List<PlotConfigurationTemplate> autoTemplates = PlotConfigurationInstancer.getAutoTemplates();
        PlotConfigurationInstancer.createCellsFromSignalSource(source, null, autoTemplates, childCells, false);
        PlotConfiguration samples = null;
        for (ICell c : childCells) {
            container.addChild(c);
            if (!(c instanceof PlotConfiguration)) continue;
            samples = (PlotConfiguration)c;
        }
        return samples;
    }

    public ViewConfiguration addConfiguration(String name, String description) {
        return this.addViewConfiguration(name, description);
    }

    public ViewConfiguration addConfiguration(ViewConfiguration config) {
        return this.addViewConfiguration(config);
    }

    public ViewConfiguration addConfigurations(byte[] wallet) {
        return this.addViewConfigurations(wallet);
    }

    public PlotConfiguration addSamplesConfiguration(ICell container, String name, String description) {
        return this.addPlotConfiguration(container, name, description);
    }

    public PlotConfiguration addSamplesConfiguration(ICell container, AbstractSignal source) {
        return this.addPlotConfiguration(container, source);
    }
}

