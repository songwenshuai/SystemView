/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples;

import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.samples.ILogicDetector;
import de.toem.impulse.samples.IReadableValue;
import de.toem.impulse.values.CompoundPack;
import de.toem.impulse.values.CompoundValue;
import de.toem.impulse.values.Enumeration;
import de.toem.impulse.values.IAttachment;
import de.toem.impulse.values.Logic;
import de.toem.impulse.values.Struct;
import java.math.BigDecimal;
import java.math.BigInteger;
import java.util.List;

public interface IReadableSample
extends IReadableValue {
    @Override
    public int getIndex();

    @Override
    public long getUnits();

    @Override
    public DomainValue getPosition();

    public int getOrder();

    @Override
    public int getGroup();

    public Object val();

    public CompoundValue compound();

    public CompoundValue compound(int var1);

    public CompoundPack packed();

    @Override
    public List<IAttachment> attachments(int var1);

    public boolean isNone();

    @Deprecated
    public boolean isConflict();

    @Override
    public boolean isTagged();

    @Override
    public int getTag();

    public Logic logicValue();

    public int logicState();

    public boolean isHigh(ILogicDetector var1);

    public boolean isHigh();

    public boolean isLow(ILogicDetector var1);

    public boolean isLow();

    public Number numberValue();

    public float floatValue();

    public double doubleValue();

    public BigDecimal bigDecimalValue();

    public long longValue();

    public int intValue();

    public BigInteger bigIntValue();

    public Struct structValue();

    public String stringValue();

    public Enumeration enumValue();

    public byte[] bytesValue();

    @Override
    public String format(int var1);

    public int defaultFormat();

    public String fhex();

    public String fdec();

    public String foct();

    public String fbin();

    public String fascii();
}

