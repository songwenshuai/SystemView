/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.producer;

import de.toem.impulse.domain.DomainBase;
import de.toem.impulse.domain.IDomainBase;
import de.toem.impulse.domain.IDomainBaseProvider;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.samples.ISamplesProducer;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.bundles.Bundles;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.properties.IPropertyModel;
import de.toem.toolkits.pattern.registry.IRegistry;
import de.toem.toolkits.pattern.registry.Registration;
import de.toem.toolkits.pattern.registry.RegistryDescriptor;
import de.toem.toolkits.pattern.systemLog.SystemLog;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.List;

public class SamplesProducerDescriptor
extends RegistryDescriptor<ISamplesProducer> {
    private char handleProcess;
    private char handleType;
    private char handleDescriptor;
    private char handleProductionBase;
    private char handleDefinition;
    private String process;
    private String type;
    private String descriptor;
    private String productionBase;
    private String definition;
    private String language;
    private boolean showParameters;
    private boolean showMultipleInputs;
    private static final char SHOW = '!';
    private static final char HIDE = '#';

    @Override
    public void init(IRegistry registry) {
        super.init(registry);
        if (registry != null) {
            this.process = registry.getAttribute("process");
            if (!(Utils.isEmpty(this.process) || this.process.charAt(0) != '!' && this.process.charAt(0) != '#')) {
                this.handleProcess = this.process.charAt(0);
                this.process = this.process.substring(1);
            }
            this.type = registry.getAttribute("type");
            if (!(Utils.isEmpty(this.type) || this.type.charAt(0) != '!' && this.type.charAt(0) != '#')) {
                this.handleType = this.type.charAt(0);
                this.type = this.type.substring(1);
            }
            this.descriptor = registry.getAttribute("descriptor");
            if (!(Utils.isEmpty(this.descriptor) || this.descriptor.charAt(0) != '!' && this.descriptor.charAt(0) != '#')) {
                this.handleDescriptor = this.descriptor.charAt(0);
                this.descriptor = this.descriptor.substring(1);
            }
            this.productionBase = registry.getAttribute("productionBase");
            if (!(Utils.isEmpty(this.productionBase) || this.productionBase.charAt(0) != '!' && this.productionBase.charAt(0) != '#')) {
                this.handleProductionBase = this.productionBase.charAt(0);
                this.productionBase = this.productionBase.substring(1);
            }
            this.definition = registry.getAttribute("definition");
            if (!(Utils.isEmpty(this.definition) || this.definition.charAt(0) != '!' && this.definition.charAt(0) != '#')) {
                this.handleDefinition = this.definition.charAt(0);
                this.definition = this.definition.substring(1);
            }
            this.language = registry.getAttribute("language");
            this.showParameters = Boolean.TRUE.toString().equalsIgnoreCase(registry.getAttribute("showParameters"));
            this.showMultipleInputs = Boolean.TRUE.toString().equalsIgnoreCase(registry.getAttribute("showMultipleInputs"));
            try {
                String icon = registry.getAttribute("icon");
                if (Bundles.hasBundleEntry(registry.getDeclaringBundleId(), icon)) {
                    Registration.images.addEntry(this.id, registry.getDeclaringBundleId(), icon);
                } else if (icon.startsWith("#")) {
                    Registration.images.addReference(this.id, icon.substring(1));
                } else {
                    Registration.images.addIdentifier(this.id, icon);
                }
                this.iconId = this.id;
            }
            catch (Throwable e) {
                SystemLog.log(e);
            }
        }
    }

    public boolean overrideProcess() {
        return !Utils.isEmpty(this.process);
    }

    public boolean overrideType() {
        return !Utils.isEmpty(this.type);
    }

    public boolean overrideDescriptor() {
        return !Utils.isEmpty(this.descriptor);
    }

    public boolean overrideProductionBase() {
        return !Utils.isEmpty(this.productionBase);
    }

    public boolean overrideDefinition() {
        return !Utils.isEmpty(this.definition);
    }

    public String getProcess() {
        return this.process;
    }

    public String getType() {
        return this.type;
    }

    public String getDescriptor() {
        return this.descriptor;
    }

    public String getProductionBase() {
        return this.productionBase;
    }

    public String getDefinition() {
        return this.definition;
    }

    public String getLanguage() {
        return this.language;
    }

    public boolean showProcess() {
        return (Utils.isEmpty(this.process) || this.handleProcess == '!') && this.handleProcess != '#';
    }

    public boolean showType() {
        return (Utils.isEmpty(this.type) || this.handleType == '!') && this.handleType != '#';
    }

    public boolean showDescriptor() {
        return (Utils.isEmpty(this.descriptor) || this.handleDescriptor == '!') && this.handleDescriptor != '#';
    }

    public boolean showProductionBase() {
        return (Utils.isEmpty(this.productionBase) || this.handleProductionBase == '!') && this.handleProductionBase != '#';
    }

    public boolean showDefinition() {
        return (!Utils.isEmpty(this.language) && Utils.isEmpty(this.definition) || this.handleDefinition == '!') && this.handleDefinition != '#';
    }

    public boolean showParameters() {
        return this.showParameters;
    }

    public boolean showMultipleInputs() {
        return this.showMultipleInputs;
    }

    public ISamplesProducer newInstance(String id, String name, List<IReadableSamples> sources, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase productionBase, String start, String end, String rate, String definition, String language, IPropertyModel parameters, IDomainBaseProvider readerBase) {
        if (this.overrideProcess()) {
            processType = ISamples.ProcessType.parse(this.getProcess());
        } else if (!this.showProcess()) {
            processType = ISamples.ProcessType.Unknown;
        }
        if (this.overrideType()) {
            signalType = ISamples.SignalType.parse(this.getType());
        } else if (!this.showType()) {
            signalType = ISamples.SignalType.Unknown;
        }
        if (this.overrideDescriptor()) {
            signalDescriptor = ISamples.SignalDescriptor.parse(this.getDescriptor());
        } else if (!this.showDescriptor()) {
            signalDescriptor = null;
        }
        if (this.overrideProductionBase()) {
            productionBase = DomainBase.parse(this.getProductionBase());
        } else if (!this.showProductionBase()) {
            productionBase = DomainBase.Unknown;
            start = null;
            end = null;
            rate = null;
        }
        if (this.overrideDefinition()) {
            definition = this.getDefinition();
        } else if (!this.showDefinition()) {
            definition = null;
        }
        if (!this.showParameters()) {
            parameters = this.getPropertyModel();
        }
        return (ISamplesProducer)this.newInstance(new Class[]{String.class, String.class, List.class, ISamples.ProcessType.class, ISamples.SignalType.class, ISamples.SignalDescriptor.class, IDomainBase.class, String.class, String.class, String.class, String.class, String.class, IPropertyModel.class, IDomainBaseProvider.class}, new Object[]{id, name, sources, processType, signalType, signalDescriptor, productionBase, start, end, rate, definition, language, parameters, readerBase});
    }

    public int update(ISamplesProducer previous, String id, String name, List<IReadableSamples> sources, ISamples.ProcessType processType, ISamples.SignalType signalType, ISamples.SignalDescriptor signalDescriptor, IDomainBase productionBase, String start, String end, String rate, String definition, String language, IPropertyModel parameters, IDomainBaseProvider readerBase) {
        if (previous == null || !previous.getClass().equals(this.getClazz())) {
            return -1;
        }
        if (this.overrideProcess()) {
            processType = ISamples.ProcessType.parse(this.getProcess());
        }
        if (this.overrideType()) {
            signalType = ISamples.SignalType.parse(this.getType());
        }
        if (this.overrideDescriptor()) {
            signalDescriptor = ISamples.SignalDescriptor.parse(this.getDescriptor());
        }
        if (this.overrideProductionBase()) {
            productionBase = DomainBase.parse(this.getProductionBase());
        }
        if (this.overrideDefinition()) {
            definition = this.getDefinition();
        }
        return previous.update(id, name, sources, processType, signalType, signalDescriptor, productionBase, start, end, rate, definition, language, parameters, readerBase);
    }

    public int update(ISamplesProducer previous, List<IReadableSamples> sources) {
        if (previous == null || !previous.getClass().equals(this.getClazz())) {
            return -1;
        }
        return previous.update(sources);
    }

    public IControlProvider getDefinitionControls(Field field, Field language) {
        try {
            Method method;
            Method method2 = method = this.clazz != null ? this.clazz.getMethod("getDefinitionControls", Field.class, Field.class) : null;
            if (method != null) {
                return (IControlProvider)method.invoke(null, field, language);
            }
        }
        catch (SecurityException securityException) {
        }
        catch (NoSuchMethodException noSuchMethodException) {
        }
        catch (IllegalArgumentException illegalArgumentException) {
        }
        catch (IllegalAccessException illegalAccessException) {
        }
        catch (InvocationTargetException invocationTargetException) {}
        return null;
    }
}

