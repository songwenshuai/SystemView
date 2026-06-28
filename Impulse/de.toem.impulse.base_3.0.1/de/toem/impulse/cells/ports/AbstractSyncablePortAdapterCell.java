/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.ports;

import de.toem.impulse.cells.ports.AbstractPortAdapterBaseCell;
import de.toem.impulse.cells.ports.IPortAdapter3;
import de.toem.impulse.cells.ports.IPortSync;
import de.toem.impulse.cells.record.PortScope;
import de.toem.impulse.cells.record.RecordContent;
import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISamplePointer;
import de.toem.impulse.samples.iterator.SamplePointer;
import de.toem.impulse.scripting.DefaultScriptContextProvider;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.scripting.IScriptContextProvider;
import de.toem.toolkits.pattern.scripting.IScripting;
import de.toem.toolkits.pattern.scripting.Scripting;

public abstract class AbstractSyncablePortAdapterCell
extends AbstractPortAdapterBaseCell
implements IScriptContextProvider,
IPortAdapter3 {
    public boolean enableSync;
    public String syncScript;
    public String syncScriptLanguage;

    @Override
    public void provideToScriptContext(IScriptContextProvider.IScriptContextInterface context) {
        if (Utils.equals(context.getContextId(), "syncScript")) {
            DefaultScriptContextProvider.provideDefaultScriptContext(context, true, true, true, true, false, true);
            context.addSymbol("insertPoint", PortScope.class);
            context.addSymbol("base", RecordContent.class);
            context.addSymbol("isync", IPortSync.class);
            context.setScript(this.syncScript, this.syncScriptLanguage);
        }
    }

    @Override
    public boolean needsSync() {
        return !Utils.isEmpty(this.syncScript) && this.enableSync;
    }

    @Override
    public boolean sync(final PortScope insertPoint, final ICell base) {
        final boolean[] cont = new boolean[]{true};
        IPortSync result = new IPortSync(){

            @Override
            public IReadableSamples getReadable(String cellPath) {
                IReadableSamples readable = null;
                ICell cell = base.getCell(cellPath);
                if (cell instanceof Signal) {
                    readable = ((Signal)cell).getSamples();
                }
                if (readable != null) {
                    readable.ensureSettled(null);
                }
                return readable;
            }

            @Override
            public ISamplePointer getPointer(String cellPath) {
                IReadableSamples readable = this.getReadable(cellPath);
                if (readable != null) {
                    return new SamplePointer(readable);
                }
                return null;
            }

            @Override
            public void setSynced(DomainValue offset) {
                insertPoint.domainSync0 = offset != null ? offset.toString(0) : null;
                insertPoint.synced = offset != null ? 2 : 1;
                cont[0] = false;
            }
        };
        IScripting scripting = Scripting.create(this, "syncScript", s -> {
            s.setSymbol("insertPoint", insertPoint);
            s.setSymbol("base", base);
            s.setSymbol("isync", result);
        });
        scripting.run(null);
        return !cont[0];
    }

    @Override
    public void prepareInsertPoint(ICell insertPoint) {
    }
}

