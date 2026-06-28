/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.convert;

import de.toem.impulse.samples.ILogicDetector;
import de.toem.impulse.samples.ISample;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.convert.SampleConverter;
import de.toem.impulse.values.Enumeration;
import de.toem.impulse.values.Logic;
import de.toem.impulse.values.Struct;
import java.math.BigDecimal;
import java.math.BigInteger;

public abstract class ConvertedSamples
extends SampleConverter
implements ISamples {
    protected abstract int getCount();

    protected abstract Object valueAt(int var1);

    protected abstract long unitsAt(int var1);

    protected abstract int groupAt(int var1);

    protected abstract int orderAt(int var1);

    public Logic logicValueAt(int idx) {
        return this.logicValue(this.valueAt(idx));
    }

    public int logicStateAt(int idx) {
        return this.logicState(this.valueAt(idx));
    }

    public boolean isEdgeAt(int idx, int edge) {
        return this.isEdgeAt(idx, edge, null);
    }

    public boolean isEdgeAt(int idx, int edge, ILogicDetector detector) {
        int ls = this.logicStateAt(idx);
        int lp = this.logicStateAt(idx > 0 ? idx - 1 : idx);
        if (!(edge != 1 && edge != 0 || ls != 1 && ls != 5 || lp != 0 && lp != 4)) {
            return true;
        }
        return !(edge != -1 && edge != 0 || ls != 0 && ls != 4 || lp != 1 && lp != 5);
    }

    public boolean isHighAt(int idx, ILogicDetector detector) {
        return this.isHigh(this.valueAt(idx), detector);
    }

    public boolean isLowAt(int idx, ILogicDetector detector) {
        return this.isLow(this.valueAt(idx), detector);
    }

    public boolean isHighAt(int idx) {
        return this.isHigh(this.valueAt(idx), null);
    }

    public boolean isLowAt(int idx) {
        return this.isLow(this.valueAt(idx), null);
    }

    public boolean booleanValueAt(int idx) {
        return this.booleanValue(this.valueAt(idx));
    }

    public Number numberValueAt(int idx) {
        return this.numberValue(this.valueAt(idx));
    }

    public float floatValueAt(int idx) {
        return this.floatValue(this.valueAt(idx));
    }

    public double doubleValueAt(int idx) {
        return this.doubleValue(this.valueAt(idx));
    }

    public BigDecimal bigDecimalValueAt(int idx) {
        return this.bigDecimalValue(this.valueAt(idx));
    }

    public long longValueAt(int idx) {
        return this.longValue(this.valueAt(idx));
    }

    public int intValueAt(int idx) {
        return this.intValue(this.valueAt(idx));
    }

    public BigInteger bigIntValueAt(int idx) {
        return this.bigIntValue(this.valueAt(idx));
    }

    public String stringValueAt(int idx) {
        return this.stringValue(this.valueAt(idx));
    }

    public Enumeration enumValueAt(int idx) {
        return this.enumValue(this.valueAt(idx));
    }

    public byte[] bytesValueAt(int idx) {
        return this.bytesValue(this.valueAt(idx));
    }

    public Struct structValueAt(int idx) {
        return this.structValue(this.valueAt(idx));
    }

    public String formatAt(int idx, int format) {
        if ((format & 0xFFFF) == 65535) {
            format = format & 0xFFFF0000 | this.defaultFormatAt(idx) & 0xFFFF;
        }
        if ((format & 0xFFFF0000) == -65536) {
            format = format & 0xFFFF | this.defaultFormatAt(idx) & 0xFFFF0000;
        }
        if (idx >= this.getCount() || idx < 0) {
            return null;
        }
        if ((format & 0xFFFF) == 0) {
            return null;
        }
        if ((format & 0xFFFF) == 16) {
            return String.valueOf(idx);
        }
        if ((format & 0xFFFF) == 16) {
            return String.valueOf(idx);
        }
        if ((format & 0xFFFF) == 19) {
            return this.groupAt(idx) >= 0 ? String.valueOf(this.groupAt(idx)) : null;
        }
        if ((format & 0xFFFF) == 20) {
            return this.groupAt(idx) >= 0 ? ISample.GROUP_ORDER_LABELS[this.orderAt(idx) & 7] : null;
        }
        if ((format & 0xFFFF) == 17) {
            if (idx >= this.getCount() - 1) {
                return null;
            }
            return this.getDomainBase().toString(this.unitsAt(idx + 1) - this.unitsAt(idx));
        }
        if ((format & 0xFFFF) == 18) {
            if (idx >= this.getCount() - 1) {
                return null;
            }
            Number n0 = this.numberValueAt(idx);
            Number n1 = this.numberValueAt(idx + 1);
            return n1 != null && n0 != null ? String.valueOf(n1.doubleValue() - n0.doubleValue()) : null;
        }
        return super.format(this.valueAt(idx), format);
    }

    @Override
    public String format(Object value, int format) {
        if ((format & 0xFFFF) == 65535) {
            format = format & 0xFFFF0000 | this.defaultFormatAt(-1) & 0xFFFF;
        }
        if ((format & 0xFFFF0000) == -65536) {
            format = format & 0xFFFF | this.defaultFormatAt(-1) & 0xFFFF0000;
        }
        return super.format(value, format);
    }

    public int defaultFormatAt(int idx) {
        return ConvertedSamples.getDefaultFormat(this.getSignalType(), this.getSignalDescriptor());
    }

    public String fhexAt(int idx) {
        return this.formatAt(idx, 3);
    }

    public String fdecAt(int idx) {
        return this.formatAt(idx, 5);
    }

    public String foctAt(int idx) {
        return this.formatAt(idx, 2);
    }

    public String fbinAt(int idx) {
        return this.formatAt(idx, 1);
    }

    public String fasciiAt(int idx) {
        return this.formatAt(idx, 4);
    }

    @Override
    public abstract String getName();

    @Override
    public String getLabel() {
        return this.getName();
    }

    @Override
    public String getDescription() {
        return null;
    }

    @Override
    public String getIconId() {
        return null;
    }
}

