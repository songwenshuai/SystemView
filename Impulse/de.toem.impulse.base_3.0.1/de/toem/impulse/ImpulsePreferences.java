/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse;

import de.toem.impulse.cells.preferences.ImpulseCharts;
import de.toem.impulse.cells.preferences.ImpulseNatives;
import de.toem.impulse.cells.preferences.ImpulseSerializers;
import de.toem.impulse.cells.serializer.ReaderConfiguration;
import de.toem.impulse.cells.serializer.Serializer;
import de.toem.impulse.cells.view.PlotConfiguration;
import de.toem.impulse.cells.view.PlotConfigurationTemplate;
import de.toem.impulse.domain.AmpsBase;
import de.toem.impulse.domain.DomainBase;
import de.toem.impulse.domain.FrequencyBase;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.domain.TimeBase;
import de.toem.impulse.domain.UnknownBase;
import de.toem.impulse.domain.VoltsBase;
import de.toem.impulse.i18n.I18n;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.bundles.Bundles;
import de.toem.toolkits.pattern.element.ElementHierarchyModifier;
import de.toem.toolkits.pattern.element.ElementListener;
import de.toem.toolkits.pattern.element.ElementModifierEvent;
import de.toem.toolkits.pattern.element.Elements;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.element.initializer.CellInitializer;
import de.toem.toolkits.pattern.element.initializer.ElementInitializer;
import de.toem.toolkits.pattern.element.initializer.RegistryObjectCellSynchronizer;
import de.toem.toolkits.pattern.element.initializer.UpdatableCellInitializer;
import de.toem.toolkits.pattern.element.instancer.IInstancer;
import de.toem.toolkits.pattern.element.serializer.SerializerDesciptors;
import de.toem.toolkits.pattern.element.serializer.SerializerDescriptor;
import de.toem.toolkits.pattern.preferences.Preferences;
import de.toem.toolkits.pattern.registry.AbstractRegistryObjectCell;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.List;

public class ImpulsePreferences {
    public static final IElement generalPreferences = Preferences.getElement("de.toem.impulse.base", "preferences.impulse.settings").addInitializer(new ElementInitializer("preferences.impulse.settings")).load();
    public static final IElement viewPreferences = Preferences.getElement("de.toem.impulse.base", "preferences.views").addInitializer(new ElementInitializer("preferences.views")).load();
    public static final IElement portPreferences = Preferences.getElement("de.toem.impulse.base", "preferences.impulse.ports").addInitializer(new ElementInitializer("preferences.impulse.ports")).load();
    public static final IElement licensePreferences = Preferences.getGlobalElement("de.toem.impulse.base", "preferences.licence").addInitializer(new ElementInitializer("preferences.licence")).load();
    public static final IElement chartPreferences = Preferences.getElement("de.toem.impulse.base", "preferences.impulse.charts").addInitializer(new CellInitializer("preferences.impulse.charts"){

        @Override
        protected void init() {
            String[] types = Elements.cells.getDynamicChildren("preferences.impulse.charts");
            for (IInstancer<?, ?> instancer : Elements.instancers.getAllWithTypes(types)) {
                List<ICell> intials = instancer.create("INITIAL", null, null);
                if (Utils.isEmpty(intials)) {
                    ICell cell = instancer.createOne(instancer.getDefaultId(), null, IElement.NONE);
                    cell.setName(instancer.getLabel());
                    cell.setValue("description", (Object)(String.valueOf(I18n.General_Default) + " " + instancer.getLabel()));
                    this.add(cell);
                    continue;
                }
                for (ICell i : intials) {
                    this.add(i);
                }
            }
        }
    }).load();
    public static final IElement templatePreferences = Preferences.getElement("de.toem.impulse.base", "preferences.impulse.templates").addInitializer(new UpdatableCellInitializer("preferences.impulse.templates"){

        @Override
        protected void init() {
            try {
                byte[] templatesData = Bundles.getBundleEntryAsByteArray("de.toem.impulse.base", "templates.walML");
                if (templatesData != null) {
                    Elements.getElement(templatesData).load(false, element -> {
                        if (element.isBound() && element.hasCell()) {
                            List<PlotConfigurationTemplate> templates = element.getCell().getChildren(PlotConfigurationTemplate.class);
                            for (ICell iCell : templates) {
                                this.add(iCell.clone());
                            }
                            List<PlotConfiguration> list = element.getCell().getChildren(PlotConfiguration.class);
                            for (ICell iCell : list) {
                                PlotConfigurationTemplate template = new PlotConfigurationTemplate();
                                template.setName(iCell.getName());
                                template.useInMenu = true;
                                template.addChild(iCell.clone());
                                this.add(template);
                            }
                        }
                    });
                }
            }
            catch (IOException iOException) {}
        }
    }).load();
    public static final IElement searchPreferences = Preferences.getElement("de.toem.impulse.base", "preferences.impulse.search").addInitializer(new ElementInitializer("preferences.impulse.search")).load();
    public static final IElement serializerPreferences = Preferences.getElement("de.toem.impulse.base", "preferences.impulse.serializers").addInitializer(new RegistryObjectCellSynchronizer(Elements.serializers, "preferences.impulse.serializers", "impulse.serializer", true){

        @Override
        protected Iterable<?> getObjectChildren(Object object) {
            if (object instanceof SerializerDesciptors) {
                Collection all = ((SerializerDesciptors)object).getAll();
                ArrayList<SerializerDescriptor> impulse = new ArrayList<SerializerDescriptor>();
                for (SerializerDescriptor s : all) {
                    if (!s.getId().contains("impulse")) continue;
                    impulse.add(s);
                }
                return impulse;
            }
            return null;
        }
    }).load();
    public static final IElement partsPreferences = Preferences.getElement("de.toem.impulse.base", "preferences.impulse.parts").addInitializer(new ElementInitializer("preferences.impulse.parts")).addInitializer(new CellInitializer("preferences.impulse.parts"){

        @Override
        protected void init() {
            String[] types = Elements.cells.getDynamicChildren("preferences.impulse.parts");
            for (IInstancer<?, ?> instancer : Elements.instancers.getAllWithTypes(types)) {
                ICell cell = instancer.createOne(instancer.getDefaultId(), null, IElement.NONE);
                cell.setName(I18n.General_Default);
                cell.setValue("description", (Object)(String.valueOf(I18n.General_Default) + " " + " " + instancer.getLabel()));
                this.add(cell);
            }
        }
    }).load();
    public static final IElement nativePreferences = Preferences.getElement("de.toem.impulse.base", "preferences.impulse.natives").addInitializer(new ElementInitializer("preferences.impulse.natives")).load();

