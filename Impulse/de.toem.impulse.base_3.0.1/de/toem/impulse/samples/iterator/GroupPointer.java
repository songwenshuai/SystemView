/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.iterator;

import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.samples.IGroupPointer;
import de.toem.impulse.samples.ILogicDetector;
import de.toem.impulse.samples.IMemberDescriptor;
import de.toem.impulse.samples.IReadableMembers;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISamplePointer;
import de.toem.impulse.samples.iterator.AbstractPointer;
import de.toem.impulse.samples.iterator.SamplePointer;
import de.toem.impulse.values.Enumeration;
import de.toem.impulse.values.GroupedValue;
import de.toem.impulse.values.IAttachment;
import de.toem.impulse.values.Logic;
import java.math.BigDecimal;
import java.math.BigInteger;
import java.util.ArrayList;
import java.util.List;

public class GroupPointer
extends AbstractPointer
implements IGroupPointer {
    public GroupPointer(IReadableSamples readable) {
        super(readable);
    }

    public GroupPointer(IReadableSamples readable, int index) {
        super(readable, index);
    }

    public GroupPointer(IReadableSamples readable, long units) {
        super(readable, units);
    }

    public GroupPointer(IReadableSamples readable, DomainValue position) {
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
    public int getMaxIndex() {
        return this.getGroups() - 1;
    }

    @Override
    public int getMinIndex() {
        return 0;
    }

    @Override
    public IGroupPointer relative(long delta) {
        GroupPointer p = new GroupPointer((IReadableSamples)this.reference);
        p.setUnits(this.getUnits() + delta);
        return p;
    }

    @Override
    public long getUnits(boolean atEvent) {
        int idx = this.indexAtGroup(this.index);
        return atEvent ? this.unitsAt(idx) : this.unitsAt(idx) + this.delta;
    }

    @Override
    public long getUnits() {
        int idx = this.indexAtGroup(this.index);
        return this.unitsAt(idx) + this.delta;
    }

    @Override
    public void setUnits(long units) {
        int idx = this.indexAt(units);
        this.index = this.groupAt(idx);
        this.delta = units - this.unitsAt(this.index);
    }

    @Override
    public DomainValue getPosition(boolean atEvent) {
        int idx = this.indexAtGroup(this.index);
        DomainValue d = this.positionAt(idx);
        if (!atEvent) {
            d.units += this.delta;
        }
        return d;
    }

    @Override
    public DomainValue getPosition() {
        int idx = this.indexAtGroup(this.index);
        DomainValue d = this.positionAt(idx);
        d.units += this.delta;
        return d;
    }

    @Override
    public void setPosition(DomainValue position) {
        int idx = this.indexAt(position);
        this.index = this.groupAt(idx);
        idx = this.indexAtGroup(this.index);
        if ((position = position.convertTo(this.getDomainBase())) != null) {
            long sample = this.unitsAt(idx);
            this.delta = position.units - sample;
        }
    }

    @Override
    public GroupedValue val() {
        return this.valuesAtGroup(this.index);
    }

    @Override
    public List<IAttachment> attachments(int type) {
        return this.attachmentsAtGroup(this.index, type);
    }

    @Override
    public String format(int format) {
        GroupedValue groupValues = this.valuesAtGroup(this.index);
        if (groupValues != null) {
            return groupValues.format(format);
        }
        return null;
    }

    @Override
    public int defaultFormat() {
        GroupedValue groupValues = this.valuesAtGroup(this.index);
        if (groupValues != null) {
            return groupValues.defaultFormat();
        }
        return -1;
    }

    @Override
    public String nameOf(int memberIndex) {
        GroupedValue groupValues = this.valuesAtGroup(this.index);
        if (groupValues instanceof IReadableMembers) {
            return ((IReadableMembers)((Object)groupValues)).nameOf(memberIndex);
        }
        return null;
    }

    @Override
    public String pathOf(int memberIndex) {
        GroupedValue groupValues = this.valuesAtGroup(this.index);
        if (groupValues instanceof IReadableMembers) {
            return ((IReadableMembers)((Object)groupValues)).pathOf(memberIndex);
        }
        return null;
    }

    @Override
    public int idOf(int memberIndex) {
        GroupedValue groupValues = this.valuesAtGroup(this.index);
        if (groupValues instanceof IReadableMembers) {
            return ((IReadableMembers)((Object)groupValues)).idOf(memberIndex);
        }
        return -1;
    }

    @Override
    public String textOf(int memberIndex) {
        GroupedValue groupValues = this.valuesAtGroup(this.index);
        if (groupValues instanceof IReadableMembers) {
            return ((IReadableMembers)((Object)groupValues)).textOf(memberIndex);
        }
        return null;
    }

    @Override
    public int indexOf(Object memberIdentifier) {
        GroupedValue groupValues = this.valuesAtGroup(this.index);
        if (groupValues instanceof IReadableMembers) {
            return ((IReadableMembers)((Object)groupValues)).indexOf(memberIdentifier);
        }
        return -1;
    }

    @Override
    public int noOfMembers() {
        GroupedValue groupValues = this.valuesAtGroup(this.index);
        if (groupValues instanceof IReadableMembers) {
            return ((IReadableMembers)((Object)groupValues)).noOfMembers();
        }
        return 0;
    }

    @Override
    public boolean hasMember(Object memberIdentifier) {
        GroupedValue groupValues = this.valuesAtGroup(this.index);
        if (groupValues instanceof IReadableMembers) {
            return ((IReadableMembers)((Object)groupValues)).hasMember(memberIdentifier);
        }
        return false;
    }

    @Override
    public IMemberDescriptor descriptorOf(Object memberIdentifier) {
        GroupedValue groupValues = this.valuesAtGroup(this.index);
        if (groupValues instanceof IReadableMembers) {
            return ((IReadableMembers)((Object)groupValues)).descriptorOf(memberIdentifier);
        }
        return null;
    }

    @Override
    public Object valueOf(Object memberIdentifier) {
        GroupedValue groupValues = this.valuesAtGroup(this.index);
        if (groupValues instanceof IReadableMembers) {
            return ((IReadableMembers)((Object)groupValues)).valueOf(memberIdentifier);
        }
        return null;
    }

    @Override
    public Logic logicValueOf(Object memberIdentifier) {
        GroupedValue groupValues = this.valuesAtGroup(this.index);
        if (groupValues instanceof IReadableMembers) {
            return ((IReadableMembers)((Object)groupValues)).logicValueOf(memberIdentifier);
        }
        return null;
    }

    @Override
    public int logicStateOf(Object memberIdentifier) {
        GroupedValue groupValues = this.valuesAtGroup(this.index);
        if (groupValues instanceof IReadableMembers) {
            return ((IReadableMembers)((Object)groupValues)).logicStateOf(memberIdentifier);
        }
        return 15;
    }

    @Override
    public boolean isHighOf(Object memberIdentifier, ILogicDetector detector) {
        GroupedValue groupValues = this.valuesAtGroup(this.index);
        if (groupValues instanceof IReadableMembers) {
            return ((IReadableMembers)((Object)groupValues)).isHighOf(memberIdentifier, detector);
        }
        return false;
    }

    @Override
    public boolean isLowOf(Object memberIdentifier, ILogicDetector detector) {
        GroupedValue groupValues = this.valuesAtGroup(this.index);
        if (groupValues instanceof IReadableMembers) {
            return ((IReadableMembers)((Object)groupValues)).isLowOf(memberIdentifier, detector);
        }
        return false;
    }

    @Override
    public boolean booleanValueOf(Object memberIdentifier) {
        GroupedValue groupValues = this.valuesAtGroup(this.index);
        if (groupValues instanceof IReadableMembers) {
            return ((IReadableMembers)((Object)groupValues)).booleanValueOf(memberIdentifier);
        }
        return false;
    }

    @Override
    public Number numberValueOf(Object memberIdentifier) {
        GroupedValue groupValues = this.valuesAtGroup(this.index);
        if (groupValues instanceof IReadableMembers) {
            return ((IReadableMembers)((Object)groupValues)).numberValueOf(memberIdentifier);
        }
        return null;
    }

    @Override
    public float floatValueOf(Object memberIdentifier) {
        GroupedValue groupValues = this.valuesAtGroup(this.index);
        if (groupValues instanceof IReadableMembers) {
            return ((IReadableMembers)((Object)groupValues)).floatValueOf(memberIdentifier);
        }
        return 0.0f;
    }

    @Override
    public double doubleValueOf(Object memberIdentifier) {
        GroupedValue groupValues = this.valuesAtGroup(this.index);
        if (groupValues instanceof IReadableMembers) {
            return ((IReadableMembers)((Object)groupValues)).doubleValueOf(memberIdentifier);
        }
        return 0.0;
    }

    @Override
    public BigDecimal bigDecimalValueOf(Object memberIdentifier) {
        GroupedValue groupValues = this.valuesAtGroup(this.index);
        if (groupValues instanceof IReadableMembers) {
            return ((IReadableMembers)((Object)groupValues)).bigDecimalValueOf(memberIdentifier);
        }
        return null;
    }

    @Override
    public long longValueOf(Object memberIdentifier) {
        GroupedValue groupValues = this.valuesAtGroup(this.index);
        if (groupValues instanceof IReadableMembers) {
            return ((IReadableMembers)((Object)groupValues)).longValueOf(memberIdentifier);
        }
        return 0L;
    }

    @Override
    public int intValueOf(Object memberIdentifier) {
        GroupedValue groupValues = this.valuesAtGroup(this.index);
        if (groupValues instanceof IReadableMembers) {
            return ((IReadableMembers)((Object)groupValues)).intValueOf(memberIdentifier);
        }
        return 0;
    }

    @Override
    public BigInteger bigIntValueOf(Object memberIdentifier) {
        GroupedValue groupValues = this.valuesAtGroup(this.index);
        if (groupValues instanceof IReadableMembers) {
            return ((IReadableMembers)((Object)groupValues)).bigIntValueOf(memberIdentifier);
        }
        return null;
    }

    @Override
    public String stringValueOf(Object memberIdentifier) {
        GroupedValue groupValues = this.valuesAtGroup(this.index);
        if (groupValues instanceof IReadableMembers) {
            return ((IReadableMembers)((Object)groupValues)).stringValueOf(memberIdentifier);
        }
        return null;
    }

    @Override
    public Enumeration enumValueOf(Object memberIdentifier) {
        GroupedValue groupValues = this.valuesAtGroup(this.index);
        if (groupValues instanceof IReadableMembers) {
            return ((IReadableMembers)((Object)groupValues)).enumValueOf(memberIdentifier);
        }
        return null;
    }

    @Override
    public byte[] bytesValueOf(Object memberIdentifier) {
        GroupedValue groupValues = this.valuesAtGroup(this.index);
        if (groupValues instanceof IReadableMembers) {
            return ((IReadableMembers)((Object)groupValues)).bytesValueOf(memberIdentifier);
        }
        return null;
    }

    @Override
    public String formatOf(Object memberIdentifier, int format) {
        GroupedValue groupValues = this.valuesAtGroup(this.index);
        if (groupValues instanceof IReadableMembers) {
            return ((IReadableMembers)((Object)groupValues)).formatOf(memberIdentifier, format);
        }
        return null;
    }

    @Override
    public String fhexOf(Object memberIdentifier) {
        GroupedValue groupValues = this.valuesAtGroup(this.index);
        if (groupValues instanceof IReadableMembers) {
            return ((IReadableMembers)((Object)groupValues)).fhexOf(memberIdentifier);
        }
        return null;
    }

    @Override
    public String fdecOf(Object memberIdentifier) {
        GroupedValue groupValues = this.valuesAtGroup(this.index);
        if (groupValues instanceof IReadableMembers) {
            return ((IReadableMembers)((Object)groupValues)).fdecOf(memberIdentifier);
        }
        return null;
    }

    @Override
    public String foctOf(Object memberIdentifier) {
        GroupedValue groupValues = this.valuesAtGroup(this.index);
        if (groupValues instanceof IReadableMembers) {
            return ((IReadableMembers)((Object)groupValues)).foctOf(memberIdentifier);
        }
        return null;
    }

    @Override
    public String fbinOf(Object memberIdentifier) {
        GroupedValue groupValues = this.valuesAtGroup(this.index);
        if (groupValues instanceof IReadableMembers) {
            return ((IReadableMembers)((Object)groupValues)).fbinOf(memberIdentifier);
        }
        return null;
    }

    @Override
    public String fasciiOf(Object memberIdentifier) {
        GroupedValue groupValues = this.valuesAtGroup(this.index);
        if (groupValues instanceof IReadableMembers) {
            return ((IReadableMembers)((Object)groupValues)).fasciiOf(memberIdentifier);
        }
        return null;
    }

    @Override
    public int defaultFormatOf(Object memberIdentifier) {
        GroupedValue groupValues = this.valuesAtGroup(this.index);
        if (groupValues instanceof IReadableMembers) {
            return ((IReadableMembers)((Object)groupValues)).defaultFormatOf(memberIdentifier);
        }
        return 0;
    }
}

