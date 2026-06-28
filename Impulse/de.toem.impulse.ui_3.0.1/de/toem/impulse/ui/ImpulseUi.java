/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.ui;

import de.toem.impulse.ImpulseBase;
import de.toem.toolkits.bundles.Activator;
import org.osgi.framework.Bundle;
import org.osgi.framework.BundleContext;

public class ImpulseUi
extends Activator {
    public static final String BUNDLE_ID = "de.toem.impulse.ui";
    public static ImpulseUi instance;

    public ImpulseUi() {
        super(BUNDLE_ID);
        instance = this;
    }

    public static ImpulseUi getInstance() {
        return instance;
    }

    public static BundleContext getContext() {
        return instance != null ? ImpulseUi.instance.context : null;
    }

    public static Bundle getBundle() {
        return instance != null && ImpulseUi.instance.context != null ? ImpulseUi.instance.context.getBundle() : null;
    }

    public static ClassLoader getClassLoader() {
        return ImpulseBase.class.getClassLoader();
    }

    public static float getVersion() {
        if (ImpulseUi.getBundle() != null) {
            ImpulseUi.getBundle().getVersion();
        }
        return instance != null ? instance.extractVersion() : 0.0f;
    }
}

