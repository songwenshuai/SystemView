/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse;

import de.toem.toolkits.bundles.Activator;
import de.toem.toolkits.pattern.ide.IConsoleStream;
import de.toem.toolkits.pattern.ide.Ide;
import org.osgi.framework.Bundle;
import org.osgi.framework.BundleContext;

public class ImpulseBase
extends Activator {
    public static final String BUNDLE_ID = "de.toem.impulse.base";
    public static ImpulseBase instance;

    public ImpulseBase() {
        super(BUNDLE_ID);
        instance = this;
    }

    public static ImpulseBase getInstance() {
        return instance;
    }

    public static BundleContext getContext() {
        return instance != null ? ImpulseBase.instance.context : null;
    }

    public static Bundle getBundle() {
        return instance != null && ImpulseBase.instance.context != null ? ImpulseBase.instance.context.getBundle() : null;
    }

    public static ClassLoader getClassLoader() {
        return ImpulseBase.class.getClassLoader();
    }

    public static float getVersion() {
        if (ImpulseBase.getBundle() != null) {
            ImpulseBase.getBundle().getVersion();
        }
        return instance != null ? instance.extractVersion() : 0.0f;
    }

    @Override
    public void start(BundleContext context) throws Exception {
        super.start(context);
        Ide.registerConsole(Ide.DEFAULT_CONSOLE, "impulse");
    }

    public static IConsoleStream defaultConsoleStream() {
        return Ide.defaultConsoleStream(Ide.DEFAULT_CONSOLE);
    }

    public static IConsoleStream newConsoleStream() {
        return Ide.openConsoleStream(Ide.DEFAULT_CONSOLE);
    }

    public void activeConsole() {
    }
}

