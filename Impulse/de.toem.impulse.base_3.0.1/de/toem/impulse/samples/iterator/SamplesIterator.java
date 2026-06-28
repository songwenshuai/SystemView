/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.iterator;

import de.toem.impulse.domain.DomainBase;
import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.samples.ILogicDetector;
import de.toem.impulse.samples.ISamplePointer;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesIterator;
import de.toem.impulse.samples.ISamplesWriter;
import java.util.List;

public class SamplesIterator
implements ISamplesIterator {
    protected ISamplePointer[] pointers;
    protected IDomainBase domainBase;
    protected ISamplesWriter target;
    protected boolean prepared;
    private boolean initial;
    private long start;
    private long prev;
    private long current;
    private long next;
    private long end;
    private long[] _prev;
    private long[] _current;
    private long[] _next;

    public SamplesIterator(List<ISamplePointer> pointers) {
        this(null, pointers);
    }

    public SamplesIterator(ISamplePointer ... pointers) {
        this((ISamplesWriter)null, pointers);
    }

    public SamplesIterator(ISamplesWriter target, List<ISamplePointer> pointers) {
        int length = pointers != null ? pointers.size() : 0;
        this.pointers = pointers.toArray(new ISamplePointer[length]);
        this._next = new long[length];
        this._current = new long[length];
        this._prev = new long[length];
        this.target = target;
        this.prepareDomainRange();
        this.setCurrent(this.start, true);
    }

    public SamplesIterator(ISamplesWriter target, ISamplePointer ... pointers) {
        int length = pointers != null ? pointers.length : 0;
        this.pointers = new ISamplePointer[length];
        if (pointers != null) {
            System.arraycopy(pointers, 0, this.pointers, 0, length);
        }
        this._next = new long[length];
        this._current = new long[length];
        this._prev = new long[length];
        this.target = target;
        this.prepareDomainRange();
        this.setCurrent(this.start, true);
    }

    public ISamplePointer[] pointers() {
        return this.pointers;
    }

    @Override
    public void remove() {
        throw new UnsupportedOperationException();
    }

    public void reset() {
        this.prepared = false;
        this.next = 0L;
        this.prev = 0L;
        this.start = 0L;
        this.end = 0L;
        this.current = 0L;
        this.prepareDomainRange();
        this.setCurrent(this.start, true);
    }

    public void update() {
        this.prepared = false;
        this.next = 0L;
        this.prev = 0L;
        this.start = 0L;
        this.end = 0L;
        this.prepareDomainRange();
        this.updateCurrent();
    }

    public SamplesIterator startAgain() {
        this.setCurrent(this.start, true);
        return this;
    }

    public SamplesIterator startFromTheEnd() {
        this.setCurrent(this.end, true);
        return this;
    }

    protected void prepareDomainRange() {
        IDomainBase domainBase = null;
        this.prepared = false;
        boolean volatiles = false;
        ISamplePointer[] iSamplePointerArray = this.pointers;
        int n = this.pointers.length;
        int n2 = 0;
        while (n2 < n) {
            ISamplePointer readable = iSamplePointerArray[n2];
            if (readable != null && readable.isVolatile()) {
                volatiles = true;
                break;
            }
            ++n2;
        }
        long start = Long.MAX_VALUE;
        long end = volatiles ? Long.MAX_VALUE : Long.MIN_VALUE;
        ISamplePointer[] iSamplePointerArray2 = this.pointers;
        int n3 = this.pointers.length;
        int n4 = 0;
        while (n4 < n3) {
            ISamplePointer readable = iSamplePointerArray2[n4];
            if (readable != null) {
                if (readable.getDomainBase() == null) {
                    return;
                }
                if (domainBase == null) {
                    domainBase = readable.getDomainBase();
                } else if (!domainBase.equals(readable.getDomainBase())) {
                    return;
                }
                if (readable.getStartUnits() < start) {
                    start = readable.getStartUnits();
                }
                if (!volatiles && readable.getEndUnits() > end) {
                    end = readable.getEndUnits();
                } else if (volatiles && readable.getEndUnits() < end) {
                    end = readable.getEndUnits();
                }
            }
            ++n4;
        }
        if (domainBase != null && this.target != null && this.target.getDomainBase().isCompatible(domainBase)) {
            long tstart = this.target.getDomainBase().convertTo(domainBase, this.target.getStartUnits());
            long tmax = this.target.getDomainBase().convertTo(domainBase, this.target.getMaxUnits());
            if (tstart > start) {
                start = tstart;
            }
            if (tmax > tstart && tmax < end) {
                end = tmax;
            }
        }
        if (domainBase != null && domainBase != DomainBase.Unknown && start < Long.MAX_VALUE && end > Long.MIN_VALUE && end > start) {
            this.domainBase = domainBase;
            this.start = start;
            this.end = end;
            this.prepared = true;
        }
    }

    public void updateCurrent() {
        this.next = Long.MAX_VALUE;
        this.prev = Long.MIN_VALUE;
        if (!this.prepared) {
            return;
        }
        int n = 0;
        while (n < this.pointers.length) {
            if (this.pointers[n] != null) {
                int index = this.pointers[n].getIndex();
                this._current[n] = this.pointers[n].unitsAt(index);
                this._next[n] = this.pointers[n].unitsAt(index + 1);
                this._prev[n] = this._current[n] == this.current ? this.pointers[n].unitsAt(index - 1) : this._current[n];
                if (this._next[n] < this.next) {
                    this.next = this._next[n];
                }
                if (this._current[n] > this.prev && this._current[n] < this.current) {
                    this.prev = this._current[n];
                }
                if (this._prev[n] > this.prev) {
                    this.prev = this._prev[n];
                }
            }
            ++n;
        }
    }

    @Override
    public void setCurrent(DomainValue position, boolean iterationStart) {
        if (this.domainBase == null || this.domainBase == DomainBase.Unknown) {
            return;
        }
        this.setCurrent(DomainValue.units(this.domainBase, position), iterationStart);
    }

    @Override
    public DomainValue getStartPosition() {
        return new DomainValue(this.domainBase, this.start);
    }

    @Override
    public DomainValue getEndPosition() {
        return new DomainValue(this.domainBase, this.end);
    }

    @Override
    public IDomainBase getDomainBase() {
        return this.domainBase;
    }

    @Override
    public long getStart() {
        return this.start;
    }

    @Override
    public long getEnd() {
        return this.end;
    }

    @Override
    public long current() {
        return this.current;
    }

    @Override
    public long current(ISamplesWriter target) {
        if (target != null && this.domainBase != null && target.getDomainBase() != this.domainBase) {
            if (!this.domainBase.isCompatible(target.getDomainBase())) {
                return 0L;
            }
            return this.domainBase.convertTo(target.getDomainBase(), this.current);
        }
        return this.current;
    }

    @Override
    public boolean isCurrent(ISamplePointer samples) {
        return samples.unitsAt(samples.getIndex()) == this.current;
    }

    @Override
    public DomainValue getCurrentPosition() {
        return new DomainValue(this.domainBase, this.current);
    }

    @Override
    public void setCurrent(long units, boolean initial) {
        this.current = units;
        this.next = Long.MAX_VALUE;
        this.prev = Long.MIN_VALUE;
        this.initial = initial;
        if (!this.prepared) {
            return;
        }
        int n = 0;
        while (n < this.pointers.length) {
            if (this.pointers[n] != null) {
                this.pointers[n].setChanged(initial);
                this.pointers[n].setUnits(units);
                int index = this.pointers[n].getIndex();
                this._current[n] = this.pointers[n].unitsAt(index);
                this._next[n] = this.pointers[n].unitsAt(index + 1);
                this._prev[n] = this.pointers[n].unitsAt(index - 1);
                if (this._next[n] < this.next) {
                    this.next = this._next[n];
                }
                if (this._current[n] > this.prev && this._current[n] < this.current) {
                    this.prev = this._current[n];
                }
                if (this._prev[n] > this.prev) {
                    this.prev = this._prev[n];
                }
            }
            ++n;
        }
        if (initial) {
            this.next = this.prev = this.current;
        }
    }

    @Override
    public boolean hasNext() {
        return this.prepared && this.next <= this.end;
    }

    @Override
    public Long next() {
        if (!this.prepared) {
            return this.current;
        }
        this.prev = this.current;
        this.current = this.next;
        this.next = Long.MAX_VALUE;
        int n = 0;
        while (n < this.pointers.length) {
            ISamplePointer pointer = this.pointers[n];
            if (pointer != null) {
                if (!this.initial && this._next[n] == this.current) {
                    if (pointer.goNext()) {
                        this._prev[n] = this._current[n];
                        this._current[n] = this._next[n];
                        this._next[n] = pointer.unitsAt(pointer.getIndex() + 1);
                    } else {
                        this._next[n] = Long.MAX_VALUE;
                    }
                } else {
                    pointer.setChanged(this.initial);
                }
                pointer.setDelta(this.current - this._current[n]);
                if (this._next[n] < this.next && this._next[n] >= this.current) {
                    this.next = this._next[n];
                }
            }
            ++n;
        }
        this.initial = false;
        return this.current;
    }

    @Override
    public boolean hasPrev() {
        return this.prepared && this.prev >= this.start;
    }

    @Override
    public Long prev() {
        if (!this.prepared) {
            return this.current;
        }
        this.next = this.current;
        this.current = this.prev;
        this.prev = Long.MIN_VALUE;
        int n = 0;
        while (n < this.pointers.length) {
            ISamplePointer pointer = this.pointers[n];
            if (pointer != null) {
                if (!this.initial && (this.current < this._current[n] || this.current == this._prev[n] && this.next == this.current)) {
                    if (pointer.goPrev()) {
                        this._next[n] = this._current[n];
                        this._current[n] = this._prev[n];
                        this._prev[n] = pointer.unitsAt(pointer.getIndex() - 1);
                    } else {
                        this._prev[n] = Long.MIN_VALUE;
                    }
                } else {
                    pointer.setChanged(this.initial);
                }
                pointer.setDelta(this.current - this._current[n]);
                if (this._current[n] > this.prev && this._current[n] < this.current) {
                    this.prev = this._current[n];
                }
                if (this._prev[n] > this.prev) {
                    this.prev = this._prev[n];
                }
            }
            ++n;
        }
        this.initial = false;
        return this.current;
    }

    @Override
    public Long next(ISamplesWriter target) {
        this.next();
        if (target != null && this.domainBase != null) {
            if (!this.domainBase.isCompatible(target.getDomainBase())) {
                return 0L;
            }
            if (target.getProcessType() == ISamples.ProcessType.Continuous) {
                long mcurrent = this.current;
                long start = target.getDomainBase().convertTo(this.domainBase, target.getStartUnits());
                long rate = target.getDomainBase().convertTo(this.domainBase, target.getRateUnits());
                if (rate == 0L) {
                    return 0L;
                }
                long rem = (mcurrent - start) % rate;
                if (rem != 0L) {
                    mcurrent += mcurrent - start > 0L ? rate - rem : -rem;
                }
                if (mcurrent != this.current) {
                    this.setCurrent(mcurrent, false);
                }
            }
            return this.domainBase.convertTo(target.getDomainBase(), this.current);
        }
        return this.current;
    }

    @Override
    public Long prev(ISamplesWriter target) {
        this.prev();
        if (target != null && this.domainBase != null) {
            if (!this.domainBase.isCompatible(target.getDomainBase())) {
                return 0L;
            }
            if (target.getProcessType() == ISamples.ProcessType.Continuous) {
                long mcurrent = this.current;
                long start = target.getDomainBase().convertTo(this.domainBase, target.getStartUnits());
                long rate = target.getDomainBase().convertTo(this.domainBase, target.getRateUnits());
                if (rate == 0L) {
                    return 0L;
                }
                long rem = (mcurrent - start) % rate;
                if (rem != 0L) {
                    mcurrent += mcurrent - start > 0L ? -rem : rate - rem;
                }
                if (mcurrent != this.current) {
                    this.setCurrent(mcurrent, false);
                }
            }
            return this.domainBase.convertTo(target.getDomainBase(), this.current);
        }
        return this.current;
    }

    @Override
    public boolean prev(ISamplePointer samples) {
        if (samples.goPrev()) {
            this.setCurrent(samples.getUnits(), false);
        }
        return false;
    }

    @Override
    public boolean next(ISamplePointer samples) {
        if (samples.goNext()) {
            this.setCurrent(samples.getUnits(), false);
        }
        return false;
    }

    @Override
    public boolean prevEdge(ISamplePointer samples, int edge) {
        if (samples.goPrevEdge(edge)) {
            this.setCurrent(samples.getUnits(), false);
            return true;
        }
        return false;
    }

    @Override
    public boolean nextEdge(ISamplePointer samples, int edge) {
        if (samples.goNextEdge(edge)) {
            this.setCurrent(samples.getUnits(), false);
            return true;
        }
        return false;
    }

    @Override
    public boolean prevEdge(ISamplePointer samples, int edge, ILogicDetector detector) {
        if (samples.goPrevEdge(edge, detector)) {
            this.setCurrent(samples.getUnits(), false);
            return true;
        }
        return false;
    }

    @Override
    public boolean nextEdge(ISamplePointer samples, int edge, ILogicDetector detector) {
        if (samples.goNextEdge(edge, detector)) {
            this.setCurrent(samples.getUnits(), false);
            return true;
        }
        return false;
    }
}

