/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.serializer.base;

import de.toem.impulse.ImpulseBase;
import de.toem.impulse.cells.serializer.ReaderConfiguration;
import de.toem.impulse.samples.ISingleDomainRecordGenerator;
import de.toem.impulse.scripting.DefaultScriptContextProvider;
import de.toem.impulse.serializer.AbstractSingleDomainRecordReader;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.serializer.IUriStream;
import de.toem.toolkits.pattern.element.serializer.ParseException;
import de.toem.toolkits.pattern.properties.IPropertyModel;
import de.toem.toolkits.pattern.properties.PropertyModel;
import de.toem.toolkits.pattern.scripting.IScriptContextProvider;
import de.toem.toolkits.pattern.scripting.IScripting;
import de.toem.toolkits.pattern.scripting.Scripting;
import de.toem.toolkits.pattern.threading.IProgress;
import java.io.File;
import java.io.InputStream;
import java.net.URI;

public class RecJsReader
extends AbstractSingleDomainRecordReader {
    private long current;
    int changed;

    public RecJsReader() {
    }

    public RecJsReader(String id, InputStream in) {
        super(id, in);
    }

    public static IPropertyModel getPropertyModel() {
        return RecJsReader.getDefaultPropertyModel();
    }

    public static IPropertyModel getPropertyModel(Class<?> cs) {
        return new PropertyModel().add("p0", "", "P0", null, null).add("p1", "", "P1", null, null).add("p2", "", "P2", null, null).add("p3", "", "P3", null, null).add("p4", "", "P4", null, null).add("p5", "", "P5", null, null).add("p6", "", "P6", null, null).add("p7", "", "P7", null, null).add("p8", "", "P8", null, null).add("p9", "", "P9", null, null);
    }

    @Override
    protected int isApplicable(String name, String contentType) {
        if (name != null && !name.endsWith(".recJs")) {
            return -1;
        }
        return 10;
    }

    @Override
    protected int isApplicable(byte[] buffer) {
        if (new String(buffer, 0, buffer.length).toLowerCase().contains("recjs")) {
            return 1;
        }
        return -1;
    }

    @Override
    protected void parse(IProgress progress, InputStream in) throws ParseException {
        String script = Utils.readStringFromInputStream(in, null, true);
        ParseException[] exception = new ParseException[1];
        IScripting scripting = Scripting.create(RecJsReader.getScriptContextProvider(script, null), "script", s -> {
            URI location;
            s.setSymbol("generator", this);
            if (this.configurationProperties != null) {
                for (String key : this.configurationProperties.keys()) {
                    s.setSymbol(key, this.configurationProperties.get(key));
                }
            }
            if (in instanceof IUriStream && (location = ((IUriStream)((Object)in)).getLocation()) != null) {
                s.setSymbol("file", new File(location));
            }
            s.onException(e -> {
                ParseException parseException = new ParseException(0, e);
            });
        });
        scripting.run(progress);
        if (exception[0] != null) {
            throw exception[0];
        }
    }

    public static IScriptContextProvider getScriptContextProvider(final String script, final String scriptLanguage) {
        IScriptContextProvider contextProvider = new IScriptContextProvider(){

            @Override
            public void provideToScriptContext(IScriptContextProvider.IScriptContextInterface context) {
                DefaultScriptContextProvider.provideDefaultScriptContext(context, true, true, false, false, true, true);
                context.addSymbol("generator", ISingleDomainRecordGenerator.class);
                context.addSymbol("file", File.class);
                IPropertyModel model = RecJsReader.getPropertyModel(ReaderConfiguration.class);
                for (String key : model.keys()) {
                    context.addSymbol(key, String.class);
                }
                context.setScript(script, scriptLanguage);
                context.setLoader(ImpulseBase.getClassLoader());
            }
        };
        return contextProvider;
    }
}

