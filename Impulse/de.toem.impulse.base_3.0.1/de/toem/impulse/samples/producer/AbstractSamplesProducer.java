/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.producer;

import de.toem.impulse.domain.DomainBase;
import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.domain.IDomainBaseProvider;
import de.toem.impulse.i18n.I18n;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesProducer;
import de.toem.impulse.samples.base.ReferencedReadableSamples;
import de.toem.toolkits.core.Assert;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.properties.IPropertyModel;
import de.toem.toolkits.pattern.threading.IProgress;
import de.toem.toolkits.pattern.threading.IProgressStatus;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public abstract class AbstractSamplesProducer
extends ReferencedReadableSamples
implements ISamplesProducer {
    protected String name;
    protected String id;
    protected String producer;
    protected static final int FLAG_processType = 1;
    protected static final int FLAG_signalType = 2;
    protected static final int FLAG_signalDescriptor = 4;
    protected static final int FLAG_productionBase = 8;
    protected static final int FLAG_start = 16;
    protected static final int FLAG_end = 32;
    protected static final int FLAG_rate = 64;
    protected int flags;
    protected ISamples.ProcessType processType;
    protected ISamples.SignalType signalType;
    protected ISamples.SignalDescriptor signalDescriptor;
    protected IDomainBase productionBase;
    protected long start;
    protected long end;
    protected long rate;
    protected String definition;
    protected String language;
    protected IPropertyModel parameters;
    protected IDomainBase readerBase;
    protected int mode;
    protected List<IReadableSamples> sources = new ArrayList<IReadableSamples>();
    protected List<Integer> releases = new ArrayList<Integer>();
    protected boolean volatileSources;
    protected boolean sourcesAreMonotonous;
    protected boolean settling;
    protected String error;
    protected int productionRelease;
    protected boolean updated;
    private Object[] data;

    public AbstractSamplesProducer() {
    }

    public AbstractSamplesProducer(String id, String name, int mode, Object sources, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase productionBase, String start, String end, String rate, String definition, String language, IPropertyModel parameters, IDomainBaseProvider readerBaseProvider) {
        this.id = id;
        this.name = name;
        this.mode = mode;
        if (sources instanceof List) {
            for (Object source : (List)sources) {
                if (source != null && !(source instanceof IReadableSamples)) continue;
                this.sources.add((IReadableSamples)source);
                this.releases.add(source != null ? ((IReadableSamples)source).getRelease() : 0);
                this.volatileSources |= source != null ? ((IReadableSamples)source).isVolatile() : false;
                this.sourcesAreMonotonous |= source != null ? ((IReadableSamples)source).isMonotonous() : false;
            }
        } else if (sources == null || sources instanceof IReadableSamples) {
            this.sources.add((IReadableSamples)sources);
            this.releases.add(sources != null ? ((IReadableSamples)sources).getRelease() : 0);
            this.volatileSources |= sources != null ? ((IReadableSamples)sources).isVolatile() : false;
            this.sourcesAreMonotonous |= sources != null ? ((IReadableSamples)sources).isMonotonous() : false;
        }
        this.processType = processType;
        this.signalType = signalType;
        this.signalDescriptor = signalDescriptor;
        this.definition = definition;
        this.language = language;
        this.parameters = parameters;
        this.productionBase = productionBase;
        this.init(start, end, rate, readerBaseProvider);
    }

    @Override
    public int update(String id, String name, Object sources, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase productionBase, String sstart, String send, String srate, String definition, String language, IPropertyModel parameters, IDomainBaseProvider readerBaseProvider) {
        if (!Utils.equals(this.id, id)) {
            return -1;
        }
        if (!Utils.equals(this.name, name)) {
            return -1;
        }
        if ((this.flags & 1) != 0 && this.processType != processType) {
            return -1;
        }
        if ((this.flags & 2) != 0 && this.signalType != signalType) {
            return -1;
        }
        if ((this.flags & 4) != 0 && !Utils.equals(this.signalDescriptor, signalDescriptor)) {
            return -1;
        }
        if ((this.flags & 8) != 0 && this.productionBase != productionBase) {
            return -1;
        }
        if ((this.flags & 0x10) != 0 && (sstart == null || this.start != this.productionBase.parseUnits(sstart))) {
            return -1;
        }
        if ((this.flags & 0x20) != 0 && (send == null || this.end != this.productionBase.parseUnits(send))) {
            return -1;
        }
        if ((this.flags & 0x40) != 0 && (srate == null || this.rate != this.productionBase.parseUnits(srate))) {
            return -1;
        }
        if ((this.flags & 1) == 0 && ISamples.ProcessType.Unknown != processType) {
            return -1;
        }
        if ((this.flags & 2) == 0 && ISamples.SignalType.Unknown != signalType) {
            return -1;
        }
        if ((this.flags & 4) == 0 && signalDescriptor != null) {
            return -1;
        }
        if ((this.flags & 8) == 0 && DomainBase.Unknown != productionBase) {
            return -1;
        }
        if (this.productionBase != DomainBase.Unknown) {
            if ((this.flags & 0x10) == 0 && !Utils.isEmpty(sstart)) {
                return -1;
            }
            if ((this.flags & 0x20) == 0 && !Utils.isEmpty(send)) {
                return -1;
            }
            if ((this.flags & 0x40) == 0 && !Utils.isEmpty(srate)) {
                return -1;
            }
        }
        if (!Utils.equals(this.definition, definition)) {
            return -1;
        }
        if (!Utils.equals(this.language, language)) {
            return -1;
        }
        if (!Utils.equals(this.parameters, parameters)) {
            return -1;
        }
        if (readerBaseProvider != null && !Utils.equals(this.readerBase, readerBaseProvider.getDomainBase(this.productionBase))) {
            return -1;
        }
        return this.update(sources);
    }

    @Override
    public int update(Object sources) {
        if (sources instanceof List && !Utils.equals(this.sources, sources)) {
            return -1;
        }
        if (sources instanceof IReadableSamples && (this.sources.size() != 1 || !Utils.equals(this.sources.get(0), sources))) {
            return -1;
        }
        if (sources == null && this.sources.size() != 0) {
            return -1;
        }
        return this.update();
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    @Override
    public int update() {
        boolean monotonousUpdate = false;
        boolean volatileSources = false;
        int n = 0;
        while (n < this.sources.size()) {
            boolean volatileSource;
            IReadableSamples source = this.sources.get(n);
            int release = source != null ? source.getRelease() : 0;
            boolean bl = volatileSource = source != null ? source.isVolatile() : false;
            if (release > this.releases.get(n)) {
                monotonousUpdate = true;
                volatileSources |= volatileSource;
                break;
            }
            Assert.isTrue(release == this.releases.get(n));
            ++n;
        }
        if (monotonousUpdate) {
            if (!this.isVolatile() || !this.sourcesAreMonotonous) {
                return -1;
            }
            long end = Long.MIN_VALUE;
            if ((this.flags & 0x20) == 0 && this.productionBase != DomainBase.Unknown) {
                for (IReadableSamples readable : this.sources) {
                    if (readable == null) continue;
                    if (readable.getDomainBase().getClass() != this.productionBase.getClass()) break;
                    long cend = readable.getDomainBase().convertTo(this.productionBase, readable.getEndUnits());
                    if (cend <= end) continue;
                    end = cend;
                }
                if (end < this.end) {
                    return -1;
                }
            }
            if (!this.isSettling()) {
                AbstractSamplesProducer abstractSamplesProducer = this;
                synchronized (abstractSamplesProducer) {
                    block14: {
                        if (this.isVolatile()) break block14;
                        return -1;
                    }
                    ++this.productionRelease;
                    this.updated = true;
                    int n2 = 0;
                    while (n2 < this.sources.size()) {
                        IReadableSamples source = this.sources.get(n2);
                        int release = source != null ? source.getRelease() : 0;
                        this.releases.set(n2, release);
                        ++n2;
                    }
                    this.volatileSources = volatileSources;
                    if (end != Long.MIN_VALUE) {
                        this.end = end;
                    }
                }
                this.updateInit();
                return 1;
            }
        }
        return 0;
    }

    @Override
    public String getError() {
        if (this.error != null) {
            return this.error;
        }
        return super.getError();
    }

    public void setError(String message) {
        if (Utils.isEmpty(this.error)) {
            this.error = message;
        }
    }

    public boolean hasError() {
        return this.error != null;
    }

    @Override
    public ISamplesProducer getProducer() {
        return this;
    }

    @Override
    public IDomainBase getProductionBase() {
        return this.productionBase;
    }

    @Override
    public List<IReadableSamples> getSources() {
        return this.sources;
    }

    @Override
    public int getRelease() {
        return this.productionRelease;
    }

    @Override
    public abstract boolean isVolatile();

    @Override
    public boolean isMonotonous() {
        return true;
    }

    @Override
    public boolean isReleased() {
        return true;
    }

    @Override
    public String getName() {
        return this.name;
    }

    @Override
    public String getId() {
        return this.id;
    }

    @Override
    public ISamples.ProcessType getProcessType() {
        return this.processType;
    }

    @Override
    public ISamples.SignalType getSignalType() {
        return this.signalType;
    }

    @Override
    public ISamples.SignalDescriptor getSignalDescriptor() {
        return this.signalDescriptor != null ? this.signalDescriptor : ISamples.SignalDescriptor.DEFAULT;
    }

    @Override
    public IDomainBase getDomainBase() {
        return this.readerBase;
    }

    @Override
    public DomainValue getStart() {
        return new DomainValue(this.readerBase, this.getStartUnits());
    }

    @Override
    public DomainValue getEnd() {
        return new DomainValue(this.readerBase, this.getEndUnits());
    }

    @Override
    public DomainValue getRate() {
        return new DomainValue(this.readerBase, this.getRateUnits());
    }

    @Override
    public long getStartUnits() {
        return this.productionBase.convertTo(this.readerBase, this.start);
    }

    @Override
    public long getEndUnits() {
        return this.productionBase.convertTo(this.readerBase, this.end);
    }

    @Override
    public long getRateUnits() {
        return this.productionBase.convertTo(this.readerBase, this.rate);
    }

    /*
     * Exception decompiling
     */
    protected void init(String sstart, String send, String srate, IDomainBaseProvider readerBaseProvider) {
        /*
         * This method has failed to decompile.  When submitting a bug report, please provide this stack trace, and (if you hold appropriate legal rights) the relevant class file.
         * 
         * org.benf.cfr.reader.util.ConfusedCFRException: Tried to end blocks [0[TRYBLOCK]], but top level block is 11[DOLOOP]
         *     at org.benf.cfr.reader.bytecode.analysis.opgraph.Op04StructuredStatement.processEndingBlocks(Op04StructuredStatement.java:435)
         *     at org.benf.cfr.reader.bytecode.analysis.opgraph.Op04StructuredStatement.buildNestedBlocks(Op04StructuredStatement.java:484)
         *     at org.benf.cfr.reader.bytecode.analysis.opgraph.Op03SimpleStatement.createInitialStructuredBlock(Op03SimpleStatement.java:736)
         *     at org.benf.cfr.reader.bytecode.CodeAnalyser.getAnalysisInner(CodeAnalyser.java:850)
         *     at org.benf.cfr.reader.bytecode.CodeAnalyser.getAnalysisOrWrapFail(CodeAnalyser.java:278)
         *     at org.benf.cfr.reader.bytecode.CodeAnalyser.getAnalysis(CodeAnalyser.java:201)
         *     at org.benf.cfr.reader.entities.attributes.AttributeCode.analyse(AttributeCode.java:94)
         *     at org.benf.cfr.reader.entities.Method.analyse(Method.java:531)
         *     at org.benf.cfr.reader.entities.ClassFile.analyseMid(ClassFile.java:1055)
         *     at org.benf.cfr.reader.entities.ClassFile.analyseTop(ClassFile.java:942)
         *     at org.benf.cfr.reader.Driver.doJarVersionTypes(Driver.java:257)
         *     at org.benf.cfr.reader.Driver.doJar(Driver.java:139)
         *     at org.benf.cfr.reader.CfrDriverImpl.analyse(CfrDriverImpl.java:76)
         *     at org.benf.cfr.reader.Main.main(Main.java:54)
         */
        throw new IllegalStateException("Decompilation failed");
    }

    protected void modifyInit() {
    }

    protected void updateInit() {
    }

    @Override
    public final boolean isSettled() {
        if (this.error != null) {
            return false;
        }
        if (this.reference instanceof IReadableSamples) {
            return super.isSettled();
        }
        return false;
    }

    @Override
    public final boolean isSettling() {
        return this.settling;
    }

    public final boolean isUpdated() {
        return this.updated;
    }

    @Override
    public final boolean areSourcesSettling() {
        for (IReadableSamples source : this.sources) {
            if (source instanceof ISamplesProducer && ((ISamplesProducer)source).areSourcesSettling()) {
                return true;
            }
            if (source == null || !source.isSettling()) continue;
            return true;
        }
        return false;
    }

    @Override
    public boolean ensureSettled(IProgress p) {
        return this.ensureSettled(p, true);
    }

    /*
     * Enabled aggressive block sorting
     * Enabled unnecessary exception pruning
     * Enabled aggressive exception aggregation
     */
    protected boolean ensureSettled(IProgress p, boolean produce) {
        if (!this.requiresSettlement(produce)) {
            return true;
        }
        if (this.error != null) {
            return false;
        }
        AbstractSamplesProducer abstractSamplesProducer = this;
        synchronized (abstractSamplesProducer) {
            if (!this.requiresSettlement(produce)) {
                return true;
            }
            if (this.error != null) {
                return false;
            }
            try {
                boolean settled;
                this.settling = true;
                this.volatileSources = false;
                boolean monotonousUpdate = false;
                int n = 0;
                while (true) {
                    if (n >= this.sources.size()) {
                        if (monotonousUpdate) {
                            ++this.productionRelease;
                        }
                        try {
                            if (!produce) break;
                            this.produce(p, Collections.unmodifiableList(this.sources));
                        }
                        catch (Throwable e) {
                            this.error = e.toString();
                        }
                        break;
                    }
                    IReadableSamples source = this.sources.get(n);
                    if (source != null) {
                        this.volatileSources |= source.isVolatile();
                        if (source.getRelease() > this.releases.get(n)) {
                            this.releases.set(n, source.getRelease());
                            monotonousUpdate = true;
                        }
                        if (!source.ensureSettled(p)) {
                            if (this.error == null) {
                                this.error = I18n.Producer_SourceErrors_;
                            }
                            if (source.getError() != null) {
                                this.error = String.valueOf(this.error) + source.getError();
                            }
                            return false;
                        }
                    }
                    ++n;
                }
                if (!(settled = super.ensureSettled(p)) && this.error == null) {
                    String string = this.error = this.reference != null ? this.reference.getError() : null;
                    if (this.error == null) {
                        this.error = "Result could not be settled";
                    }
                }
                if (p != null && p.isCanceled()) {
                    if (p instanceof IProgressStatus && ((IProgressStatus)p).hadTimeout()) {
                        this.setError(I18n.General_Timeout);
                    } else {
                        this.setError(I18n.General_Cancelled);
                    }
                }
                boolean bl = settled;
                return bl;
            }
            finally {
                this.settling = false;
                this.updated = false;
            }
        }
    }

    protected boolean requiresSettlement(boolean produce) {
        return !this.isSettled() || this.isUpdated();
    }

    public abstract void produce(IProgress var1, List<IReadableSamples> var2);

    @Override
    public boolean equals(Object obj) {
        if (!(obj instanceof AbstractSamplesProducer)) {
            return false;
        }
        AbstractSamplesProducer that = (AbstractSamplesProducer)obj;
        if (!Utils.equals(this.getClass(), that.getClass())) {
            return false;
        }
        if (!Utils.equals(this.id, that.id)) {
            return false;
        }
        if (!Utils.equals(this.name, that.name)) {
            return false;
        }
        if (this.flags != that.flags) {
            return false;
        }
        if ((this.flags & 1) != 0 && this.processType != that.processType) {
            return false;
        }
        if ((this.flags & 2) != 0 && this.signalType != that.signalType) {
            return false;
        }
        if ((this.flags & 4) != 0 && !Utils.equals(this.signalDescriptor, that.signalDescriptor)) {
            return false;
        }
        if ((this.flags & 8) != 0 && this.productionBase != that.productionBase) {
            return false;
        }
        if (this.productionBase != DomainBase.Unknown) {
            if ((this.flags & 0x10) != 0 && this.start != that.start) {
                return false;
            }
            if ((this.flags & 0x20) != 0 && this.end != that.end) {
                return false;
            }
            if ((this.flags & 0x40) != 0 && this.rate != that.rate) {
                return false;
            }
        }
        if (!Utils.equals(this.sources, that.sources)) {
            return false;
        }
        if (!Utils.equals(this.definition, that.definition)) {
            return false;
        }
        if (!Utils.equals(this.language, that.language)) {
            return false;
        }
        if (!Utils.equals(this.parameters, that.parameters)) {
            return false;
        }
        return Utils.equals(this.readerBase, that.readerBase);
    }

    @Override
    public Object getData() {
        return this.data != null ? this.data[0] : null;
    }

    @Override
    public Object getData(String key) {
        if (this.data != null) {
            int n = 1;
            while (n < this.data.length) {
                if (key.equals(this.data[n])) {
                    return this.data[n + 1];
                }
                n += 2;
            }
        }
        return null;
    }

    @Override
    public void setData(Object value) {
        if (this.data == null) {
            this.data = new Object[1];
        }
        this.data[0] = value;
    }

    @Override
    public void setData(String key, Object value) {
        if (this.data != null) {
            int n = 1;
            while (n < this.data.length) {
                if (key.equals(this.data[n])) {
                    this.data[n + 1] = value;
                    if (value == null) {
                        this.data[n] = null;
                    }
                    return;
                }
                n += 2;
            }
            if (value != null) {
                n = 1;
                while (n < this.data.length) {
                    if (this.data[n] == null) {
                        this.data[n + 1] = value;
                        this.data[n] = key;
                        return;
                    }
                    n += 2;
                }
                Object[] newOne = new Object[this.data.length + 2];
                System.arraycopy(this.data, 0, newOne, 0, this.data.length);
                newOne[this.data.length] = key;
                newOne[this.data.length + 1] = value;
                this.data = newOne;
                return;
            }
        } else {
            this.data = new Object[3];
            this.data[1] = key;
            this.data[2] = value;
        }
    }
}

