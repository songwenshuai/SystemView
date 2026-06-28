/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.serializer;

import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.scripting.Scripting;
import de.toem.toolkits.ui.instancer.AbstractDefaultInstancer;

public class ScriptedReaderConfigurationInstancer
extends AbstractDefaultInstancer {
    @Override
    public String getCellType() {
        return "impulse.serializer.configuration.scripted";
    }

    @Override
    protected void initOne(String id, ICell cell, IElement preferences) {
        cell.setValue("script", (Object)Scripting.loadScriptFromResources(ScriptedReaderConfigurationInstancer.class, "scriptedReader.js"));
    }
}

