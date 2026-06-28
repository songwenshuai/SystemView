/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.base;

import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.convert.ConvertedSamples;
import de.toem.toolkits.pattern.element.exploits.Markers;

public abstract class Samples
extends ConvertedSamples
implements ISamples {
    protected String name;
    protected String id;
    protected String error;
    protected ISamples.ProcessType processType;
    protected ISamples.SignalType signalType;
    protected ISamples.SignalDescriptor signalDescriptor;
    protected ISamples.TagDomain tagDomain;
    protected boolean tagged;
    protected IDomainBase domainBase;
    protected long start;
    protected long end;
    protected long rate;
    private Object[] data;

    @Override
    public String getId() {
        return this.id;
    }

    @Override
    public String getName() {
        return this.name;
    }

    @Override
    public String getError() {
        return this.error;
    }

    public void setError(String message) {
        this.error = message;
    }

    @Override
    public String getMessage() {
        return this.error;
    }

    @Override
    public final ISamples.ProcessType getProcessType() {
        return this.processType;
    }

    @Override
    public final ISamples.SignalType getSignalType() {
        return this.signalType;
    }

    @Override
    public final ISamples.SignalDescriptor getSignalDescriptor() {
        return this.signalDescriptor != null ? this.signalDescriptor : ISamples.SignalDescriptor.DEFAULT;
    }

    @Override
    public String getContent() {
        return this.signalDescriptor != null ? this.signalDescriptor.getContent() : ISamples.SignalDescriptor.DEFAULT.getContent();
    }

    @Override
    public int getScale() {
        return this.signalDescriptor != null ? this.signalDescriptor.getScale() : ISamples.SignalDescriptor.DEFAULT.getScale();
    }

    @Override
    public int getAccuracy() {
        return this.signalDescriptor != null ? this.signalDescriptor.getAccuracy() : ISamples.SignalDescriptor.DEFAULT.getAccuracy();
    }

    @Override
    public int getFlags() {
        return this.signalDescriptor != null ? this.signalDescriptor.getFlags() : ISamples.SignalDescriptor.DEFAULT.getFlags();
    }

    @Override
    public int getFormat() {
        return this.signalDescriptor != null ? this.signalDescriptor.getFormat() : ISamples.SignalDescriptor.DEFAULT.getFormat();
    }

    @Override
    public final IDomainBase getDomainBase() {
        return this.domainBase;
    }

    @Override
    public final DomainValue getStart() {
        return new DomainValue(this.domainBase, this.start);
    }

    @Override
    public final DomainValue getEnd() {
        return new DomainValue(this.domainBase, this.end);
    }

    @Override
    public final DomainValue getRate() {
        return new DomainValue(this.domainBase, this.rate);
    }

    @Override
    public final long getStartUnits() {
        return this.start;
    }

    @Override
    public final long getEndUnits() {
        return this.end;
    }

    @Override
    public final long getRateUnits() {
        return this.rate;
    }

    @Override
    public int getRelease() {
        return 0;
    }

    @Override
    public boolean isMonotonous() {
        return false;
    }

    @Override
    public boolean isVolatile() {
        return true;
    }

    @Override
    public boolean isReleased() {
        return true;
    }

    @Override
    @Deprecated
    public final boolean hasConflict() {
        return this.tagged;
    }

    @Override
    public final boolean hasTag() {
        return this.tagged;
    }

    @Override
    public ISamples.TagDomain getTagDomain() {
        return this.tagDomain;
    }

    @Override
    public Markers getMarkers() {
        return null;
    }

    @Override
    public Object getService(Class<?> cs) {
        return null;
    }

    @Override
    public Object valueAt(int idx) {
        return null;
    }

    @Override
    public long unitsAt(int idx) {
        return 0L;
    }

    @Override
    public int groupAt(int idx) {
        return 0;
    }

    @Override
    public int orderAt(int idx) {
        return 0;
    }

    public String toString() {
        return this.name != null ? this.name : super.toString();
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

