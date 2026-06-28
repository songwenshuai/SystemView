/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.serializer;

import de.toem.impulse.cells.serializer.ReaderConfiguration;
import de.toem.impulse.cells.serializer.Serializer;
import de.toem.toolkits.pattern.element.CellAnnotation;
import de.toem.toolkits.pattern.element.Elements;
import de.toem.toolkits.pattern.element.serializer.SerializerDescriptor;

@CellAnnotation(type="impulse.serializer.configuration.default", dynamicChildOf={"impulse.serializer"})
public class DefaultReaderConfiguration
extends ReaderConfiguration {
    public static final String TYPE = "impulse.serializer.configuration.default";

    @Override
    public boolean supports(Serializer serializer) {
        SerializerDescriptor registered = (SerializerDescriptor)Elements.serializers.get(serializer.id);
        return registered.usesDefaultConfiguration();
    }
}

