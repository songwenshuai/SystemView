/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse;

import de.toem.impulse.ImpulsePreferences;
import de.toem.toolkits.pattern.element.Elements;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.element.Link;

public class HintTool {
    public static final String TOOL_VIEWER = "de.toem.impulse.viewer";
    public static final String TOOL_DIFF_VIEWER = "de.toem.impulse.diffViewer";
    public static final String R_configuration = "configuration";
    public static final String R_configurationFilter = "configurationFilter";
    public static final String R_children = "check.children.value";
    public static final String R_regular = "check.regular.value";
    public static final String R_diff = "check.diff.value";
    public static final String R_conflict = "check.conflict.value";
    public static final String R_configWidth = "samples.configWidth";
    public static final String R_valueColumnWidth = "samples.valueColumnWidth";
    public static final String R_showCursorDetails = "samples.showCursorDetails";
    public static final String R_showValueColumn = "samples.showValueColumn";
    public static final String R_showGrid = "samples.showGrid";
    public static final String R_showAxis = "samples.showAxis";
    public static final String R_showIcons = "samples.showIcons";
    public static final String R_showNamesRightAlligned = "samples.showNamesRightAlligned";
    public static final String R_samplesVDefault = "samples.v.default";
    public static final String R_samplesVMaximize = "samples.v.maximize";
    public static final String R_showAll = "showAll";
    public static final String C_expanded = "EXPANDED";
    public static final String C_position = "POSITION";
    public static final String C_showAll = "showAll";

    public static String getResourceHint(String tool, String rkey, Object resource) {
        IElement element = Elements.getElement(resource);
        if (element.isBound()) {
            return element.getHint(String.valueOf(tool) + "." + rkey);
        }
        return null;
    }

    public static void setResourceHint(String tool, String rkey, Object resource, String value) {
        IElement element = Elements.getElement(resource);
        if (element.isBound()) {
            element.setHint(String.valueOf(tool) + "." + rkey, value);
        }
    }

    public static String getConfigurationHint(String ckey, String configurationPath) {
        Link link = Link.fromPath(configurationPath);
        IElement element = link.resolveElement(ImpulsePreferences.viewPreferences);
        if (element.isBound()) {
            return element.getHint(ckey);
        }
        return null;
    }

    public static void setConfigurationHint(String ckey, String configurationPath, String value) {
        Link link = Link.fromPath(configurationPath);
        IElement element = link.resolveElement(ImpulsePreferences.viewPreferences);
        if (element.isBound()) {
            element.setHint(ckey, value);
        }
    }
}

