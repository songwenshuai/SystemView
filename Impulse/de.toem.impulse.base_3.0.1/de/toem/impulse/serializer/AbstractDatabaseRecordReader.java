/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.serializer;

import de.toem.impulse.serializer.AbstractSingleDomainRecordReader;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.serializer.IUriStream;
import de.toem.toolkits.storage.TemporaryStorage;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.URI;

public abstract class AbstractDatabaseRecordReader
extends AbstractSingleDomainRecordReader {
    protected URI location;

    public AbstractDatabaseRecordReader() {
    }

    public AbstractDatabaseRecordReader(String id, InputStream in) {
        super(id, in);
        try {
            if (in instanceof IUriStream) {
                this.location = ((IUriStream)((Object)in)).getLocation();
            } else if (in != null) {
                OutputStream output = TemporaryStorage.getOutput(4, null, this.toString(), false);
                Utils.write(output, in);
                File file = TemporaryStorage.getFile(4, null, this.toString());
                this.location = file.toURI();
            }
        }
        catch (IOException iOException) {}
    }
}

