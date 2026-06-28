/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.ports;

import java.io.Closeable;
import java.io.InputStream;

public abstract class IClosableInputStreamProvider
implements Closeable {
    public abstract InputStream getInputStream();
}

