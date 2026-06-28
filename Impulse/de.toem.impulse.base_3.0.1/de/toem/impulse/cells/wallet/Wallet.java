/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.wallet;

import de.toem.impulse.ImpulseBase;
import de.toem.impulse.ImpulsePreferences;
import de.toem.impulse.cells.charts.AbstractChartCell;
import de.toem.impulse.cells.ports.IRecordPort;
import de.toem.impulse.cells.preferences.AbstractSubViewPreferenceCell;
import de.toem.impulse.cells.serializer.ReaderConfiguration;
import de.toem.impulse.cells.serializer.Serializer;
import de.toem.impulse.cells.view.PlotConfigurationTemplate;
import de.toem.impulse.cells.view.SearchConfiguration;
import de.toem.impulse.cells.view.ViewConfiguration;
import de.toem.impulse.cells.wallet.ResourceFile;
import de.toem.impulse.cells.wallet.ResourceFolder;
import de.toem.impulse.i18n.I18n;
import de.toem.impulse.scripting.DefaultScriptContextProvider;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.Cell;
import de.toem.toolkits.pattern.element.CellAnnotation;
import de.toem.toolkits.pattern.element.ElementHierarchyModifier;
import de.toem.toolkits.pattern.element.Elements;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.element.IElementModifier;
import de.toem.toolkits.pattern.element.Link;
import de.toem.toolkits.pattern.element.constant.ConstantByteArrayElement;
import de.toem.toolkits.pattern.scripting.IScriptContextProvider;
import java.lang.reflect.Field;
import java.util.List;

