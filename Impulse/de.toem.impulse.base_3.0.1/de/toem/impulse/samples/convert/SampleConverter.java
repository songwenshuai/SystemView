/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.convert;

import de.toem.impulse.ImpulsePreferences;
import de.toem.impulse.cells.view.PlotConfiguration;
import de.toem.impulse.samples.ILogicDetector;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISample;
import de.toem.impulse.samples.ISampleConverter;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.convert.SampleConverterConfiguration;
import de.toem.impulse.values.Enumeration;
import de.toem.impulse.values.Logic;
import de.toem.impulse.values.Struct;
import de.toem.impulse.values.StructMember;
import de.toem.impulse.values.Transaction;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.ICell;
import java.io.UnsupportedEncodingException;
import java.lang.reflect.Array;
import java.math.BigDecimal;
import java.math.BigInteger;
import java.text.DecimalFormat;
import java.util.ArrayList;

public class SampleConverter
implements ISampleConverter {
    public static SampleConverter instance = new SampleConverter();

    public static SampleConverterConfiguration createConverterConfiguration(IReadableSamples readable, PlotConfiguration configuration) {
        ISamples.SignalType signalType = readable.getSignalType();
        ISamples.SignalDescriptor signalDescriptor = readable.getSignalDescriptor();
        SampleConverterConfiguration formatterConfig = new SampleConverterConfiguration();
        if (signalType == ISamples.SignalType.Logic) {
            if (configuration.transformLinear) {
                formatterConfig.flags = (short)(formatterConfig.flags | 1);
                formatterConfig.base = configuration.transformLinearB;
                formatterConfig.factor = configuration.transformLinearM;
            }
            switch (configuration.dataInterpretation) {
                case 0: {
                    break;
                }
                case 1: {
                    formatterConfig.flags = (short)(formatterConfig.flags | 2);
                    break;
                }
                case 2: {
                    formatterConfig.flags = (short)(formatterConfig.flags | 4);
                }
            }
        } else if (signalType == ISamples.SignalType.Integer && signalDescriptor.getScale() == 1) {
            if (configuration.transformLinear) {
                formatterConfig.flags = (short)(formatterConfig.flags | 1);
                formatterConfig.base = configuration.transformLinearB;
                formatterConfig.factor = configuration.transformLinearM;
            }
        } else if (signalType == ISamples.SignalType.Float && signalDescriptor.getScale() == 1 && configuration.transformLinear) {
            formatterConfig.flags = (short)(formatterConfig.flags | 1);
            formatterConfig.base = configuration.transformLinearB;
            formatterConfig.factor = configuration.transformLinearM;
        }
        return formatterConfig.flags != 0 ? formatterConfig : null;
    }

    @Override
    public SampleConverterConfiguration getConverterConfiguration() {
        return null;
    }

    @Override
    public Logic logicValue(Object val) {
        if (val instanceof Logic) {
            return (Logic)val;
        }
        if (val instanceof String) {
            return Logic.valueOf((String)val);
        }
        if (val instanceof Boolean) {
            return Logic.valueOf((Boolean)val);
        }
        if ((val = this.numberValue(val)) instanceof Number) {
            return Logic.valueOf((Number)val);
        }
        return null;
    }

    @Override
    public int logicState(Object val) {
        if (val instanceof Logic) {
            return ((Logic)val).getState();
        }
        if (val instanceof Boolean) {
            return val == Boolean.TRUE ? 1 : 3;
        }
        if (val instanceof Number) {
            int n = ((Number)val).intValue();
            return n == 0 ? 0 : (n > 0 ? 1 : 3);
        }
        if (val instanceof String && (val = Logic.valueOf((String)val)) != null) {
            return ((Logic)val).getState();
        }
        return 15;
    }

    @Override
    public boolean isHigh(Object val, ILogicDetector detector) {
        int ls = this.logicState(val);
        return ls == 1 || ls == 5;
    }

    @Override
    public boolean isLow(Object val, ILogicDetector detector) {
        int ls = this.logicState(val);
        return ls == 0 || ls == 4;
    }

    @Override
    public boolean booleanValue(Object val) {
        int ls;
        if (val instanceof String) {
            if (Boolean.TRUE.toString().equalsIgnoreCase((String)val)) {
                return true;
            }
            if (Boolean.FALSE.toString().equalsIgnoreCase((String)val)) {
                return true;
            }
        }
        return (ls = this.logicState(val)) == 1 || ls == 5;
    }

    @Override
    public Number numberValue(Object val) {
        if (val instanceof Logic) {
            return SampleConverterConfiguration.logicToNumber((Logic)val, this.getConverterConfiguration());
        }
        if (val instanceof Number) {
            return SampleConverterConfiguration.linearize((Number)val, this.getConverterConfiguration());
        }
        if (val instanceof String) {
            return Utils.parseDouble((String)val, 0.0);
        }
        if (val instanceof Enumeration) {
            return ((Enumeration)val).value;
        }
        if (val instanceof Boolean) {
            return val == Boolean.TRUE ? 1 : 0;
        }
        if (val instanceof Struct) {
            Struct struct = (Struct)val;
            StructMember m = struct.getMemberWithContent("state");
            if (m != null) {
                return m.getIntValue();
            }
        } else if (val instanceof Enumeration[] && ((Enumeration[])val).length > 0) {
            return ((Enumeration[])val)[0].value;
        }
        return null;
    }

    @Override
    public float floatValue(Object val) {
        Number n = this.numberValue(val);
        if (n != null) {
            return n.floatValue();
        }
        return 0.0f;
    }

    @Override
    public double doubleValue(Object val) {
        Number n = this.numberValue(val);
        if (n != null) {
            return n.doubleValue();
        }
        return 0.0;
    }

    @Override
    public BigDecimal bigDecimalValue(Object val) {
        if (val instanceof BigDecimal) {
            return (BigDecimal)val;
        }
        if (val instanceof Number) {
            return new BigDecimal(((Number)val).doubleValue());
        }
        if (val instanceof String) {
            try {
                return new BigDecimal((String)val);
            }
            catch (Throwable throwable) {}
        }
        return null;
    }

    @Override
    public long longValue(Object val) {
        Number n = this.numberValue(val);
        if (n != null) {
            return n.longValue();
        }
        return 0L;
    }

    @Override
    public int intValue(Object val) {
        Number n = this.numberValue(val);
        if (n != null) {
            return n.intValue();
        }
        return 0;
    }

    @Override
    public BigInteger bigIntValue(Object val) {
        if (val instanceof BigInteger) {
            return (BigInteger)val;
        }
        if (val instanceof Number) {
            return BigInteger.valueOf(((Number)val).longValue());
        }
        if (val instanceof String) {
            try {
                return new BigInteger((String)val, 10);
            }
            catch (Throwable throwable) {}
        }
        return null;
    }

    @Override
    public String stringValue(Object val) {
        return this.stringValue(val, false);
    }

    @Override
    public String stringValue(Object val, boolean multiLine) {
        if (val instanceof String) {
            return (String)val;
        }
        if (val instanceof Logic) {
            return ((Logic)val).toString(1);
        }
        if (val instanceof Number) {
            return ((Number)val).toString();
        }
        if (val instanceof Struct) {
            return ((Struct)val).toString(-1, multiLine);
        }
        if (val instanceof StructMember) {
            return ((StructMember)val).toString(-1);
        }
        if (val instanceof byte[]) {
            byte[] bytes = (byte[])val;
            char[] hexArray = "0123456789ABCDEF".toCharArray();
            char[] hexChars = new char[bytes.length * 2 + 1];
            hexChars[0] = 120;
            int j = 0;
            while (j < bytes.length) {
                int v = bytes[j] & 0xFF;
                hexChars[j * 2 + 1] = hexArray[v >>> 4];
                hexChars[j * 2 + 2] = hexArray[v & 0xF];
                ++j;
            }
            return new String(hexChars);
        }
        if (val instanceof Enumeration) {
            return ((Enumeration)val).toString(7);
        }
        if (val != null && val.getClass().isArray()) {
            String formatted = "";
            int n = 0;
            while (n < Array.getLength(val)) {
                formatted = String.valueOf(formatted) + (formatted.isEmpty() ? "" : (multiLine ? "\n" : "; ")) + this.stringValue(Array.get(val, n), false);
                ++n;
            }
            return formatted;
        }
        return null;
    }

    @Override
    public String charValue(Object val, boolean multiChar) {
        if (val instanceof Logic) {
            val = ((Logic)val).toNumber(false);
        } else if (val instanceof Enumeration) {
            val = ((Enumeration)val).value;
        }
        if (val instanceof Integer || val instanceof Long || val instanceof BigInteger) {
            int n = ((Number)val).intValue();
            if (n > 0 && Character.isDefined((char)n)) {
                return String.valueOf((char)n);
            }
            return null;
        }
        return null;
    }

    @Override
    public Enumeration enumValue(Object val) {
        if (val instanceof Enumeration) {
            return (Enumeration)val;
        }
        if (val instanceof String) {
            return new Enumeration(0, (String)val, -1);
        }
        if (val instanceof Integer) {
            return new Enumeration(0, null, (Integer)val);
        }
        return null;
    }

    @Override
    public byte[] bytesValue(Object val) {
        if (val instanceof byte[]) {
            return (byte[])val;
        }
        if (val instanceof Double) {
            val = Double.doubleToLongBits((Double)val);
        } else if (val instanceof Float) {
            val = Float.floatToIntBits(((Float)val).floatValue());
        }
        if (val instanceof Number && !(val instanceof BigInteger)) {
            val = BigInteger.valueOf(((Number)val).longValue());
        }
        if (val instanceof BigInteger) {
            return ((BigInteger)val).toByteArray();
        }
        if (val instanceof String) {
            try {
                return ((String)val).getBytes("UTF-8");
            }
            catch (UnsupportedEncodingException unsupportedEncodingException) {}
        }
        return null;
    }

    @Override
    public Struct structValue(Object val) {
        if (val instanceof Struct) {
            return (Struct)val;
        }
        return new Struct(new StructMember("Value", 1, null, -1, this.stringValue(val)));
    }

    @Override
    public String format(Object value, int format) {
        return this.format(value, format, false);
    }

    @Override
    public String format(Object value, int format, boolean multiLine) {
        String formatted = null;
        if (value instanceof Struct) {
            formatted = ((Struct)value).toString(format, multiLine);
        } else if (value instanceof Transaction) {
            formatted = ((Transaction)value).toString(format, multiLine);
        }
        if (formatted != null) {
            return formatted;
        }
        if (format == 8) {
            format = 327679;
        }
        if (format == 9) {
            format = 393215;
        }
        if (value != null && value.getClass().isArray() && !(value instanceof byte[])) {
            switch (format & 0xFFFF0000) {
                case 262144: 
                case 327680: 
                case 393216: 
                case 458752: {
                    int idx = format - 262144 >> 16;
                    if (Array.getLength(value) > idx) {
                        return this.format(Array.get(value, idx), format, false);
                    }
                    return null;
                }
                case 65536: {
                    formatted = "";
                    boolean containsValue = false;
                    int n = 0;
                    while (n < Array.getLength(value)) {
                        formatted = String.valueOf(formatted) + (formatted.isEmpty() ? "" : (multiLine ? "\n" : "; ")) + String.valueOf(n) + ":";
                        String val = this.format(Array.get(value, n), format, false);
                        if (val != null) {
                            formatted = String.valueOf(formatted) + val;
                            containsValue = true;
                        }
                        ++n;
                    }
                    if (containsValue) {
                        return formatted;
                    }
                    return null;
                }
            }
            formatted = "";
            boolean containsValue = false;
            int n = 0;
            while (n < Array.getLength(value)) {
                formatted = String.valueOf(formatted) + (formatted.isEmpty() ? "" : (multiLine ? "\n" : "; "));
                String val = this.format(Array.get(value, n), format, false);
                if (val != null) {
                    formatted = String.valueOf(formatted) + val;
                    containsValue = true;
                }
                ++n;
            }
            if (containsValue) {
                return formatted;
            }
            return null;
        }
        switch (format & 0xFFFF) {
            case 1: {
                if (value instanceof Integer) {
                    return String.valueOf('b') + Integer.toBinaryString((Integer)value);
                }
                if (value instanceof Long) {
                    return String.valueOf('b') + Long.toBinaryString((Long)value);
                }
                if (value instanceof BigInteger) {
                    BigInteger bi = (BigInteger)value;
                    if (bi.signum() < 0) {
                        bi = new BigInteger(1, bi.toByteArray());
                    }
                    return String.valueOf('b') + bi.toString(2);
                }
                if (!((value = this.logicValue(value)) instanceof Logic)) break;
                return ((Logic)value).toString(format);
            }
            case 2: {
                if (value instanceof Integer) {
                    return String.valueOf('o') + Integer.toOctalString((Integer)value);
                }
                if (value instanceof Long) {
                    return String.valueOf('o') + Long.toOctalString((Long)value);
                }
                if (value instanceof BigInteger) {
                    BigInteger bi = (BigInteger)value;
                    if (bi.signum() < 0) {
                        bi = new BigInteger(1, bi.toByteArray());
                    }
                    return String.valueOf('o') + bi.toString(8);
                }
                if (!((value = this.logicValue(value)) instanceof Logic)) break;
                return ((Logic)value).toString(format);
            }
            case 3: {
                if (value instanceof Integer) {
                    return String.valueOf('x') + Integer.toHexString((Integer)value);
                }
                if (value instanceof Long) {
                    return String.valueOf('x') + Long.toHexString((Long)value);
                }
                if (value instanceof BigInteger) {
                    BigInteger bi = (BigInteger)value;
                    if (bi.signum() < 0) {
                        bi = new BigInteger(1, bi.toByteArray());
                    }
                    return String.valueOf('x') + bi.toString(16);
                }
                if (value instanceof byte[]) {
                    return this.stringValue(value);
                }
                if (!((value = this.logicValue(value)) instanceof Logic)) break;
                return ((Logic)value).toString(format);
            }
            case 4: {
                return this.charValue(value, false);
            }
            case 5: {
                value = this.numberValue(value);
                return value instanceof Number ? ((Number)value).toString() : null;
            }
            case 32: 
            case 33: 
            case 34: 
            case 35: 
            case 36: 
            case 37: 
            case 38: 
            case 39: {
                try {
                    value = this.numberValue(value);
                    ICell prefs = ImpulsePreferences.generalPreferences.getCell();
                    String decimalFormat = prefs.getValue("formatDecimal" + ((format & 0xFFFF) - 32) + "Format", String.class);
                    return value instanceof Number && decimalFormat != null ? new DecimalFormat(decimalFormat).format((Number)value) : null;
                }
                catch (Throwable throwable) {
                    return null;
                }
            }
            case 6: {
                if (value instanceof String) {
                    return (String)value;
                }
                if (value instanceof Enumeration) {
                    return ((Enumeration)value).toString(format);
                }
                return this.stringValue(value);
            }
            case 7: {
                value = this.enumValue(value);
                if (!(value instanceof Enumeration)) break;
                return ((Enumeration)value).toString(format);
            }
            case 10: {
                return this.booleanValue(value) ? Boolean.TRUE.toString() : Boolean.FALSE.toString();
            }
        }
        return null;
    }

    public static int getDefaultFormat(ISamples.SignalType type, ISamples.SignalDescriptor signalDescriptor) {
        int format;
        int n = format = signalDescriptor != null ? signalDescriptor.getFormat() : -1;
        if ((format & 0xFFFF) == 65535) {
            int valueFormat = 65535;
            if (type == ISamples.SignalType.Logic && signalDescriptor != null && signalDescriptor.getScale() == 1) {
                valueFormat = 1;
            } else if (type == ISamples.SignalType.Logic && signalDescriptor.getScale() > 1) {
                valueFormat = 3;
            } else if (type == ISamples.SignalType.Integer || type == ISamples.SignalType.IntegerArray) {
                valueFormat = 5;
            } else if (type == ISamples.SignalType.Float || type == ISamples.SignalType.FloatArray) {
                valueFormat = 5;
            } else if (type == ISamples.SignalType.Event || type == ISamples.SignalType.EventArray) {
                valueFormat = 7;
            } else if (type == ISamples.SignalType.Struct) {
                valueFormat = -1;
            } else if (type == ISamples.SignalType.Text || type == ISamples.SignalType.TextArray) {
                valueFormat = 6;
            } else if (type == ISamples.SignalType.Binary && signalDescriptor.isImage()) {
                valueFormat = 16;
            } else if (type == ISamples.SignalType.Binary) {
                valueFormat = 3;
            }
            format = format & 0xFFFF0000 | valueFormat;
        }
        return format;
    }

    public static Object getDefaultValue(ISamples.SignalType type, ISamples.SignalDescriptor signalDescriptor) {
        if (type == ISamples.SignalType.Logic && signalDescriptor != null && signalDescriptor.getScale() == 1) {
            return new Logic("Z");
        }
        if (type == ISamples.SignalType.Logic && signalDescriptor.getScale() > 1) {
            return new Logic("1100ZX");
        }
        if (type == ISamples.SignalType.Integer) {
            return 815;
        }
        if (type == ISamples.SignalType.IntegerArray) {
            return new int[]{815, 915};
        }
        if (type == ISamples.SignalType.Float) {
            return 8.1514;
        }
        if (type == ISamples.SignalType.FloatArray) {
            return new double[]{8.1514, 9.1524};
        }
        if (type == ISamples.SignalType.Event) {
            return new Enumeration(0, "label", 1);
        }
        if (type == ISamples.SignalType.Struct) {
            return new Struct(new StructMember("", 3, null, -1, (Object)256));
        }
        if (type == ISamples.SignalType.Text) {
            return "aText";
        }
        if (type == ISamples.SignalType.TextArray) {
            return new String[]{"aText", "another"};
        }
        if (type == ISamples.SignalType.Binary) {
            return new byte[]{12, 45, 76};
        }
        return 0;
    }

    public static String getFormatLabel(int format) {
        int idx = 0;
        format &= 0xFFFF;
        Object[] objectArray = formatValueOptions;
        int n = formatValueOptions.length;
        int n2 = 0;
        while (n2 < n) {
            Object o = objectArray[n2];
            int f = (Integer)o;
            if (f == format) {
                if (f >= 32 && f <= 39) {
                    ICell prefs = ImpulsePreferences.generalPreferences.getCell();
                    String decimalFormat = prefs.getValue("formatDecimal" + (f - 32) + "Format", String.class);
                    String decimalLabel = prefs.getValue("formatDecimal" + (f - 32) + "Label", String.class);
                    if (decimalFormat != null) {
                        return !Utils.isEmpty(decimalLabel) ? decimalLabel : formatValueLabels[idx];
                    }
                }
                return formatValueLabels[idx];
            }
            ++idx;
            ++n2;
        }
        return String.valueOf(format);
    }

    public static final Object[] formatValueOptions() {
        ArrayList<Integer> options = new ArrayList<Integer>();
        ICell prefs = ImpulsePreferences.generalPreferences.getCell();
        Object[] objectArray = formatValueOptions;
        int n = formatValueOptions.length;
        int n2 = 0;
        while (n2 < n) {
            Object o = objectArray[n2];
            int f = (Integer)o;
            if (f >= 32 && f <= 39) {
                String decimalFormat = prefs.getValue("formatDecimal" + (f - 32) + "Format", String.class);
                if (decimalFormat != null) {
                    options.add(f);
                }
            } else {
                options.add(f);
            }
            ++n2;
        }
        return options.toArray();
    }

    public static final Object[][] formatValueExampleOptions(ISamples.SignalType type, ISamples.SignalDescriptor signalDescriptor) {
        ArrayList<Integer> options = new ArrayList<Integer>();
        ArrayList<String> labels = new ArrayList<String>();
        ArrayList<String> examples = new ArrayList<String>();
        Object[] objectArray = formatValueOptions;
        int n = formatValueOptions.length;
        int n2 = 0;
        while (n2 < n) {
            String label;
            Object o = objectArray[n2];
            int f = (Integer)o;
            Object value = SampleConverter.getDefaultValue(type, signalDescriptor);
            String formatted = instance.format(value, f);
            if (formatted == null) {
                if (f == 17) {
                    formatted = "234ns";
                } else if (f == 18) {
                    formatted = "4.3456";
                } else if (f == 16) {
                    formatted = "99";
                } else if (f == 19) {
                    formatted = "3";
                } else if (f == 20) {
                    formatted = ISample.GROUP_ORDER_LABELS[1];
                }
            }
            boolean show = (label = SampleConverter.getFormatLabel(f)) != null;
            if (show &= formatted != null || f == -1 || f == 0 || type == ISamples.SignalType.Unknown) {
                labels.add(label);
                options.add(f);
                examples.add(formatted);
            }
            ++n2;
        }
        return new Object[][]{labels.toArray(new String[labels.size()]), options.toArray(new Object[options.size()]), examples.toArray(new String[examples.size()])};
    }

    public static int parseFormatText(String text, int def) {
        int idx = 0;
        int format = 0;
        boolean found = false;
        if (!Utils.isEmpty(text)) {
            String i;
            text = text.toLowerCase().replace("_", " ");
            String[] stringArray = formatValueLabels;
            int n = formatValueLabels.length;
            int n2 = 0;
            while (n2 < n) {
                i = stringArray[n2];
                if (text.contains(i.toLowerCase())) {
                    format |= ((Integer)formatValueOptions[idx]).intValue();
                    found = true;
                    break;
                }
                ++idx;
                ++n2;
            }
            idx = 0;
            stringArray = formatCollectionLabels;
            n = formatCollectionLabels.length;
            n2 = 0;
            while (n2 < n) {
                i = stringArray[n2];
                if (text.contains(i.toLowerCase())) {
                    if (format == 0) {
                        format = 65535;
                    }
                    format |= ((Integer)formatCollectionOptions[idx]).intValue();
                    found = true;
                    break;
                }
                ++idx;
                ++n2;
            }
        }
        if (found) {
            return format;
        }
        return Utils.parseInt(text, def);
    }
}

