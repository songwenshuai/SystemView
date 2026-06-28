/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.writer;

import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.IStructSamplesWriter;
import de.toem.impulse.samples.legend.DefaultSamplesLegend;
import de.toem.impulse.samples.writer.SamplesWriter;
import de.toem.impulse.values.Struct;
import de.toem.impulse.values.StructMember;

public class StructSamplesWriter
extends SamplesWriter
implements IStructSamplesWriter {
    public StructSamplesWriter(String id, String name, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase domainBase) {
        super(id, name, processType, signalType, signalDescriptor, domainBase);
    }

    @Override
    public final boolean write(long units, boolean tag, StructMember[] struct) {
        return this.write(units, tag ? 1 : 0, struct);
    }

    @Override
    public final boolean write(long units, int tag, StructMember[] struct) {
        int begin;
        int n;
        if (this.signalType != ISamples.SignalType.Struct) {
            return false;
        }
        DefaultSamplesLegend context = (DefaultSamplesLegend)this.legend;
        int dlength = 0;
        if (struct != null) {
            StructMember[] structMemberArray = struct;
            n = struct.length;
            int n2 = 0;
            while (n2 < n) {
                StructMember member = structMemberArray[n2];
                if (member != null && member.isValid()) {
                    member.assignLegend(context);
                    member.pack();
                    dlength += member.getPackLength();
                }
                ++n2;
            }
        }
        if ((begin = this.beginWrite(units, dlength)) < 0) {
            return false;
        }
        this.buffer[this.buffered++] = (byte)(0x40 | (tag > 0 ? (tag > 1 ? 33 : 1) : 0));
        if (dlength <= 15) {
            this.buffer[this.buffered++] = (byte)dlength;
            if (tag > 1) {
                this.buffer[this.buffered++] = (byte)(tag & 0xF);
            }
        } else {
            int s = dlength;
            this.buffer[this.buffered++] = (byte)(s & 0xF | 0x10);
            if (tag > 1) {
                this.buffer[this.buffered++] = (byte)(tag & 0xF);
            }
            s >>>= 4;
            while (true) {
                if (s <= 127) {
                    this.buffer[this.buffered++] = (byte)(s & 0x7F);
                    break;
                }
                this.buffer[this.buffered++] = (byte)(s & 0x7F | 0x80);
                s >>>= 7;
            }
        }
        if (struct != null) {
            StructMember[] structMemberArray = struct;
            int n3 = struct.length;
            n = 0;
            while (n < n3) {
                StructMember member = structMemberArray[n];
                if (member != null && member.isValid()) {
                    this.buffered = member.write(this.buffer, this.buffered);
                }
                ++n;
            }
        }
        return this.endWrite(units, begin);
    }

    @Override
    public final boolean write(long units, boolean tag, int group, int order, int layer, StructMember[] struct) {
        return this.write(units, tag ? 1 : 0, group, order, layer, struct);
    }

    @Override
    public final boolean write(long units, int tag, int group, int order, int layer, StructMember[] struct) {
        int begin;
        int n;
        if (this.signalType != ISamples.SignalType.Struct || (order == 1 || order == 5) && group != this.groups || group > this.groups) {
            return false;
        }
        DefaultSamplesLegend context = (DefaultSamplesLegend)this.legend;
        int dlength = 0;
        if (struct != null) {
            StructMember[] structMemberArray = struct;
            n = struct.length;
            int n2 = 0;
            while (n2 < n) {
                StructMember member = structMemberArray[n2];
                if (member != null && member.isValid()) {
                    member.assignLegend(context);
                    member.pack();
                    dlength += member.getPackLength();
                }
                ++n2;
            }
        }
        if ((begin = this.beginWrite(units, dlength)) < 0) {
            return false;
        }
        this.buffer[this.buffered++] = (byte)(0x40 | order << 1 & 6 | (tag > 0 ? (tag > 1 ? 33 : 1) : 0));
        if (dlength <= 15) {
            this.buffer[this.buffered++] = (byte)dlength;
            if (tag > 1) {
                this.buffer[this.buffered++] = (byte)(tag & 0xF);
            }
        } else {
            int s = dlength;
            this.buffer[this.buffered++] = (byte)(s & 0xF | 0x10);
            if (tag > 1) {
                this.buffer[this.buffered++] = (byte)(tag & 0xF);
            }
            s >>>= 4;
            while (true) {
                if (s <= 127) {
                    this.buffer[this.buffered++] = (byte)(s & 0x7F);
                    break;
                }
                this.buffer[this.buffered++] = (byte)(s & 0x7F | 0x80);
                s >>>= 7;
            }
        }
        if (order != 0) {
            if (order == 1 || order == 5) {
                layer = order == 5 ? (layer |= 0x80) : (layer &= 0x7F);
                this.buffer[this.buffered++] = (byte)layer;
                ++this.groups;
            }
            this.buffered += StructSamplesWriter.plusWrite(this.buffer, this.buffered, group);
        }
        if (struct != null) {
            StructMember[] structMemberArray = struct;
            int n3 = struct.length;
            n = 0;
            while (n < n3) {
                StructMember member = structMemberArray[n];
                if (member != null && member.isValid()) {
                    this.buffered = member.write(this.buffer, this.buffered);
                }
                ++n;
            }
        }
        return this.endWrite(units, begin);
    }

    @Override
    public boolean writeSample(long units, int tag, Object value) {
        if (value == null) {
            return this.write(units, tag);
        }
        if (value instanceof StructMember[]) {
            return this.write(units, tag, (StructMember[])value);
        }
        if (value instanceof Struct) {
            return this.write(units, tag, ((Struct)value).getArray());
        }
        return false;
    }

    @Override
    public boolean writeSample(long units, int tag, int group, int order, int layer, Object value) {
        if (value == null) {
            return this.write(units, tag);
        }
        if (value instanceof StructMember[]) {
            return this.write(units, tag, group, order, layer, (StructMember[])value);
        }
        if (value instanceof Struct) {
            return this.write(units, tag, group, order, layer, ((Struct)value).getArray());
        }
        return false;
    }

    @Override
    public StructMember[] createMembers(int size) {
        return new StructMember[size];
    }

    @Override
    public StructMember createMember(StructMember[] members, int idx, String name, int type, String content, int format) {
        StructMember member = new StructMember(name, type, content, format);
        if (members != null && members.length > idx && idx >= 0) {
            members[idx] = member;
        }
        return member;
    }
}

