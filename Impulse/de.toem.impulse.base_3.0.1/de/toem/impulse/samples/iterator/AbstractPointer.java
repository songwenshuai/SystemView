/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.iterator;

import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.samples.IPointer;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.base.ReferencedReadableSamples;

public abstract class AbstractPointer
extends ReferencedReadableSamples
implements IPointer {
    protected int index = -1;
    protected long delta = 0L;
    protected boolean changed;

    public AbstractPointer(IReadableSamples readable) {
        super(readable);
    }

    public AbstractPointer(IReadableSamples readable, int index) {
        super(readable);
        this.setIndex(index);
    }

    public AbstractPointer(IReadableSamples readable, long units) {
        super(readable);
        this.setUnits(units);
    }

    public AbstractPointer(IReadableSamples readable, DomainValue position) {
        super(readable);
        this.setPosition(position);
    }

    @Override
    public final int getIndex() {
        return this.index;
    }

    @Override
    public final void setIndex(int index) {
        if (index < this.getMinIndex()) {
            index = this.getMinIndex();
        }
        if (index > this.getMaxIndex()) {
            index = this.getMaxIndex();
        }
        this.index = index;
        this.delta = 0L;
    }

    @Override
    public final long getDelta() {
        return this.delta;
    }

    @Override
    public final void setDelta(long delta) {
        this.delta = delta >= 0L ? delta : 0L;
    }

    @Override
    public final boolean hasIndexChanged() {
        return this.changed;
    }

    @Override
    public final void setChanged(boolean changed) {
        this.changed = changed;
    }

    @Override
    public final boolean goPrev() {
        if (this.index > this.getMinIndex()) {
            --this.index;
            this.delta = 0L;
            this.changed = true;
            return true;
        }
        return false;
    }

    @Override
    public final boolean goNext() {
        if (this.index < this.getMaxIndex()) {
            ++this.index;
            this.delta = 0L;
            this.changed = true;
            return true;
        }
        return false;
    }

    @Override
    public final boolean hasPrev() {
        return this.index > this.getMinIndex();
    }

    @Override
    public final boolean hasNext() {
        return this.index < this.getMaxIndex();
    }

    @Override
    public final boolean goPos1() {
        if (this.index > this.getMinIndex()) {
            this.index = this.getMinIndex();
            this.delta = 0L;
            this.changed = true;
            return true;
        }
        return false;
    }

    @Override
    public final boolean goEnd() {
        if (this.index < this.getMaxIndex()) {
            this.index = this.getMaxIndex();
            this.delta = 0L;
            this.changed = true;
            return true;
        }
        return false;
    }

    @Override
    public final IPointer relative(String delta) {
        DomainValue v = DomainValue.parse(delta);
        if (v != null) {
            return this.relative(v);
        }
        return null;
    }

    @Override
    public final IPointer relative(DomainValue delta) {
        DomainValue v = delta.convertTo(this.getDomainBase());
        if (v != null) {
            return this.relative(v.units);
        }
        return null;
    }

    @Override
    public final boolean equals(Object obj) {
        if (!(obj instanceof AbstractPointer)) {
            return false;
        }
        AbstractPointer that = (AbstractPointer)obj;
        if (this.index != that.index) {
            return false;
        }
        if (this.delta != that.delta) {
            return false;
        }
        return super.equals(obj);
    }
}

