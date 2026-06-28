/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.writer;

import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesLegend;
import de.toem.impulse.samples.ISamplesWriter;
import de.toem.impulse.values.CompoundPack;
import de.toem.impulse.values.CompoundValue;
import de.toem.toolkits.pattern.element.exploits.Marker;
import de.toem.toolkits.pattern.element.exploits.Markers;
import de.toem.toolkits.pattern.pageable.Pageable;

public class ReferencedSamplesWriter
implements ISamplesWriter {
    ISamplesWriter writer;

    protected ReferencedSamplesWriter(ISamplesWriter writer) {
        this.writer = writer;
    }

    @Override
    public void setDomainBase(IDomainBase domainBase) {
        this.writer.setDomainBase(domainBase);
    }

    @Override
    public boolean isOpen() {
        return this.writer.isOpen();
    }

    @Override
    public boolean open(long units) {
        return this.writer.open(units);
    }

    @Override
    public boolean open(long units, Pageable<byte[]> samples) {
        return this.writer.open(units, samples);
    }

    @Override
    public boolean open(long units, int mode, int limitation, Pageable<byte[]> samples) {
        return this.writer.open(units, mode, limitation, samples);
    }

    @Override
    public boolean open(long units, long rate, int mode, int limitation, Pageable<byte[]> samples) {
        return this.writer.open(units, rate, mode, limitation, samples);
    }

    @Override
    public boolean open(long units, long maxUnits, long rate, int mode, int limitation, Pageable<byte[]> samples) {
        return this.writer.open(units, maxUnits, rate, mode, limitation, samples);
    }

    @Override
    public long getMaxUnits() {
        return this.writer.getMaxUnits();
    }

    @Override
    public void close(long units) {
        this.writer.close(units);
    }

    @Override
    public void close() {
        this.writer.close();
    }

    @Override
    public void flush() {
        this.writer.flush();
    }

    @Override
    public void flush(long units) {
        this.writer.flush(units);
    }

    @Override
    public boolean apply(Signal signal) {
        return this.writer.apply(signal);
    }

    @Override
    public int getPackVersion() {
        return this.writer.getPackVersion();
    }

    @Override
    public Pageable<byte[]> getSamples() {
        return this.writer.getSamples();
    }

    @Override
    public ISamplesLegend getLegend() {
        return this.writer.getLegend();
    }

    @Override
    public ISamples.SignalType getSignalType() {
        return this.writer.getSignalType();
    }

    @Override
    public ISamples.ProcessType getProcessType() {
        return this.writer.getProcessType();
    }

    @Override
    public int getCount() {
        return this.writer.getCount();
    }

    @Override
    public int getGroups() {
        return this.writer.getGroups();
    }

    @Override
    public boolean isEmpty() {
        return this.writer.isEmpty();
    }

    @Override
    public IDomainBase getDomainBase() {
        return this.writer.getDomainBase();
    }

    @Override
    public DomainValue getStart() {
        return this.writer.getStart();
    }

    @Override
    public DomainValue getEnd() {
        return this.writer.getEnd();
    }

    @Override
    public DomainValue getRate() {
        return this.writer.getRate();
    }

    @Override
    public long getStartUnits() {
        return this.writer.getStartUnits();
    }

    @Override
    public long getEndUnits() {
        return this.writer.getEndUnits();
    }

    @Override
    public long getRateUnits() {
        return this.writer.getRateUnits();
    }

    @Override
    public ISamples.SignalDescriptor getSignalDescriptor() {
        return this.writer.getSignalDescriptor();
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
        return this.writer.hasTag();
    }

    @Override
    public boolean hasTag() {
        return this.writer.hasTag();
    }

    @Override
    public ISamples.TagDomain getTagDomain() {
        return this.writer.getTagDomain();
    }

    @Override
    public void setTagDomain(ISamples.TagDomain tagDomain) {
        this.writer.setTagDomain(tagDomain);
    }

    @Override
    public boolean write(long units, boolean tag) {
        return this.writer.write(units, tag);
    }

    @Override
    public boolean write(long units, int tag) {
        return this.writer.write(units, tag);
    }

    @Override
    public boolean writeNone(long units, boolean tag) {
        return this.writer.writeNone(units, tag);
    }

    @Override
    public boolean writeNone(long units, int tag) {
        return this.writer.writeNone(units, tag);
    }

    @Override
    public boolean writeSample(long units, byte format0) {
        return this.writer.writeSample(units, format0);
    }

    @Override
    public boolean writeSample(long units, byte format0, byte data0) {
        return this.writer.writeSample(units, format0, data0);
    }

    @Override
    public boolean writeSample(long units, byte format0, byte[] data, int start, int dlength) {
        return this.writer.writeSample(units, format0, data, start, dlength);
    }

    @Override
    public boolean writeSample(long units, byte format0, int group, int layer, byte[] data, int start, int dlength) {
        return this.writer.writeSample(units, format0, group, layer, data, start, dlength);
    }

    @Override
    public boolean writeSample(long units, boolean tag, Object value) {
        return this.writer.writeSample(units, tag, value);
    }

    @Override
    public boolean writeSample(long units, boolean tag, int group, int order, int layer, Object value) {
        return this.writer.writeSample(units, tag, group, order, layer, value);
    }

    @Override
    public boolean writeSample(long units, int tag, Object value) {
        return this.writer.writeSample(units, tag, value);
    }

    @Override
    public boolean writeSample(long units, int tag, int group, int order, int layer, Object value) {
        return this.writer.writeSample(units, tag, group, order, layer, value);
    }

    @Override
    public boolean writeSample(CompoundValue value) {
        return this.writer.writeSample(value);
    }

    @Override
    public boolean writeSample(CompoundPack packed) {
        return this.writer.writeSample(packed);
    }

    @Override
    public IDomainBase getSamplesDomainBase() {
        return this.writer.getSamplesDomainBase();
    }

    @Override
    public String getId() {
        return this.writer.getId();
    }

    @Override
    public String getName() {
        return this.writer.getName();
    }

    @Override
    public String getLabel() {
        return this.writer.getLabel();
    }

    @Override
    public String getDescription() {
        return this.writer.getDescription();
    }

    @Override
    public String getIconId() {
        return this.writer.getIconId();
    }

    @Override
    public String getMessage() {
        return this.writer.getMessage();
    }

    @Override
    public String getError() {
        return this.writer.getError();
    }

    @Override
    public void setLegend(ISamplesLegend legend) {
        this.writer.setLegend(legend);
    }

    @Override
    public int addMember(String name, String content, int format) {
        return this.writer.addMember(name, content, format);
    }

    @Override
    public boolean setMember(int id, String name, String content, int format) {
        return this.writer.setMember(id, name, content, format);
    }

    @Override
    public boolean setMember(int id, String name, int type, String content, int format) {
        return this.writer.setMember(id, name, type, content, format);
    }

    @Override
    public boolean setMember(int id, int parentId, String name, int type, String content, int format) {
        return this.writer.setMember(id, parentId, name, type, content, format);
    }

    @Override
    public boolean setEnum(int enumerationGroup, String label, int value) {
        return this.writer.setEnum(enumerationGroup, label, value);
    }

    @Override
    public boolean attachRelation(String target, String style, long diff) {
        return this.writer.attachRelation(target, style, diff);
    }

    @Override
    public boolean attachRelation(int type, String target, String style, long position) {
        return this.writer.attachRelation(type, target, style, position);
    }

    @Override
    public boolean attachRelation(int to, int style, long diff) {
        return this.writer.attachRelation(to, style, diff);
    }

    @Override
    public boolean attachRelation(int type, int to, int style, long diff) {
        return this.writer.attachRelation(type, to, style, diff);
    }

    @Override
    public boolean attachRelation(int type, String target, String style, long position, IDomainBase targetBase) {
        return this.writer.attachRelation(type, target, style, position, targetBase);
    }

    @Override
    public boolean attachRelation(int type, int targetId, int styleId, long position, int targetBaseId) {
        return this.writer.attachRelation(type, targetId, styleId, position, targetBaseId);
    }

    @Override
    public boolean attachRelation(int type, String target, String style, long targetPosition, IDomainBase targetBase, int targetIdx, int targetLayer) {
        return this.writer.attachRelation(type, target, style, targetPosition, targetBase, targetIdx, targetLayer);
    }

    @Override
    public boolean attachRelation(int type, int targetId, int styleId, long targetPosition, int targetBaseId, int targetIdx, int targetLayer) {
        return this.writer.attachRelation(type, targetId, styleId, targetPosition, targetBaseId, targetIdx, targetLayer);
    }

    @Override
    public boolean attachLabel(String style) {
        return this.writer.attachLabel(style);
    }

    @Override
    public boolean attachLabel(int styleId) {
        return this.writer.attachLabel(styleId);
    }

    @Override
    public boolean addMarker(Marker marker) {
        return this.writer.addMarker(marker);
    }

    @Override
    public Markers getMarkers() {
        return this.writer.getMarkers();
    }

    @Override
    public Object getData() {
        return this.writer.getData();
    }

    @Override
    public Object getData(String key) {
        return this.writer.getData(key);
    }

    @Override
    public void setData(Object value) {
        this.writer.setData(value);
    }

    @Override
    public void setData(String key, Object value) {
        this.writer.setData(key, value);
    }

    @Override
    public Object getService(Class<?> cs) {
        return this.writer.getService(cs);
    }

    @Override
    public int getRelease() {
        return this.writer.getRelease();
    }

    @Override
    public boolean isVolatile() {
        return this.writer.isVolatile();
    }

    @Override
    public boolean isMonotonous() {
        return this.writer.isMonotonous();
    }

    @Override
    public boolean isReleased() {
        return this.writer.isReleased();
    }
}

