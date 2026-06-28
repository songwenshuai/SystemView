/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.utils;

import de.toem.impulse.samples.IStructSamplesWriter;
import de.toem.impulse.samples.utils.Log;
import de.toem.impulse.values.StructMember;
import de.toem.toolkits.core.Utils;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public final class LogWriter {
    private IStructSamplesWriter writer;
    private List<StructMember> members = new ArrayList<StructMember>();
    private List<String> memberNames = new ArrayList<String>();
    private StructMember[] memberArray;
    long current;
    int tag;

    public LogWriter(IStructSamplesWriter writer) {
        this(writer, null, null, null, null);
    }

    public LogWriter(IStructSamplesWriter writer, List<String> memberNames, Map<String, Integer> types, Map<String, String> contents, Map<String, Integer> formats) {
        this.writer = writer;
        if (memberNames == null) {
            String[] stringArray = Log.MEMBERS;
            int n = Log.MEMBERS.length;
            int n2 = 0;
            while (n2 < n) {
                String string = stringArray[n2];
                int type = Log.memberType(string);
                String content = Log.memberContent(string);
                int format = Log.memberFormat(string);
                this.members.add(new StructMember(string, type, content, format));
                ++n2;
            }
        }
        if (memberNames != null) {
            for (String string : memberNames) {
                int format;
                String content;
                int type;
                if (!Log.contains(string)) {
                    type = types != null && types.containsKey(string) ? types.get(string) : 1;
                    content = contents != null && contents.containsKey(string) ? contents.get(string) : null;
                    format = formats != null && formats.containsKey(string) ? formats.get(string) : -1;
                    this.members.add(new StructMember(string, type, content, format));
                    continue;
                }
                type = Log.memberType(string);
                content = Log.memberContent(string);
                format = Log.memberFormat(string);
                this.members.add(new StructMember(string, type, content, format));
            }
        }
        for (StructMember structMember : this.members) {
            this.memberNames.add(structMember.getName());
        }
        this.memberArray = this.members.toArray(new StructMember[this.members.size()]);
    }

    public void setTag(int tag) {
        this.tag = tag;
    }

    public int getTag() {
        return this.tag;
    }

    public void start(long units) {
        this.current = units;
        StructMember[] structMemberArray = this.memberArray;
        int n = this.memberArray.length;
        int n2 = 0;
        while (n2 < n) {
            StructMember member = structMemberArray[n2];
            member.setValid(false);
            ++n2;
        }
        this.tag = 0;
    }

    public void finish() {
        this.writer.write(this.current, this.tag, this.memberArray);
    }

    public void open(long current) {
        this.current = current;
        this.writer.open(current);
    }

    public boolean isOpen() {
        return this.writer.isOpen();
    }

    public void write(long position, int tag, Object[] values) {
        StructMember[] structMemberArray = this.memberArray;
        int n = this.memberArray.length;
        int n2 = 0;
        while (n2 < n) {
            StructMember member = structMemberArray[n2];
            member.setValid(false);
            ++n2;
        }
        this.setValues(values);
        this.writer.write(position, tag, this.memberArray);
        this.current = position;
    }

    public void setCurrent(long current) {
        this.current = current;
    }

    public Long getCurrent() {
        return this.current;
    }

    public int indexOf(String memberName) {
        return this.memberNames.lastIndexOf(memberName);
    }

    public void setValue(String memberName, String text) {
        this.setValue(this.indexOf(memberName), text);
    }

    public void setValue(int index, String text) {
        if (index < 0 || index >= this.memberArray.length) {
            return;
        }
        StructMember member = this.memberArray[index];
        Object value = text;
        switch (member.getType()) {
            case 4: {
                text = text.replace(",", ".").replace(" ", "").trim();
                value = Utils.parseDouble(text, 0.0);
                break;
            }
            case 3: {
                text = text.replace(".", "").replace(" ", "").trim();
                value = Utils.parseLong(text, 0L);
            }
        }
        member.setValue(value);
        member.setValid(true);
    }

    public void setValue(String memberName, Number number) {
        this.setValue(this.indexOf(memberName), number);
    }

    public void setValue(int index, Number number) {
        if (index < 0 || index >= this.memberArray.length) {
            return;
        }
        StructMember member = this.memberArray[index];
        Number value = null;
        switch (member.getType()) {
            case 4: {
                value = number.doubleValue();
                break;
            }
            case 3: {
                value = number.longValue();
            }
        }
        if (value != null) {
            member.setValue(value);
            member.setValid(true);
        }
    }

    public void setValues(Map<String, Object> values) {
        for (Map.Entry<String, Object> entry : values.entrySet()) {
            int idx = this.indexOf(entry.getKey());
            Object value = entry.getValue();
            if (idx < 0) continue;
            if (value instanceof String) {
                this.setValue(idx, (String)value);
                continue;
            }
            if (!(value instanceof Integer)) continue;
            this.setValue(idx, (Number)((Integer)value));
        }
    }

    public void setValues(Object[] values) {
        int n = 0;
        while (n < values.length) {
            Object value = values[n];
            if (value instanceof String) {
                this.setValue(n, (String)value);
            } else if (value instanceof Integer) {
                this.setValue(n, (Number)((Integer)value));
            }
            ++n;
        }
    }

    public void appendValue(String memberName, String value) {
        this.appendValue(this.indexOf(memberName), value);
    }

    public void appendValue(int index, String value) {
        if (index < 0 || index >= this.memberArray.length) {
            return;
        }
        StructMember member = this.memberArray[index];
        if (!member.isValid()) {
            this.setValue(index, value);
        } else if (member.getValue() instanceof String) {
            member.setValue(String.valueOf((String)member.getValue()) + value);
        }
    }
}

