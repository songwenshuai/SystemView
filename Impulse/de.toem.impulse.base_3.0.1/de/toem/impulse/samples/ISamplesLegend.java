/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples;

import de.toem.impulse.samples.IMemberDescriptor;
import de.toem.impulse.samples.ISample;
import de.toem.impulse.values.Enumeration;
import de.toem.impulse.values.StructMember;
import java.util.List;

public interface ISamplesLegend
extends ISample {
    public int addMember(String var1, int var2, String var3, int var4);

    public int addMember(int var1, String var2, int var3, String var4, int var5);

    public boolean setMember(int var1, String var2, int var3, String var4, int var5);

    public boolean setMember(int var1, int var2, String var3, int var4, String var5, int var6);

    public List<IMemberDescriptor> getMembers();

    public IMemberDescriptor getMember(Object var1);

    public List<Object> getMemberIdentifier(String var1);

    public int findMatch(StructMember var1);

    public int addEnum(int var1, String var2);

    public int addEnum(int var1, String var2, int var3);

    public boolean setEnum(int var1, String var2, int var3);

    public List<Enumeration> getEnums(int var1);

    public List<Enumeration> getEnums(Object var1);

    public Enumeration getEnum(Object var1, int var2);

    public Enumeration getEnum(Object var1, String var2);

    public Enumeration getEnum(int var1, int var2);

    public Enumeration getEnum(int var1, String var2);

    public boolean containsEnum(int var1, String var2);

    public boolean containsEnum(int var1, int var2);

    public int valOfEnum(int var1, String var2);

    public String labelOfEnum(int var1, int var2);
}