    static {
        ElementListener listener = new ElementListener(){

            private void adjustPrefferedDomains() {
                if (generalPreferences.isBound() && generalPreferences.hasCell()) {
                    String[] splitted;
                    TimeBase.resetPreferred();
                    FrequencyBase.resetPreferred();
                    VoltsBase.resetPreferred();
                    AmpsBase.resetPreferred();
                    String preferredDomainUnit = generalPreferences.getCell().getValue("preferredDomainUnit", String.class);
                    if (preferredDomainUnit == null) {
                        return;
                    }
                    String[] stringArray = splitted = preferredDomainUnit.split(",");
                    int n = splitted.length;
                    int n2 = 0;
                    while (n2 < n) {
                        String s = stringArray[n2];
                        IDomainBase base = DomainBase.parse(s);
                        if (base != UnknownBase.Unknown) {
                            base.setPreferred(true);
                        }
                        ++n2;
                    }
                }
            }

            @Override
            public void elementModified(ElementModifierEvent event) {
                if (event == null || event.getField() != null && Utils.equals(event.getField().getName(), "preferredDomainUnit")) {
                    this.adjustPrefferedDomains();
                }
            }

            @Override
            public void elementLoaded(IElement element, boolean coverOnly) {
                this.adjustPrefferedDomains();
            }
        };
        generalPreferences.addListener(listener);
        listener.elementResetted(generalPreferences);
        Thread save = new Thread(() -> ImpulsePreferences.save());
        Runtime.getRuntime().addShutdownHook(save);
    }

    public static ICell getChart(String name) {
        if (chartPreferences.isBound() && chartPreferences.hasCell(ImpulseCharts.class)) {
            return chartPreferences.getCell().getChildByName(name);
        }
        return null;
    }

    public static List<Serializer> getAllSerializer() {
        if (serializerPreferences.isBound() && serializerPreferences.hasCell(ImpulseSerializers.class)) {
            return serializerPreferences.getCell().getChildren(Serializer.class);
        }
        return Collections.EMPTY_LIST;
    }

    public static Serializer getSerializer(String id) {
        if (serializerPreferences.isBound() && serializerPreferences.hasCell(ImpulseSerializers.class)) {
            for (Serializer cell : serializerPreferences.getCell().getChildren(Serializer.class)) {
                if (!cell.id.equals(id)) continue;
                return cell;
            }
        }
        return null;
    }

    public static List<ReaderConfiguration> getSerializerAllConfigurations(String id) {
        Serializer serializer = ImpulsePreferences.getSerializer(id);
        return serializer != null ? serializer.getChildren(ReaderConfiguration.class) : Collections.EMPTY_LIST;
    }

    public static ReaderConfiguration getSerializerConfiguration(String id, String name) {
        Serializer serializer = ImpulsePreferences.getSerializer(id);
        return serializer != null ? serializer.getChildByName(name, ReaderConfiguration.class) : null;
    }

    public static ICell getNative(String id, AbstractRegistryObjectCell defaultCell) {
        if (nativePreferences.isBound() && nativePreferences.hasCell(ImpulseNatives.class)) {
            Object cell = null;
            for (AbstractRegistryObjectCell child : nativePreferences.getCell().getChildren(AbstractRegistryObjectCell.class)) {
                if (!child.id.equals(id)) continue;
                cell = child;
                break;
            }
            if (cell == null || !Utils.equals(defaultCell != null ? defaultCell.getClass() : null, cell.getClass())) {
                defaultCell.id = id;
                ElementHierarchyModifier.add(nativePreferences, defaultCell.getElement(), null).doIt(null);
            } else {
                return cell;
            }
        }
        return defaultCell;
    }

    public static void save() {
        if (viewPreferences.isBound() && viewPreferences.isDirty()) {
            viewPreferences.save(false, null);
        }
        if (portPreferences.isBound() && portPreferences.isDirty()) {
            portPreferences.save(false, null);
        }
        if (serializerPreferences.isBound() && serializerPreferences.isDirty()) {
            serializerPreferences.save(false, null);
        }
        if (chartPreferences != null && chartPreferences.isBound() && chartPreferences.isDirty()) {
            chartPreferences.save(false, null);
        }
        if (templatePreferences != null && templatePreferences.isBound() && templatePreferences.isDirty()) {
            templatePreferences.save(false, null);
        }
        if (searchPreferences != null && searchPreferences.isBound() && searchPreferences.isDirty()) {
            searchPreferences.save(false, null);
        }
        if (partsPreferences != null && partsPreferences.isBound() && partsPreferences.isDirty()) {
            partsPreferences.save(false, null);
        }
    }
}

