/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.values;

import de.toem.impulse.samples.IMemberDescriptor;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.convert.ConvertedSamples;
import de.toem.toolkits.core.Utils;

public class MemberDescriptor
implements IMemberDescriptor,
Cloneable {
    protected int id;
    protected Object parent;
    protected String name;
    protected String path;
    protected int type;
    protected String content;
    protected int format;

    public MemberDescriptor() {
    }

    public MemberDescriptor(int id, String name, int type, String content, int format) {
        this.id = id;
        this.parent = null;
        this.path = this.name = name;
        this.type = type;
        this.content = content != null ? content.toLowerCase() : null;
        this.format = format;
    }

    public MemberDescriptor(int id, IMemberDescriptor parent, String name, int type, String content, int format) {
        this.id = id;
        this.name = name;
        this.parent = parent;
        this.path = parent != null ? String.valueOf(parent.getPath()) + '.' + name : name;
        this.type = type;
        this.content = content != null ? content.toLowerCase() : null;
        this.format = format;
    }

    public boolean equals(Object obj) {
        if (obj instanceof MemberDescriptor) {
            MemberDescriptor that = (MemberDescriptor)obj;
            return that.type == this.type && that.format == this.format && Utils.equals(that.name, this.name) && Utils.equals(that.content, this.content) && that.getParentId() == this.getParentId();
        }
        return false;
    }

    @Override
    public int getId() {
        return this.id;
    }

    @Override
    public int getParentId() {
        return this.parent instanceof Integer ? (Integer)this.parent : (this.parent instanceof IMemberDescriptor ? ((IMemberDescriptor)this.parent).getId() : -1);
    }

    @Override
    public String getName() {
        return this.name;
    }

    @Override
    public String getPath() {
        return this.path;
    }

    @Override
    public String getContent() {
        return this.content;
    }

    @Override
    public int getFormat() {
        return this.format;
    }

    @Override
    public int defaultFormat() {
        return ConvertedSamples.getDefaultFormat(ISamples.SignalType.fromMember(this), ISamples.SignalDescriptor.fromMember(this));
    }

    @Override
    public int getType() {
        return this.type & 0xF;
    }

    public void adjustType(int type) {
        if (this.type == 0) {
            this.type = type;
        }
    }

    @Override
    public int getRawType() {
        return this.type;
    }

    public void setHidden(boolean hidden) {
        this.type = hidden ? (this.type |= 0x80) : (this.type &= 0xFFFFFF7F);
    }

    @Override
    public boolean isHidden() {
        return (this.type & 0x80) != 0;
    }

    @Override
    public boolean isValidUntilChange() {
        return (this.type & 0x40) != 0;
    }

    public String toString() {
        return String.valueOf(this.getPath());
    }

    public MemberDescriptor clone() {
        try {
            return (MemberDescriptor)super.clone();
        }
        catch (CloneNotSupportedException cloneNotSupportedException) {
            return null;
        }
    }
}

