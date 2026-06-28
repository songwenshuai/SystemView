/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.disclosures;

import de.toem.impulse.ImpulseLicense;
import de.toem.impulse.cells.record.Record;
import de.toem.impulse.cells.record.Scope;
import de.toem.impulse.provider.ISamplesCache;
import de.toem.impulse.provider.ISamplesContext;
import de.toem.impulse.provider.ISamplesProvider;
import de.toem.impulse.provider.ISignalProvider;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.element.disclosure.AbstractDisclosure;
import de.toem.toolkits.pattern.provider.IContext;
import java.util.ArrayList;
import java.util.List;

public abstract class AbstractReadableDisclosure
extends AbstractDisclosure {
    @Override
    public Object disclose(IElement element, int format, Object properties, IContext context) {
        Object data;
        if (ImpulseLicense.isFeatureLocked("de.toem.impulse.featureDomain.disclosures", "de.toem.impulse.feature.default", this.getId())) {
            return null;
        }
        if (element.hasCell(new Class[]{Record.class, Scope.class, ISignalProvider.class}) && (data = this.discloseCell(element.getCell(), properties, context)) != null) {
            return this.format(data, format);
        }
        return null;
    }

    private Object discloseCell(ICell cell, Object properties, IContext context) {
        Object data = null;
        if (cell instanceof ISamplesProvider) {
            IReadableSamples readable = ((ISamplesProvider)((Object)cell)).getSamples(context);
            if (readable != null && (data = readable.getData(this.getId())) == null) {
                data = this.extractData(cell, readable, properties);
                readable.setData(this.getId(), data);
            }
        } else if (this.combineReadables()) {
            ISamplesCache cached = ((ISamplesContext)context).getSamplesCache();
            if (context instanceof ISamplesContext && cached != null) {
                data = cached.get(cell, this.getId());
            }
            if (data == null) {
                data = this.combineData(cell, this.discloseChildren(cell, properties, context), properties);
                if (cached != null && data != null) {
                    cached.put(cell, this.getId(), data);
                }
            }
        }
        return data;
    }

    private List<Object> discloseChildren(ICell parent, Object properties, IContext context) {
        ArrayList<Object> list = new ArrayList<Object>();
        for (ICell cell : parent.getChildren()) {
            if (!(cell instanceof ISamplesProvider)) continue;
            list.add(this.discloseCell(cell, properties, context));
        }
        return list;
    }

    protected abstract Object extractData(ICell var1, IReadableSamples var2, Object var3);

    protected abstract boolean combineReadables();

    protected abstract Object combineData(ICell var1, List<Object> var2, Object var3);

    protected abstract Object format(Object var1, int var2);
}

