/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.wallet;

import de.toem.impulse.cells.wallet.ResourceFile;
import de.toem.impulse.cells.wallet.ResourceToolLauncher;
import de.toem.impulse.cells.wallet.Wallet;
import de.toem.toolkits.pattern.element.Cell;
import de.toem.toolkits.pattern.element.CellAnnotation;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.constant.ConstantByteArrayElement;

@CellAnnotation(type="wallet.resource.folder", dynamicChildren={"wallet.resource.folder", "wallet.resource.file", "wallet.resource.tool"})
public class ResourceFolder
extends Cell {
    public static final String TYPE = "wallet.resource.folder";
    public String description;
    public boolean enabled = true;

    public ConstantByteArrayElement createConstantElement() {
        ConstantByteArrayElement element = new ConstantByteArrayElement(((Wallet)this.getRoot()).replace(this.getName()), 8, null);
        for (ICell cell : this.getChildren()) {
            if (cell instanceof ResourceFolder) {
                element.add(((ResourceFolder)cell).createConstantElement());
                continue;
            }
            if (cell instanceof ResourceFile) {
                element.add(((ResourceFile)cell).createConstantElement());
                continue;
            }
            if (!(cell instanceof ResourceToolLauncher)) continue;
            ConstantByteArrayElement[] constantByteArrayElementArray = ((ResourceToolLauncher)cell).createConstantElements();
            int n = constantByteArrayElementArray.length;
            int n2 = 0;
            while (n2 < n) {
                ConstantByteArrayElement e = constantByteArrayElementArray[n2];
                element.add(e);
                ++n2;
            }
        }
        return element;
    }
}

