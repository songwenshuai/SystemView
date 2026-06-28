/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples;

import de.toem.impulse.samples.ILogicDetector;
import de.toem.impulse.samples.ISample;
import de.toem.impulse.samples.convert.SampleConverterConfiguration;
import de.toem.impulse.values.Enumeration;
import de.toem.impulse.values.Logic;
import de.toem.impulse.values.Struct;
import java.math.BigDecimal;
import java.math.BigInteger;

public interface ISampleConverter
extends ISample {
    public SampleConverterConfiguration getConverterConfiguration();

    public Logic logicValue(Object var1);

    public int logicState(Object var1);

    public boolean isHigh(Object var1, ILogicDetector var2);

    public boolean isLow(Object var1, ILogicDetector var2);

    public boolean booleanValue(Object var1);

    public Number numberValue(Object var1);

    public float floatValue(Object var1);

    public double doubleValue(Object var1);

    public BigDecimal bigDecimalValue(Object var1);

    public long longValue(Object var1);

    public int intValue(Object var1);

    public BigInteger bigIntValue(Object var1);

    public String stringValue(Object var1);

    public String stringValue(Object var1, boolean var2);

    public String charValue(Object var1, boolean var2);

    public Enumeration enumValue(Object var1);

    public byte[] bytesValue(Object var1);

    public Struct structValue(Object var1);

    public String format(Object var1, int var2);

    public String format(Object var1, int var2, boolean var3);
}

