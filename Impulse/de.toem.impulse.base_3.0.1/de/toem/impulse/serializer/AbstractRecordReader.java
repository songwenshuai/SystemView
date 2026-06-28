/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.serializer;

import de.toem.impulse.ImpulseLicense;
import de.toem.impulse.ImpulsePreferences;
import de.toem.impulse.cells.preferences.ImpulseSerializers;
import de.toem.impulse.cells.serializer.ReaderConfiguration;
import de.toem.impulse.cells.serializer.Serializer;
import de.toem.impulse.serializer.IRecordReader;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.Cover;
import de.toem.toolkits.pattern.element.Elements;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.ICover;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.element.serializer.AbstractCellReader;
import de.toem.toolkits.pattern.element.serializer.SerializerDescriptor;
import de.toem.toolkits.pattern.properties.IPropertyModel;
import de.toem.toolkits.pattern.properties.PropertyModel;
import de.toem.toolkits.pattern.threading.IProgress;
import java.io.InputStream;

public abstract class AbstractRecordReader
extends AbstractCellReader
implements IRecordReader {
    protected Serializer preferences;
    protected IPropertyModel properties;
    protected ReaderConfiguration configuration;
    protected IPropertyModel configurationProperties;

    public AbstractRecordReader() {
    }

    public AbstractRecordReader(String id, InputStream in) {
        super(id, in);
        this.readPreferences(id);
    }

    public boolean supportsStreaming() {
        return false;
    }

    public boolean supportsPortIntro() {
        return false;
    }

    @Override
    public boolean supports(Object request, IElement target) {
        if (Integer.valueOf(4096).equals(request)) {
            return this.supportsStreaming();
        }
        if (Integer.valueOf(8192).equals(request)) {
            return this.supportsPortIntro();
        }
        return false;
    }

    @Override
    public final int isApplicable(String name, String contentType, String cellType, InputStream in) {
        int applicable;
        block10: {
            block9: {
                block8: {
                    block7: {
                        try {
                            if (this.preferences == null || this.preferences.enabled) break block7;
                            return -1;
                        }
                        catch (Throwable throwable) {}
                    }
                    if (cellType == null || cellType.equals("record")) break block8;
                    return -1;
                }
                if (!ImpulseLicense.isFeatureLocked("de.toem.impulse.featureDomain.serializer", "de.toem.impulse.feature.default", this.id)) break block9;
                return -1;
            }
            applicable = this.isApplicable(name, contentType, new AbstractCellReader.InputRequest(in));
            if (applicable > -1) break block10;
            return -1;
        }
        if (applicable == 1 || applicable == 0) {
            return applicable;
        }
        return -1;
    }

    private void readPreferences(String id) {
        if (ImpulsePreferences.serializerPreferences.isBound() && ImpulsePreferences.serializerPreferences.hasCell(ImpulseSerializers.class)) {
            for (ICell iCell : ImpulsePreferences.serializerPreferences.getCell().getChildren(Serializer.class)) {
                if (!((Serializer)iCell).id.equals(id)) continue;
                this.preferences = (Serializer)iCell;
                this.properties = ((SerializerDescriptor)Elements.serializers.get(id)).getPropertyModel();
                if (this.properties == null) break;
                this.properties.setTotal(this.preferences.parameters);
                break;
            }
        }
    }

    public static IPropertyModel getPropertyModel() {
        return new PropertyModel();
    }

    @Override
    public String getConfigurationName() {
        return this.configuration != null ? this.configuration.getName() : null;
    }

    protected ReaderConfiguration findConfiguration(String configurationName) {
        ReaderConfiguration configuration = null;
        if (this.preferences != null && !Utils.isEmpty(configurationName)) {
            for (ICell iCell : this.preferences.getChildren(ReaderConfiguration.class)) {
                if (!configurationName.equals(iCell.getName())) continue;
                configuration = (ReaderConfiguration)iCell;
                break;
            }
        }
        if (configuration == null && Utils.isEmpty(configurationName)) {
            return this.defaultConfiguration();
        }
        return configuration;
    }

    protected ReaderConfiguration defaultConfiguration() {
        return null;
    }

    protected IPropertyModel readConfigurationProperties(ReaderConfiguration configuration, IPropertyModel propecrties) {
        SerializerDescriptor serializer = (SerializerDescriptor)Elements.serializers.get(this.id);
        if (serializer != null && configuration != null) {
            IPropertyModel configurationProperties = ((SerializerDescriptor)Elements.serializers.get(this.id)).getPropertyModel(ReaderConfiguration.class);
            if (configurationProperties != null) {
                configurationProperties.setTotal(configuration.parameters);
                if (this.properties != null) {
                    for (String key : configurationProperties.keys()) {
                        if (!propecrties.containsKey(key)) continue;
                        this.properties.set(key, configurationProperties.get(key));
                    }
                }
            }
            return configurationProperties;
        }
        return null;
    }

    @Override
    public ICover read(IProgress progress, String configurationName) {
        return this.read(progress, configurationName, null, 0);
    }

    @Override
    public ICover readCover() {
        return new Cover("record");
    }

    @Override
    public int hasChanged() {
        return 0;
    }

    @Override
    public ICover flush() {
        return null;
    }
}

