/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.flux;

public interface IFluxInitialisation {
    public static final int FLX_OK = 0;
    public static final int FLX_ERROR = -1;
    public static final int FLX_TYPE_UNKNOWN = 0;
    public static final int FLX_TYPE_EVENT = 1;
    public static final int FLX_TYPE_INTEGER = 2;
    public static final int FLX_TYPE_LOGIC = 3;
    public static final int FLX_TYPE_FLOAT = 4;
    public static final int FLX_TYPE_TEXT = 5;
    public static final int FLX_TYPE_BINARY = 6;
    public static final int FLX_TYPE_STRUCT = 7;
    public static final int FLX_TYPE_EVENT_ARRAY = 8;
    public static final int FLX_TYPE_INTEGER_ARRAY = 9;
    public static final int FLX_TYPE_FLOAT_ARRAY = 10;
    public static final int FLX_TYPE_TEXT_ARRAY = 11;
    public static final int FLX_STRUCTTYPE_UNKNOWN = 0;
    public static final int FLX_STRUCTTYPE_TEXT = 1;
    public static final int FLX_STRUCTTYPE_ENUM = 2;
    public static final int FLX_STRUCTTYPE_INTEGER = 3;
    public static final int FLX_STRUCTTYPE_FLOAT = 4;
    public static final int FLX_STRUCTTYPE_BINARY = 6;
    public static final int FLX_STRUCTTYPE_LOCAL_ENUM = 7;
    public static final int FLX_STRUCTTYPE_MERGE_ENUM = 8;
    public static final int FLX_STRUCTTYPE_MASK_BASE = 15;
    public static final int FLX_STRUCTTYPE_MOD_HIDDEN = 128;
    public static final int FLX_ENUM_GLOBAL = 0;
    public static final int FLX_ENUM_RELATION_TARGET = 1;
    public static final int FLX_ENUM_RELATION_STYLE = 2;
    public static final int FLX_ENUM_LABEL_STYLE = 3;
    public static final int FLX_ENUM_MEMBER_0 = 8;

    public int flxAddSignal(int var1, int var2, String var3, String var4, byte var5, String var6);

    public int flxAddScatteredSignal(int var1, int var2, String var3, String var4, byte var5, String var6, int var7, int var8);

    public int flxAddSignals(int var1, int var2, int var3, String var4, String var5, byte var6, String var7);

    public int flxAddSignalReference(int var1, int var2, String var3, String var4);

    public int flxAddScatteredSignalReference(int var1, int var2, String var3, String var4, int var5, int var6);

    public int flxAddScope(int var1, int var2, String var3, String var4);

    public int flxOpen(int var1, String var2, long var3, long var5);

    public int flxClose(int var1, long var2);

    public int flxWriteEnumDef(int var1, int var2, String var3, int var4);

    public int flxWriteArrayDef(int var1, int var2, String var3);

    public int flxWriteMemberDef(int var1, int var2, String var3, byte var4, String var5);
}

