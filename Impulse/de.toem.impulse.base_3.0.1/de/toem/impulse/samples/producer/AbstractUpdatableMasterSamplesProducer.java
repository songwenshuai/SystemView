/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.producer;

import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.domain.IDomainBaseProvider;
import de.toem.impulse.paint.IPaintStyle;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesDisplayInformation;
import de.toem.impulse.samples.ISamplesProducer;
import de.toem.impulse.samples.ISamplesReader;
import de.toem.impulse.samples.ISamplesWriter;
import de.toem.impulse.samples.base.PackedSamples;
import de.toem.impulse.samples.base.ReferencedReadableSamples;
import de.toem.impulse.samples.producer.AbstractUpdatableSamplesProducer;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.Link;
import de.toem.toolkits.pattern.properties.IPropertyModel;
import de.toem.toolkits.pattern.threading.IProgress;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public abstract class AbstractUpdatableMasterSamplesProducer
extends AbstractUpdatableSamplesProducer
implements ISamplesProducer.IMasterProducer {
    protected static final int MODE_MASTER_SLAVE = 1;
    protected static final int MODE_LOOPTHROUGH_SLAVE = 2;
    protected List<AbstractSlaveProduction> slaveProductions;
    protected Map<String, AbstractSlaveProduction> idMap;
    protected int slaveRelease = -1;

    public AbstractUpdatableMasterSamplesProducer() {
    }

    public AbstractUpdatableMasterSamplesProducer(String id, String name, int mode, Object sources, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase, String start, String end, String rate, String definition, String language, IPropertyModel parameters, IDomainBaseProvider readerBaseProvider) {
        super(id, name, mode, sources, processType, signalType, signalDescriptor, domainBase, start, end, rate, definition, language, parameters, readerBaseProvider);
    }

    protected IReadableSamples loopThroughSource() {
        return null;
    }

    @Override
    protected void init(String sstart, String send, String srate, IDomainBaseProvider readerBaseProvider) {
        if (this.mode == 2) {
            this.setReference(this.loopThroughSource());
        }
        super.init(sstart, send, srate, readerBaseProvider);
    }

    @Override
    public boolean hasChildSlaves(String slaveId) {
        return this.mode != 0 && slaveId == null;
    }

    @Override
    public List<String> getChildSlaveIds(String slaveId) {
        ArrayList<String> list = new ArrayList<String>();
        if (this.mode != 0 && this.slaveProductions != null) {
            for (AbstractSlaveProduction c : this.slaveProductions) {
                if (!Utils.equals(slaveId, c.parentId)) continue;
                list.add(c.slaveId);
            }
        }
        return list;
    }

    @Override
    public IReadableSamples getSlaveProduction(String slaveId) {
        if (slaveId == null) {
            return this;
        }
        if (this.mode != 0 && this.idMap != null) {
            return this.idMap.get(slaveId);
        }
        return null;
    }

    @Override
    public boolean ensureSettled(IProgress p) {
        return super.ensureSettled(p, this.mode != 2);
    }

    protected boolean ensureSlavesSettled(IProgress p) {
        return super.ensureSettled(p, true);
    }

    @Override
    protected boolean requiresSettlement(boolean produce) {
        return !this.isSettled() || this.isUpdated() || produce && this.productionRelease != this.slaveRelease;
    }

    @Override
    public void produce(IProgress p, List<IReadableSamples> sourceList) {
        super.produce(p, sourceList);
        this.slaveRelease = this.productionRelease;
    }

    @Override
    protected void flushOrClose(boolean continueProducing) {
        super.flushOrClose(continueProducing);
        if (this.mode != 0 && this.slaveProductions != null) {
            for (AbstractSlaveProduction slave : this.slaveProductions) {
                slave.flushOrClose(continueProducing);
            }
        }
    }

    @Override
    protected boolean updateReader() {
        boolean readerCreated = super.updateReader();
        if (this.mode != 0 && this.slaveProductions != null) {
            for (AbstractSlaveProduction slave : this.slaveProductions) {
                readerCreated |= slave.updateReader();
            }
        }
        return readerCreated;
    }

    @Override
    protected void destroy() {
        super.destroy();
        if (this.mode != 0 && this.slaveProductions != null) {
            for (AbstractSlaveProduction slave : this.slaveProductions) {
                slave.destroy();
            }
        }
    }

    protected abstract class AbstractSlaveProduction
    extends ReferencedReadableSamples
    implements ISamplesProducer.ISlaveProduction,
    ISamplesDisplayInformation {
        public String samplesId;
        public String slaveId;
        public String parentId;
        public String name;
        public ISamples.SignalType signalType;
        public ISamples.SignalDescriptor signalDescriptor;
        public boolean previousTag;
        public Object previousValue;

        public AbstractSlaveProduction(String slaveId, String parentId, String name, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor) {
            Link sid = Link.parse(AbstractUpdatableMasterSamplesProducer.this.getId());
            sid.setParameter("cid", slaveId);
            this.samplesId = sid.toString();
            this.slaveId = slaveId;
            this.parentId = parentId;
            this.name = name;
            this.signalType = signalType;
            this.signalDescriptor = signalDescriptor;
            if (AbstractUpdatableMasterSamplesProducer.this.slaveProductions == null) {
                AbstractUpdatableMasterSamplesProducer.this.slaveProductions = new ArrayList<AbstractSlaveProduction>();
            }
            if (AbstractUpdatableMasterSamplesProducer.this.idMap == null) {
                AbstractUpdatableMasterSamplesProducer.this.idMap = new HashMap<String, AbstractSlaveProduction>();
            }
            AbstractUpdatableMasterSamplesProducer.this.slaveProductions.add(this);
            AbstractUpdatableMasterSamplesProducer.this.idMap.put(this.slaveId, this);
        }

        @Override
        public String getSlaveId() {
            return this.slaveId;
        }

        public boolean isInstantiated() {
            return this.reference != null;
        }

        public abstract void instantiate();

        public abstract void flushOrClose(boolean var1);

        public abstract boolean updateReader();

        protected abstract void destroy();

        @Override
        public IPaintStyle getPaintStyle() {
            return null;
        }

        @Override
        public int getValueColumnFormat() {
            return -1;
        }

        @Override
        public Object getColor() {
            return null;
        }

        @Override
        public String getIconId() {
            return null;
        }

        @Override
        public boolean ensureSettled(IProgress p) {
            return AbstractUpdatableMasterSamplesProducer.this.ensureSlavesSettled(p) && super.ensureSettled(p);
        }

        @Override
        public final boolean isSettled() {
            return AbstractUpdatableMasterSamplesProducer.this.isSettled() && super.isSettled();
        }

        @Override
        public final boolean isSettling() {
            return AbstractUpdatableMasterSamplesProducer.this.isSettling() || AbstractUpdatableMasterSamplesProducer.this.areSourcesSettling() || super.isSettling();
        }

        @Override
        public boolean isReleased() {
            return AbstractUpdatableMasterSamplesProducer.this.isReleased();
        }

        @Override
        public int getRelease() {
            return AbstractUpdatableMasterSamplesProducer.this.getRelease();
        }

        @Override
        public boolean isMonotonous() {
            return AbstractUpdatableMasterSamplesProducer.this.isMonotonous();
        }

        @Override
        public boolean isVolatile() {
            return AbstractUpdatableMasterSamplesProducer.this.isVolatile();
        }

        @Override
        public String getName() {
            return this.name;
        }

        @Override
        public String getId() {
            return this.samplesId;
        }

        @Override
        public ISamples.ProcessType getProcessType() {
            return AbstractUpdatableMasterSamplesProducer.this.getProcessType();
        }

        @Override
        public ISamples.SignalType getSignalType() {
            return this.signalType;
        }

        @Override
        public ISamples.SignalDescriptor getSignalDescriptor() {
            return this.signalDescriptor;
        }

        @Override
        public IDomainBase getDomainBase() {
            return AbstractUpdatableMasterSamplesProducer.this.getDomainBase();
        }

        @Override
        public DomainValue getStart() {
            return AbstractUpdatableMasterSamplesProducer.this.getStart();
        }

        @Override
        public DomainValue getEnd() {
            return AbstractUpdatableMasterSamplesProducer.this.getEnd();
        }

        @Override
        public DomainValue getRate() {
            return AbstractUpdatableMasterSamplesProducer.this.getRate();
        }

        @Override
        public long getStartUnits() {
            return AbstractUpdatableMasterSamplesProducer.this.getStartUnits();
        }

        @Override
        public long getEndUnits() {
            return AbstractUpdatableMasterSamplesProducer.this.getEndUnits();
        }

        @Override
        public long getRateUnits() {
            return AbstractUpdatableMasterSamplesProducer.this.getRateUnits();
        }
    }

    protected class AbstractWritableSlaveProduction
    extends AbstractSlaveProduction
    implements ISamplesProducer.IWritableSlaveProduction {
        public ISamplesWriter targetWriter;

        public AbstractWritableSlaveProduction(String slaveId, String parentId, String name, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor) {
            super(slaveId, parentId, name, signalType, signalDescriptor);
        }

        @Override
        public void instantiate() {
            this.targetWriter = PackedSamples.createWriter(AbstractUpdatableMasterSamplesProducer.this.processType, this.signalType, this.signalDescriptor, AbstractUpdatableMasterSamplesProducer.this.productionBase);
            if (this.targetWriter != null) {
                this.targetWriter.open(AbstractUpdatableMasterSamplesProducer.this.start, (AbstractUpdatableMasterSamplesProducer.this.flags & 0x20) != 0 ? AbstractUpdatableMasterSamplesProducer.this.end : AbstractUpdatableMasterSamplesProducer.this.start, AbstractUpdatableMasterSamplesProducer.this.rate, 0, 0, null);
                this.setReference(PackedSamples.createReader(this.targetWriter, AbstractUpdatableMasterSamplesProducer.this.readerBase));
            }
        }

        @Override
        public void flushOrClose(boolean continueProducing) {
            if (this.targetWriter != null) {
                if (continueProducing) {
                    this.targetWriter.flush(AbstractUpdatableMasterSamplesProducer.this.iter.current(this.targetWriter));
                } else {
                    this.targetWriter.close(AbstractUpdatableMasterSamplesProducer.this.end);
                }
            }
        }

        @Override
        public boolean updateReader() {
            ISamples reference;
            boolean readerCreated = false;
            if (this.targetWriter != null && (reference = this.getReference()) instanceof ISamplesReader && ((ISamplesReader)reference).update(this.targetWriter, AbstractUpdatableMasterSamplesProducer.this.readerBase) < 0) {
                reference = PackedSamples.createReader(this.targetWriter, AbstractUpdatableMasterSamplesProducer.this.readerBase);
                readerCreated = true;
                this.setReference(reference);
            }
            return readerCreated;
        }

        @Override
        protected void destroy() {
            if (this.targetWriter != null) {
                this.targetWriter = null;
            }
        }

        @Override
        public ISamplesWriter getWriter() {
            return this.targetWriter;
        }
    }
}

