/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.serializer;

import de.toem.impulse.ImpulseBase;
import de.toem.impulse.cells.serializer.ReaderConfiguration;
import de.toem.impulse.cells.serializer.Serializer;
import de.toem.impulse.samples.ISingleDomainRecordGenerator;
import de.toem.impulse.scripting.DefaultScriptContextProvider;
import de.toem.impulse.serializer.IParsingRecordReader;
import de.toem.impulse.serializer.base.ScriptedReader;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.CellAnnotation;
import de.toem.toolkits.pattern.element.FieldAnnotation;
import de.toem.toolkits.pattern.properties.IPropertyModel;
import de.toem.toolkits.pattern.scripting.IScriptContextProvider;
import java.io.File;
import java.io.InputStream;
import java.net.URI;

@CellAnnotation(type="impulse.serializer.configuration.scripted", dynamicChildOf={"impulse.serializer"})
public class ScriptedReaderConfiguration
extends ReaderConfiguration
implements IScriptContextProvider {
    public static final String TYPE = "impulse.serializer.configuration.scripted";
    @FieldAnnotation(language="language")
    public String script;
    public String language;

    @Override
    public boolean supports(Serializer serializer) {
        return "de.toem.impulse.serializer.scripted".equals(serializer.id);
    }

    public String language() {
        return Utils.isEmpty(this.language) ? "js" : this.language;
    }

    @Override
    public void provideToScriptContext(IScriptContextProvider.IScriptContextInterface context) {
        DefaultScriptContextProvider.provideDefaultScriptContext(context, true, true, false, false, true, true);
        context.addSymbol("thiz", IParsingRecordReader.class, ISingleDomainRecordGenerator.class);
        context.addSymbol("generator", ISingleDomainRecordGenerator.class);
        context.addSymbol("inputStream", InputStream.class);
        context.addSymbol("file", File.class);
        context.addSymbol("uri", URI.class);
        IPropertyModel model = ScriptedReader.getPropertyModel(ReaderConfiguration.class);
        for (String key : model.keys()) {
            context.addSymbol(key, String.class);
        }
        context.setScript(this.script, this.language);
        context.setLoader(ImpulseBase.getClassLoader());
    }
}

