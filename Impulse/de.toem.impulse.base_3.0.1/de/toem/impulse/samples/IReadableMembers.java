/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples;

import de.toem.impulse.samples.ILogicDetector;
import de.toem.impulse.samples.IMemberDescriptor;
import de.toem.impulse.samples.ISample;
import de.toem.impulse.values.Enumeration;
import de.toem.impulse.values.Logic;
import java.math.BigDecimal;
import java.math.BigInteger;
import java.util.List;

public interface IReadableMembers
extends ISample {
    public int noOfMembers();

    public String nameOf(int var1);

    public int idOf(int var1);

    public String pathOf(int var1);

    public String textOf(int var1);

    public int indexOf(Object var1);

    public List<Object> membersWithContent(String var1);

    public boolean hasMember(Object var1);

    public IMemberDescriptor descriptorOf(Object var1);

    public Object valueOf(Object var1);

    public Logic logicValueOf(Object var1);

    public int logicStateOf(Object var1);

    public boolean isHighOf(Object var1, ILogicDetector var2);

    public boolean isLowOf(Object var1, ILogicDetector var2);

    public boolean booleanValueOf(Object var1);

    public Number numberValueOf(Object var1);

    public float floatValueOf(Object var1);

    public double doubleValueOf(Object var1);

    public BigDecimal bigDecimalValueOf(Object var1);

    public long longValueOf(Object var1);

    public int intValueOf(Object var1);

    public BigInteger bigIntValueOf(Object var1);

    public String stringValueOf(Object var1);

    public Enumeration enumValueOf(Object var1);

    public byte[] bytesValueOf(Object var1);

    public String formatOf(Object var1, int var2);

    public int defaultFormatOf(Object var1);

    public String fhexOf(Object var1);

    public String fdecOf(Object var1);

    public String foctOf(Object var1);

    public String fbinOf(Object var1);

    public String fasciiOf(Object var1);
}

