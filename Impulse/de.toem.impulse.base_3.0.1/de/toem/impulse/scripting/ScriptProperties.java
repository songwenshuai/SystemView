/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.scripting;

import de.toem.impulse.i18n.I18n;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.properties.IPropertyModel;
import de.toem.toolkits.pattern.properties.PropertyModel;

public class ScriptProperties {
    public static IPropertyModel getPropertyModel() {
        return new PropertyModel().add("timeout", "", I18n.General_TimeoutMs, null).add("scriptStructure", 0, "Script Structure", new String[]{"Flat", "Methods"}, null).add("showLogOutput", 1, "Show Log Output", new String[]{"None", "On any log", "From infos onwards", "From warnings onwards", "On errors only"}, null).add("enableLogging", 1, "Enable Logging", new String[]{"None", "For any log", "From infos onwards", "From warnings onwards", "For errors only"}, null);
    }

    public static int timeout(IPropertyModel properties) {
        return properties != null ? Utils.parseInt(properties.get("timeout"), 0) : 0;
    }

    public static boolean methodScriptStructure(IPropertyModel properties) {
        return properties != null ? Utils.parseInt(properties.get("scriptStructure"), 0) != 0 : false;
    }

    public static int logging(IPropertyModel properties) {
        return (properties != null ? Utils.parseInt(properties.get("enableLogging"), 0) : 0) | (properties != null ? Utils.parseInt(properties.get("showLogOutput"), 0) : 0) << 8;
    }
}

