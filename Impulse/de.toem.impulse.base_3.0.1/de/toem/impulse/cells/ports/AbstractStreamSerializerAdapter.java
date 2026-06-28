/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.ports;

import de.toem.impulse.ImpulseBase;
import de.toem.impulse.cells.ports.AbstractSyncableAdapter;
import de.toem.impulse.cells.ports.IClosableInputStreamProvider;
import de.toem.impulse.i18n.I18n;
import de.toem.toolkits.pattern.element.Elements;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.Link;
import de.toem.toolkits.pattern.element.serializer.ICellReader;
import de.toem.toolkits.pattern.element.serializer.SerializerDescriptor;
import java.io.Closeable;
import java.io.InputStream;

public abstract class AbstractStreamSerializerAdapter
extends AbstractSyncableAdapter {
    public String serializer;
    public Link configuration;

    @Override
    public boolean validate(ICell insertPoint) {
        SerializerDescriptor descr = (SerializerDescriptor)Elements.serializers.get(this.serializer);
        if (descr != null && descr.hasReader()) {
            return true;
        }
        ImpulseBase.defaultConsoleStream().println(String.valueOf(new String(this.name)) + ": " + I18n.Adapter_NoReaderSelected);
        return false;
    }

    @Override
    public ICellReader newReader(Closeable input) {
        SerializerDescriptor descr = (SerializerDescriptor)Elements.serializers.get(this.serializer);
        if (input instanceof IClosableInputStreamProvider) {
            input = ((IClosableInputStreamProvider)input).getInputStream();
        }
        if (input instanceof InputStream && descr != null && descr.hasReader()) {
            ICellReader reader = descr.newReader((InputStream)input);
            return reader;
        }
        return null;
    }

    @Override
    public int getNature() {
        SerializerDescriptor descr = (SerializerDescriptor)Elements.serializers.get(this.serializer);
        int nature = 0;
        if (descr != null) {
            if (descr.supports(4096, null)) {
                nature |= 0x23;
            }
            if (descr.supports(8192, null)) {
                nature |= 0x20;
            }
        }
        return nature;
    }

    @Override
    public Link getConfiguration() {
        return this.configuration;
    }
}

