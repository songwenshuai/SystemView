/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.editor;

import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISamplesEditor;
import de.toem.impulse.samples.ISamplesLegend;
import de.toem.impulse.samples.ISamplesProducer;
import de.toem.impulse.samples.ISamplesReader;
import de.toem.impulse.samples.ISamplesWriter;
import de.toem.impulse.samples.base.ReferencedReadableSamples;
import de.toem.impulse.samples.editor.SampleModification;
import de.toem.impulse.values.CompoundPack;
import de.toem.impulse.values.CompoundValue;
import de.toem.impulse.values.GroupedValue;
import de.toem.impulse.values.IAttachment;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

public class SamplesEditor
extends ReferencedReadableSamples
implements ISamplesEditor,
IReadableSamples {
    private List<SampleModification> modifications = new ArrayList<SampleModification>();
    private Chunk[] index;

    public SamplesEditor(IReadableSamples samples) {
        super(samples);
    }

    @Override
    public synchronized boolean reOpen() {
        if (this.modifications != null) {
            return false;
        }
        this.modifications = new ArrayList<SampleModification>();
        return true;
    }

    @Override
    public synchronized void close() {
        if (this.modifications != null) {
            this.modifications = null;
        }
    }

    public synchronized void write(ISamplesWriter writer) {
        writer.open(this.getStartUnits(), this.getRateUnits(), 0, 0, null);
        int n = 0;
        while (n < this.getCount()) {
            CompoundValue value = this.compoundAt(n);
            if (value != null) {
                writer.writeSample(value);
            }
            ++n;
        }
        writer.close(this.getEndUnits());
    }

    @Override
    public synchronized boolean isOpen() {
        return this.modifications != null;
    }

    @Override
    public synchronized boolean insertSample(CompoundValue value, int options) {
        if (this.modifications == null) {
            return false;
        }
        this.index = null;
        this.modifications.add(SampleModification.insert(value, options));
        return true;
    }

    @Override
    public synchronized boolean removeSample(int idx, int options) {
        if (this.modifications == null) {
            return false;
        }
        this.index = null;
        this.modifications.add(SampleModification.remove(idx, options));
        return true;
    }

    @Override
    public synchronized boolean changeSample(CompoundValue value, int options) {
        if (this.modifications == null) {
            return false;
        }
        this.index = null;
        this.modifications.add(SampleModification.change(value.getIndex(), value, options));
        return true;
    }

    public synchronized boolean moveSample(int idx, long delta, int options) {
        if (this.modifications == null) {
            return false;
        }
        this.index = null;
        this.modifications.add(SampleModification.move(idx, delta, options));
        return true;
    }

    @Override
    public synchronized boolean modifyRange(long start, long end, long rate) {
        return false;
    }

    @Override
    public synchronized boolean modifyDomainBase(IDomainBase base) {
        return false;
    }

    public static void main(String[] args) {
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    private final Chunk[] index() {
        Chunk[] index = this.index;
        if (index != null) {
            return index;
        }
        SamplesEditor samplesEditor = this;
        synchronized (samplesEditor) {
            Chunk c;
            Object index2 = this.index;
            if (index2 != null) {
                return index2;
            }
            index2 = new ArrayList();
            index2.add(this.newRefChunk(0, super.getCount()));
            for (SampleModification m : this.modifications) {
                int line = 0;
                int pos = 0;
                Chunk chunk = null;
                Iterator iterator = index2.iterator();
                while (iterator.hasNext()) {
                    Chunk c2 = (Chunk)iterator.next();
                    if (m.idx >= pos && m.idx < pos + c2.getCount()) {
                        chunk = c2;
                        break;
                    }
                    ++line;
                    pos += c2.getCount();
                }
                if (chunk == null) continue;
                if (m.type == 1) {
                    if (pos == m.idx) {
                        index2.add(line, this.newLocalChunk(m.value));
                        continue;
                    }
                    index2.set(line, this.newRefChunk(chunk, 0, m.idx - pos));
                    index2.add(line + 1, this.newLocalChunk(m.value));
                    index2.add(line + 2, this.newRefChunk(chunk, m.idx - pos, -1));
                    continue;
                }
                if (m.type == 2) {
                    if (chunk.isRef() && chunk.iRange > 1) {
                        if (pos == m.idx) {
                            index2.set(line, this.newRefChunk(chunk, 1, -1));
                            continue;
                        }
                        index2.set(line, this.newRefChunk(chunk, 0, m.idx - pos));
                        index2.add(line + 1, this.newRefChunk(chunk, m.idx - pos + 1, -1));
                        continue;
                    }
                    if (!chunk.isLocal() || chunk.iRange != 1) continue;
                    index2.remove(line);
                    continue;
                }
                if (m.type == 3) {
                    if (chunk.isRef()) {
                        if (pos == m.idx) {
                            index2.set(line, this.newLocalChunk(m.value));
                            index2.add(line + 1, this.newRefChunk(chunk, 1, -1));
                            continue;
                        }
                        index2.set(line, this.newRefChunk(chunk, 0, m.idx - pos));
                        index2.add(line + 1, this.newLocalChunk(m.value));
                        index2.add(line + 2, this.newRefChunk(chunk, m.idx - pos + 1, -1));
                        continue;
                    }
                    if (!chunk.isLocal()) continue;
                    chunk.value = m.value;
                    chunk.uOffset = 0L;
                    continue;
                }
                if (m.type != 4) continue;
                long previous = chunk.uOffset;
                if (chunk.isRef()) {
                    chunk.uOffset = super.unitsAt(chunk.iOffset) - m.position;
                } else if (chunk.isLocal()) {
                    chunk.uOffset = chunk.value.getUnits() - m.position;
                }
                if ((m.options & 1) == 0) continue;
                int n = line + 1;
                while (n < index2.size()) {
                    Chunk chunk2 = (Chunk)index2.get(n);
                    chunk2.uOffset = chunk2.uOffset + (chunk.uOffset - previous);
                    ++n;
                }
            }
            long start = this.getStartUnits();
            int count = 0;
            Chunk last = null;
            Iterator<Object> iterator = index2.iterator();
            while (iterator.hasNext()) {
                c = (Chunk)iterator.next();
                c.calcIndex(count);
                c.calcDomain(last != null ? last.getEndPos() : start);
                count += c.getCount();
                c.getGroups();
                if (last != null) {
                    last.calcRange(c.getStartPos());
                }
                last = c;
            }
            if (last != null) {
                last.calcRange(this.getEndUnits());
            }
            for (SampleModification m : this.modifications) {
                m.log();
            }
            iterator = index2.iterator();
            while (iterator.hasNext()) {
                c = (Chunk)iterator.next();
                c.log();
            }
            this.index = index2.toArray(new Chunk[index2.size()]);
            return this.index;
        }
    }

    @Override
    @Deprecated
    public boolean hasConflict() {
        Chunk[] index;
        Chunk[] chunkArray = index = this.index();
        int n = index.length;
        int n2 = 0;
        while (n2 < n) {
            Chunk chunk = chunkArray[n2];
            if (chunk.hasTag()) {
                return true;
            }
            ++n2;
        }
        return false;
    }

    @Override
    public boolean hasTag() {
        Chunk[] index;
        Chunk[] chunkArray = index = this.index();
        int n = index.length;
        int n2 = 0;
        while (n2 < n) {
            Chunk chunk = chunkArray[n2];
            if (chunk.hasTag()) {
                return true;
            }
            ++n2;
        }
        return false;
    }

    @Override
    public int getCount() {
        Chunk[] index;
        int count = 0;
        Chunk[] chunkArray = index = this.index();
        int n = index.length;
        int n2 = 0;
        while (n2 < n) {
            Chunk chunk = chunkArray[n2];
            count += chunk.getCount();
            ++n2;
        }
        return count;
    }

    @Override
    public int getGroups() {
        Chunk[] index;
        int count = 0;
        Chunk[] chunkArray = index = this.index();
        int n = index.length;
        int n2 = 0;
        while (n2 < n) {
            Chunk chunk = chunkArray[n2];
            count += chunk.getGroups();
            ++n2;
        }
        return count;
    }

    @Override
    public boolean isEmpty() {
        Chunk[] index = this.index();
        return index.length == 0;
    }

    @Override
    public ISamplesLegend getLegend() {
        return this.reference != null ? this.reference.getLegend() : null;
    }

    @Override
    public Object getService(Class<?> cs) {
        return null;
    }

    @Override
    public boolean equals(Object obj) {
        return false;
    }

    @Override
    public boolean isNoneAt(int idx) {
        Chunk[] index;
        Chunk[] chunkArray = index = this.index();
        int n = index.length;
        int n2 = 0;
        while (n2 < n) {
            Chunk chunk = chunkArray[n2];
            if (idx >= chunk.iStart && idx < chunk.iStart + chunk.iRange) {
                return chunk.isNoneAt(idx);
            }
            ++n2;
        }
        return true;
    }

    @Override
    @Deprecated
    public boolean isConflictAt(int idx) {
        Chunk[] index;
        Chunk[] chunkArray = index = this.index();
        int n = index.length;
        int n2 = 0;
        while (n2 < n) {
            Chunk chunk = chunkArray[n2];
            if (idx >= chunk.iStart && idx < chunk.iStart + chunk.iRange) {
                return chunk.isTaggedAt(idx);
            }
            ++n2;
        }
        return false;
    }

    @Override
    public boolean isTaggedAt(int idx) {
        Chunk[] index;
        Chunk[] chunkArray = index = this.index();
        int n = index.length;
        int n2 = 0;
        while (n2 < n) {
            Chunk chunk = chunkArray[n2];
            if (idx >= chunk.iStart && idx < chunk.iStart + chunk.iRange) {
                return chunk.isTaggedAt(idx);
            }
            ++n2;
        }
        return false;
    }

    @Override
    public int groupAt(int idx) {
        Chunk[] index;
        Chunk[] chunkArray = index = this.index();
        int n = index.length;
        int n2 = 0;
        while (n2 < n) {
            Chunk chunk = chunkArray[n2];
            if (idx >= chunk.iStart && idx < chunk.iStart + chunk.iRange) {
                return chunk.groupAt(idx);
            }
            ++n2;
        }
        return 0;
    }

    @Override
    public int orderAt(int idx) {
        Chunk[] index;
        Chunk[] chunkArray = index = this.index();
        int n = index.length;
        int n2 = 0;
        while (n2 < n) {
            Chunk chunk = chunkArray[n2];
            if (idx >= chunk.iStart && idx < chunk.iStart + chunk.iRange) {
                return chunk.orderAt(idx);
            }
            ++n2;
        }
        return 0;
    }

    @Override
    public Object valueAt(int idx) {
        Chunk[] index;
        Chunk[] chunkArray = index = this.index();
        int n = index.length;
        int n2 = 0;
        while (n2 < n) {
            Chunk chunk = chunkArray[n2];
            if (idx >= chunk.iStart && idx < chunk.iStart + chunk.iRange) {
                return chunk.valueAt(idx);
            }
            ++n2;
        }
        return null;
    }

    @Override
    public CompoundValue compoundAt(int idx) {
        return null;
    }

    @Override
    public CompoundPack packedAt(int idx) {
        return null;
    }

    @Override
    public DomainValue positionAt(int idx) {
        return null;
    }

    @Override
    public long unitsAt(int idx) {
        Chunk[] index;
        Chunk[] chunkArray = index = this.index();
        int n = index.length;
        int n2 = 0;
        while (n2 < n) {
            Chunk chunk = chunkArray[n2];
            if (idx >= chunk.iStart && idx < chunk.iStart + chunk.iRange) {
                return chunk.unitsAt(idx);
            }
            ++n2;
        }
        return 0L;
    }

    @Override
    public int indexAt(long units) {
        Chunk[] index;
        Chunk[] chunkArray = index = this.index();
        int n = index.length;
        int n2 = 0;
        while (n2 < n) {
            Chunk chunk = chunkArray[n2];
            if (units >= chunk.uStart && units < chunk.uStart + chunk.uRange) {
                return chunk.indexAt(units);
            }
            ++n2;
        }
        return 0;
    }

    @Override
    public int indexAt(DomainValue position) {
        return 0;
    }

    @Override
    public List<IAttachment> attachmentsAt(int arg0, int arg1) {
        return null;
    }

    @Override
    public List<IAttachment> attachmentsAtGroup(int arg0, int arg1) {
        return null;
    }

    @Override
    public ISamplesProducer getProducer() {
        return null;
    }

    @Override
    public ISamplesReader getReader() {
        return null;
    }

    @Override
    public int indexAtGroup(int arg0) {
        return 0;
    }

    @Override
    public GroupedValue valuesAtGroup(int arg0) {
        return null;
    }

    private Chunk newLocalChunk(CompoundValue value) {
        Chunk c = new Chunk();
        c.local = true;
        c.value = value;
        c.iRange = 1;
        c.uOffset = 0L;
        return c;
    }

    private Chunk newRefChunk(int refIdx, int refRange) {
        Chunk c = new Chunk();
        c.local = false;
        c.value = null;
        c.iRange = refRange;
        c.iOffset = refIdx;
        c.uOffset = 0L;
        return c;
    }

    private Chunk newRefChunk(Chunk chunk, int from, int to) {
        Chunk c = new Chunk();
        c.local = false;
        c.value = null;
        c.iRange = to > from ? to - from : chunk.iRange - from;
        c.iOffset = chunk.iOffset + from;
        c.uOffset = chunk.uOffset;
        return c;
    }

    class Chunk {
        private boolean local;
        private int iStart;
        private int iRange;
        private int iOffset;
        private long uStart;
        private long uRange;
        private long uOffset;
        int group;
        CompoundValue value;

        Chunk() {
        }

        public void calcIndex(int iStart) {
            this.iStart = iStart;
            if (!this.local) {
                this.iOffset -= this.iStart;
            }
        }

        public long calcDomain(long uPreviousEnd) {
            long delta = 0L;
            this.uStart = this.local ? this.value.getUnits() - this.uOffset : this.unitsAt(this.iStart) - this.uOffset;
            if (this.uStart < uPreviousEnd) {
                this.uStart = uPreviousEnd;
            }
            return delta;
        }

        public void calcRange(long uNextStart) {
            this.uRange = uNextStart - this.uStart;
        }

        public boolean isRef() {
            return !this.local;
        }

        public boolean isLocal() {
            return this.local;
        }

        public void log() {
        }

        int getCount() {
            return !this.local ? this.iRange : 1;
        }

        public int getGroups() {
            return 0;
        }

        public long getRange() {
            return this.uRange;
        }

        public long getStartPos() {
            return this.uStart;
        }

        public long getEndPos() {
            return this.uStart + this.uRange;
        }

        public boolean hasTag() {
            return false;
        }

        public boolean isNoneAt(int idx) {
            return this.local ? this.value.isNone() : SamplesEditor.super.isNoneAt(idx + this.iOffset);
        }

        public boolean isTaggedAt(int idx) {
            return this.local ? this.value.isTagged() : SamplesEditor.super.isTaggedAt(idx + this.iOffset);
        }

        public int groupAt(int idx) {
            return this.local ? this.group : SamplesEditor.super.groupAt(idx + this.iOffset) + this.group;
        }

        public int orderAt(int idx) {
            return this.local ? this.value.getOrder() : SamplesEditor.super.orderAt(idx + this.iOffset);
        }

        public Object valueAt(int idx) {
            return this.local ? this.value.val() : SamplesEditor.super.valueAt(idx + this.iOffset);
        }

        public long unitsAt(int idx) {
            return this.local ? this.uStart : SamplesEditor.super.unitsAt(idx + this.iOffset) - this.uOffset;
        }

        public int indexAt(long units) {
            return this.local ? this.iStart : SamplesEditor.super.indexAt(units + this.uOffset) - this.iOffset;
        }
    }
}

