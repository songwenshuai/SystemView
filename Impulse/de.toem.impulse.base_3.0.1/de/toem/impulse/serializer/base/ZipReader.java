/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.serializer.base;

import de.toem.impulse.serializer.AbstractMultiInputReader;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.threading.IProgress;
import java.io.InputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

public class ZipReader
extends AbstractMultiInputReader {
    public ZipReader() {
    }

    public ZipReader(String id, InputStream in) {
        super(id, in);
    }

    @Override
    protected int isApplicable(String name, String contentType) {
        return 1;
    }

    @Override
    protected void parse(IProgress progress, InputStream in, ICell base) throws Throwable {
        ZipInputStream zip = new ZipInputStream(in);
        ZipEntry entry = zip.getNextEntry();
        while (entry != null) {
            entry.isDirectory();
            entry = zip.getNextEntry();
        }
        zip.closeEntry();
        zip.close();
    }
}

