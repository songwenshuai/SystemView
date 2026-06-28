/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse;

import de.toem.impulse.IKeyListener;
import de.toem.toolkits.pattern.registry.IRegistryObject;
import java.util.Map;

public interface IKeyProvider
extends IRegistryObject {
    public String getKey();

    public int getPriority();

    public void addListener(IKeyListener var1);

    public void removeListener(IKeyListener var1);

    public Map<String, String> getPreset();
}

