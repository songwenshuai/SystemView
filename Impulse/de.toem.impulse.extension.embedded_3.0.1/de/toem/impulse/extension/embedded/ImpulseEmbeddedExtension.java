/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.extension.embedded;

import de.toem.toolkits.bundles.Activator;
import org.osgi.framework.Bundle;
import org.osgi.framework.BundleContext;

public class ImpulseEmbeddedExtension
extends Activator {
    public static final String BUNDLE_ID = "de.toem.impulse.extension.embedded";
    public static ImpulseEmbeddedExtension instance;

    public ImpulseEmbeddedExtension() {
        super(BUNDLE_ID);
        instance = this;
    }

    public static ImpulseEmbeddedExtension getInstance() {
        return instance;
    }

    public static BundleContext getContext() {
        return instance != null ? ImpulseEmbeddedExtension.instance.context : null;
    }

    public static Bundle getBundle() {
        return instance != null && ImpulseEmbeddedExtension.instance.context != null ? ImpulseEmbeddedExtension.instance.context.getBundle() : null;
    }

    public static float getVersion() {
        if (ImpulseEmbeddedExtension.getBundle() != null) {
            ImpulseEmbeddedExtension.getBundle().getVersion();
        }
        return instance != null ? instance.extractVersion() : 0.0f;
    }
}

