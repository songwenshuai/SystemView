/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse;

import de.toem.impulse.IKeyListener;
import de.toem.impulse.IKeyProvider;
import de.toem.toolkits.pattern.registry.AbstractRegistryObject;
import java.util.Map;

public class EvalKeyProvider
extends AbstractRegistryObject
implements IKeyProvider {
    private Map<String, String> preset;

    @Override
    public String getKey() {
        return "CBWBeaN/lvfIOfKmOjBjQFtHVPGplFQK8o2X+tflRZh0YXxzYo0k1LUjXYU6yX7ccjDevRv5bSlxjwAyBr7wv97NK9NXexeKAMucNGJ4pAvGJwJRNJ/UdXkcEgQuQMfeuRy2BD9TrilEApKtHYl2bcF50ae+eT6fEz2WwPJQp9k=";
    }

    @Override
    public int getPriority() {
        return 2;
    }

    @Override
    public void addListener(IKeyListener listener) {
    }

    @Override
    public void removeListener(IKeyListener listener) {
    }

    @Override
    public Map<String, String> getPreset() {
        return this.preset;
    }
}

