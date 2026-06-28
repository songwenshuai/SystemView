/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse;

import de.toem.impulse.IKeyListener;
import de.toem.impulse.IKeyProvider;
import de.toem.toolkits.pattern.registry.AbstractRegistryObject;
import java.util.Map;

public class DefaultKeyProvider
extends AbstractRegistryObject
implements IKeyProvider {
    @Override
    public String getKey() {
        return "wui95G4j2i7ksobwauXUWmpo3tEEesX8QWrn8/rY6aMTN33P/8CA3t28w8FjjDwOPaZypqmL+YGt0CKJy6gJ5XsCVHixYrJQ4O0cqh/NKkenC3pcluMlz2rW8uc7bR8fbLf2f+pOTmGDGnHEyunDH/yYXMH+iQoa";
    }

    @Override
    public int getPriority() {
        return 0;
    }

    @Override
    public void addListener(IKeyListener listener) {
    }

    @Override
    public void removeListener(IKeyListener listener) {
    }

    @Override
    public Map<String, String> getPreset() {
        return null;
    }
}

