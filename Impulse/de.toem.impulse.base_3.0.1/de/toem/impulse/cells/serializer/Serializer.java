/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.serializer;

import de.toem.toolkits.pattern.element.CellAnnotation;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.serializer.AbstractSerializerCell;
import de.toem.toolkits.pattern.element.serializer.SerializerDescriptor;
import de.toem.toolkits.pattern.registry.IRegistryObject;
import java.util.List;

@CellAnnotation(type="impulse.serializer")
public class Serializer
extends AbstractSerializerCell {
    public static final String TYPE = "impulse.serializer";
    public String[][] parameters;

    @Override
    public boolean init(IRegistryObject object) {
        boolean result = super.init(object);
        return result;
    }

    @Override
    public void sync(IRegistryObject object) {
        List<ICell> initalConfigurations;
        super.sync(object);
        if (object instanceof SerializerDescriptor && (initalConfigurations = ((SerializerDescriptor)object).getInitialConfigurations()) != null && !initalConfigurations.isEmpty()) {
            this.updateCells(initalConfigurations);
        }
    }
}