@CellAnnotation(type="wallet", dynamicChildren={"any", "wallet.resource.file", "wallet.resource.folder"})
public class Wallet
extends Cell
implements IScriptContextProvider {
    public static final String TYPE = "wallet";
    public static final String REPLACE_HEADER = "${wallet_";
    public static final String REPLACE_CONTAINER_PATH = "${wallet_container_path}";
    public static final String REPLACE_CONTAINER_LOC = "${wallet_container_loc}";
    public static final String REPLACE_NAME = "${wallet_name}";
    public static final String REPLACE_FILE_NAME = "${wallet_file_name}";
    public static final String REPLACE_OS = "${wallet_os}";
    public static final String REPLACE_JAVA = "${java}";
    public static final int INSTALL = 1;
    public static final int UNINSTALL = 2;
    public static final int UPDATE = 2;
    public String description;
    public boolean installable;
    public String script;
    public String scriptLanguage;
    public String minVersion;
    public String maxVersion;

    public boolean createModifiers(int operation, List<IElementModifier> modifiers, StringBuilder messages, List<ICell> limited) {
        boolean ok = true;
        if (operation == 1) {
            if (!Utils.isEmpty(this.minVersion) && ImpulseBase.getVersion() < ImpulseBase.parseVersion(this.minVersion)) {
                if (messages != null) {
                    messages.append(" * Minumun version required:" + this.minVersion + "!" + Utils.lineSeparator);
                }
                ok = false;
            }
            if (!Utils.isEmpty(this.maxVersion) && ImpulseBase.getVersion() > ImpulseBase.parseVersion(this.maxVersion)) {
                if (messages != null) {
                    messages.append(" * Maximum version required: " + this.minVersion + "!" + Utils.lineSeparator);
                }
                ok = false;
            }
        }
        for (ICell cell : this.getChildren()) {
            IElement element;
            if (limited != null && !limited.contains(cell)) continue;
            String name = this.replace(cell.getName());
            if (cell instanceof ViewConfiguration) {
                element = operation == 2 ? ImpulsePreferences.viewPreferences.getChild(name) : (operation == 1 ? this.createInstallElement(cell) : this.createUpdateElement(ImpulsePreferences.viewPreferences.getChild(name)));
                ok &= this.createModifiers(operation, element, name, I18n.General_View, operation == 2 ? this.getElement() : ImpulsePreferences.viewPreferences, modifiers, messages);
                continue;
            }
            if (cell instanceof IRecordPort) {
                element = operation == 2 ? ImpulsePreferences.portPreferences.getChild(name) : (operation == 1 ? this.createInstallElement(cell) : this.createUpdateElement(ImpulsePreferences.portPreferences.getChild(name)));
                ok &= this.createModifiers(operation, element, name, I18n.General_Port, operation == 2 ? this.getElement() : ImpulsePreferences.portPreferences, modifiers, messages);
                continue;
            }
            if (cell instanceof Serializer) {
                IElement serializer = ImpulsePreferences.serializerPreferences.getChild(cell.getName());
                if (!serializer.isBound()) continue;
                for (ICell iCell : cell.getChildren(ReaderConfiguration.class)) {
                    IElement element2 = operation == 2 ? serializer.getChild(this.replace(iCell.getName())) : (operation == 1 ? this.createInstallElement(iCell) : cell.getElement());
                    ok &= this.createModifiers(operation, element2, this.replace(iCell.getName()), I18n.General_Serializer, serializer, modifiers, messages);
                }
                continue;
            }
            if (cell instanceof AbstractChartCell) {
                element = operation == 2 ? ImpulsePreferences.chartPreferences.getChild(name) : (operation == 1 ? this.createInstallElement(cell) : this.createUpdateElement(ImpulsePreferences.chartPreferences.getChild(name)));
                ok &= this.createModifiers(operation, element, name, I18n.General_Chart, operation == 2 ? this.getElement() : ImpulsePreferences.chartPreferences, modifiers, messages);
                continue;
            }
            if (cell instanceof PlotConfigurationTemplate) {
                element = operation == 2 ? ImpulsePreferences.templatePreferences.getChild(name) : (operation == 1 ? this.createInstallElement(cell) : this.createUpdateElement(ImpulsePreferences.templatePreferences.getChild(name)));
                ok &= this.createModifiers(operation, element, name, I18n.General_Template, operation == 2 ? this.getElement() : ImpulsePreferences.templatePreferences, modifiers, messages);
                continue;
            }
            if (cell instanceof SearchConfiguration) {
                element = operation == 2 ? ImpulsePreferences.searchPreferences.getChild(name) : (operation == 1 ? this.createInstallElement(cell) : this.createUpdateElement(ImpulsePreferences.searchPreferences.getChild(name)));
                ok &= this.createModifiers(operation, element, name, I18n.General_Search, operation == 2 ? this.getElement() : ImpulsePreferences.searchPreferences, modifiers, messages);
                continue;
            }
            if (cell instanceof AbstractSubViewPreferenceCell) {
                element = operation == 2 ? ImpulsePreferences.partsPreferences.getChild(name) : (operation == 1 ? this.createInstallElement(cell) : this.createUpdateElement(ImpulsePreferences.partsPreferences.getChild(name)));
                ok &= this.createModifiers(operation, element, name, I18n.General_Parts, operation == 2 ? this.getElement() : ImpulsePreferences.partsPreferences, modifiers, messages);
                continue;
            }
            if (cell instanceof ResourceFolder || cell instanceof ResourceFile) {
                if (Utils.equals(name, this.getElement().getName())) {
                    if (messages == null) continue;
                    messages.append(" * " + name + "not supported for installation (ignored)" + Utils.lineSeparator);
                    continue;
                }
                element = operation == 2 ? this.getElement().getContainer().getChild(name) : (operation == 1 ? this.createInstallElement(cell) : this.createUpdateElement(ImpulsePreferences.viewPreferences.getChild(name)));
                ok &= this.createModifiers(operation, element, name, I18n.General_Resource, this.getElement().getContainer(), modifiers, messages);
                continue;
            }
            if (messages == null) continue;
            messages.append(" *" + name + "not supported for installation (ignored)" + Utils.lineSeparator);
        }
        return ok;
    }

    boolean createModifiers(int operation, IElement element, String name, String type, IElement target, List<IElementModifier> modifiers, StringBuilder messages) {
        boolean ok = true;
        if (operation == 1) {
            if (target.hasChild(element.getName())) {
                if (messages != null) {
                    messages.append(" * " + name + " (" + type + ") allready existing!" + Utils.lineSeparator);
                }
                ok = false;
            }
            if (target.hasChild(element.getName()) && modifiers != null) {
                modifiers.add(ElementHierarchyModifier.remove(target, target.getChild(element.getName())));
                if (messages != null) {
                    messages.append(" * " + name + " (" + type + ") removed!" + Utils.lineSeparator);
                }
            }
            if (modifiers != null) {
                modifiers.add(ElementHierarchyModifier.add(target, element, null));
                if (messages != null) {
                    messages.append(" * " + name + " (" + type + ") installed!" + Utils.lineSeparator);
                }
            }
        } else if (operation == 2) {
            if (!element.isBound() && messages != null) {
                messages.append(" * " + name + " (" + type + ") not existing!" + Utils.lineSeparator);
            }
            if (modifiers != null && element.isBound()) {
                modifiers.add(ElementHierarchyModifier.remove(target, element));
                if (messages != null) {
                    messages.append(" * " + name + " (" + type + ") un-installed!" + Utils.lineSeparator);
                }
            }
        } else if (operation == 2) {
            if (!element.isBound()) {
                if (messages != null) {
                    messages.append(" * " + name + " (" + type + ") does not exist!" + Utils.lineSeparator);
                }
                ok = false;
            }
            if (modifiers != null && element.isBound()) {
                modifiers.add(ElementHierarchyModifier.add(target, element, element.getName()));
            }
        }
        return ok;
    }

    private IElement createInstallElement(ICell cell) {
        if (cell instanceof ResourceFolder || cell instanceof ResourceFile) {
            ConstantByteArrayElement constant = cell instanceof ResourceFolder ? ((ResourceFolder)cell).createConstantElement() : ((ResourceFile)cell).createConstantElement();
            return constant;
        }
        ICell cloned = cell.clone();
        for (ICell c : cloned.getTribe(true)) {
            Field[] fieldArray = c.getClass().getFields();
            int n = fieldArray.length;
            int n2 = 0;
            while (n2 < n) {
                Object value;
                Field f = fieldArray[n2];
                if (f.getType() == String.class) {
                    try {
                        value = (String)f.get(c);
                        if (value != null && ((String)value).contains(REPLACE_HEADER)) {
                            value = this.replace((String)value);
                            f.set(c, value);
                        }
                    }
                    catch (IllegalAccessException | IllegalArgumentException exception) {}
                }
                if (f.getType() == Link.class) {
                    try {
                        String svalue;
                        value = (Link)f.get(c);
                        String string = svalue = value != null ? ((Link)value).toString() : null;
                        if (svalue != null && svalue.contains(REPLACE_HEADER)) {
                            svalue = this.replace(svalue);
                            f.set(c, Link.parse(svalue));
                        }
                    }
                    catch (IllegalAccessException | IllegalArgumentException exception) {}
                }
                ++n2;
            }
        }
        return cloned.getElement();
    }

    private IElement createUpdateElement(IElement element) {
        if (element.hasCell()) {
            return element.getCell().clone().getElement();
        }
        return IElement.NONE;
    }

    public String replace(String value) {
        if (value != null) {
            String replace;
            if (value.contains(REPLACE_CONTAINER_LOC)) {
                replace = this.getElement().getContainer().getResourceOsPath();
                value = value.replace(REPLACE_CONTAINER_LOC, replace);
            }
            if (value.contains(REPLACE_CONTAINER_PATH)) {
                replace = this.getElement().getContainer().getPath(this.getElement().getRootContainer());
                value = value.replace(REPLACE_CONTAINER_PATH, replace);
            }
            if (value.contains(REPLACE_NAME)) {
                replace = this.getElement().getName().replace(".walML", "").replace(".walMZ", "");
                value = value.replace(REPLACE_NAME, replace);
            }
            if (value.contains(REPLACE_FILE_NAME)) {
                replace = this.getElement().getName();
                value = value.replace(REPLACE_FILE_NAME, replace);
            }
            if (value.contains(REPLACE_JAVA)) {
                replace = Utils.getJavaLocation();
                value = value.replace(REPLACE_JAVA, replace);
            }
        }
        return value;
    }

    @Override
    public void provideToScriptContext(IScriptContextProvider.IScriptContextInterface context) {
        if (Utils.equals(context.getContextId(), "script")) {
            DefaultScriptContextProvider.provideDefaultScriptContext(context, false, false, false, false, false, true);
            context.addClasses(Elements.class, ImpulsePreferences.class);
            context.setScript(this.replace(this.script), this.scriptLanguage);
        }
    }
}

