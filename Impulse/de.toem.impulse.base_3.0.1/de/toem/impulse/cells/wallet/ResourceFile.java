/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.wallet;

import de.toem.toolkits.pattern.element.Cell;
import de.toem.toolkits.pattern.element.CellAnnotation;
import de.toem.toolkits.pattern.element.constant.ConstantByteArrayElement;

@CellAnnotation(type="wallet.resource.file", dynamicChildren={})
public class ResourceFile
extends Cell {
    public static final String TYPE = "wallet.resource.file";
    public byte[] content;
    public boolean replace;
    public boolean execute;

    public ConstantByteArrayElement createConstantElement() {
        ConstantByteArrayElement constant = new ConstantByteArrayElement(this.getName(), 2, this.content);
        return constant;
    }
}

