/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.serializer.base;

import de.toem.impulse.cells.serializer.ScriptedReaderConfiguration;
import de.toem.impulse.serializer.AbstractSingleDomainRecordReader;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.serializer.IInputRequest;
import de.toem.toolkits.pattern.element.serializer.IUriStream;
import de.toem.toolkits.pattern.element.serializer.ParseException;
import de.toem.toolkits.pattern.properties.IPropertyModel;
import de.toem.toolkits.pattern.properties.PropertyModel;
import de.toem.toolkits.pattern.scripting.IScripting;
import de.toem.toolkits.pattern.scripting.Scripting;
import de.toem.toolkits.pattern.threading.Actives;
import de.toem.toolkits.pattern.threading.IProgress;
import java.io.File;
import java.io.InputStream;
import java.net.URI;

public class ScriptedReader
extends AbstractSingleDomainRecordReader {
    private IScripting scripting;

    public ScriptedReader() {
    }

    public ScriptedReader(String id, InputStream in) {
        super(id, in);
    }

    public static IPropertyModel getPropertyModel(Class<?> cs) {
        if (cs != null) {
            return new PropertyModel().add("streamReader", "false", "Stream Reader", null, null).add("scriptStructure", "flat", "Script Structure", new String[]{"flat", "methods"}, null);
        }
        return ScriptedReader.getDefaultPropertyModel();
    }

    protected void finalize() throws Throwable {
        if (this.scripting != null && this.scripting.hasFunction("finalize")) {
            this.scripting.invoke("finalize", new Object[0]);
        }
        super.finalize();
    }

    @Override
    protected int isApplicable(String name, String contentType, IInputRequest request) {
        if (this.preferences != null) {
            for (ICell iCell : this.preferences.getChildren(ScriptedReaderConfiguration.class)) {
                int a;
                IScripting scripting;
                IPropertyModel configurationProperties;
                ScriptedReaderConfiguration configuration;
                if (!((ScriptedReaderConfiguration)iCell).enabled || (configuration = (ScriptedReaderConfiguration)iCell).isApplicable(name) == -1 || (configurationProperties = this.readConfigurationProperties(configuration, null)) == null || !"methods".equals(configurationProperties.get("scriptStructure")) || !configuration.script.contains("isApplicable") || (scripting = Scripting.create(configuration, null, s -> {})) == null) continue;
                Object result = null;
                try {
                    result = Actives.timeout(p -> {
                        scripting.run(p);
                        if (scripting.hasFunction("isApplicable")) {
                            return scripting.invoke("isApplicable", name, contentType, request);
                        }
                        return 0;
                    }, 1000);
                }
                catch (Throwable throwable) {}
                int n = a = result instanceof Number ? ((Number)result).intValue() : -1;
                if (a != 1) continue;
                return 1;
            }
        }
        return 0;
    }

    @Override
    public boolean supportsStreaming() {
        return true;
    }

    @Override
    protected void parse(IProgress progress, InputStream in) throws ParseException {
        if (!(this.configuration instanceof ScriptedReaderConfiguration)) {
            throw new ParseException(-1, "Configuration required");
        }
        ParseException[] exception = new ParseException[1];
        this.scripting = Scripting.create((ScriptedReaderConfiguration)this.configuration, null, s -> {
            URI location;
            s.setSymbol("thiz", this);
            s.setSymbol("generator", this);
            s.setSymbol("inputStream", in);
            if (in instanceof IUriStream && (location = ((IUriStream)((Object)in)).getLocation()) != null) {
                s.setSymbol("file", new File(location));
                s.setSymbol("uri", location);
            }
            if (this.configurationProperties != null) {
                for (String key : this.configurationProperties.keys()) {
                    s.setSymbol(key, this.configurationProperties.get(key));
                }
            }
            s.onException(e -> {
                ParseException parseException = new ParseException(0, e);
            });
        });
        this.scripting.run(progress);
        if (exception[0] != null) {
            throw exception[0];
        }
        if (this.scripting.hasFunction("parse")) {
            this.scripting.invoke("parse", progress, in);
        }
    }
}

