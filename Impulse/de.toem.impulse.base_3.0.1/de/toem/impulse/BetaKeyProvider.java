/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse;

import de.toem.impulse.IKeyListener;
import de.toem.impulse.IKeyProvider;
import de.toem.toolkits.pattern.registry.AbstractRegistryObject;
import java.util.Map;

public class BetaKeyProvider
extends AbstractRegistryObject
implements IKeyProvider {
    @Override
    public String getKey() {
        return "O7Y6dpXj6IO3pIIDk6BmJVLcEdrLa5PRNw3g6A/CmgpiOn651z0vN2IXWw8nVl5VQFOb3P+Wz0sXs8nbBZfJqMNLhDu4df9kNCB0eWfcNXiKltNrTjo72NKHpgfT6YSOpRiLudoxrcPFyza3A2u3F3hBsvylvWxJTc/QSD6LfEDq7ZUDL7M/Myjp7h/0jV7e";
    }

    @Override
    public int getPriority() {
        return 3;
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

