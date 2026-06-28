/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.flux;

import de.toem.impulse.ImpulsePreferences;
import de.toem.impulse.i18n.I18n;
import de.toem.toolkits.core.Platform;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.bundles.Bundles;
import de.toem.toolkits.pattern.element.CellAnnotation;
import de.toem.toolkits.pattern.ide.IConsoleStream;
import de.toem.toolkits.pattern.ide.Ide;
import de.toem.toolkits.pattern.registry.AbstractRegistryObjectCell;
import java.io.BufferedReader;
import java.io.File;
import java.io.InputStreamReader;
import java.net.URI;
import java.util.ArrayList;

@CellAnnotation(type="impulse.native.flux", dynamicChildOf={"preferences.impulse.natives"})
public class FluxNative
extends AbstractRegistryObjectCell {
    public static final String TYPE = "impulse.native.flux";
    public static final String linux_x86_64 = "linux-x86_64";
    public static final String linux_x86 = "linux-x86";
    public static final String win32_x86_64 = "win32-x86_64";
    public static final String win32_x86 = "win32-x86";
    public static final String macosx_x86_64 = "macosx-x86_64";
    public static final String macosx_x86 = "macosx-x86";
    public String libPath;
    public String incPath;
    public String makeTarget;
    public String makeCmd;
    public String addFlags;
    public String addLibs;

    public static String[] getPlatformLabels(String name) {
        return new String[]{"Default", linux_x86_64, linux_x86, win32_x86_64, win32_x86, macosx_x86_64, macosx_x86, "clean", "other"};
    }

    public static Object[] getPlatformOptions(String name) {
        return new Object[]{"", "linux-x86_64/" + name, "linux-x86/" + name, "win32-x86_64/" + name + ".exe", "win32-x86/" + name + ".exe", "macosx-x86_64/" + name, "macosx-x86/" + name, "clean", "other"};
    }

    public void performMake(String bundleId, String name, String fluxPath, String makeFile) {
        try {
            String line;
            URI uri = Bundles.getBundleEntryAsFileUri(bundleId, fluxPath);
            String execute = Utils.isEmpty(this.makeCmd) ? (Platform.isWin32() ? "mingw32-make" : "make") : this.makeCmd;
            String target = Utils.isEmpty(this.makeTarget) ? FluxNative.getDefaultMakeTarget(name) : this.makeTarget;
            String addFlags = "ADD_FLAGS=";
            if (!Utils.isEmpty(this.incPath)) {
                addFlags = String.valueOf(addFlags) + " -I '" + this.incPath + "'";
            }
            if (!Utils.isEmpty(this.libPath)) {
                addFlags = String.valueOf(addFlags) + " -L '" + this.libPath + "'";
            }
            if (!Utils.isEmpty(this.addFlags)) {
                addFlags = String.valueOf(addFlags) + " " + this.addFlags;
            }
            String addLibs = "ADD_LIBS=";
            if (!Utils.isEmpty(this.addLibs)) {
                addLibs = String.valueOf(addLibs) + this.addLibs;
            }
            ArrayList<String> parameters = new ArrayList<String>();
            parameters.add(execute);
            parameters.add("-f");
            parameters.add(makeFile);
            parameters.add(target);
            if (addFlags.length() > 10) {
                parameters.add(addFlags);
            }
            if (addLibs.length() > 9) {
                parameters.add(addLibs);
            }
            parameters.add("FORCE=1");
            parameters.add("RELEASE=1");
            ProcessBuilder builder = new ProcessBuilder(parameters.toArray(new String[parameters.size()]));
            builder.directory(new File(uri.getPath()));
            builder.redirectErrorStream(true);
            Process process = builder.start();
            BufferedReader in = new BufferedReader(new InputStreamReader(process.getInputStream()));
            Ide.showConsole(Ide.DEFAULT_CONSOLE);
            IConsoleStream consoleStream = Ide.defaultConsoleStream(Ide.DEFAULT_CONSOLE);
            while ((line = in.readLine()) != null) {
                consoleStream.println(line);
            }
            in.close();
        }
        catch (Throwable throwable) {
            Ide.openError(I18n.Natives_Make, I18n.Natives_CouldNotExecuteMake);
        }
    }

    public static String getDefaultMakeTarget(String name) {
        return Platform.getExecutableSubEntryPath(name);
    }

    public static String getLibPath(String serializerId) {
        String value = ImpulsePreferences.getNative(serializerId, new FluxNative()).getValue("libPath", String.class);
        return value != null ? value : "";
    }

    public static String getIncPath(String serializerId) {
        String value = ImpulsePreferences.getNative(serializerId, new FluxNative()).getValue("incPath", String.class);
        return value != null ? value : "";
    }

    public static String getMakeTarget(String serializerId) {
        String value = ImpulsePreferences.getNative(serializerId, new FluxNative()).getValue("makeTarget", String.class);
        return value != null ? value : "";
    }

    public static String getMakeCmd(String serializerId) {
        String value = ImpulsePreferences.getNative(serializerId, new FluxNative()).getValue("makeCmd", String.class);
        return value != null ? value : "";
    }

    public static String getAddFlags(String serializerId) {
        String value = ImpulsePreferences.getNative(serializerId, new FluxNative()).getValue("addFlags", String.class);
        return value != null ? value : "";
    }

    public static String getAddLibs(String serializerId) {
        String value = ImpulsePreferences.getNative(serializerId, new FluxNative()).getValue("addLibs", String.class);
        return value != null ? value : "";
    }
}

