/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.flux;

import de.toem.impulse.cells.serializer.FluxReaderConfiguration;
import de.toem.impulse.flux.AbstractFluxHandler;
import de.toem.impulse.flux.FluxParser;
import de.toem.impulse.i18n.I18n;
import de.toem.impulse.serializer.BinaryParseBuffer;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.scripting.IScripting;
import de.toem.toolkits.pattern.scripting.Scripting;
import de.toem.toolkits.pattern.threading.IProgress;

public class FluxConfigurationHandler
extends AbstractFluxHandler {
    FluxReaderConfiguration configuration;

    public FluxConfigurationHandler(FluxReaderConfiguration configuration) {
        this.configuration = configuration;
    }

    @Override
    public void handleCreated(IProgress p, FluxParser.Trace trace, BinaryParseBuffer b) {
        this.runScript(p, 0, trace, 0, b);
    }

    @Override
    public void handleOpened(IProgress p, FluxParser.Trace trace, int itemId, BinaryParseBuffer b) {
        this.runScript(p, 0, trace, 0, b);
    }

    @Override
    public void handleClosed(IProgress p, FluxParser.Trace trace, int itemId, BinaryParseBuffer b) {
        this.runScript(p, 0, trace, 0, b);
    }

    protected void runScript(IProgress p, int state, FluxParser.Trace trace, int itemId, BinaryParseBuffer b) {
        String script = this.configuration.script;
        if (Utils.isEmpty(script)) {
            return;
        }
        IScripting scripting = Scripting.create(this.configuration, "script", s -> {
            s.setSymbol("trace", trace);
            s.setSymbol("itemId", itemId);
            s.onException(e -> b.setError(String.valueOf(I18n.Flux_ScriptException) + " -> " + e.getLocalizedMessage()));
        });
        switch (state) {
            case 0: {
                scripting.run(String.valueOf(script) + "handleCreated(trace);", p);
                break;
            }
            case 1: {
                scripting.run(String.valueOf(script) + "handleOpened(trace,itemId);", p);
                break;
            }
            case 2: {
                scripting.run(String.valueOf(script) + "handleClosed(trace,itemId);", p);
            }
        }
    }
}

