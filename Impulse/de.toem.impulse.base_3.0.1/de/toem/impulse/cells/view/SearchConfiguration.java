/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.view;

import de.toem.impulse.cells.view.SourceReference;
import de.toem.impulse.samples.IPointer;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISamplePointer;
import de.toem.impulse.samples.ISamplesIterator;
import de.toem.impulse.scripting.DefaultScriptContextProvider;
import de.toem.toolkits.pattern.element.Cell;
import de.toem.toolkits.pattern.element.CellAnnotation;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.scripting.IScriptContextProvider;

@CellAnnotation(type="configuration.search", dynamicChildren={"configuration.srcref"})
public class SearchConfiguration
extends Cell
implements IScriptContextProvider {
    public static final String TYPE = "configuration.search";
    public String description;
    public boolean enabled;
    public String expression;
    public String expressionLanguage;
    public boolean reverse;
    public boolean wrap;

    @Override
    public void provideToScriptContext(IScriptContextProvider.IScriptContextInterface context) {
        int index = 0;
        DefaultScriptContextProvider.provideDefaultScriptContext(context, true, false, false, false, false, false);
        context.addSymbol("iter", ISamplesIterator.class);
        for (ICell iCell : this.getChildren(SourceReference.class)) {
            if (!((SourceReference)iCell).enabled) continue;
            context.addSymbol("s" + index++, IPointer.class, ISamplePointer.class, IReadableSamples.class);
        }
        context.setScript(this.expression, this.expressionLanguage);
    }
}

