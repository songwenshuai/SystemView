/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.serializer;

import de.toem.impulse.serializer.IRecordReader;

public interface IParsingRecordReader
extends IRecordReader {
    public void changed(int var1, long var2);

    public void changed(int var1);

    public long current();
}

