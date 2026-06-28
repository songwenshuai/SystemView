/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.serializer;

import de.toem.impulse.cells.serializer.Serializer;
import de.toem.toolkits.pattern.element.serializer.AbstractSerializerCell;

public abstract class ReaderConfiguration
extends AbstractSerializerCell.AbstractReaderConfiguration {
    public String[][] parameters;

    public abstract boolean supports(Serializer var1);
}

