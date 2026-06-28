/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.values;

import de.toem.impulse.samples.IMemberDescriptor;
import de.toem.impulse.samples.IReadableMembers;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesLegend;
import de.toem.impulse.samples.convert.ConvertedMembers;
import de.toem.impulse.samples.convert.SampleConverter;
import de.toem.impulse.values.Enumeration;
import de.toem.impulse.values.StructMember;
import java.util.ArrayList;
import java.util.List;

public class Struct
extends ConvertedMembers
implements IReadableMembers,
Cloneable {
    StructMember[] members;

    public Struct(byte[] bytes, int pos, int dataLength, ISamplesLegend context, int flags) {
        int endPos = pos + dataLength;
        ArrayList<StructMember> members = new ArrayList<StructMember>();
        while (pos < endPos) {
            StructMember member = new StructMember(context, bytes, pos, flags);
            members.add(member);
            pos += member.getPackLength();
        }
        this.members = members.toArray(new StructMember[members.size()]);
        for (StructMember member : members) {
            int n;
            Enumeration en;
            if (member.getType() != 8 || !(member.getValue() instanceof Enumeration) || (en = (Enumeration)member.getValue()) == null || en.label.isEmpty()) continue;
            if (en.label.contains("%%")) {
                StringBuilder values = new StringBuilder();
                n = 0;
                while (n < members.size()) {
                    StructMember other = (StructMember)members.get(n);
                    if (other != member && other.isHidden()) {
                        if (values.length() > 0) {
                            values.append("; ");
                        }
                        values.append(other.getName());
                        values.append(':');
                        values.append(other.toString());
                    }
                    ++n;
                }
                en.label = en.label.replace("%%", values.toString());
                continue;
            }
            StringBuilder value = new StringBuilder(en.label);
            n = 0;
            while (n < members.size()) {
                String key;
                int idx;
                if (members.get(n) != member && (idx = value.indexOf(key = "%" + n + "%")) >= 0) {
                    String formatted = ((StructMember)members.get(n)).toString();
                    value.replace(idx, idx + key.length(), formatted != null ? formatted : "");
                }
                ++n;
            }
            en.label = value.toString();
        }
    }

    public Struct(StructMember ... members) {
        this.members = members;
    }

    public StructMember[] getArray() {
        return this.members;
    }

    public int length() {
        return this.members.length;
    }

    public StructMember getMember(int memberIndex) {
        if (this.members != null && memberIndex >= 0 && memberIndex < this.members.length) {
            return this.members[memberIndex];
        }
        return null;
    }

    public StructMember getMemberWithContent(String content) {
        if (content != null) {
            StructMember[] structMemberArray = this.members;
            int n = this.members.length;
            int n2 = 0;
            while (n2 < n) {
                StructMember m = structMemberArray[n2];
                if (content.equals(m.getContent())) {
                    return m;
                }
                ++n2;
            }
        }
        return null;
    }

    public StructMember getMember(Object memberIdentifier) {
        if (memberIdentifier instanceof String) {
            String nameOrPath = (String)memberIdentifier;
            StructMember[] structMemberArray = this.members;
            int n = this.members.length;
            int n2 = 0;
            while (n2 < n) {
                StructMember member;
                StructMember m = structMemberArray[n2];
                if (nameOrPath != null && (nameOrPath.equals(m.getName()) || nameOrPath.equals(m.getPath()))) {
                    return m;
                }
                if (m.getValue() instanceof Struct && nameOrPath.startsWith(m.getPath()) && (member = ((Struct)m.getValue()).getMember(memberIdentifier)) != null) {
                    return member;
                }
                ++n2;
            }
            return null;
        }
        if (memberIdentifier instanceof Integer || memberIdentifier instanceof IMemberDescriptor) {
            int memberId = memberIdentifier instanceof IMemberDescriptor ? ((IMemberDescriptor)memberIdentifier).getId() : ((Integer)memberIdentifier).intValue();
            StructMember[] structMemberArray = this.members;
            int n = this.members.length;
            int n3 = 0;
            while (n3 < n) {
                StructMember member;
                StructMember m = structMemberArray[n3];
                if (memberId == m.getId()) {
                    return m;
                }
                if (m.getValue() instanceof Struct && (member = ((Struct)m.getValue()).getMember(memberIdentifier)) != null) {
                    return member;
                }
                ++n3;
            }
        }
        return null;
    }

    public int getMemberIndex(Object memberIdentifier) {
        int idx = 0;
        if (memberIdentifier instanceof String) {
            String memberName = (String)memberIdentifier;
            StructMember[] structMemberArray = this.members;
            int n = this.members.length;
            int n2 = 0;
            while (n2 < n) {
                StructMember m = structMemberArray[n2];
                if (memberName != null && memberName.equals(m.getName())) {
                    return idx;
                }
                ++idx;
                ++n2;
            }
        } else if (memberIdentifier instanceof Integer || memberIdentifier instanceof IMemberDescriptor) {
            int memberId = memberIdentifier instanceof IMemberDescriptor ? ((IMemberDescriptor)memberIdentifier).getId() : ((Integer)memberIdentifier).intValue();
            StructMember[] structMemberArray = this.members;
            int n = this.members.length;
            int n3 = 0;
            while (n3 < n) {
                StructMember m = structMemberArray[n3];
                if (memberId == m.getId()) {
                    return idx;
                }
                ++n3;
            }
            ++idx;
        }
        return -1;
    }

    @Override
    public int noOfMembers() {
        return this.length();
    }

    @Override
    public String nameOf(int memberIndex) {
        StructMember m = this.getMember(memberIndex);
        return m != null ? m.getName() : null;
    }

    @Override
    public String pathOf(int memberIndex) {
        StructMember m = this.getMember(memberIndex);
        return m != null ? m.getPath() : null;
    }

    @Override
    public int idOf(int memberIndex) {
        StructMember m = this.getMember(memberIndex);
        return m != null ? m.getId() : -1;
    }

    @Override
    public String textOf(int memberIndex) {
        StructMember m = this.getMember(memberIndex);
        return m != null ? m.toString() : null;
    }

    @Override
    public boolean hasMember(Object memberIdentifier) {
        return this.getMember(memberIdentifier) != null;
    }

    @Override
    public IMemberDescriptor descriptorOf(Object memberIdentifier) {
        return this.getMember(memberIdentifier);
    }

    @Override
    public int indexOf(Object memberIdentifier) {
        return this.getMemberIndex(memberIdentifier);
    }

    @Override
    public List<Object> membersWithContent(String content) {
        ArrayList<Object> list = new ArrayList<Object>();
        if (content != null) {
            StructMember[] structMemberArray = this.members;
            int n = this.members.length;
            int n2 = 0;
            while (n2 < n) {
                StructMember m = structMemberArray[n2];
                if (m != null && (content == null || m.content != null && m.content.contains(content))) {
                    list.add(m.getId());
                }
                ++n2;
            }
        }
        return list;
    }

    @Override
    public Object valueOf(Object memberIdentifier) {
        StructMember m = this.getMember(memberIdentifier);
        return m != null ? m.getValue() : null;
    }

    @Override
    public int defaultFormatOf(Object memberIdentifier) {
        StructMember m = this.getMember(memberIdentifier);
        if (m != null) {
            if (m.getFormat() != -1) {
                return m.getFormat();
            }
            return m.defaultFormat();
        }
        return 0;
    }

    @Override
    protected Object val() {
        return this;
    }

    @Override
    protected int defaultFormat() {
        return SampleConverter.getDefaultFormat(ISamples.SignalType.Struct, ISamples.SignalDescriptor.DEFAULT);
    }

    public String toString() {
        return this.toString(131071, false);
    }

    public String toString(int format, boolean multiLine) {
        if (format == 8) {
            format = 327679;
        }
        if (format == 9) {
            format = 393215;
        }
        if ((format & 0xFFFF0000) == -65536) {
            format = 0x10000 | format & 0xFFFF;
        }
        StructMember member = null;
        if ((format & 0xFFFF) == 0) {
            return null;
        }
        if ((format & 0xFFFF0000) == 262144) {
            member = this.getMember(0);
        } else if ((format & 0xFFFF0000) == 327680) {
            member = this.getMember(1);
        } else if ((format & 0xFFFF0000) == 393216) {
            member = this.getMember(2);
        } else if ((format & 0xFFFF0000) == 458752) {
            member = this.getMember(3);
        } else {
            if ((format & 0xFFFF0000) == 0) {
                String formatted = "";
                boolean containsValue = false;
                StructMember[] structMemberArray = this.getArray();
                int n = structMemberArray.length;
                int n2 = 0;
                while (n2 < n) {
                    StructMember e = structMemberArray[n2];
                    if (!e.isHidden()) {
                        formatted = String.valueOf(formatted) + (formatted.isEmpty() ? "" : (multiLine ? "\n" : "; "));
                        String val = e.toString(format & 0xFFFF);
                        if (val != null) {
                            formatted = String.valueOf(formatted) + val;
                            containsValue = true;
                        }
                    }
                    ++n2;
                }
                if (containsValue) {
                    return formatted;
                }
                return null;
            }
            if ((format & 0xFFFF0000) == 65536) {
                String formatted = "";
                boolean containsValue = false;
                StructMember[] structMemberArray = this.getArray();
                int n = structMemberArray.length;
                int n3 = 0;
                while (n3 < n) {
                    StructMember e = structMemberArray[n3];
                    if (!e.isHidden()) {
                        formatted = String.valueOf(formatted) + (formatted.isEmpty() ? "" : (multiLine ? "\n" : "; ")) + e.getName() + ":";
                        String val = e.toString(format & 0xFFFF);
                        if (val != null) {
                            formatted = String.valueOf(formatted) + val;
                            containsValue = true;
                        }
                    }
                    ++n3;
                }
                if (containsValue) {
                    return formatted;
                }
                return null;
            }
        }
        if (member != null) {
            return member.toString(format);
        }
        return null;
    }

    public Struct clone() {
        Struct clone = new Struct(new StructMember[0]);
        if (this.members != null) {
            clone.members = new StructMember[this.members.length];
            int n = 0;
            while (n < this.members.length) {
                if (this.members[n] != null) {
                    clone.members[n] = this.members[n].clone();
                }
                ++n;
            }
        }
        return clone;
    }
}

