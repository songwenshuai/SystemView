/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.serializer;

import de.toem.impulse.serializer.AbstractMultiInputReader;
import de.toem.toolkits.pattern.element.Elements;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.ICover;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.element.serializer.ParseException;
import de.toem.toolkits.pattern.element.serializer.SerializerDescriptor;
import de.toem.toolkits.pattern.threading.IProgress;
import java.io.BufferedReader;
import java.io.File;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public abstract class AbstractMultiElementReader
extends AbstractMultiInputReader {
    protected List<String> references;
    protected Map<String, IElement> elements;

    public AbstractMultiElementReader() {
    }

    public AbstractMultiElementReader(String id, InputStream in) {
        super(id, in);
    }

    @Override
    protected void parse(IProgress progress, InputStream in, ICell base) throws Throwable {
        String line;
        BufferedReader reader = new BufferedReader(new InputStreamReader(in));
        this.references = new ArrayList<String>();
        while (!((line = reader.readLine()) == null || progress != null && progress.isCanceled())) {
            if (line.trim().isEmpty()) continue;
            this.references.add(line);
        }
        this.elements = new HashMap<String, IElement>();
        for (String ref : this.references) {
            File file;
            if (progress != null && progress.isCanceled()) continue;
            IElement element = Elements.getElement(ref);
            if (!element.isBound() && (file = new File(ref)).exists()) {
                element = Elements.getElement(file);
            }
            if (!element.isBound()) {
                throw new ParseException("Can not find " + ref);
            }
            this.elements.put(ref, element);
        }
    }

    protected ICover read(IProgress progress, String label, IElement element, ICell base) throws ParseException {
        InputStream in = element.getResourceData(null);
        if (in == null) {
            throw new ParseException("Can not get input stream " + element.getName());
        }
        SerializerDescriptor serializer = element.getReader(in);
        if (serializer == null) {
            throw new ParseException("Can not find serializer" + element.getName());
        }
        this.current = serializer.newReader(in);
        if (progress != null) {
            progress.doing(String.valueOf(label) + " " + element.getName());
        }
        return this.current.read(progress, null, base, base != null ? 2 : 0);
    }
}

