/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.base;

import de.toem.impulse.domain.DomainBase;
import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.samples.IReferencedSamples;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesLegend;
import de.toem.impulse.samples.convert.ConvertedSamples;
import de.toem.toolkits.pattern.element.exploits.Markers;

public abstract class ReferencedSamples
extends ConvertedSamples
implements IReferencedSamples {
    protected ISamples reference;

    public ReferencedSamples() {
    }

    public ReferencedSamples(ISamples reference) {
        this.reference = reference;
    }

    protected void setReference(ISamples reference) {
        this.reference = reference;
    }

    @Override
    public ISamples getReference() {
        return this.reference;
    }

    protected boolean hasReference() {
        return this.reference != null;
    }

    @Override
    public ISamples getReference(Class<? extends ISamples> cs) {
        if (cs == null) {
            return this.reference;
        }
        if (cs.isInstance(this.reference)) {
            return this.reference;
        }
        if (this.reference instanceof ReferencedSamples) {
            return ((ReferencedSamples)this.reference).getReference(cs);
        }
        return null;
    }

    @Override
    public String getId() {
        return this.reference != null ? this.reference.getId() : null;
    }

    @Override
    public String getName() {
        return this.reference != null ? this.reference.getName() : null;
    }

    @Override
    public String getMessage() {
        return this.reference != null ? this.reference.getMessage() : null;
    }

    @Override
    public String getError() {
        return this.reference != null ? this.reference.getError() : null;
    }

    @Override
    public Markers getMarkers() {
        return this.reference != null ? this.reference.getMarkers() : null;
    }

    @Override
    public ISamples.SignalType getSignalType() {
        return this.reference != null ? this.reference.getSignalType() : ISamples.SignalType.Unknown;
    }

    @Override
    public ISamples.SignalDescriptor getSignalDescriptor() {
        return this.reference != null ? this.reference.getSignalDescriptor() : ISamples.SignalDescriptor.DEFAULT;
    }

    @Override
    public String getContent() {
        return this.getSignalDescriptor().getContent();
    }

    @Override
    public int getScale() {
        return this.getSignalDescriptor().getScale();
    }

    @Override
    public int getAccuracy() {
        return this.getSignalDescriptor().getAccuracy();
    }

    @Override
    public int getFlags() {
        return this.getSignalDescriptor().getFlags();
    }

    @Override
    public int getFormat() {
        return this.getSignalDescriptor().getFormat();
    }

    @Override
    @Deprecated
    public boolean hasConflict() {
        return this.reference != null ? this.reference.hasTag() : false;
    }

    @Override
    public boolean hasTag() {
        return this.reference != null ? this.reference.hasTag() : false;
    }

    @Override
    public ISamples.TagDomain getTagDomain() {
        return this.reference != null ? this.reference.getTagDomain() : ISamples.TagDomain.Unknown;
    }

    @Override
    public ISamples.ProcessType getProcessType() {
        return this.reference != null ? this.reference.getProcessType() : ISamples.ProcessType.Unknown;
    }

    @Override
    public IDomainBase getDomainBase() {
        return this.reference != null ? this.reference.getDomainBase() : DomainBase.Unknown;
    }

    @Override
    public DomainValue getStart() {
        return this.reference != null ? this.reference.getStart() : DomainValue.NONE;
    }

    @Override
    public DomainValue getEnd() {
        return this.reference != null ? this.reference.getEnd() : DomainValue.NONE;
    }

    @Override
    public DomainValue getRate() {
        return this.reference != null ? this.reference.getRate() : DomainValue.NONE;
    }

    @Override
    public long getStartUnits() {
        return this.reference != null ? this.reference.getStartUnits() : 0L;
    }

    @Override
    public long getEndUnits() {
        return this.reference != null ? this.reference.getEndUnits() : 0L;
    }

    @Override
    public long getRateUnits() {
        return this.reference != null ? this.reference.getRateUnits() : 0L;
    }

    @Override
    public ISamplesLegend getLegend() {
        return this.reference != null ? this.reference.getLegend() : null;
    }

    @Override
    public Object getData() {
        return this.reference != null ? this.reference.getData() : null;
    }

    @Override
    public Object getData(String key) {
        return this.reference != null ? this.reference.getData(key) : null;
    }

    @Override
    public void setData(Object value) {
        if (this.reference != null) {
            this.reference.setData(value);
        }
    }

    @Override
    public void setData(String key, Object value) {
        if (this.reference != null) {
            this.reference.setData(key, value);
        }
    }

    @Override
    public Object getService(Class<?> cs) {
        if (this.reference != null) {
            return this.reference.getService(cs);
        }
        return null;
    }

    @Override
    public int getRelease() {
        if (this.reference != null) {
            return this.reference.getRelease();
        }
        return 0;
    }

    @Override
    public boolean isVolatile() {
        if (this.reference != null) {
            return this.reference.isVolatile();
        }
        return false;
    }

    @Override
    public boolean isReleased() {
        if (this.reference != null) {
            return this.reference.isReleased();
        }
        return false;
    }

    @Override
    public boolean isMonotonous() {
        if (this.reference != null) {
            return this.reference.isMonotonous();
        }
        return false;
    }

    public boolean equals(Object obj) {
        if (!(obj instanceof ReferencedSamples)) {
            return false;
        }
        ReferencedSamples that = (ReferencedSamples)obj;
        return this.reference == that.reference;
    }
}

