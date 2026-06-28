/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.values;

import de.toem.impulse.samples.IMemberDescriptor;
import de.toem.impulse.samples.IReadableMembers;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.convert.SampleConverter;
import de.toem.impulse.values.CompoundValue;
import de.toem.impulse.values.GroupedValue;
import de.toem.impulse.values.Struct;
import de.toem.impulse.values.StructMember;
import java.util.ArrayList;
import java.util.List;

public class Transaction
extends GroupedValue
implements IReadableMembers {
    StructMember[] members;

    public Transaction(List<CompoundValue> values) {
        super(values);
        ArrayList<StructMember> members = new ArrayList<StructMember>();
        for (CompoundValue value : values) {
            Struct struct = value.structValue();
            if (struct == null) continue;
            StructMember[] structMemberArray = struct.getArray();
            int n = structMemberArray.length;
            int n2 = 0;
            while (n2 < n) {
                StructMember member = structMemberArray[n2];
                boolean inserted = false;
                int n3 = 0;
                while (n3 < members.size()) {
                    if (((StructMember)members.get(n3)).equals(member)) {
                        members.set(n3, member);
                        inserted = true;
                        break;
                    }
                    ++n3;
                }
                if (!inserted) {
                    members.add(member);
                }
                ++n2;
            }
        }
        this.members = members.toArray(new StructMember[members.size()]);
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
            String memberName = (String)memberIdentifier;
            StructMember[] structMemberArray = this.members;
            int n = this.members.length;
            int n2 = 0;
            while (n2 < n) {
                StructMember m = structMemberArray[n2];
                if (memberName != null && memberName.equals(m.getName())) {
                    return m;
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
                StructMember m = structMemberArray[n3];
                if (memberId == m.id) {
                    return m;
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
                if (memberId == m.id) {
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
                    list.add(m.id);
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
    public int defaultFormat() {
        return SampleConverter.getDefaultFormat(ISamples.SignalType.Struct, ISamples.SignalDescriptor.StructTransaction);
    }

    @Override
    public String format(int format) {
        if ((format & 0xFFFF) == 65535) {
            format = format & 0xFFFF0000 | this.defaultFormat() & 0xFFFF;
        }
        if ((format & 0xFFFF0000) == -65536) {
            format = format & 0xFFFF | this.defaultFormat() & 0xFFFF0000;
        }
        if ((format & 0xFFFF) == 19) {
            return this.getGroup() >= 0 ? String.valueOf(this.getGroup()) : null;
        }
        if ((format & 0xFFFF) == 16) {
            return this.getStartIndex() >= 0L ? String.valueOf(this.getStartIndex()) : null;
        }
        return super.format(format);
    }

    @Override
    public String formatOf(Object member, int format) {
        if ((format & 0xFFFF) == 65535) {
            format = this.defaultFormatOf(member);
        }
        if ((format & 0xFFFF) == 16) {
            return String.valueOf(this.getIndex());
        }
        if ((format & 0xFFFF) == 19) {
            return this.getGroup() >= 0 ? String.valueOf(this.getGroup()) : null;
        }
        return super.formatOf(member, format);
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
}

