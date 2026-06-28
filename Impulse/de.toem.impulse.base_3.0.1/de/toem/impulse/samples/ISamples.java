/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples;

import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.i18n.I18n;
import de.toem.impulse.samples.IMemberDescriptor;
import de.toem.impulse.samples.ISample;
import de.toem.impulse.samples.ISamplesCharacteristic;
import de.toem.impulse.samples.ISamplesLegend;
import de.toem.impulse.samples.convert.SampleConverter;
import de.toem.impulse.samples.utils.Log;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.exploits.Markers;
import java.util.ArrayList;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public interface ISamples
extends ISample,
ISamplesCharacteristic {
    @Override
    public String getId();

    public String getName();

    public String getError();

    @Deprecated
    public String getMessage();

    @Override
    public ProcessType getProcessType();

    @Override
    public SignalType getSignalType();

    @Override
    public SignalDescriptor getSignalDescriptor();

    public String getContent();

    public int getScale();

    public int getAccuracy();

    public int getFlags();

    public int getFormat();

    @Override
    public IDomainBase getDomainBase();

    default public String getDomainClass() {
        return this.getDomainBase() != null ? this.getDomainBase().getClazz() : null;
    }

    public DomainValue getStart();

    public DomainValue getEnd();

    public DomainValue getRate();

    public long getStartUnits();

    public long getEndUnits();

    public long getRateUnits();

    public int getRelease();

    public boolean isVolatile();

    public boolean isMonotonous();

    public boolean isReleased();

    public boolean hasTag();

    @Deprecated
    public boolean hasConflict();

    public TagDomain getTagDomain();

    public ISamplesLegend getLegend();

    public Markers getMarkers();

    public Object getService(Class<?> var1);

    public Object getData();

    public Object getData(String var1);

    public void setData(Object var1);

    public void setData(String var1, Object var2);

    public static enum ProcessType {
        Unknown,
        Discrete,
        Continuous;


        public static ProcessType parse(String processType) {
            ProcessType type = null;
            if (processType != null) {
                try {
                    type = ProcessType.valueOf(processType);
                }
                catch (Throwable throwable) {}
            }
            return type != null ? type : Unknown;
        }

        public boolean equals(Signal signal) {
            return signal != null ? Utils.equals(this.toString(), signal.processType) : false;
        }

        public static boolean equals(ProcessType type, Signal signal) {
            return type != null ? type.equals(signal) : false;
        }

        public static String toString(ProcessType type) {
            return type != null ? type.toString() : null;
        }

        public static String[] getOptions(boolean includeUnknown) {
            ArrayList<String> list = new ArrayList<String>();
            ProcessType[] processTypeArray = ProcessType.values();
            int n = processTypeArray.length;
            int n2 = 0;
            while (n2 < n) {
                ProcessType type = processTypeArray[n2];
                if (includeUnknown || type != Unknown) {
                    list.add(type.toString());
                }
                ++n2;
            }
            return list.toArray(new String[list.size()]);
        }

        public static ProcessType valueOf(String string) {
            return Enum.valueOf(ProcessType.class, string);
        }
    }

    public static class SignalDescriptor
    implements Cloneable {
        String content = "default";
        int scale0;
        int scale1;
        int flags;
        int accuracy;
        int format = -1;
        public static final SignalDescriptor DEFAULT = new SignalDescriptor();
        public static final SignalDescriptor EventGantt = new SignalDescriptor("gantt", -1);
        public static final SignalDescriptor Float32 = new SignalDescriptor("default", 0, 1, -1);
        public static final SignalDescriptor Float64 = new SignalDescriptor("default", 0, 2, -1);
        public static final SignalDescriptor StructTransaction = new SignalDescriptor("transaction", -1);
        public static final SignalDescriptor StructLog = new SignalDescriptor("log", -1);
        public static final SignalDescriptor StructGantt = new SignalDescriptor("gantt", -1);
        private static final Pattern user1 = Pattern.compile("(\\w*)\\<([^\\>]*)\\>");
        private static final Pattern user2 = Pattern.compile("\\s*(\\w+)\\s*=([^\\\\,]*)\\,?");
        @Deprecated
        public static final String CONTENT_DEFAULT = "default";
        @Deprecated
        public static final String EVENT_CONTENT_GANTT = "gantt";
        @Deprecated
        public static final String STRUCT_CONTENT_TRANSACTION = "transaction";
        @Deprecated
        public static final String STRUCT_CONTENT_LOG = "log";
        @Deprecated
        public static final String STRUCT_CONTENT_GANTT = "gantt";
        @Deprecated
        public static final String STRUCT_CONTENT_CHART = "chart";
        @Deprecated
        public static final String BINARY_CONTENT_IMAGE = "image";
        @Deprecated
        public static final SignalDescriptor EventGannt = new SignalDescriptor("gantt", -1);
        @Deprecated
        public static final SignalDescriptor StructGannt = new SignalDescriptor("gantt", -1);

        public SignalDescriptor() {
        }

        public SignalDescriptor(String content, int format) {
            this(content, 0, 0, format);
            this.content = content;
        }

        public SignalDescriptor(String content, int scale0, int format) {
            this(content, scale0, 0, 0, 0, format);
        }

        public SignalDescriptor(String content, int scale0, int accuracy, int format) {
            this(content, scale0, 0, 0, accuracy, format);
        }

        public SignalDescriptor(String content, int scale0, int scale1, int flags, int accuracy, int format) {
            this.content = content != null ? content : CONTENT_DEFAULT;
            this.scale0 = scale0;
            this.scale1 = scale1;
            this.flags = flags;
            this.accuracy = accuracy;
            this.format = format;
        }

        public static final SignalDescriptor LogicWidth(int scale) {
            return new SignalDescriptor(CONTENT_DEFAULT, scale, 0, -1);
        }

        public static SignalDescriptor valueOf(Signal signal) {
            return signal != null ? SignalDescriptor.parse(signal.signalDescriptor) : DEFAULT;
        }

        public static SignalDescriptor fromMember(IMemberDescriptor member) {
            if (member != null && (!Utils.isEmpty(member.getContent()) && !CONTENT_DEFAULT.equals(member.getContent()) || member.getFormat() != -1)) {
                return new SignalDescriptor(member.getContent(), member.getFormat());
            }
            return DEFAULT;
        }

        public static SignalDescriptor parse(String signalDescriptor) {
            String[] splitted;
            SignalDescriptor descriptor = new SignalDescriptor(CONTENT_DEFAULT, -1);
            if (signalDescriptor != null && (splitted = signalDescriptor.split("/")).length >= 5) {
                descriptor.content = splitted[0];
                descriptor.scale0 = Utils.parseInt(splitted[1], 0);
                descriptor.scale1 = Utils.parseInt(splitted[2], 0);
                descriptor.flags = Utils.parseInt(splitted[3], 0);
                descriptor.accuracy = Utils.parseInt(splitted[4], 0);
                if (splitted.length >= 6) {
                    descriptor.format = Utils.parseInt(splitted[5], 0);
                }
            }
            return descriptor;
        }

        public String toString() {
            return (String.valueOf(String.valueOf(this.content)) + "/" + String.valueOf(this.scale0) + "/" + String.valueOf(this.scale1) + "/" + String.valueOf(this.flags) + "/" + String.valueOf(this.accuracy) + "/" + String.valueOf(this.format)).intern();
        }

        public static SignalDescriptor parseUser(SignalType signalType, String signalDescriptor) {
            SignalDescriptor descriptor = new SignalDescriptor(CONTENT_DEFAULT, -1);
            Matcher m1 = user1.matcher(signalDescriptor);
            if (m1.matches()) {
                descriptor.content = m1.group(1).isEmpty() ? CONTENT_DEFAULT : m1.group(1);
                Matcher m2 = user2.matcher(m1.group(2));
                while (m2.find()) {
                    String key = m2.group(1).trim();
                    String val = m2.group(2).trim();
                    switch (signalType) {
                        case Logic: {
                            if (!"bits".equals(key)) break;
                            descriptor.scale0 = Utils.parseInt(val, 0);
                            break;
                        }
                        case EventArray: 
                        case IntegerArray: 
                        case FloatArray: 
                        case TextArray: {
                            if (!"dim".equals(key)) break;
                            descriptor.scale0 = Utils.parseInt(val, 0);
                        }
                    }
                    if (!"df".equals(key)) continue;
                    descriptor.format = SampleConverter.parseFormatText(val, -1);
                }
            }
            return descriptor;
        }

        public SignalDescriptor clone() throws CloneNotSupportedException {
            return (SignalDescriptor)super.clone();
        }

        public static boolean checkUser(SignalType signalType, String signalDescriptor) {
            if (signalDescriptor == null) {
                return false;
            }
            SignalDescriptor descriptor = new SignalDescriptor(CONTENT_DEFAULT, -1);
            Matcher m1 = user1.matcher(signalDescriptor);
            if (m1.matches()) {
                descriptor.content = m1.group(1);
                String params = m1.group(2);
                Matcher m2 = user2.matcher(params);
                int pos = 0;
                boolean comma = false;
                while (m2.find()) {
                    if (pos != m2.start()) {
                        return false;
                    }
                    pos = m2.end();
                    String key = m2.group(1);
                    String val = m2.group(2);
                    comma = m2.group().endsWith(",");
                    boolean checked = false;
                    switch (signalType) {
                        case Logic: {
                            if (!"bits".equals(key) || Utils.parseInt(val, -1) <= 0) break;
                            checked = true;
                            break;
                        }
                        case EventArray: 
                        case IntegerArray: 
                        case FloatArray: 
                        case TextArray: {
                            if (!"dim".equals(key) || Utils.parseInt(val, -1) <= 0) break;
                            checked = true;
                            break;
                        }
                    }
                    if ("df".equals(key) && SampleConverter.parseFormatText(val, Integer.MIN_VALUE) >= -1) {
                        checked = true;
                    }
                    if (checked) continue;
                    return false;
                }
                return pos == params.length() && !comma;
            }
            return false;
        }

        public String toUserString(SignalType signalType) {
            StringBuilder text = new StringBuilder();
            text.append(this.content != null ? this.content : CONTENT_DEFAULT);
            text.append('<');
            boolean comma = false;
            switch (signalType) {
                case Logic: {
                    if (this.scale0 > 0) {
                        text.append("bits=" + (this.scale0 >= 1 ? this.scale0 : 1));
                        comma = true;
                    }
                    if (this.format == -1) break;
                    if (comma) {
                        text.append(',');
                    }
                    text.append("df=" + SampleConverter.getFormatLabel(this.format));
                    break;
                }
                case EventArray: 
                case IntegerArray: 
                case FloatArray: 
                case TextArray: {
                    if (this.scale0 > 0) {
                        text.append("dim=" + (this.scale0 >= 1 ? this.scale0 : 1));
                        comma = true;
                    }
                    if (this.format == -1) break;
                    if (comma) {
                        text.append(',');
                    }
                    text.append("df=" + SampleConverter.getFormatLabel(this.format));
                    break;
                }
                default: {
                    if (this.format == -1) break;
                    text.append("df=" + SampleConverter.getFormatLabel(this.format));
                }
            }
            text.append('>');
            return text.toString();
        }

        public boolean equals(Signal signal) {
            return Utils.equals(this.toString(), signal.signalDescriptor);
        }

        public static boolean equals(SignalDescriptor descr, Signal signal) {
            return descr != null ? descr.equals(signal) : false;
        }

        public boolean equals(Object obj) {
            if (!(obj instanceof SignalDescriptor)) {
                return false;
            }
            SignalDescriptor that = (SignalDescriptor)obj;
            return Utils.equals(this.content, that.content) && this.scale0 == that.scale0 && this.scale1 == that.scale1 && this.flags == that.flags && this.accuracy == that.accuracy && this.format == that.format;
        }

        public static String toString(SignalDescriptor descr) {
            return descr != null ? descr.toString() : null;
        }

        public String getContent() {
            return this.content;
        }

        public void setContent(String content) {
            this.content = content;
        }

        public int getScale() {
            if (this.scale0 <= 1) {
                return 1;
            }
            if (this.scale0 > 65536) {
                return 65536;
            }
            return this.scale0;
        }

        public boolean hasScale() {
            return this.scale0 != 0;
        }

        public void setScale(int scale) {
            this.scale0 = scale;
        }

        public int getFlags() {
            return this.flags;
        }

        public void setFlags(int flags) {
            this.flags = flags;
        }

        public int getAccuracy() {
            return this.accuracy;
        }

        public void setAccuracy(int accuracy) {
            this.accuracy = accuracy;
        }

        public int getFormat() {
            return this.format;
        }

        public void setFormat(int format) {
            this.format = format;
        }

        public boolean isLog() {
            return STRUCT_CONTENT_LOG.equals(this.content);
        }

        public boolean isTransaction() {
            return STRUCT_CONTENT_TRANSACTION.equalsIgnoreCase(this.content);
        }

        public boolean isGantt() {
            return "gantt".equalsIgnoreCase(this.content);
        }

        public boolean isImage() {
            return !Utils.isEmpty(this.content) && this.content.contains(BINARY_CONTENT_IMAGE);
        }
    }

    public static enum SignalType {
        Unknown,
        Event,
        Integer,
        Logic,
        Float,
        Text,
        Binary,
        Struct,
        EventArray,
        IntegerArray,
        FloatArray,
        TextArray;


        public static SignalType fromMember(IMemberDescriptor member) {
            if (member != null) {
                switch (member.getType()) {
                    case 0: {
                        return Unknown;
                    }
                    case 1: {
                        return Text;
                    }
                    case 2: 
                    case 7: 
                    case 8: {
                        return Event;
                    }
                    case 3: {
                        return Integer;
                    }
                    case 4: {
                        return Float;
                    }
                    case 5: {
                        return Logic;
                    }
                    case 9: {
                        return Struct;
                    }
                    case 6: {
                        return Binary;
                    }
                    case 10: {
                        return EventArray;
                    }
                    case 11: {
                        return IntegerArray;
                    }
                    case 12: {
                        return FloatArray;
                    }
                    case 13: {
                        return TextArray;
                    }
                }
            }
            return Unknown;
        }

        public static SignalType parse(String signalType) {
            SignalType type = null;
            if (signalType != null) {
                try {
                    type = SignalType.valueOf(signalType);
                }
                catch (Throwable throwable) {}
            }
            return type != null ? type : Unknown;
        }

        public boolean equals(Signal signal) {
            return signal != null ? Utils.equals(this.toString(), signal.signalType) : false;
        }

        public boolean isSimple() {
            return this == Event || this == Integer || this == Float || this == Text || this == Logic;
        }

        public boolean isArray() {
            return this == EventArray || this == IntegerArray || this == FloatArray || this == TextArray;
        }

        public boolean isArrayOrStruct() {
            return this == EventArray || this == IntegerArray || this == FloatArray || this == TextArray || this == Struct;
        }

        public static boolean equals(SignalType type, Signal signal) {
            return type != null ? type.equals(signal) : false;
        }

        public static String toString(SignalType type) {
            return type != null ? type.toString() : null;
        }

        public static String[] getOptions(boolean includeUnknown) {
            ArrayList<String> list = new ArrayList<String>();
            SignalType[] signalTypeArray = SignalType.values();
            int n = signalTypeArray.length;
            int n2 = 0;
            while (n2 < n) {
                SignalType type = signalTypeArray[n2];
                if (includeUnknown || type != Unknown) {
                    list.add(type.toString());
                }
                ++n2;
            }
            return list.toArray(new String[list.size()]);
        }

        public static SignalType valueOf(String string) {
            return Enum.valueOf(SignalType.class, string);
        }
    }

    public static enum TagDomain {
        Unknown("", I18n.General_Tagged),
        Diff("", I18n.General_Diff),
        Log(de.toem.impulse.samples.utils.Log.TAGS);

        String[] labels;

        private TagDomain() {
        }

        private TagDomain(String ... labels) {
            this.labels = labels;
        }

        public String[] getLabels() {
            return this.labels;
        }

        public String getLabel(int tag) {
            return this.labels != null && this.labels.length > tag ? this.labels[tag] : String.valueOf(I18n.General_Tag) + tag;
        }

        public static TagDomain parse(String tagDomain) {
            TagDomain domain = null;
            if (tagDomain != null) {
                try {
                    domain = TagDomain.valueOf(tagDomain);
                }
                catch (Throwable throwable) {}
            }
            return domain != null ? domain : Unknown;
        }

        public boolean equals(Signal signal) {
            return signal != null ? Utils.equals(this.toString(), signal.tagDomain) : false;
        }

        public static boolean equals(TagDomain tagDomain, Signal signal) {
            return tagDomain != null ? tagDomain.equals(signal) : false;
        }

        public static String toString(TagDomain tagDomain) {
            return tagDomain != null ? tagDomain.toString() : null;
        }

        public static TagDomain valueOf(String string) {
            return Enum.valueOf(TagDomain.class, string);
        }
    }
}

