/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.handler;

import de.toem.impulse.ImpulseLicense;
import de.toem.impulse.cells.record.Record;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.Elements;
import de.toem.toolkits.pattern.element.ICover;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.information.BaseGroupedInformations;
import de.toem.toolkits.pattern.pageable.PageTable;
import de.toem.toolkits.pattern.threading.Actives;
import de.toem.toolkits.pattern.threading.IExecutable;
import de.toem.toolkits.pattern.threading.IProgress;
import de.toem.toolkits.ui.handler.ElementCommandHandler;
import java.util.List;

public class RecordCommandHandler
extends ElementCommandHandler {
    private Object doLoad(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            for (IElement element : this.elements) {
                if (element.isBound() && element.hasResource()) continue;
                return false;
            }
            if (doIt == 0) {
                if ("PREFERENCES".equals(data)) {
                    String id = null;
                    for (Object element : this.elements) {
                        ICover cover = element.getCover();
                        String nextId = cover.getSerializer();
                        if (id == null) {
                            id = nextId;
                            continue;
                        }
                        if (Utils.equals(id, nextId)) continue;
                        id = null;
                        break;
                    }
                    return true;
                }
                for (IElement element : this.elements) {
                    if (data instanceof String && ((String)data).startsWith("CONFIGURATION")) {
                        element.setHint("CONFIGURATION", ((String)data).substring("CONFIGURATION".length()));
                    }
                    if (data instanceof String && ((String)data).startsWith("SERIALIZER")) {
                        element.setHint("SERIALIZER", ((String)data).substring("SERIALIZER".length()));
                    }
                    element.load();
                }
                Actives.runInMain(new IExecutable(){

                    @Override
                    public void execute(IProgress p) {
                    }
                });
            }
            return true;
        }
        if (doIt == 6 && Elements.onlyDC((List<IElement>)this.elements, new Class[]{Record.class})) {
            String serializer = null;
            String configName = null;
            for (IElement element : this.elements) {
                if (element.isBound() && element.hasResource()) {
                    ICover cover = element.getCover();
                    String nextId = cover.getSerializer();
                    String nextCfg = cover.getConfiguration();
                    if (serializer == null) {
                        serializer = nextId;
                        configName = nextCfg;
                    } else if (!Utils.equals(serializer, nextId)) {
                        serializer = null;
                        break;
                    }
                    if (Utils.equals(configName, nextCfg)) continue;
                    configName = null;
                    continue;
                }
                return null;
            }
            return RecordCommandHandler.getLoadOptions(serializer, configName);
        }
        return null;
    }

    public static Object getLoadOptions(String serializer, String configName) {
        BaseGroupedInformations options = new BaseGroupedInformations();
        return options;
    }

    private Object doUnLoad(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            if (!Elements.onlyDC((List<IElement>)this.elements, new Class[]{Record.class})) {
                return false;
            }
            boolean loaded = false;
            for (IElement element : this.elements) {
                if (!element.hasCell()) continue;
                loaded = true;
            }
            if (!loaded) {
                return false;
            }
            if (doIt == 0) {
                for (IElement element : this.elements) {
                    element.reset();
                }
                Actives.runInMain(new IExecutable(){

                    @Override
                    public void execute(IProgress p) {
                    }
                });
            }
            return true;
        }
        return null;
    }

    private Object doOpenDiff(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            if (!Elements.onlyDC((List<IElement>)this.elements, new Class[]{Record.class})) {
                return false;
            }
            if (this.elements.size() != 2) {
                return false;
            }
            if (ImpulseLicense.isFeatureLocked("de.toem.impulse.featureDomain.global", "de.toem.impulse.feature.global.diff", null)) {
                return false;
            }
            return true;
        }
        return null;
    }

    private Object doUnpageSignals(Object data, int doIt, Object sender) {
        if (doIt == 0 || doIt == 1) {
            if (doIt == 0) {
                PageTable.setClearAll();
            }
            return true;
        }
        return null;
    }

    @Override
    public Object command(String id, Object data, int doIt, Object sender) {
        if (id.equals("de.toem.impulse.commands.record.load")) {
            if (doIt != 5) {
                return this.doLoad(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.record.unload")) {
            if (doIt != 5) {
                return this.doUnLoad(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.record.openDiff")) {
            if (doIt != 5) {
                return this.doOpenDiff(data, doIt, sender);
            }
        } else if (id.equals("de.toem.impulse.commands.unpageSignals")) {
            if (doIt != 5) {
                return this.doUnpageSignals(data, doIt, sender);
            }
        } else if (doIt == 5) {
            return false;
        }
        if (doIt == 5) {
            return true;
        }
        return null;
    }
}

