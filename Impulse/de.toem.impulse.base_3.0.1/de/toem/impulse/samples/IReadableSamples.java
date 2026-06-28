/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples;

import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.samples.ILogicDetector;
import de.toem.impulse.samples.IMemberDescriptor;
import de.toem.impulse.samples.ISampleConverter;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesProducer;
import de.toem.impulse.samples.ISamplesReader;
import de.toem.impulse.samples.base.SamplesStat;
import de.toem.impulse.values.CompoundPack;
import de.toem.impulse.values.CompoundValue;
import de.toem.impulse.values.Enumeration;
import de.toem.impulse.values.GroupedValue;
import de.toem.impulse.values.IAttachment;
import de.toem.impulse.values.Logic;
import de.toem.impulse.values.Struct;
import de.toem.toolkits.pattern.threading.IProgress;
import java.math.BigDecimal;
import java.math.BigInteger;
import java.util.List;

public interface IReadableSamples
extends ISamples,
ISampleConverter {
    public ISamplesReader getReader();

    public ISamplesProducer getProducer();

    public boolean isEmpty();

    public int getCount();

    public int getGroups();

    public boolean isSettled();

    public boolean ensureSettled(IProgress var1);

    public boolean isSettling();

    public int indexAt(long var1);

    public int indexAt(DomainValue var1);

    public long unitsAt(int var1);

    public DomainValue positionAt(int var1);

    public boolean isNoneAt(int var1);

    public boolean isTaggedAt(int var1);

    public boolean isConflictAt(int var1);

    public int getTagAt(int var1);

    public Object valueAt(int var1);

    public CompoundValue compoundAt(int var1);

    public CompoundValue compoundAt(int var1, int var2);

    public CompoundPack packedAt(int var1);

    public List<IAttachment> attachmentsAt(int var1, int var2);

    public Logic logicValueAt(int var1);

    public int logicStateAt(int var1);

    public boolean isEdgeAt(int var1, int var2, ILogicDetector var3);

    public boolean isEdgeAt(int var1, int var2);

    public boolean isHighAt(int var1, ILogicDetector var2);

    public boolean isHighAt(int var1);

    public boolean isLowAt(int var1, ILogicDetector var2);

    public boolean isLowAt(int var1);

    public Number numberValueAt(int var1);

    public float floatValueAt(int var1);

    public double doubleValueAt(int var1);

    public BigDecimal bigDecimalValueAt(int var1);

    public long longValueAt(int var1);

    public int intValueAt(int var1);

    public BigInteger bigIntValueAt(int var1);

    public Struct structValueAt(int var1);

    public String stringValueAt(int var1);

    public Enumeration enumValueAt(int var1);

    public byte[] bytesValueAt(int var1);

    public String formatAt(int var1, int var2);

    public int defaultFormatAt(int var1);

    public String fhexAt(int var1);

    public String fdecAt(int var1);

    public String foctAt(int var1);

    public String fbinAt(int var1);

    public String fasciiAt(int var1);

    public int groupAt(int var1);

    public int orderAt(int var1);

    public GroupedValue valuesAtGroup(int var1);

    public GroupedValue valuesAtGroup(int var1, int var2);

    public List<IAttachment> attachmentsAtGroup(int var1, int var2);

    public int indexAtGroup(int var1);

    public List<IMemberDescriptor> getMemberDescriptors();

    public List<Enumeration> getEnums(int var1);

    public IMemberDescriptor getMemberDescriptor(Object var1);

    public List<Enumeration> getMemberEnums(Object var1);

    public Enumeration getMemberEnum(Object var1, String var2);

    public Enumeration getMemberEnum(Object var1, int var2);

    public List<Object> membersWithContent(String var1);

    public SamplesStat getStat(int var1, int var2, int var3);
}

