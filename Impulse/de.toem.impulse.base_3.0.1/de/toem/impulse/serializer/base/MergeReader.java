/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.serializer.base;

import de.toem.impulse.cells.record.Scope;
import de.toem.impulse.serializer.AbstractMultiElementReader;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.threading.IProgress;
import de.toem.toolkits.pattern.threading.PartialProgress;
import java.io.InputStream;

public class MergeReader
extends AbstractMultiElementReader {
    public MergeReader() {
    }

    public MergeReader(String id, InputStream in) {
        super(id, in);
    }

    @Override
    protected int isApplicable(String name, String contentType) {
        return -1;
    }

    protected boolean asRoot(String ref) {
        return false;
    }

    @Override
    protected void parse(IProgress progress, InputStream in, ICell base) throws Throwable {
        super.parse(progress, in, base);
        for (String ref : this.references) {
            ICell scope = base;
            if (!this.asRoot(ref)) {
                scope = new Scope();
                scope.setName(((IElement)this.elements.get(ref)).getName());
                base.addChild(scope);
            }
            this.read((IProgress)new PartialProgress(progress, (float)(1.0 / (double)this.references.size())), "Merge", (IElement)this.elements.get(ref), scope);
        }
    }
}

