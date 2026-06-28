/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.legend;

import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesLegend;
import de.toem.impulse.samples.legend.DefaultSamplesLegend;
import de.toem.toolkits.pattern.pageable.BytesPageable;
import de.toem.toolkits.pattern.pageable.Pageable;

public abstract class SamplesLegend
implements ISamplesLegend {
    protected Pageable<byte[]> packed;
    protected boolean modified;
    protected int modRelease;

    public SamplesLegend() {
        this.packed = new BytesPageable(1);
    }

    public SamplesLegend(Signal signal) {
        this.packed = signal.legend;
        if (this.packed != null) {
            this.modRelease = this.packed.getRelease();
        }
    }

    public Pageable<byte[]> toBytes() {
        if (this.modified) {
            this.write();
        }
        this.modified = false;
        return this.packed;
    }

    public boolean isModified() {
        return this.modified;
    }

    public void write() {
    }

    public void read() {
    }

    public static SamplesLegend createLegend(ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor) {
        return new DefaultSamplesLegend();
    }

    public static SamplesLegend createLegend(Signal signal) {
        return new DefaultSamplesLegend(signal);
    }

    public boolean equals(Object o) {
        if (this.modified) {
            this.write();
        }
        if (o instanceof Pageable) {
            if (this.packed != o) {
                return false;
            }
            if (this.modRelease != ((Pageable)o).getRelease()) {
                return false;
            }
        } else if (o instanceof SamplesLegend) {
            return this.equals(((SamplesLegend)o).packed);
        }
        return true;
    }
}

