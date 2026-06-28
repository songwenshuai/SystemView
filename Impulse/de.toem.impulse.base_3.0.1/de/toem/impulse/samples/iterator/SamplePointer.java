/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.iterator;

import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.samples.ILogicDetector;
import de.toem.impulse.samples.IMemberDescriptor;
import de.toem.impulse.samples.IReadableMembers;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISamplePointer;
import de.toem.impulse.samples.iterator.AbstractPointer;
import de.toem.impulse.values.CompoundPack;
import de.toem.impulse.values.CompoundValue;
import de.toem.impulse.values.Enumeration;
import de.toem.impulse.values.IAttachment;
import de.toem.impulse.values.Logic;
import de.toem.impulse.values.Struct;
import java.math.BigDecimal;
import java.math.BigInteger;
import java.util.ArrayList;
import java.util.List;

public class SamplePointer
extends AbstractPointer
implements ISamplePointer,
IReadableMembers {
    public SamplePointer(IReadableSamples readable) {
        super(readable);
    }

    public SamplePointer(IReadableSamples readable, int index) {
        super(readable, index);
    }

    public SamplePointer(IReadableSamples readable, long units) {
        super(readable, units);
    }

    public SamplePointer(IReadableSamples readable, DomainValue position) {
        super(readable, position);
    }

    public static List<ISamplePointer> fromList(List<IReadableSamples> readables) {
        ArrayList<ISamplePointer> list = new ArrayList<ISamplePointer>();
        for (IReadableSamples r : readables) {
            list.add(new SamplePointer(r));
        }
        return list;
    }

    @Override
    public boolean isSample() {
        return true;
    }

    @Override
    public boolean isGroup() {
        return false;
    }

    @Override
    public int getMaxIndex() {
        return this.getCount() - 1;
    }

    @Override
    public int getMinIndex() {
        return -1;
    }

    @Override
    public long getUnits(boolean atEvent) {
        return atEvent ? this.unitsAt(this.index) : this.unitsAt(this.index) + this.delta;
    }

    @Override
    public long getUnits() {
        return this.unitsAt(this.index) + this.delta;
    }

    @Override
    public void setUnits(long units) {
        this.setIndex(this.indexAt(units));
        this.setDelta(units - this.unitsAt(this.index));
    }

    @Override
    public DomainValue getPosition(boolean atEvent) {
        DomainValue d = this.positionAt(this.index);
        if (!atEvent) {
            d.units += this.delta;
        }
        return d;
    }

    @Override
    public DomainValue getPosition() {
        DomainValue d = this.positionAt(this.index);
        d.units += this.delta;
        return d;
    }

    @Override
    public void setPosition(DomainValue position) {
        this.setIndex(this.indexAt(position));
        position = position.convertTo(this.getDomainBase());
        if (position != null) {
            this.setDelta(position.units - this.unitsAt(this.index));
        }
    }

    @Override
    public ISamplePointer relative(long delta) {
        SamplePointer p = new SamplePointer((IReadableSamples)this.reference);
        p.setUnits(this.getUnits() + delta);
        return p;
    }

    @Override
    public int getOrder() {
        return this.orderAt(this.index);
    }

    @Override
    public int getGroup() {
        return this.groupAt(this.index);
    }

    @Override
    public boolean isNone() {
        return this.isNoneAt(this.index);
    }

    @Override
    @Deprecated
    public boolean isConflict() {
        return this.isTaggedAt(this.index);
    }

    @Override
    public boolean isTagged() {
        return this.isTaggedAt(this.index);
    }

    @Override
    public int getTag() {
        return this.getTagAt(this.index);
    }

    @Override
    public Object val() {
        return this.valueAt(this.index);
    }

    @Override
    public CompoundValue compound() {
        return this.compoundAt(this.index);
    }

    @Override
    public CompoundValue compound(int attachments) {
        return this.compoundAt(this.index, attachments);
    }

    @Override
    public CompoundPack packed() {
        return this.packedAt(this.index);
    }

    @Override
    public List<IAttachment> attachments(int type) {
        return this.attachmentsAt(this.index, type);
    }

    @Override
    public Logic logicValue() {
        return this.logicValueAt(this.index);
    }

    @Override
    public int logicState() {
        return this.logicStateAt(this.index);
    }

    @Override
    public boolean isEdge(int edge) {
        return this.isEdge(edge, null);
    }

    @Override
    public boolean isEdge(int edge, ILogicDetector detector) {
        return this.delta == 0L ? this.isEdgeAt(this.index, edge, detector) : false;
    }

    @Override
    public boolean isLow() {
        return this.isLowAt(this.index, null);
    }

    @Override
    public boolean isLow(ILogicDetector detector) {
        return this.isLowAt(this.index, detector);
    }

    @Override
    public boolean isHigh() {
        return this.isHighAt(this.index, null);
    }

    @Override
    public boolean isHigh(ILogicDetector detector) {
        return this.isHighAt(this.index, detector);
    }

    @Override
    public boolean goPrevEdge(int edge) {
        while (this.goPrev()) {
            if (!this.isEdge(edge)) continue;
            return true;
        }
        return false;
    }

    @Override
    public boolean goPrevEdge(int edge, ILogicDetector detector) {
        while (this.goPrev()) {
            if (!this.isEdge(edge, detector)) continue;
            return true;
        }
        return false;
    }

    @Override
    public boolean goNextEdge(int edge) {
        while (this.goNext()) {
            if (!this.isEdge(edge)) continue;
            return true;
        }
        return false;
    }

    @Override
    public boolean goNextEdge(int edge, ILogicDetector detector) {
        while (this.goNext()) {
            if (!this.isEdge(edge, detector)) continue;
            return true;
        }
        return false;
    }

    @Override
    public Number numberValue() {
        return this.numberValueAt(this.index);
    }

    @Override
    public float floatValue() {
        return this.floatValueAt(this.index);
    }

    @Override
    public double doubleValue() {
        return this.doubleValueAt(this.index);
    }

    @Override
    public BigDecimal bigDecimalValue() {
        return this.bigDecimalValueAt(this.index);
    }

    @Override
    public long longValue() {
        return this.longValueAt(this.index);
    }

    @Override
    public int intValue() {
        return this.intValueAt(this.index);
    }

    @Override
    public BigInteger bigIntValue() {
        return this.bigIntValueAt(this.index);
    }

    @Override
    public Struct structValue() {
        return this.structValueAt(this.index);
    }

    @Override
    public String stringValue() {
        return this.stringValueAt(this.index);
    }

    @Override
    public Enumeration enumValue() {
        return this.enumValueAt(this.index);
    }

    @Override
    public byte[] bytesValue() {
        return this.bytesValueAt(this.index);
    }

    @Override
    public String format(int format) {
        return this.formatAt(this.index, format);
    }

    @Override
    public int defaultFormat() {
        return this.defaultFormatAt(this.index);
    }

    @Override
    public String fhex() {
        return this.formatAt(this.index, 3);
    }

    @Override
    public String fdec() {
        return this.formatAt(this.index, 5);
    }

    @Override
    public String foct() {
        return this.formatAt(this.index, 2);
    }

    @Override
    public String fbin() {
        return this.formatAt(this.index, 1);
    }

    @Override
    public String fascii() {
        return this.formatAt(this.index, 4);
    }

    @Override
    public String nameOf(int memberIndex) {
        CompoundValue compound = this.compoundAt(this.index);
        if (compound != null) {
            return compound.nameOf(memberIndex);
        }
        return null;
    }

    @Override
    public String pathOf(int memberIndex) {
        CompoundValue compound = this.compoundAt(this.index);
        if (compound != null) {
            return compound.pathOf(memberIndex);
        }
        return null;
    }

    @Override
    public int idOf(int memberIndex) {
        CompoundValue compound = this.compoundAt(this.index);
        if (compound != null) {
            return compound.idOf(memberIndex);
        }
        return -1;
    }

    @Override
    public String textOf(int memberIndex) {
        CompoundValue compound = this.compoundAt(this.index);
        if (compound != null) {
            return compound.textOf(memberIndex);
        }
        return null;
    }

    @Override
    public int indexOf(Object memberIdentifier) {
        CompoundValue compound = this.compoundAt(this.index);
        if (compound != null) {
            return compound.indexOf(memberIdentifier);
        }
        return 0;
    }

    @Override
    public int noOfMembers() {
        CompoundValue compound = this.compoundAt(this.index);
        if (compound != null) {
            return compound.noOfMembers();
        }
        return 0;
    }

    @Override
    public boolean hasMember(Object memberIdentifier) {
        CompoundValue compound = this.compoundAt(this.index);
        if (compound != null) {
            return compound.hasMember(memberIdentifier);
        }
        return false;
    }

    @Override
    public IMemberDescriptor descriptorOf(Object memberIdentifier) {
        CompoundValue compound = this.compoundAt(this.index);
        if (compound != null) {
            return compound.descriptorOf(memberIdentifier);
        }
        return null;
    }

    @Override
    public Object valueOf(Object memberIdentifier) {
        CompoundValue compound = this.compoundAt(this.index);
        if (compound != null) {
            return compound.valueOf(memberIdentifier);
        }
        return null;
    }

    @Override
    public Logic logicValueOf(Object memberIdentifier) {
        CompoundValue compound = this.compoundAt(this.index);
        if (compound != null) {
            return compound.logicValueOf(memberIdentifier);
        }
        return null;
    }

    @Override
    public int logicStateOf(Object memberIdentifier) {
        CompoundValue compound = this.compoundAt(this.index);
        if (compound != null) {
            return compound.logicStateOf(memberIdentifier);
        }
        return 15;
    }

    @Override
    public boolean isHighOf(Object memberIdentifier, ILogicDetector detector) {
        CompoundValue compound = this.compoundAt(this.index);
        if (compound != null) {
            return compound.isHighOf(memberIdentifier, detector);
        }
        return false;
    }

    @Override
    public boolean isLowOf(Object memberIdentifier, ILogicDetector detector) {
        CompoundValue compound = this.compoundAt(this.index);
        if (compound != null) {
            return compound.isLowOf(memberIdentifier, detector);
        }
        return false;
    }

    @Override
    public boolean booleanValueOf(Object memberIdentifier) {
        CompoundValue compound = this.compoundAt(this.index);
        if (compound != null) {
            return compound.booleanValueOf(memberIdentifier);
        }
        return false;
    }

    @Override
    public Number numberValueOf(Object memberIdentifier) {
        CompoundValue compound = this.compoundAt(this.index);
        if (compound != null) {
            return compound.numberValueOf(memberIdentifier);
        }
        return null;
    }

    @Override
    public float floatValueOf(Object memberIdentifier) {
        CompoundValue compound = this.compoundAt(this.index);
        if (compound != null) {
            return compound.floatValueOf(memberIdentifier);
        }
        return 0.0f;
    }

    @Override
    public double doubleValueOf(Object memberIdentifier) {
        CompoundValue compound = this.compoundAt(this.index);
        if (compound != null) {
            return compound.doubleValueOf(memberIdentifier);
        }
        return 0.0;
    }

    @Override
    public BigDecimal bigDecimalValueOf(Object memberIdentifier) {
        CompoundValue compound = this.compoundAt(this.index);
        if (compound != null) {
            return compound.bigDecimalValueOf(memberIdentifier);
        }
        return null;
    }

    @Override
    public long longValueOf(Object memberIdentifier) {
        CompoundValue compound = this.compoundAt(this.index);
        if (compound != null) {
            return compound.longValueOf(memberIdentifier);
        }
        return 0L;
    }

    @Override
    public int intValueOf(Object memberIdentifier) {
        CompoundValue compound = this.compoundAt(this.index);
        if (compound != null) {
            return compound.intValueOf(memberIdentifier);
        }
        return 0;
    }

    @Override
    public BigInteger bigIntValueOf(Object memberIdentifier) {
        CompoundValue compound = this.compoundAt(this.index);
        if (compound != null) {
            return compound.bigIntValueOf(memberIdentifier);
        }
        return null;
    }

    @Override
    public String stringValueOf(Object memberIdentifier) {
        CompoundValue compound = this.compoundAt(this.index);
        if (compound != null) {
            return compound.stringValueOf(memberIdentifier);
        }
        return null;
    }

    @Override
    public Enumeration enumValueOf(Object memberIdentifier) {
        CompoundValue compound = this.compoundAt(this.index);
        if (compound != null) {
            return compound.enumValueOf(memberIdentifier);
        }
        return null;
    }

    @Override
    public byte[] bytesValueOf(Object memberIdentifier) {
        CompoundValue compound = this.compoundAt(this.index);
        if (compound != null) {
            return compound.bytesValueOf(memberIdentifier);
        }
        return null;
    }

    @Override
    public String formatOf(Object memberIdentifier, int format) {
        CompoundValue compound = this.compoundAt(this.index);
        if (compound != null) {
            return compound.formatOf(memberIdentifier, format);
        }
        return null;
    }

    @Override
    public String fhexOf(Object memberIdentifier) {
        CompoundValue compound = this.compoundAt(this.index);
        if (compound != null) {
            return compound.fhexOf(memberIdentifier);
        }
        return null;
    }

    @Override
    public String fdecOf(Object memberIdentifier) {
        CompoundValue compound = this.compoundAt(this.index);
        if (compound != null) {
            return compound.fdecOf(memberIdentifier);
        }
        return null;
    }

    @Override
    public String foctOf(Object memberIdentifier) {
        CompoundValue compound = this.compoundAt(this.index);
        if (compound != null) {
            return compound.foctOf(memberIdentifier);
        }
        return null;
    }

    @Override
    public String fbinOf(Object memberIdentifier) {
        CompoundValue compound = this.compoundAt(this.index);
        if (compound != null) {
            return compound.fbinOf(memberIdentifier);
        }
        return null;
    }

    @Override
    public String fasciiOf(Object memberIdentifier) {
        CompoundValue compound = this.compoundAt(this.index);
        if (compound != null) {
            return compound.fasciiOf(memberIdentifier);
        }
        return null;
    }

    @Override
    public int defaultFormatOf(Object memberIdentifier) {
        CompoundValue compound = this.compoundAt(this.index);
        if (compound != null) {
            return compound.defaultFormatOf(memberIdentifier);
        }
        return 0;
    }
}

