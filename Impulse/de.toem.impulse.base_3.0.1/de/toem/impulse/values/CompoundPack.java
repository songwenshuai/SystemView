/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.values;

import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISample;
import java.util.Arrays;

public class CompoundPack
implements ISample {
    protected IReadableSamples samples;
    protected int idx;
    protected long units;
    protected int format0;
    protected int tag;
    protected int order;
    protected int group;
    protected int layer;
    protected byte[] bytes;

    public CompoundPack() {
    }

    public CompoundPack(IReadableSamples samples, int idx, long units, int format0, int tag, int group, int order, int layer, byte[] bytes, int pos, int dataLength) {
        this.samples = samples;
        this.idx = idx;
        this.units = units;
        this.format0 = format0;
        this.tag = tag;
        this.order = order;
        this.layer = layer;
        this.group = group;
        this.bytes = new byte[dataLength];
        System.arraycopy(bytes, pos, this.bytes, 0, dataLength);
    }

    public CompoundPack(IReadableSamples samples, int idx, long units, int format0, int xtag, byte[] bytes, int pos, int dataLength) {
        this(samples, idx, units, format0, xtag, 0, 0, 0, bytes, pos, dataLength);
    }

    public boolean equals(Object o) {
        if (!(o instanceof CompoundPack)) {
            return false;
        }
        CompoundPack that = (CompoundPack)o;
        if (this.idx != that.idx) {
            return false;
        }
        if (this.units != that.units) {
            return false;
        }
        if (this.format0 != that.format0) {
            return false;
        }
        if (this.tag != that.tag) {
            return false;
        }
        if (this.group != that.group) {
            return false;
        }
        if (this.order != that.order) {
            return false;
        }
        if (this.layer != that.layer) {
            return false;
        }
        return Arrays.equals(this.getBytes(), that.getBytes());
    }

    public boolean equalValues(CompoundPack that) {
        if (this.format0 != that.format0) {
            return false;
        }
        if (this.order != that.order) {
            return false;
        }
        if (this.layer != that.layer) {
            return false;
        }
        return Arrays.equals(this.getBytes(), that.getBytes());
    }

    public IReadableSamples getSamples() {
        return this.samples;
    }

    public int getIdx() {
        return this.idx;
    }

    public void setIdx(int idx) {
        this.idx = idx;
    }

    public long getUnits() {
        return this.units;
    }

    public void setUnits(long units) {
        this.units = units;
    }

    public int getFormat0() {
        return this.format0;
    }

    public void setFormat0(int format0) {
        this.format0 = format0;
    }

    public int getOrder() {
        return this.order;
    }

    public void setOrder(int order) {
        this.order = order;
    }

    public int getGroup() {
        return this.group;
    }

    public void setGroup(int group) {
        this.group = group;
    }

    public int getLayer() {
        return this.layer;
    }

    public void setLayer(int layer) {
        this.layer = layer;
    }

    public byte[] getBytes() {
        return this.bytes;
    }

    public void setBytes(byte[] bytes) {
        this.bytes = bytes;
    }

    public int getLength() {
        return this.bytes != null ? this.bytes.length : 0;
    }

    public void setTag(boolean tag) {
        this.format0 &= 0xFFFFFFDE;
        if (tag) {
            this.format0 |= 1;
        }
    }

    public void setTag(int tag) {
        this.format0 &= 0xFFFFFFDE;
        if (tag == 1) {
            this.format0 |= 1;
        } else if (tag > 1) {
            this.format0 |= 0x21;
            this.tag = tag;
        }
    }

    public boolean isTagged() {
        return (this.format0 & 1) != 0;
    }

    public int getTag() {
        if ((this.format0 & 1) != 0) {
            if ((this.format0 & 0x21) == 33) {
                return this.tag;
            }
            return 1;
        }
        return 0;
    }
}

