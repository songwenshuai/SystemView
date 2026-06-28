/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.values;

import de.toem.impulse.samples.IMemberDescriptor;
import de.toem.impulse.samples.ISample;
import de.toem.impulse.samples.ISamplesLegend;
import de.toem.impulse.samples.base.PackedSamples;
import de.toem.impulse.samples.convert.SampleConverter;
import de.toem.impulse.values.Enumeration;
import de.toem.impulse.values.MemberDescriptor;
import de.toem.impulse.values.Struct;
import de.toem.toolkits.core.Utils;
import java.io.UnsupportedEncodingException;
import java.math.BigDecimal;
import java.math.BigInteger;

public class StructMember
extends MemberDescriptor
implements ISample,
Cloneable {
    ISamplesLegend legend;
    private Object value;
    private boolean valid = true;
    private Object packed;

    public StructMember(String name, int type, String content, int format) {
        super(-1, name, type, content, format);
    }

    public StructMember(StructMember parent, String name, int type, String content, int format) {
        super(-1, parent, name, type, content, format);
    }

    public StructMember(String name, int type, String content, int format, Object value) {
        super(-1, name, type, content, format);
        this.value = value;
    }

    public StructMember(int id, int type, Object value) {
        super(id, null, type, null, -1);
        this.value = value;
    }

    public StructMember(ISamplesLegend legend, byte[] bytes, int pos, int flags) {
        IMemberDescriptor descriptor;
        this.legend = legend;
        this.read(bytes, pos, flags);
        IMemberDescriptor iMemberDescriptor = descriptor = legend != null ? legend.getMember(this.id) : null;
        if (descriptor != null) {
            this.name = descriptor.getName();
            this.path = descriptor.getPath();
            this.content = descriptor.getContent();
            this.format = descriptor.getFormat();
            this.parent = descriptor.getParentId();
        }
    }

    public void assignLegend(ISamplesLegend legend) {
        block8: {
            block7: {
                if (this.legend != legend) {
                    this.legend = legend;
                    if (legend != null) {
                        this.parent = -1;
                        this.id = legend.findMatch(this);
                        if (this.id < 0) {
                            this.id = legend.addMember(this.name, this.type, this.content, this.format);
                        }
                    }
                }
                if (!(this.value instanceof Struct)) break block7;
                StructMember[] structMemberArray = ((Struct)this.value).getArray();
                int n = structMemberArray.length;
                int n2 = 0;
                while (n2 < n) {
                    StructMember member = structMemberArray[n2];
                    if (member != null && member.isValid()) {
                        member.assignLegend(legend, this);
                    }
                    ++n2;
                }
                break block8;
            }
            if (!(this.value instanceof StructMember[])) break block8;
            StructMember[] structMemberArray = (StructMember[])this.value;
            int n = structMemberArray.length;
            int n3 = 0;
            while (n3 < n) {
                StructMember member = structMemberArray[n3];
                if (member != null && member.isValid()) {
                    member.assignLegend(legend, this);
                }
                ++n3;
            }
        }
    }

    protected void assignLegend(ISamplesLegend legend, StructMember parent) {
        block8: {
            block7: {
                if (this.legend != legend) {
                    this.legend = legend;
                    if (legend != null) {
                        this.parent = parent != null ? parent.getId() : -1;
                        this.id = legend.findMatch(this);
                        if (this.id < 0) {
                            this.id = legend.addMember((Integer)this.parent, this.name, this.type, this.content, this.format);
                        }
                    }
                }
                if (!(this.value instanceof Struct)) break block7;
                StructMember[] structMemberArray = ((Struct)this.value).getArray();
                int n = structMemberArray.length;
                int n2 = 0;
                while (n2 < n) {
                    StructMember member = structMemberArray[n2];
                    if (member != null && member.isValid()) {
                        member.assignLegend(legend, this);
                    }
                    ++n2;
                }
                break block8;
            }
            if (!(this.value instanceof StructMember[])) break block8;
            StructMember[] structMemberArray = (StructMember[])this.value;
            int n = structMemberArray.length;
            int n3 = 0;
            while (n3 < n) {
                StructMember member = structMemberArray[n3];
                if (member != null && member.isValid()) {
                    member.assignLegend(legend, this);
                }
                ++n3;
            }
        }
    }

    public int getPackLength() {
        return this.packed instanceof byte[] ? ((byte[])this.packed).length : (this.packed instanceof Integer ? (Integer)this.packed : 0);
    }

    public void setValue(Object value) {
        block6: {
            block5: {
                if (Utils.equals(this.value, value)) {
                    return;
                }
                this.value = value;
                if (!(this.value instanceof Struct)) break block5;
                StructMember[] structMemberArray = ((Struct)this.value).getArray();
                int n = structMemberArray.length;
                int n2 = 0;
                while (n2 < n) {
                    StructMember member = structMemberArray[n2];
                    if (member != null) {
                        member.assignLegend(null, this);
                    }
                    ++n2;
                }
                break block6;
            }
            if (!(this.value instanceof StructMember[])) break block6;
            StructMember[] structMemberArray = (StructMember[])this.value;
            int n = structMemberArray.length;
            int n3 = 0;
            while (n3 < n) {
                StructMember member = structMemberArray[n3];
                if (member != null && member.isValid()) {
                    member.assignLegend(null, this);
                }
                ++n3;
            }
        }
    }

    public void setIntValue(int value) {
        this.setValue(value);
    }

    public void setIntArrayValue(int[] value) {
        this.setValue(value);
    }

    public void setLongValue(long value) {
        this.setValue(value);
    }

    public void setLongArrayValue(long value) {
        this.setValue(value);
    }

    public void setBigIntegerValue(BigInteger value) {
        this.setValue(value);
    }

    public void setBigIntegerArrayValue(BigInteger[] value) {
        this.setValue(value);
    }

    public void setDoubleValue(double value) {
        this.setValue(value);
    }

    public void setDoubleArrayValue(double[] value) {
        this.setValue(value);
    }

    public void setFloatValue(float value) {
        this.setValue(Float.valueOf(value));
    }

    public void setFloatArrayValue(float[] value) {
        this.setValue(value);
    }

    public void setBigDecimalValue(BigDecimal value) {
        this.setValue(value);
    }

    public void setBigDecimalArrayValue(BigDecimal[] value) {
        this.setValue(value);
    }

    public void setStringValue(String value) {
        this.setValue(value);
    }

    public void setStringArrayValue(String[] value) {
        this.setValue(value);
    }

    public void setBytesValue(byte[] value) {
        this.setValue(value);
    }

    public void setStructValue(Struct value) {
        this.setValue(value);
    }

    public void setStructMemberValue(StructMember[] value) {
        this.setValue(value);
    }

    /*
     * Unable to fully structure code
     */
    public void pack() {
        bytes = null;
        bytesLength = 0;
        abytes = null;
        abytesLength = null;
        addComponentSize = true;
        addArraySize = true;
        xdf = 0;
        baseType = (byte)(this.type & 15);
        switch (baseType) {
            case 5: {
                break;
            }
            case 4: {
                if (this.value instanceof Float) {
                    bytes = new byte[4];
                    intdata = Float.floatToIntBits(((Float)this.value).floatValue());
                    bytes[bytesLength++] = (byte)(intdata & 255);
                    bytes[bytesLength++] = (byte)((intdata >>>= 8) & 255);
                    bytes[bytesLength++] = (byte)((intdata >>>= 8) & 255);
                    bytes[bytesLength++] = (byte)((intdata >>>= 8) & 255);
                    break;
                }
                if (this.value instanceof Double) {
                    bytes = new byte[8];
                    longdata = Double.doubleToLongBits((Double)this.value);
                    bytes[bytesLength++] = (byte)(longdata & 255L);
                    bytes[bytesLength++] = (byte)((longdata >>>= 8) & 255L);
                    bytes[bytesLength++] = (byte)((longdata >>>= 8) & 255L);
                    bytes[bytesLength++] = (byte)((longdata >>>= 8) & 255L);
                    bytes[bytesLength++] = (byte)((longdata >>>= 8) & 255L);
                    bytes[bytesLength++] = (byte)((longdata >>>= 8) & 255L);
                    bytes[bytesLength++] = (byte)((longdata >>>= 8) & 255L);
                    bytes[bytesLength++] = (byte)((longdata >>>= 8) & 255L);
                    break;
                }
                if (!(this.value instanceof BigDecimal)) break;
                data = ((BigDecimal)this.value).unscaledValue().toByteArray();
                bdscale = ((BigDecimal)this.value).scale();
                if (bdscale > 32767 || bdscale < -32768) break;
                bytes = new byte[data.length + 2];
                bytes[bytesLength++] = (byte)(bdscale & 255);
                bytes[bytesLength++] = (byte)((bdscale >>= 8) & 255);
                n = data.length - 1;
                while (n >= 0) {
                    bytes[bytesLength++] = data[n];
                    --n;
                }
                xdf = 24;
                break;
            }
            case 12: {
                if (this.value instanceof float[]) {
                    abytes = new byte[((float[])this.value).length][];
                    n = 0;
                    while (n < abytes.length) {
                        bytes = new byte[4];
                        intdata = Float.floatToIntBits(((float[])this.value)[n]);
                        bytesLength = 0;
                        bytes[bytesLength++] = (byte)(intdata & 255);
                        bytes[bytesLength++] = (byte)((intdata >>>= 8) & 255);
                        bytes[bytesLength++] = (byte)((intdata >>>= 8) & 255);
                        bytes[bytesLength++] = (byte)((intdata >>>= 8) & 255);
                        abytes[n] = bytes;
                        ++n;
                    }
                    xdf = 8;
                    bytes = null;
                } else if (this.value instanceof double[]) {
                    abytes = new byte[((double[])this.value).length][];
                    n = 0;
                    while (n < abytes.length) {
                        bytes = new byte[8];
                        longdata = Double.doubleToLongBits(((double[])this.value)[n]);
                        bytesLength = 0;
                        bytes[bytesLength++] = (byte)(longdata & 255L);
                        bytes[bytesLength++] = (byte)((longdata >>>= 8) & 255L);
                        bytes[bytesLength++] = (byte)((longdata >>>= 8) & 255L);
                        bytes[bytesLength++] = (byte)((longdata >>>= 8) & 255L);
                        bytes[bytesLength++] = (byte)((longdata >>>= 8) & 255L);
                        bytes[bytesLength++] = (byte)((longdata >>>= 8) & 255L);
                        bytes[bytesLength++] = (byte)((longdata >>>= 8) & 255L);
                        bytes[bytesLength++] = (byte)((longdata >>>= 8) & 255L);
                        abytes[n] = bytes;
                        ++n;
                    }
                    xdf = 16;
                    bytes = null;
                } else if (this.value instanceof BigDecimal[]) {
                    abytes = new byte[((BigDecimal[])this.value).length][];
                    n = 0;
                    while (n < abytes.length) {
                        bigdata = ((BigDecimal[])this.value)[n];
                        if (bigdata != null) {
                            data = ((BigDecimal)this.value).unscaledValue().toByteArray();
                            bdscale = ((BigDecimal)this.value).scale();
                            if (bdscale <= 32767 && bdscale >= -32768) {
                                bytes = new byte[data.length + 2];
                                bytesLength = 0;
                                bytes[bytesLength++] = (byte)(bdscale & 255);
                                bytes[bytesLength++] = (byte)((bdscale >>= 8) & 255);
                                m = data.length - 1;
                                while (m >= 0) {
                                    bytes[bytesLength++] = data[m];
                                    --m;
                                }
                                abytes[n] = bytes;
                            }
                        }
                        ++n;
                    }
                    xdf = 24;
                    bytes = null;
                }
                addComponentSize = xdf == 24;
                break;
            }
            case 3: {
                if (this.value instanceof Boolean) {
                    this.value = Boolean.TRUE == this.value ? 1 : 0;
                }
                if (!(this.value instanceof Integer)) ** GOTO lbl132
                bytes = new byte[4];
                intdata = (Integer)this.value;
                if (intdata == 0) break;
                if (intdata <= 0) ** GOTO lbl128
                while (intdata != 0) {
                    bytes[bytesLength++] = (byte)(intdata & 255);
                    intdata >>>= 8;
                }
                if ((bytes[bytesLength - 1] & 128) == 0) break;
                bytes[bytesLength++] = 0;
                break;
lbl-1000:
                // 1 sources

                {
                    bytes[bytesLength++] = (byte)(intdata & 255);
                    intdata >>= 8;
lbl128:
                    // 2 sources

                    ** while (intdata != -1)
                }
lbl129:
                // 1 sources

                if (bytesLength != 0 && (bytes[bytesLength - 1] & 128) != 0) break;
                bytes[bytesLength++] = -1;
                break;
lbl132:
                // 1 sources

                if (!(this.value instanceof Long)) ** GOTO lbl150
                bytes = new byte[8];
                longdata = (Long)this.value;
                if (longdata == 0L) break;
                if (longdata <= 0L) ** GOTO lbl146
                while (longdata != 0L) {
                    bytes[bytesLength++] = (byte)(longdata & 255L);
                    longdata >>>= 8;
                }
                if ((bytes[bytesLength - 1] & 128) == 0) break;
                bytes[bytesLength++] = 0;
                break;
lbl-1000:
                // 1 sources

                {
                    bytes[bytesLength++] = (byte)(longdata & 255L);
                    longdata >>= 8;
lbl146:
                    // 2 sources

                    ** while (longdata != -1L)
                }
lbl147:
                // 1 sources

                if (bytesLength != 0 && (bytes[bytesLength - 1] & 128) != 0) break;
                bytes[bytesLength++] = -1;
                break;
lbl150:
                // 1 sources

                if (!(this.value instanceof BigInteger)) break;
                bytes = ((BigInteger)this.value).toByteArray();
                i = 0;
                j = bytes.length - 1;
                while (j > i) {
                    tmp = bytes[j];
                    bytes[j] = bytes[i];
                    bytes[i] = tmp;
                    --j;
                    ++i;
                }
                break;
            }
            case 11: {
                if (!(this.value instanceof int[])) ** GOTO lbl192
                abytes = new byte[((int[])this.value).length][];
                abytesLength = new int[abytes.length];
                n = 0;
                while (n < abytes.length) {
                    bytes = new byte[4];
                    bytesLength = 0;
                    intdata = ((int[])this.value)[n];
                    if (intdata == 0) ** GOTO lbl185
                    if (intdata <= 0) ** GOTO lbl182
                    while (intdata != 0) {
                        bytes[bytesLength++] = (byte)(intdata & 255);
                        intdata >>>= 8;
                    }
                    if ((bytes[bytesLength - 1] & 128) == 0) ** GOTO lbl185
                    bytes[bytesLength++] = 0;
                    ** GOTO lbl185
lbl-1000:
                    // 1 sources

                    {
                        bytes[bytesLength++] = (byte)(intdata & 255);
                        intdata >>= 8;
lbl182:
                        // 2 sources

                        ** while (intdata != -1)
                    }
lbl183:
                    // 1 sources

                    if (bytesLength == 0 || (bytes[bytesLength - 1] & 128) == 0) {
                        bytes[bytesLength++] = -1;
                    }
lbl185:
                    // 6 sources

                    abytes[n] = bytes;
                    abytesLength[n] = bytesLength;
                    ++n;
                }
                xdf = 8;
                bytes = null;
                break;
lbl192:
                // 1 sources

                if (!(this.value instanceof long[])) ** GOTO lbl221
                abytes = new byte[((long[])this.value).length][];
                abytesLength = new int[abytes.length];
                n = 0;
                while (n < abytes.length) {
                    bytes = new byte[4];
                    bytesLength = 0;
                    longdata = ((long[])this.value)[n];
                    if (longdata == 0L) ** GOTO lbl214
                    if (longdata <= 0L) ** GOTO lbl211
                    while (longdata != 0L) {
                        bytes[bytesLength++] = (byte)(longdata & 255L);
                        longdata >>>= 8;
                    }
                    if ((bytes[bytesLength - 1] & 128) == 0) ** GOTO lbl214
                    bytes[bytesLength++] = 0;
                    ** GOTO lbl214
lbl-1000:
                    // 1 sources

                    {
                        bytes[bytesLength++] = (byte)(longdata & 255L);
                        longdata >>= 8;
lbl211:
                        // 2 sources

                        ** while (longdata != -1L)
                    }
lbl212:
                    // 1 sources

                    if (bytesLength == 0 || (bytes[bytesLength - 1] & 128) == 0) {
                        bytes[bytesLength++] = -1;
                    }
lbl214:
                    // 6 sources

                    abytes[n] = bytes;
                    abytesLength[n] = bytesLength;
                    ++n;
                }
                xdf = 16;
                bytes = null;
                break;
lbl221:
                // 1 sources

                if (!(this.value instanceof BigInteger[])) break;
                abytes = new byte[((BigInteger[])this.value).length][];
                n = 0;
                while (n < abytes.length) {
                    bytes = ((BigInteger)this.value).toByteArray();
                    i = 0;
                    j = bytes.length - 1;
                    while (j > i) {
                        tmp = bytes[j];
                        bytes[j] = bytes[i];
                        bytes[i] = tmp;
                        --j;
                        ++i;
                    }
                    abytes[n] = bytes;
                    ++n;
                }
                xdf = 24;
                bytes = null;
                break;
            }
            case 1: {
                if (this.value instanceof String) {
                    try {
                        bytes = ((String)this.value).getBytes("UTF-8");
                    }
                    catch (UnsupportedEncodingException v0) {}
                    bytesLength = bytes.length;
                    break;
                }
                if (!(this.value instanceof byte[])) break;
                bytes = (byte[])this.value;
                bytesLength = bytes.length;
                break;
            }
            case 13: {
                if (!(this.value instanceof String[])) break;
                abytes = new byte[((String[])this.value).length][];
                n = 0;
                while (n < abytes.length) {
                    try {
                        abytes[n] = ((String[])this.value)[n].getBytes("UTF-8");
                    }
                    catch (UnsupportedEncodingException v1) {}
                    ++n;
                }
                break;
            }
            case 9: {
                if (!(this.value instanceof StructMember[]) && !(this.value instanceof Struct) || this.legend == null) break;
                var13_51 = structmemberdata = this.value instanceof Struct != false ? ((Struct)this.value).getArray() : (StructMember[])this.value;
                var12_49 = structmemberdata.length;
                j = 0;
                while (j < var12_49) {
                    member = var13_51[j];
                    if (member != null && member.isValid()) {
                        member.assignLegend(this.legend);
                        member.pack();
                        bytesLength += member.getPackLength();
                    }
                    ++j;
                }
                bytes = new byte[bytesLength];
                written = 0;
                var14_52 = structmemberdata;
                var13_50 = structmemberdata.length;
                var12_49 = 0;
                while (var12_49 < var13_50) {
                    member = var14_52[var12_49];
                    if (member != null && member.isValid()) {
                        written = member.write(bytes, written);
                    }
                    ++var12_49;
                }
                break;
            }
            case 6: {
                if (!(this.value instanceof byte[])) break;
                bytes = (byte[])this.value;
                bytesLength = bytes.length;
                break;
            }
            case 7: 
            case 8: {
                if (this.value instanceof String && this.legend != null) {
                    this.value = this.legend.containsEnum(8 + this.id, (String)this.value) != false ? Integer.valueOf(this.legend.valOfEnum(8 + this.id, (String)this.value)) : Integer.valueOf(this.legend.addEnum(8 + this.id, (String)this.value));
                }
                if (this.value instanceof Enumeration && this.legend != null) {
                    this.value = this.legend.containsEnum(8 + this.id, ((Enumeration)this.value).label) != false ? Integer.valueOf(this.legend.valOfEnum(8 + this.id, ((Enumeration)this.value).label)) : Integer.valueOf(this.legend.addEnum(8 + this.id, ((Enumeration)this.value).label, ((Enumeration)this.value).value));
                }
                if (!(this.value instanceof Integer)) break;
            }
            case 2: {
                if (this.value instanceof String && this.legend != null) {
                    this.value = this.legend.containsEnum(0, (String)this.value) != false ? Integer.valueOf(this.legend.valOfEnum(0, (String)this.value)) : Integer.valueOf(this.legend.addEnum(0, (String)this.value));
                }
                if (this.value instanceof Enumeration && this.legend != null) {
                    this.value = this.legend.containsEnum(0, ((Enumeration)this.value).label) != false ? Integer.valueOf(this.legend.valOfEnum(0, ((Enumeration)this.value).label)) : Integer.valueOf(this.legend.addEnum(0, ((Enumeration)this.value).label, ((Enumeration)this.value).value));
                }
                if (!(this.value instanceof Integer)) break;
                bytes = new byte[4];
                intdata = (Integer)this.value;
                if (intdata == 0) break;
                if (intdata <= 0) ** GOTO lbl320
                while (intdata != 0) {
                    bytes[bytesLength++] = (byte)(intdata & 255);
                    intdata >>>= 8;
                }
                if ((bytes[bytesLength - 1] & 128) == 0) break;
                bytes[bytesLength++] = 0;
                break;
lbl-1000:
                // 1 sources

                {
                    bytes[bytesLength++] = (byte)(intdata & 255);
                    intdata >>= 8;
lbl320:
                    // 2 sources

                    ** while (intdata != -1)
                }
lbl321:
                // 1 sources

                if (bytesLength != 0 && (bytes[bytesLength - 1] & 128) != 0) break;
                bytes[bytesLength++] = -1;
                break;
            }
            case 10: {
                if (!(this.value instanceof String[]) && !(this.value instanceof Enumeration[]) && !(this.value instanceof int[])) break;
                intarraydata = null;
                if (this.value instanceof String[] && this.legend != null) {
                    intarraydata = new int[((String[])this.value).length];
                    n = 0;
                    while (n < intarraydata.length) {
                        stringdata = ((String[])this.value)[n];
                        if (stringdata != null) {
                            intarraydata[n] = this.legend.containsEnum(8 + this.id, stringdata) != false ? this.legend.valOfEnum(8 + this.id, stringdata) : this.legend.addEnum(8 + this.id, stringdata);
                        }
                        ++n;
                    }
                } else if (this.value instanceof Enumeration[] && this.legend != null) {
                    intarraydata = new int[((Enumeration[])this.value).length];
                    n = 0;
                    while (n < intarraydata.length) {
                        enumdata = ((Enumeration[])this.value)[n];
                        if (enumdata != null) {
                            intarraydata[n] = this.legend.containsEnum(8 + this.id, enumdata.label) != false ? this.legend.valOfEnum(8 + this.id, enumdata.label) : this.legend.addEnum(8 + this.id, enumdata.label, enumdata.value);
                        }
                        ++n;
                    }
                } else if (this.value instanceof int[]) {
                    intarraydata = (int[])this.value;
                }
                if (intarraydata == null) break;
                abytes = new byte[intarraydata.length][];
                abytesLength = new int[abytes.length];
                n = 0;
                while (n < abytes.length) {
                    bytes = new byte[4];
                    bytesLength = 0;
                    intdata = intarraydata[n];
                    if (intdata == 0) ** GOTO lbl371
                    if (intdata <= 0) ** GOTO lbl368
                    while (intdata != 0) {
                        bytes[bytesLength++] = (byte)(intdata & 255);
                        intdata >>>= 8;
                    }
                    if ((bytes[bytesLength - 1] & 128) == 0) ** GOTO lbl371
                    bytes[bytesLength++] = 0;
                    ** GOTO lbl371
lbl-1000:
                    // 1 sources

                    {
                        bytes[bytesLength++] = (byte)(intdata & 255);
                        intdata >>= 8;
lbl368:
                        // 2 sources

                        ** while (intdata != -1)
                    }
lbl369:
                    // 1 sources

                    if (bytesLength == 0 || (bytes[bytesLength - 1] & 128) == 0) {
                        bytes[bytesLength++] = -1;
                    }
lbl371:
                    // 6 sources

                    abytes[n] = bytes;
                    abytesLength[n] = bytesLength;
                    ++n;
                }
                bytes = null;
            }
        }
        if (abytes != null) {
            bytesLength = addArraySize != false ? PackedSamples.plusLength(abytes.length) : 0;
            n = 0;
            while (n < abytes.length) {
                len = abytesLength != null ? abytesLength[n] : (abytes[n] != null ? abytes[n].length : 0);
                bytesLength += (addComponentSize != false ? PackedSamples.plusLength(len) : 0) + len;
                ++n;
            }
            pos = 0;
            bytes = new byte[bytesLength];
            if (addArraySize) {
                pos += PackedSamples.plusWrite(bytes, pos, abytes.length);
            }
            n = 0;
            while (n < abytes.length) {
                v2 = abytesLength != null ? abytesLength[n] : (len = abytes[n] != null ? abytes[n].length : 0);
                if (addComponentSize) {
                    pos += PackedSamples.plusWrite(bytes, pos, len);
                }
                if (len > 0) {
                    System.arraycopy(abytes[n], 0, bytes, pos, len);
                }
                pos += len;
                ++n;
            }
        }
        length = bytesLength + PackedSamples.plusLength(this.id) + 1 + PackedSamples.plusLength(bytesLength);
        pos = 0;
        buffer = new byte[length];
        pos += PackedSamples.plusWrite(buffer, 0, this.id);
        buffer[pos++] = (byte)this.type;
        pos += PackedSamples.plusWrite(buffer, pos, bytesLength);
        if (bytes != null) {
            System.arraycopy(bytes, 0, buffer, pos, bytesLength);
        }
        this.packed = buffer;
    }

    public int write(byte[] buffer, int pos) {
        if (this.packed instanceof byte[]) {
            System.arraycopy(this.packed, 0, buffer, pos, ((byte[])this.packed).length);
            return pos + ((byte[])this.packed).length;
        }
        return pos;
    }

    private int read(byte[] bytes, int pos, int flags) {
        int[] nArray = new int[2];
        nArray[1] = 1;
        int[] plusr = nArray;
        this.value = null;
        int start = pos;
        PackedSamples.plusRead(plusr, bytes, pos);
        this.id = plusr[0];
        pos += plusr[1];
        this.type = bytes[pos++] & 0xFF;
        int type = this.type & 0xF;
        PackedSamples.plusRead(plusr, bytes, pos);
        int dataLength = plusr[0];
        int endPos = (pos += plusr[1]) + plusr[0];
        try {
            switch (type) {
                case 5: {
                    break;
                }
                case 4: {
                    int xdf = (this.type & 0x30) >> 1;
                    if (xdf == 24 && dataLength > 2) {
                        int bdscale = bytes[pos++] & 0xFF | bytes[pos++] << 8;
                        byte[] buffer = new byte[dataLength - 2];
                        int index = 0;
                        int i = pos + dataLength - 1 - 2;
                        while (i >= pos) {
                            buffer[index++] = bytes[i];
                            --i;
                        }
                        BigInteger big = new BigInteger(buffer);
                        this.value = new BigDecimal(big, bdscale);
                        break;
                    }
                    if (dataLength == 4) {
                        int intdata = 0;
                        int i = pos + 3;
                        while (i >= pos) {
                            intdata = intdata << 8 | 0xFF & bytes[i];
                            --i;
                        }
                        this.value = Float.valueOf(Float.intBitsToFloat(intdata));
                        break;
                    }
                    if (dataLength == 8) {
                        long longdata = 0L;
                        int i = pos + 7;
                        while (i >= pos) {
                            longdata = longdata << 8 | (long)(0xFF & bytes[i]);
                            --i;
                        }
                        this.value = Double.longBitsToDouble(longdata);
                    }
                    break;
                }
                case 12: {
                    if (dataLength > 0) {
                        PackedSamples.plusRead(plusr, bytes, pos);
                        int scale = plusr[0];
                        pos += plusr[1];
                        int xdf = (this.type & 0x30) >> 1;
                        if (xdf == 8 && (dataLength -= plusr[1]) % 4 == 0) {
                            float[] array = new float[scale];
                            int n = 0;
                            while (n < scale && pos < endPos) {
                                int intdata = 0;
                                int i = pos + 3;
                                while (i >= pos) {
                                    intdata = intdata << 8 | 0xFF & bytes[i];
                                    --i;
                                }
                                array[n] = Float.intBitsToFloat(intdata);
                                pos += 4;
                                ++n;
                            }
                            this.value = array;
                            break;
                        }
                        if (xdf == 16 && dataLength % 8 == 0) {
                            double[] array = new double[scale];
                            int n = 0;
                            while (n < scale && pos < endPos) {
                                long longdata = 0L;
                                int i = pos + 7;
                                while (i >= pos) {
                                    longdata = longdata << 8 | (long)(0xFF & bytes[i]);
                                    --i;
                                }
                                array[n] = Double.longBitsToDouble(longdata);
                                pos += 8;
                                ++n;
                            }
                            this.value = array;
                            break;
                        }
                        if (xdf == 24) {
                            BigDecimal[] array = new BigDecimal[scale];
                            int n = 0;
                            while (n < scale && pos < endPos) {
                                PackedSamples.plusRead(plusr, bytes, pos);
                                if ((pos += plusr[1]) + plusr[0] > endPos) break;
                                int bdscale = bytes[pos++] & 0xFF | bytes[pos++] << 8;
                                byte[] buffer = new byte[plusr[0] - 2];
                                int j = 0;
                                int i = pos + plusr[0] - 1;
                                while (i >= pos) {
                                    buffer[j++] = bytes[i];
                                    --i;
                                }
                                pos += plusr[0];
                                BigInteger bi = new BigInteger(buffer);
                                array[n] = new BigDecimal(bi, bdscale);
                                ++n;
                            }
                            this.value = array;
                        }
                    }
                    break;
                }
                case 3: {
                    if (dataLength == 0) {
                        this.value = 0;
                        break;
                    }
                    if (dataLength <= 4) {
                        int value = (bytes[pos + dataLength - 1] & 0x80) != 0 ? -1 : 0;
                        int i = pos + dataLength - 1;
                        while (i >= pos) {
                            value = value << 8 | 0xFF & bytes[i];
                            --i;
                        }
                        this.value = value;
                        break;
                    }
                    if (dataLength <= 8) {
                        long value = (bytes[pos + dataLength - 1] & 0x80) != 0 ? -1L : 0L;
                        int i = pos + dataLength - 1;
                        while (i >= pos) {
                            value = value << 8 | (long)(0xFF & bytes[i]);
                            --i;
                        }
                        this.value = value;
                        break;
                    }
                    byte[] buffer = new byte[dataLength];
                    int index = 0;
                    int i = pos + dataLength - 1;
                    while (i >= pos) {
                        buffer[index++] = bytes[i];
                        --i;
                    }
                    this.value = new BigInteger(buffer);
                    break;
                }
                case 11: {
                    if (dataLength > 0) {
                        PackedSamples.plusRead(plusr, bytes, pos);
                        int scale = plusr[0];
                        pos += plusr[1];
                        dataLength -= plusr[1];
                        int xdf = (this.type & 0x30) >> 1;
                        if (xdf == 8) {
                            int[] array = new int[scale];
                            int n = 0;
                            while (n < scale && pos < endPos) {
                                PackedSamples.plusRead(plusr, bytes, pos);
                                if ((pos += plusr[1]) + plusr[0] > endPos) break;
                                int val = 0;
                                if (plusr[0] != 0 && plusr[0] <= 4) {
                                    val = (bytes[pos + plusr[0] - 1] & 0x80) != 0 ? -1 : 0;
                                    int i = pos + plusr[0] - 1;
                                    while (i >= pos) {
                                        val = val << 8 | 0xFF & bytes[i];
                                        --i;
                                    }
                                }
                                pos += plusr[0];
                                array[n] = val;
                                ++n;
                            }
                            this.value = array;
                            break;
                        }
                        if (xdf == 16) {
                            long[] array = new long[scale];
                            int n = 0;
                            while (n < scale && pos < endPos) {
                                PackedSamples.plusRead(plusr, bytes, pos);
                                if ((pos += plusr[1]) + plusr[0] > endPos) break;
                                long val = 0L;
                                if (plusr[0] != 0 && plusr[0] <= 8) {
                                    val = (bytes[pos + plusr[0] - 1] & 0x80) != 0 ? -1 : 0;
                                    int i = pos + plusr[0] - 1;
                                    while (i >= pos) {
                                        val = val << 8 | (long)(0xFF & bytes[i]);
                                        --i;
                                    }
                                }
                                pos += plusr[0];
                                array[n] = val;
                                ++n;
                            }
                            this.value = array;
                            break;
                        }
                        if (xdf == 24) {
                            BigInteger[] array = new BigInteger[scale];
                            int n = 0;
                            while (n < scale && pos < endPos) {
                                PackedSamples.plusRead(plusr, bytes, pos);
                                if ((pos += plusr[1]) + plusr[0] > endPos) break;
                                BigInteger val = null;
                                byte[] buffer = new byte[plusr[0]];
                                int index = 0;
                                int i = pos + plusr[0] - 1;
                                while (i >= pos) {
                                    buffer[index++] = bytes[i];
                                    --i;
                                }
                                val = new BigInteger(buffer);
                                pos += plusr[0];
                                array[n] = val;
                                ++n;
                            }
                            this.value = array;
                        }
                    }
                    break;
                }
                case 1: {
                    this.value = new String(bytes, pos, dataLength, "UTF-8");
                    break;
                }
                case 13: {
                    if (dataLength > 0) {
                        PackedSamples.plusRead(plusr, bytes, pos);
                        int scale = plusr[0];
                        pos += plusr[1];
                        dataLength -= plusr[1];
                        String[] array = new String[scale];
                        int n = 0;
                        while (n < scale && pos < endPos) {
                            int[] length;
                            if ((pos += (length = PackedSamples.plusRead(bytes, pos))[1]) + length[0] > endPos) break;
                            String val = new String(bytes, pos, length[0], "UTF-8");
                            pos += length[0];
                            array[n] = val;
                            ++n;
                        }
                        this.value = array;
                    }
                    break;
                }
                case 9: {
                    this.value = new Struct(bytes, pos, dataLength, this.legend, flags);
                    break;
                }
                case 6: {
                    byte[] value = new byte[dataLength];
                    System.arraycopy(bytes, pos, value, 0, dataLength);
                    this.value = value;
                    break;
                }
                case 2: 
                case 7: 
                case 8: {
                    int value = 0;
                    if (dataLength != 0 && dataLength <= 4) {
                        value = (bytes[pos + dataLength - 1] & 0x80) != 0 ? -1 : 0;
                        int i = pos + dataLength - 1;
                        while (i >= pos) {
                            value = value << 8 | 0xFF & bytes[i];
                            --i;
                        }
                    }
                    if ((flags & 1) != 0) {
                        this.value = value;
                        break;
                    }
                    Enumeration e = null;
                    if (this.legend != null) {
                        if (type == 2) {
                            e = this.legend.getEnum(0, value);
                        } else if (type == 7 || type == 8) {
                            e = this.legend.getEnum(8 + this.id, value);
                        }
                    }
                    if (e == null) {
                        e = new Enumeration(0, value);
                    }
                    this.value = e;
                    break;
                }
                case 10: {
                    if (dataLength <= 0) break;
                    PackedSamples.plusRead(plusr, bytes, pos);
                    int scale = plusr[0];
                    pos += plusr[1];
                    dataLength -= plusr[1];
                    if ((flags & 1) != 0) {
                        int[] array = new int[scale];
                        int n = 0;
                        while (n < scale && pos < endPos) {
                            int[] length;
                            if ((pos += (length = PackedSamples.plusRead(bytes, pos))[1]) + length[0] > endPos) break;
                            int value = 0;
                            if (length[0] != 0 && length[0] <= 4) {
                                value = (bytes[pos + length[0] - 1] & 0x80) != 0 ? -1 : 0;
                                int i = pos + length[0] - 1;
                                while (i >= pos) {
                                    value = value << 8 | 0xFF & bytes[i];
                                    --i;
                                }
                            }
                            pos += length[0];
                            array[n] = value;
                            ++n;
                        }
                        this.value = array;
                        break;
                    }
                    Enumeration[] array = new Enumeration[scale];
                    int n = 0;
                    while (n < scale && pos < endPos) {
                        int[] length;
                        if ((pos += (length = PackedSamples.plusRead(bytes, pos))[1]) + length[0] > endPos) break;
                        int value = 0;
                        if (length[0] != 0 && length[0] <= 4) {
                            value = (bytes[pos + length[0] - 1] & 0x80) != 0 ? -1 : 0;
                            int i = pos + length[0] - 1;
                            while (i >= pos) {
                                value = value << 8 | 0xFF & bytes[i];
                                --i;
                            }
                        }
                        pos += length[0];
                        Enumeration e = null;
                        if (this.legend != null) {
                            e = this.legend.getEnum(8 + n, value);
                        }
                        if (e == null) {
                            e = new Enumeration(0, value);
                        }
                        array[n] = e;
                        ++n;
                    }
                    this.value = array;
                }
                default: {
                    break;
                }
            }
        }
        catch (Throwable throwable) {}
        this.packed = endPos - start;
        return endPos;
    }

    public Object getValue() {
        return this.value;
    }

    public int getIntValue() {
        if (this.value instanceof Number) {
            return ((Number)this.value).intValue();
        }
        return 0;
    }

    public long getLongValue() {
        if (this.value instanceof Number) {
            return ((Number)this.value).longValue();
        }
        return 0L;
    }

    public double getDoubleValue() {
        if (this.value instanceof Number) {
            return ((Number)this.value).doubleValue();
        }
        return 0.0;
    }

    public float getFloatValue() {
        if (this.value instanceof Number) {
            return ((Number)this.value).floatValue();
        }
        return 0.0f;
    }

    public String getStringValue() {
        if (this.value instanceof String) {
            return (String)this.value;
        }
        return null;
    }

    public byte[] getBytesValue() {
        if (this.value instanceof byte[]) {
            return (byte[])this.value;
        }
        return null;
    }

    public byte[] getPack() {
        return (byte[])(this.packed instanceof byte[] ? this.packed : null);
    }

    public String toString(int format) {
        if ((format & 0xFFFF) == 65535) {
            return this.toString();
        }
        return new SampleConverter().format(this.value, format);
    }

    @Override
    public String toString() {
        if ((this.format & 0xFFFF) == 65535) {
            return new SampleConverter().format(this.value, this.defaultFormat());
        }
        return new SampleConverter().format(this.value, this.format);
    }

    public boolean isValid() {
        return this.valid;
    }

    public void setValid(boolean valid) {
        this.valid = valid;
    }

    @Override
    public boolean equals(Object obj) {
        return obj instanceof StructMember && Utils.equals(((StructMember)obj).name, this.name) && ((StructMember)obj).type == this.type;
    }

    @Override
    public StructMember clone() {
        StructMember clone = (StructMember)super.clone();
        if (this.value instanceof Struct) {
            clone.value = ((Struct)this.value).clone();
        } else if (this.value instanceof StructMember[]) {
            clone.value = new StructMember[((StructMember[])this.value).length];
            int n = 0;
            while (n < ((StructMember[])clone.value).length) {
                if (((StructMember[])this.value)[n] != null) {
                    ((StructMember[])clone.value)[n] = ((StructMember[])this.value)[n].clone();
                }
                ++n;
            }
        }
        return clone;
    }
}

