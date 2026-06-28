/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.serializer;

import de.toem.impulse.cells.serializer.ReaderConfiguration;
import de.toem.impulse.cells.serializer.Serializer;
import de.toem.impulse.flux.IFluxInitialisation;
import de.toem.impulse.scripting.DefaultScriptContextProvider;
import de.toem.toolkits.pattern.element.CellAnnotation;
import de.toem.toolkits.pattern.scripting.IScriptContextProvider;

@CellAnnotation(type="impulse.serializer.configuration.flux", dynamicChildOf={"impulse.serializer"})
public class FluxReaderConfiguration
extends ReaderConfiguration
implements IScriptContextProvider {
    public static final String TYPE = "impulse.serializer.configuration.flux";
    public String script;

    @Override
    public void provideToScriptContext(IScriptContextProvider.IScriptContextInterface context) {
        DefaultScriptContextProvider.provideDefaultScriptContext(context, false, false, false, true, false, true);
        context.addSymbol("trace", IFluxInitialisation.class);
    }

    @Override
    public boolean supports(Serializer serializer) {
        return "de.toem.impulse.serializer.flux".equals(serializer.id);
    }
}

