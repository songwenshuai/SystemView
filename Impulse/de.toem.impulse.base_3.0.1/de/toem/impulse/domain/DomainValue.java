/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.domain;

import de.toem.impulse.domain.DomainBase;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.domain.UnknownBase;

public class DomainValue {
    public IDomainBase base;
    public long units;
    public static final DomainValue NONE = new DomainValue(UnknownBase.Unknown, 0L);
    public static final DomainValue MAX = new DomainValue(UnknownBase.Unknown, Long.MAX_VALUE);
    public static final DomainValue MIN = new DomainValue(UnknownBase.Unknown, Long.MIN_VALUE);

    public DomainValue(IDomainBase base, long units) {
        this.base = base != null ? base : DomainBase.Unknown;
        this.units = units;
    }

    public String toString() {
        return this.isMax() || this.isMin() ? "" : (this.base != null ? this.base.toString(this.units) : "");
    }

    public String toString(int style) {
        return this.isMax() || this.isMin() ? "" : (this.base != null ? this.base.toString(this.units, style) : "");
    }

    public static DomainValue parse(String value) {
        IDomainBase base;
        int domainLen;
        block8: {
            block7: {
                block6: {
                    if (value != null) break block6;
                    return null;
                }
                try {
                    domainLen = 0;
                    value = value.trim();
                    while (domainLen < value.length() && Character.isLetter(value.charAt(value.length() - 1 - domainLen))) {
                        ++domainLen;
                    }
                    if (domainLen != 0) break block7;
                    return null;
                }
                catch (Throwable throwable) {
                    return null;
                }
            }
            base = DomainBase.parse(value.substring(value.length() - domainLen));
            if (base != UnknownBase.Unknown) break block8;
            return null;
        }
        String numbers = value.substring(0, value.length() - domainLen).trim();
        long units = Long.parseLong(numbers);
        return new DomainValue(base, units);
    }

    public boolean isMax() {
        return this.units == Long.MAX_VALUE;
    }

    public boolean isMin() {
        return this.units == Long.MIN_VALUE;
    }

    public boolean isNull() {
        return this.units == 0L;
    }

    public boolean lt(DomainValue that) {
        if (that != null) {
            if (this.isMin() && !that.isMin()) {
                return true;
            }
            if (!this.isMax() && that.isMax()) {
                return true;
            }
            if (this.base == that.base) {
                return this.units < that.units;
            }
            if (this.base.isCompatible(that.base)) {
                return this.toCommonBase() < that.toCommonBase();
            }
        }
        return false;
    }

    public boolean le(DomainValue that) {
        if (that != null) {
            if (this.isMin()) {
                return true;
            }
            if (that.isMax()) {
                return true;
            }
            if (this.base == that.base) {
                return this.units <= that.units;
            }
            if (this.base.isCompatible(that.base)) {
                return this.toCommonBase() <= that.toCommonBase();
            }
        }
        return false;
    }

    public boolean gt(DomainValue that) {
        if (that != null) {
            return that.lt(this);
        }
        return true;
    }

    public boolean ge(DomainValue that) {
        if (that != null) {
            return that.le(this);
        }
        return true;
    }

    public DomainValue sub(DomainValue that) {
        if (that != null) {
            if (this.base == that.base) {
                return new DomainValue(this.base, this.units - that.units);
            }
            if (this.base.isCompatible(that.base)) {
                long thatConverted = that.base.convertTo(this.base, that.units);
                return new DomainValue(this.base, this.units - thatConverted);
            }
        }
        return this;
    }

    public DomainValue add(DomainValue that) {
        if (that != null) {
            if (this.base == that.base) {
                return new DomainValue(this.base, this.units + that.units);
            }
            if (this.base.isCompatible(that.base)) {
                long thatConverted = that.base.convertTo(this.base, that.units);
                return new DomainValue(this.base, this.units + thatConverted);
            }
        }
        return this;
    }

    public static int compare(DomainValue n1, DomainValue n2, int direction) {
        if (direction == 0) {
            return 0;
        }
        if (direction == 2) {
            direction = -1;
        }
        return (n1.lt(n2) ? 1 : (n1.gt(n2) ? -1 : 0)) * direction;
    }

    public static DomainValue min(DomainValue a, DomainValue b) {
        if (a.isMax()) {
            return b;
        }
        if (b.isMax()) {
            return a;
        }
        if (a.base == b.base) {
            return a.units < b.units ? a : b;
        }
        if (a.base.isCompatible(b.base)) {
            long ac = a.base.convertTo(b.base, a.units);
            return ac < b.units ? a : b;
        }
        return MAX;
    }

    public static DomainValue max(DomainValue a, DomainValue b) {
        if (a.isMin()) {
            return b;
        }
        if (b.isMin()) {
            return a;
        }
        if (a.base == b.base) {
            return a.units > b.units ? a : b;
        }
        if (a.base.isCompatible(b.base)) {
            long ac = a.base.convertTo(b.base, a.units);
            return ac > b.units ? a : b;
        }
        return MIN;
    }

    public DomainValue convertTo(IDomainBase domainBase) {
        if (this.base == domainBase) {
            return this;
        }
        if (this.base.isCompatible(domainBase)) {
            long units = this.base.convertTo(domainBase, this.units);
            return new DomainValue(domainBase, units);
        }
        return null;
    }

    public static long units(IDomainBase domainBase, DomainValue position) {
        if (position.base == domainBase) {
            return position.units;
        }
        if (position.base.isCompatible(domainBase)) {
            return position.base.convertTo(domainBase, position.units);
        }
        return 0L;
    }

    public double toCommonBase() {
        return this.base.toCommonBase(this.units);
    }

    public boolean equals(Object o) {
        if (!(o instanceof DomainValue)) {
            return false;
        }
        DomainValue that = (DomainValue)o;
        if (that != null) {
            if (this.isMin() && that.isMin()) {
                return true;
            }
            if (this.isMax() && that.isMax()) {
                return true;
            }
            if (this.base == that.base) {
                return this.units == that.units;
            }
            if (this.base.isCompatible(that.base)) {
                if (this.base.isFinerThan(that.base)) {
                    long thatConverted = that.base.convertTo(this.base, that.units);
                    return this.units == thatConverted;
                }
                long thisConverted = this.base.convertTo(that.base, this.units);
                return that.units == thisConverted;
            }
        }
        return false;
    }
}

