/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.writer;

import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.samples.IFloatSamplesWriter;
import de.toem.impulse.samples.ILogicSamplesWriter;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesLegend;
import de.toem.impulse.samples.ISamplesWriter;
import de.toem.impulse.samples.writer.IDivergingSamplesWriter;
import de.toem.impulse.values.CompoundPack;
import de.toem.impulse.values.CompoundValue;
import de.toem.impulse.values.Logic;
import de.toem.toolkits.pattern.element.exploits.Marker;
import de.toem.toolkits.pattern.element.exploits.Markers;
import de.toem.toolkits.pattern.pageable.Pageable;
import java.math.BigDecimal;
import java.util.ArrayList;
import java.util.List;

public class DivergingSamplesWriter
implements ILogicSamplesWriter,
IFloatSamplesWriter,
IDivergingSamplesWriter {
    List<ISamplesWriter> writers = new ArrayList<ISamplesWriter>();

    public DivergingSamplesWriter() {
    }

    public DivergingSamplesWriter(ISamplesWriter writer) {
        this.writers.add(writer);
    }

    @Override
    public void addDestination(ISamplesWriter writer) {
        this.writers.add(writer);
    }

    @Override
    public void setDomainBase(IDomainBase domainBase) {
        for (ISamplesWriter writer : this.writers) {
            writer.setDomainBase(domainBase);
        }
    }

    @Override
    public boolean isOpen() {
        boolean result = true;
        for (ISamplesWriter writer : this.writers) {
            result &= writer.isOpen();
        }
        return result;
    }

    @Override
    public boolean open(long units) {
        boolean result = true;
        for (ISamplesWriter writer : this.writers) {
            result &= writer.open(units);
        }
        return result;
    }

    @Override
    public boolean open(long units, Pageable<byte[]> samples) {
        boolean result = true;
        for (ISamplesWriter writer : this.writers) {
            result &= writer.open(units, samples);
        }
        return result;
    }

    @Override
    public boolean open(long units, int mode, int limitation, Pageable<byte[]> samples) {
        boolean result = true;
        for (ISamplesWriter writer : this.writers) {
            result &= writer.open(units, mode, limitation, samples);
        }
        return result;
    }

    @Override
    public boolean open(long units, long rate, int mode, int limitation, Pageable<byte[]> samples) {
        boolean result = true;
        for (ISamplesWriter writer : this.writers) {
            result &= writer.open(units, rate, mode, limitation, samples);
        }
        return result;
    }

    @Override
    public boolean open(long units, long maxUnits, long rate, int mode, int limitation, Pageable<byte[]> samples) {
        boolean result = true;
        for (ISamplesWriter writer : this.writers) {
            result &= writer.open(units, maxUnits, rate, mode, limitation, samples);
        }
        return result;
    }

    @Override
    public long getMaxUnits() {
        return this.writers.get(0).getMaxUnits();
    }

    @Override
    public void close(long units) {
        for (ISamplesWriter writer : this.writers) {
            writer.close(units);
        }
    }

    @Override
    public void close() {
        for (ISamplesWriter writer : this.writers) {
            writer.close();
        }
    }

    @Override
    public void flush() {
        for (ISamplesWriter writer : this.writers) {
            writer.flush();
        }
    }

    @Override
    public void flush(long units) {
        for (ISamplesWriter writer : this.writers) {
            writer.flush(units);
        }
    }

    @Override
    public boolean apply(Signal signal) {
        return this.writers.get(0).apply(signal);
    }

    @Override
    public int getPackVersion() {
        return this.writers.get(0).getPackVersion();
    }

    @Override
    public Pageable<byte[]> getSamples() {
        return this.writers.get(0).getSamples();
    }

    @Override
    public ISamplesLegend getLegend() {
        return this.writers.get(0).getLegend();
    }

    @Override
    public ISamples.SignalType getSignalType() {
        return this.writers.get(0).getSignalType();
    }

    @Override
    public ISamples.ProcessType getProcessType() {
        return this.writers.get(0).getProcessType();
    }

    @Override
    public int getCount() {
        return this.writers.get(0).getCount();
    }

    @Override
    public int getGroups() {
        return this.writers.get(0).getGroups();
    }

    @Override
    public boolean isEmpty() {
        return this.writers.get(0).isEmpty();
    }

    @Override
    public IDomainBase getDomainBase() {
        return this.writers.get(0).getDomainBase();
    }

    @Override
    public DomainValue getStart() {
        return this.writers.get(0).getStart();
    }

    @Override
    public DomainValue getEnd() {
        return this.writers.get(0).getEnd();
    }

    @Override
    public DomainValue getRate() {
        return this.writers.get(0).getRate();
    }

    @Override
    public long getStartUnits() {
        return this.writers.get(0).getStartUnits();
    }

    @Override
    public long getEndUnits() {
        return this.writers.get(0).getEndUnits();
    }

    @Override
    public long getRateUnits() {
        return this.writers.get(0).getRateUnits();
    }

    @Override
    public ISamples.SignalDescriptor getSignalDescriptor() {
        return this.writers.get(0).getSignalDescriptor();
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
        return this.writers.get(0).hasTag();
    }

    @Override
    public boolean hasTag() {
        return this.writers.get(0).hasTag();
    }

    @Override
    public ISamples.TagDomain getTagDomain() {
        return this.writers.get(0).getTagDomain();
    }

    @Override
    public void setTagDomain(ISamples.TagDomain tagDomain) {
    }

    @Override
    public IDomainBase getSamplesDomainBase() {
        return this.writers.get(0).getSamplesDomainBase();
    }

    @Override
    public String getId() {
        return this.writers.get(0).getId();
    }

    @Override
    public String getName() {
        return this.writers.get(0).getName();
    }

    @Override
    public String getLabel() {
        return this.writers.get(0).getLabel();
    }

    @Override
    public String getDescription() {
        return this.writers.get(0).getDescription();
    }

    @Override
    public String getIconId() {
        return this.writers.get(0).getIconId();
    }

    @Override
    public String getMessage() {
        return this.writers.get(0).getMessage();
    }

    @Override
    public String getError() {
        return this.writers.get(0).getError();
    }

    @Override
    public Object getService(Class<?> cs) {
        return null;
    }

    @Override
    public int getRelease() {
        return 0;
    }

    @Override
    public boolean isVolatile() {
        return true;
    }

    @Override
    public boolean isMonotonous() {
        return false;
    }

    @Override
    public boolean isReleased() {
        return true;
    }

    @Override
    public void setLegend(ISamplesLegend legend) {
    }

    @Override
    public int addMember(String name, String content, int format) {
        return -1;
    }

    @Override
    public boolean setMember(int id, String name, String content, int format) {
        return false;
    }

    @Override
    public boolean setMember(int id, String name, int type, String content, int format) {
        return false;
    }

    @Override
    public boolean setMember(int id, int parentId, String name, int type, String content, int format) {
        return false;
    }

    @Override
    public boolean setEnum(int enumerationGroup, String label, int value) {
        return false;
    }

    @Override
    public boolean attachRelation(String to, String style, long diff) {
        return false;
    }

    @Override
    public boolean attachRelation(int type, String target, String style, long position) {
        return false;
    }

    @Override
    public boolean attachLabel(String style) {
        return false;
    }

    @Override
    public boolean attachLabel(int styleId) {
        return false;
    }

    @Override
    public boolean attachRelation(int to, int style, long diff) {
        return false;
    }

    @Override
    public boolean attachRelation(int type, int to, int style, long diff) {
        return false;
    }

    @Override
    public boolean attachRelation(int type, String target, String style, long position, IDomainBase targetBase) {
        return false;
    }

    @Override
    public boolean attachRelation(int type, int targetId, int styleId, long position, int targetBaseId) {
        return false;
    }

    @Override
    public boolean attachRelation(int type, String target, String style, long targetPosition, IDomainBase targetBase, int targetIdx, int targetLayer) {
        return false;
    }

    @Override
    public boolean attachRelation(int type, int targetId, int styleId, long targetPosition, int targetBaseId, int targetIdx, int targetLayer) {
        return false;
    }

    @Override
    public boolean addMarker(Marker marker) {
        return false;
    }

    @Override
    public Markers getMarkers() {
        return this.writers.get(0).getMarkers();
    }

    @Override
    public Object getData() {
        return this.writers.get(0).getData();
    }

    @Override
    public Object getData(String key) {
        return this.writers.get(0).getData(key);
    }

    @Override
    public void setData(Object value) {
        for (ISamplesWriter writer : this.writers) {
            writer.setData(value);
        }
    }

    @Override
    public void setData(String key, Object value) {
        for (ISamplesWriter writer : this.writers) {
            writer.setData(key, value);
        }
    }

    @Override
    public boolean write(long units, boolean tag) {
        for (ISamplesWriter writer : this.writers) {
            if (!(writer instanceof ILogicSamplesWriter)) continue;
            ((ILogicSamplesWriter)writer).write(units, tag);
        }
        return true;
    }

    @Override
    public boolean writeNone(long units, boolean tag) {
        for (ISamplesWriter writer : this.writers) {
            if (!(writer instanceof ILogicSamplesWriter)) continue;
            ((ILogicSamplesWriter)writer).writeNone(units, tag);
        }
        return true;
    }

    @Override
    public boolean write(long units, int tag) {
        for (ISamplesWriter writer : this.writers) {
            if (!(writer instanceof ILogicSamplesWriter)) continue;
            ((ILogicSamplesWriter)writer).write(units, tag);
        }
        return true;
    }

    @Override
    public boolean writeNone(long units, int tag) {
        for (ISamplesWriter writer : this.writers) {
            if (!(writer instanceof ILogicSamplesWriter)) continue;
            ((ILogicSamplesWriter)writer).writeNone(units, tag);
        }
        return true;
    }

    @Override
    public boolean writeSample(long units, byte format0) {
        boolean result = true;
        for (ISamplesWriter writer : this.writers) {
            result &= writer.writeSample(units, format0);
        }
        return result;
    }

    @Override
    public boolean writeSample(long units, byte format0, byte data0) {
        boolean result = true;
        for (ISamplesWriter writer : this.writers) {
            result &= writer.writeSample(units, format0, data0);
        }
        return result;
    }

    @Override
    public boolean writeSample(long units, byte format0, byte[] data, int start, int dlength) {
        boolean result = true;
        for (ISamplesWriter writer : this.writers) {
            result &= writer.writeSample(units, format0, data, start, dlength);
        }
        return result;
    }

    @Override
    public boolean writeSample(long units, byte format0, int group, int layer, byte[] data, int start, int dlength) {
        boolean result = true;
        for (ISamplesWriter writer : this.writers) {
            result &= writer.writeSample(units, format0, group, layer, data, start, dlength);
        }
        return result;
    }

    @Override
    public boolean writeSample(CompoundPack packed) {
        boolean result = true;
        for (ISamplesWriter writer : this.writers) {
            result &= writer.writeSample(packed);
        }
        return result;
    }

    @Override
    public int getBitWidth() {
        return this.writers.get(0) instanceof ILogicSamplesWriter ? ((ILogicSamplesWriter)this.writers.get(0)).getBitWidth() : 1;
    }

    @Override
    public boolean write(long units, boolean tag, byte states) {
        for (ISamplesWriter writer : this.writers) {
            if (!(writer instanceof ILogicSamplesWriter)) continue;
            ((ILogicSamplesWriter)writer).write(units, tag, states);
        }
        return true;
    }

    @Override
    public boolean write(long units, boolean tag, int stateLevel, byte states) {
        for (ISamplesWriter writer : this.writers) {
            if (!(writer instanceof ILogicSamplesWriter)) continue;
            ((ILogicSamplesWriter)writer).write(units, tag, stateLevel, states);
        }
        return true;
    }

    @Override
    public boolean write(long units, boolean tag, int stateLevel, byte precedingStates, byte state) {
        for (ISamplesWriter writer : this.writers) {
            if (!(writer instanceof ILogicSamplesWriter)) continue;
            ((ILogicSamplesWriter)writer).write(units, tag, stateLevel, precedingStates, state);
        }
        return true;
    }

    @Override
    public boolean write(long units, boolean tag, int stateLevel, byte precedingStates, byte[] states, int start, int length) {
        for (ISamplesWriter writer : this.writers) {
            if (!(writer instanceof ILogicSamplesWriter)) continue;
            ((ILogicSamplesWriter)writer).write(units, tag, stateLevel, precedingStates, states, start, length);
        }
        return true;
    }

    @Override
    public boolean write(long units, boolean tag, byte precedingStates, byte[] states, int start, int length) {
        for (ISamplesWriter writer : this.writers) {
            if (!(writer instanceof ILogicSamplesWriter)) continue;
            ((ILogicSamplesWriter)writer).write(units, tag, precedingStates, states, start, length);
        }
        return true;
    }

    @Override
    public boolean write(long units, boolean tag, byte precedingStates, String states) {
        for (ISamplesWriter writer : this.writers) {
            if (!(writer instanceof ILogicSamplesWriter)) continue;
            ((ILogicSamplesWriter)writer).write(units, tag, precedingStates, states);
        }
        return true;
    }

    @Override
    public boolean write(long units, Logic logic) {
        return this.write(units, false, logic);
    }

    @Override
    public boolean write(long units, boolean tag, Logic logic) {
        for (ISamplesWriter writer : this.writers) {
            if (!(writer instanceof ILogicSamplesWriter)) continue;
            logic.write((ILogicSamplesWriter)writer, units, tag);
        }
        return true;
    }

    @Override
    public boolean writeByte(long units, boolean tag, byte states) {
        return this.write(units, tag, states);
    }

    @Override
    public boolean writeBytesP(long units, boolean tag, byte precedingStates, byte[] states, int start, int length) {
        return this.write(units, tag, precedingStates, states, start, length);
    }

    @Override
    public boolean writeStringP(long units, boolean tag, byte precedingStates, String states) {
        return this.write(units, tag, precedingStates, states);
    }

    @Override
    public boolean writeByteS(long units, boolean tag, int stateLevel, byte states) {
        return this.write(units, tag, stateLevel, states);
    }

    @Override
    public boolean writeByteSP(long units, boolean tag, int stateLevel, byte precedingStates, byte state) {
        return this.write(units, tag, stateLevel, precedingStates, state);
    }

    @Override
    public boolean writeBytesSP(long units, boolean tag, int stateLevel, byte precedingStates, byte[] states, int start, int length) {
        return this.write(units, tag, stateLevel, precedingStates, states, start, length);
    }

    @Override
    public boolean writeLogic(long units, Logic logic) {
        return this.write(units, false, logic);
    }

    @Override
    public boolean writeLogic(long units, boolean tag, Logic logic) {
        return this.write(units, tag, logic);
    }

    @Override
    public boolean write(long units, double value) {
        for (ISamplesWriter writer : this.writers) {
            if (!(writer instanceof IFloatSamplesWriter)) continue;
            ((IFloatSamplesWriter)writer).write(units, value);
        }
        return true;
    }

    @Override
    public boolean write(long units, boolean tag, double value) {
        for (ISamplesWriter writer : this.writers) {
            if (!(writer instanceof IFloatSamplesWriter)) continue;
            ((IFloatSamplesWriter)writer).write(units, tag, value);
        }
        return true;
    }

    @Override
    public boolean write(long units, boolean tag, float value) {
        for (ISamplesWriter writer : this.writers) {
            if (!(writer instanceof IFloatSamplesWriter)) continue;
            ((IFloatSamplesWriter)writer).write(units, tag, value);
        }
        return true;
    }

    @Override
    public boolean write(long units, boolean tag, BigDecimal value) {
        for (ISamplesWriter writer : this.writers) {
            if (!(writer instanceof IFloatSamplesWriter)) continue;
            ((IFloatSamplesWriter)writer).write(units, tag, value);
        }
        return true;
    }

    @Override
    public boolean write(long units, boolean tag, Number value) {
        for (ISamplesWriter writer : this.writers) {
            if (!(writer instanceof IFloatSamplesWriter)) continue;
            ((IFloatSamplesWriter)writer).write(units, tag, value);
        }
        return true;
    }

    @Override
    public boolean writeFloat(long units, boolean tag, float value) {
        return this.write(units, tag, value);
    }

    @Override
    public boolean writeDouble(long units, boolean tag, double value) {
        return this.write(units, tag, value);
    }

    @Override
    public boolean writeBig(long units, boolean tag, BigDecimal value) {
        return this.write(units, tag, value);
    }

    @Override
    public boolean write(long units, boolean tag, float[] value) {
        for (ISamplesWriter writer : this.writers) {
            if (!(writer instanceof IFloatSamplesWriter)) continue;
            ((IFloatSamplesWriter)writer).write(units, tag, value);
        }
        return true;
    }

    @Override
    public boolean write(long units, boolean tag, double[] value) {
        for (ISamplesWriter writer : this.writers) {
            if (!(writer instanceof IFloatSamplesWriter)) continue;
            ((IFloatSamplesWriter)writer).write(units, tag, value);
        }
        return true;
    }

    @Override
    public boolean write(long units, boolean tag, BigDecimal[] value) {
        for (ISamplesWriter writer : this.writers) {
            if (!(writer instanceof IFloatSamplesWriter)) continue;
            ((IFloatSamplesWriter)writer).write(units, tag, value);
        }
        return true;
    }

    @Override
    public boolean writeFloatArray(long units, boolean tag, float[] value) {
        return this.write(units, tag, value);
    }

    @Override
    public boolean writeDoubleArray(long units, boolean tag, double[] value) {
        return this.write(units, tag, value);
    }

    @Override
    public boolean writeFloatArgs(long units, boolean tag, float ... value) {
        return this.write(units, tag, value);
    }

    @Override
    public boolean writeDoubleArgs(long units, boolean tag, double ... value) {
        return this.write(units, tag, value);
    }

    @Override
    public boolean writeBigArray(long units, boolean tag, BigDecimal[] value) {
        return this.write(units, tag, value);
    }

    @Override
    public boolean writeBigArgs(long units, boolean tag, BigDecimal ... value) {
        return this.write(units, tag, value);
    }

    @Override
    public boolean writeSample(CompoundValue value) {
        return false;
    }

    @Override
    public boolean writeSample(long units, boolean tag, Object value) {
        return false;
    }

    @Override
    public boolean writeSample(long units, boolean tag, int group, int order, int layer, Object value) {
        return false;
    }

    @Override
    public boolean writeSample(long units, int tag, Object value) {
        return false;
    }

    @Override
    public boolean writeSample(long units, int tag, int group, int order, int layer, Object value) {
        return false;
    }
}

