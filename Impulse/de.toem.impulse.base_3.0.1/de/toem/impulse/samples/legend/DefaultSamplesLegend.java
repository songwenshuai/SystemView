/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.legend;

import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.samples.IMemberDescriptor;
import de.toem.impulse.samples.base.PackedSamples;
import de.toem.impulse.samples.legend.SamplesLegend;
import de.toem.impulse.values.Enumeration;
import de.toem.impulse.values.MemberDescriptor;
import de.toem.impulse.values.StructMember;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.pageable.BytesPageable;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class DefaultSamplesLegend
extends SamplesLegend {
    private static final int BUFFER_SIZE = 4096;
    private static final int MAX_MEMBERS = 4096;
    private static final byte[] ID = "Legend".getBytes();
    private static final byte[] ID_legacy1 = "StructContext".getBytes();
    private static final byte[] ID_legacy2 = "EventContext".getBytes();
    private static final int VERSION = 4;
    List<IMemberDescriptor> members = new ArrayList<IMemberDescriptor>();
    Map<String, Integer> memberHash = new HashMap<String, Integer>();
    List<Enumeration> enums = new ArrayList<Enumeration>();

    public DefaultSamplesLegend() {
    }

    public DefaultSamplesLegend(Signal signal) {
        super(signal);
        this.read();
    }

    @Override
    public int addMember(String name, int type, String context, int format) {
        if (name == null || this.members.size() >= 4096 || type < 0 || (type & 0xF) > 13) {
            return -1;
        }
        this.modified = true;
        int id = this.members.size();
        MemberDescriptor descriptor = new MemberDescriptor(id, name, type, context, format);
        this.memberHash.put(descriptor.getPath(), id);
        this.members.add(descriptor);
        return id;
    }

    @Override
    public boolean setMember(int id, String name, int type, String context, int format) {
        if (id < 0 || id > 4096 || name == null || type < 0 || (type & 0xF) > 13) {
            return false;
        }
        this.modified = true;
        while (id >= this.members.size()) {
            this.members.add(null);
        }
        MemberDescriptor descriptor = new MemberDescriptor(id, name, type, context, format);
        this.memberHash.put(descriptor.getPath(), id);
        this.members.set(id, descriptor);
        return true;
    }

    @Override
    public int addMember(int parent, String name, int type, String context, int format) {
        if (name == null || this.members.size() >= 4096 || type < 0 || (type & 0xF) > 13 || parent >= 0 && this.getMember(parent) == null) {
            return -1;
        }
        this.modified = true;
        int id = this.members.size();
        MemberDescriptor descriptor = new MemberDescriptor(id, this.getMember(parent), name, type, context, format);
        this.memberHash.put(descriptor.getPath(), id);
        this.members.add(descriptor);
        return id;
    }

    @Override
    public boolean setMember(int id, int parent, String name, int type, String context, int format) {
        if (id < 0 || id > 4096 || name == null || type < 0 || (type & 0xF) > 13 || parent >= 0 && this.getMember(parent) == null) {
            return false;
        }
        this.modified = true;
        while (id >= this.members.size()) {
            this.members.add(null);
        }
        MemberDescriptor descriptor = new MemberDescriptor(id, this.getMember(parent), name, type, context, format);
        this.memberHash.put(descriptor.getPath(), id);
        this.members.set(id, descriptor);
        return true;
    }

    public int idOfMember(String path) {
        if (path != null && this.memberHash.containsKey(path)) {
            return this.memberHash.get(path);
        }
        return -1;
    }

    @Override
    public int findMatch(StructMember member) {
        if (member == null) {
            return -1;
        }
        int n = 0;
        while (n < this.members.size()) {
            IMemberDescriptor m = this.members.get(n);
            if (m != null && Utils.equals(member.getName(), m.getName()) && Utils.equals(member.getRawType(), m.getRawType()) && Utils.equals(member.getFormat(), m.getFormat()) && Utils.equals(member.getContent(), m.getContent()) && member.getParentId() == m.getParentId()) {
                return n;
            }
            ++n;
        }
        return -1;
    }

    private IMemberDescriptor getMember(String name) {
        int id = this.idOfMember(name);
        if (id >= 0) {
            return this.getMember(id);
        }
        return null;
    }

    private IMemberDescriptor getMember(int id) {
        if (id >= 0 && id < this.members.size()) {
            return this.members.get(id);
        }
        return null;
    }

    @Override
    public IMemberDescriptor getMember(Object memberIdentifier) {
        if (memberIdentifier instanceof String) {
            return this.getMember((String)memberIdentifier);
        }
        if (memberIdentifier instanceof Integer) {
            return this.getMember((Integer)memberIdentifier);
        }
        if (memberIdentifier instanceof Object[] && ((Object[])memberIdentifier).length == 2) {
            if (((Object[])memberIdentifier)[1] instanceof Integer) {
                return this.getMember((Integer)((Object[])memberIdentifier)[1]);
            }
            if (((Object[])memberIdentifier)[0] instanceof String) {
                return this.getMember((String)((Object[])memberIdentifier)[0]);
            }
        }
        return null;
    }

    @Override
    public List<Object> getMemberIdentifier(String content) {
        ArrayList<Object> list = new ArrayList<Object>();
        int n = 0;
        while (n < this.members.size()) {
            IMemberDescriptor m = this.members.get(n);
            if (m != null && (content == null || m.getContent() != null && m.getContent().contains(content))) {
                list.add(n);
            }
            ++n;
        }
        return list;
    }

    @Override
    public List<IMemberDescriptor> getMembers() {
        ArrayList<IMemberDescriptor> list = new ArrayList<IMemberDescriptor>();
        for (IMemberDescriptor m : this.members) {
            if (m == null) continue;
            list.add(m);
        }
        return list;
    }

    @Override
    public int addEnum(int enumeration, String label) {
        return this.addEnum(enumeration, label, 1);
    }

    @Override
    public int addEnum(int enumeration, String label, int prefered) {
        if (label == null || enumeration > Short.MAX_VALUE || enumeration < 0) {
            return -1;
        }
        boolean ok = false;
        List<Enumeration> available = this.getEnums(enumeration);
        block0: while (!ok) {
            ok = true;
            for (Enumeration e : available) {
                if (e.value != prefered) continue;
                ok = false;
                ++prefered;
                continue block0;
            }
        }
        this.enums.add(new Enumeration(enumeration, label, prefered));
        this.modified = true;
        return prefered;
    }

    @Override
    public boolean setEnum(int enumeration, String label, int value) {
        if (label == null || enumeration > Short.MAX_VALUE || enumeration < 0) {
            return false;
        }
        boolean ok = false;
        List<Enumeration> available = this.getEnums(enumeration);
        while (!ok) {
            ok = true;
            for (Enumeration e : available) {
                if (e.value != value) continue;
                e.label = label;
                this.modified = true;
                return true;
            }
        }
        this.enums.add(new Enumeration(enumeration, label, value));
        this.modified = true;
        return true;
    }

    @Override
    public List<Enumeration> getEnums(int enumeration) {
        ArrayList<Enumeration> enums = new ArrayList<Enumeration>();
        for (Enumeration e : this.enums) {
            if (e.enumeration != enumeration) continue;
            enums.add(e);
        }
        return enums;
    }

    @Override
    public List<Enumeration> getEnums(Object memberIdentifier) {
        if (memberIdentifier instanceof String) {
            int id = this.idOfMember((String)memberIdentifier);
            return id >= 0 ? this.getEnums(id + 8) : Collections.EMPTY_LIST;
        }
        if (memberIdentifier instanceof Integer) {
            int id = (Integer)memberIdentifier;
            return id >= 0 ? this.getEnums(id + 8) : Collections.EMPTY_LIST;
        }
        return Collections.EMPTY_LIST;
    }

    @Override
    public Enumeration getEnum(int enumeration, int value) {
        for (Enumeration e : this.enums) {
            if (e.enumeration != enumeration || e.value != value) continue;
            return e;
        }
        return null;
    }

    @Override
    public Enumeration getEnum(Object memberIdentifier, int value) {
        if (memberIdentifier instanceof String) {
            int id = this.idOfMember((String)memberIdentifier);
            return id >= 0 ? this.getEnum(id + 8, value) : null;
        }
        if (memberIdentifier instanceof Integer) {
            int id = (Integer)memberIdentifier;
            return id >= 0 ? this.getEnum(id + 8, value) : null;
        }
        return null;
    }

    @Override
    public Enumeration getEnum(int enumeration, String label) {
        if (label == null) {
            return null;
        }
        for (Enumeration e : this.enums) {
            if (e.enumeration != enumeration || !label.equals(e.label)) continue;
            return e;
        }
        return null;
    }

    @Override
    public Enumeration getEnum(Object memberIdentifier, String value) {
        if (memberIdentifier instanceof String) {
            int id = this.idOfMember((String)memberIdentifier);
            return id >= 0 ? this.getEnum(id + 8, value) : null;
        }
        if (memberIdentifier instanceof Integer) {
            int id = (Integer)memberIdentifier;
            return id >= 0 ? this.getEnum(id + 8, value) : null;
        }
        return null;
    }

    @Override
    public int valOfEnum(int enumeration, String value) {
        Enumeration e = this.getEnum(enumeration, value);
        return e != null ? e.value : -1;
    }

    @Override
    public String labelOfEnum(int enumeration, int value) {
        Enumeration e = this.getEnum(enumeration, value);
        return e != null ? e.label : null;
    }

    @Override
    public boolean containsEnum(int enumeration, String label) {
        return this.getEnum(enumeration, label) != null;
    }

    @Override
    public boolean containsEnum(int enumeration, int value) {
        return this.getEnum(enumeration, value) != null;
    }

    /*
     * Unable to fully structure code
     */
    @Override
    public void write() {
        this.packed = new BytesPageable(1);
        this.packed.open();
        buffer = new byte[4096];
        buffered = 0;
        try {
            System.arraycopy(DefaultSamplesLegend.ID, 0, buffer, buffered, DefaultSamplesLegend.ID.length);
            buffered += DefaultSamplesLegend.ID.length;
            buffer[buffered++] = 4;
            count = 0;
            for (IMemberDescriptor member : this.members) {
                if (member == null) continue;
                ++count;
            }
            buffered += PackedSamples.plusWrite(buffer, buffered, count);
            for (IMemberDescriptor member : this.members) {
                if (member == null) continue;
                bytes = (member.getName() != null ? member.getName() : "").getBytes("UTF-8");
                if (bytes.length + buffered + 8 >= 4096) {
                    this.packed.extend(false, buffer, 0, buffered);
                    buffered = 0;
                }
                buffered += PackedSamples.plusWrite(buffer, buffered, bytes.length);
                System.arraycopy(bytes, 0, buffer, buffered, bytes.length);
                buffered += bytes.length;
                buffer[buffered++] = (byte)member.getRawType();
                bytes = (member.getContent() != null ? member.getContent() : "").getBytes("UTF-8");
                if (bytes.length + buffered + 8 >= 4096) {
                    this.packed.extend(false, buffer, 0, buffered);
                    buffered = 0;
                }
                buffered += PackedSamples.plusWrite(buffer, buffered, bytes.length);
                System.arraycopy(bytes, 0, buffer, buffered, bytes.length);
                buffered += bytes.length;
                buffer[buffered++] = (byte)member.getFormat();
                buffered += PackedSamples.plusWrite(buffer, buffered, member.getId());
                buffered += PackedSamples.plusWrite(buffer, buffered, member.getParentId() < 0 ? 0 : member.getParentId() + 1);
            }
            count = 0;
            for (Enumeration e : this.enums) {
                if (e == null) continue;
                ++count;
            }
            buffered += PackedSamples.plusWrite(buffer, buffered, count);
            for (Enumeration e : this.enums) {
                block16: {
                    if (e == null) continue;
                    v0 = label = e.label != null ? e.label.getBytes("UTF-8") : new byte[]{};
                    if (label.length + buffered + 16 >= 4096) {
                        this.packed.extend(false, buffer, 0, buffered);
                        buffered = 0;
                    }
                    buffered += PackedSamples.plusWrite(buffer, buffered, label.length);
                    System.arraycopy(label, 0, buffer, buffered, label.length);
                    buffered += label.length;
                    buffered += PackedSamples.plusWrite(buffer, buffered, e.enumeration);
                    lenPos = buffered++;
                    dlength = 0;
                    value = e.value;
                    if (value == 0) break block16;
                    if (value <= 0) ** GOTO lbl72
                    while (value != 0) {
                        buffer[buffered++] = (byte)(value & 255);
                        ++dlength;
                        value >>>= 8;
                    }
                    if ((buffer[buffered - 1] & 128) == 0) break block16;
                    buffer[buffered++] = 0;
                    ++dlength;
                    break block16;
lbl-1000:
                    // 1 sources

                    {
                        buffer[buffered++] = (byte)(value & 255);
                        ++dlength;
                        value >>= 8;
lbl72:
                        // 2 sources

                        ** while (value != -1)
                    }
lbl73:
                    // 1 sources

                    if ((buffer[buffered - 1] & 128) == 0) {
                        buffer[buffered++] = -1;
                        ++dlength;
                    }
                }
                buffer[lenPos] = (byte)dlength;
            }
            if (buffered > 0) {
                this.packed.extend(false, buffer, 0, buffered);
            }
            this.packed.close();
        }
        catch (UnsupportedEncodingException v1) {}
    }

    private boolean compare(byte[] ref, byte[] buffer, int pos) {
        int n = 0;
        while (n < ref.length) {
            if (ref[n] != buffer[n + pos]) {
                return false;
            }
            ++n;
        }
        return true;
    }

    @Override
    public void read() {
        if (this.packed == null) {
            return;
        }
        byte[] bytes = (byte[])this.packed.getFirst();
        if (bytes == null) {
            return;
        }
        int pos = 0;
        try {
            int[] length;
            int n;
            int[] count;
            byte version = 1;
            boolean readMembers = true;
            if (bytes.length > ID.length + 1 && this.compare(ID, bytes, 0)) {
                version = bytes[ID.length];
                pos = ID.length + 1;
            } else if (bytes.length > ID_legacy1.length + 1 && this.compare(ID_legacy1, bytes, 0)) {
                version = bytes[ID_legacy1.length];
                pos = ID_legacy1.length + 1;
            } else if (bytes.length > ID_legacy2.length + 1 && this.compare(ID_legacy2, bytes, 0)) {
                version = bytes[ID_legacy2.length];
                pos = ID_legacy2.length + 1;
                readMembers = false;
            } else {
                return;
            }
            if (readMembers) {
                count = PackedSamples.plusRead(bytes, pos);
                pos += count[1];
                n = 0;
                while (n < count[0]) {
                    length = PackedSamples.plusRead(bytes, pos);
                    String name = new String(bytes, pos += length[1], length[0], "UTF-8");
                    pos += length[0];
                    if (version >= 2) {
                        int type = bytes[pos++] & 0xFF;
                        length = PackedSamples.plusRead(bytes, pos);
                        String content = new String(bytes, pos += length[1], length[0], "UTF-8");
                        pos += length[0];
                        byte format = bytes[pos++];
                        if (version >= 4) {
                            int[] id = PackedSamples.plusRead(bytes, pos);
                            int[] parent = PackedSamples.plusRead(bytes, pos += id[1]);
                            pos += parent[1];
                            parent[0] = parent[0] > 0 ? parent[0] - 1 : -1;
                            this.setMember(id[0], parent[0], name, type, content, format);
                        } else {
                            this.addMember(name, type, content, format);
                        }
                    } else {
                        this.addMember(name, 0, null, 0);
                    }
                    ++n;
                }
            }
            count = PackedSamples.plusRead(bytes, pos);
            pos += count[1];
            n = 0;
            while (n < count[0]) {
                length = PackedSamples.plusRead(bytes, pos);
                String label = new String(bytes, pos += length[1], length[0], "UTF-8");
                pos += length[0];
                if (version >= 3) {
                    length = PackedSamples.plusRead(bytes, pos);
                    int enumeration = length[0];
                    pos += length[1];
                    length = PackedSamples.plusRead(bytes, pos);
                    pos += length[1];
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
                    this.addEnum(enumeration, label, value);
                } else {
                    this.addEnum(0, label, n);
                }
                ++n;
            }
        }
        catch (UnsupportedEncodingException unsupportedEncodingException) {}
        this.modified = false;
    }
}

