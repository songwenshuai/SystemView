/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.ports;

import de.toem.impulse.ImpulseBase;
import de.toem.impulse.cells.ports.AbstractStreamSerializerAdapter;
import de.toem.impulse.cells.ports.MultiPipePort;
import de.toem.impulse.cells.record.PortScope;
import de.toem.impulse.cells.record.Record;
import de.toem.impulse.i18n.I18n;
import de.toem.impulse.scripting.DefaultScriptContextProvider;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.CellAnnotation;
import de.toem.toolkits.pattern.element.Elements;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.element.serializer.SerializerDescriptor;
import de.toem.toolkits.pattern.scripting.IScriptContextProvider;
import de.toem.toolkits.pattern.scripting.IScripting;
import de.toem.toolkits.pattern.scripting.Scripting;
import de.toem.toolkits.pattern.threading.Actives;
import de.toem.toolkits.pattern.threading.IExecutable;
import de.toem.toolkits.pattern.threading.IProgress;
import de.toem.toolkits.storage.KeepReadingFileInputStream;
import de.toem.toolkits.storage.LateInputStream;
import java.io.Closeable;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

@CellAnnotation(type="port.record.pipe", dynamicChildOf={"port.records", "preferences.impulse.ports", "port.multi.pipe"})
public class PipeAdapter
extends AbstractStreamSerializerAdapter {
    public static final String TYPE = "port.record.pipe";
    public static final int MODE_NORMAL = 0;
    public static final int MODE_READ_UNTIL_CLOSE = 1;
    public int mode = 0;
    public String path;
    public boolean enableStimulation;
    public String stimulationScript;
    public String stimulationScriptLanguage;

    public String definition() {
        return this.path != null ? this.path : "";
    }

    @Override
    public void provideToScriptContext(IScriptContextProvider.IScriptContextInterface context) {
        super.provideToScriptContext(context);
        if (Utils.equals(context.getContextId(), "stimulationScript")) {
            DefaultScriptContextProvider.provideDefaultScriptContext(context, false, false, true, false, false, true);
            context.addSymbol("file", File.class);
            context.addSymbol("log", OutputStream.class);
            context.setScript(this.stimulationScript, this.stimulationScriptLanguage);
        }
    }

    String path() {
        if (this.path == null) {
            return null;
        }
        if (this.getParent() instanceof MultiPipePort && !this.path.trim().startsWith(File.separator) && !Utils.isEmpty(((MultiPipePort)this.getParent()).pathBase)) {
            return String.valueOf(((MultiPipePort)this.getParent()).pathBase) + File.separator + this.path;
        }
        return this.path;
    }

    @Override
    public boolean validate(ICell insertPoint) {
        if (!super.validate(insertPoint)) {
            return false;
        }
        if (Utils.isEmpty(this.path())) {
            return false;
        }
        File file = new File(this.path());
        if (!file.exists()) {
            ImpulseBase.defaultConsoleStream().println(String.valueOf(new String(this.name)) + ": " + I18n.Adapter_FileDoesNotExist);
            return false;
        }
        return true;
    }

    @Override
    public Closeable getInput(final IProgress iProgress) {
        final File file = new File(this.path());
        final int mode = this.mode;
        if (file.exists()) {
            LateInputStream in = new LateInputStream(){

                @Override
                protected InputStream prepareStream() {
                    if (mode == 0) {
                        return Utils.getInput(file);
                    }
                    if (mode == 1) {
                        try {
                            return new KeepReadingFileInputStream(file){

                                @Override
                                public boolean keepReading() {
                                    return !closed;
                                }
                            };
                        }
                        catch (FileNotFoundException fileNotFoundException) {}
                    }
                    return null;
                }
            };
            if (in != null && this.enableStimulation) {
                IExecutable stimulation = new IExecutable(){

                    @Override
                    public void execute(IProgress p) {
                        IScripting scripting = Scripting.create(PipeAdapter.this, "stimulationScript", s -> s.setSymbol("file", file));
                        scripting.run(iProgress);
                    }
                };
                Actives.run(stimulation);
            }
            return in;
        }
        return null;
    }

    @Override
    public int getNature() {
        SerializerDescriptor descr = (SerializerDescriptor)Elements.serializers.get(this.serializer);
        int nature = 0;
        if (descr != null) {
            if (descr.supports(4096, null)) {
                nature |= 0x23;
            }
            if (descr.supports(8192, null)) {
                nature |= 0x20;
            }
        }
        return nature;
    }

    IElement resourceElement() {
        File file = new File(this.path());
        if (file.exists()) {
            return Elements.getElement(file);
        }
        return IElement.NONE;
    }

    @Override
    public void prepareInsertPoint(ICell insertPoint) {
        IElement element;
        if (insertPoint instanceof PortScope && (element = this.resourceElement()).isBound() && element.hasCell(Record.class)) {
            ((PortScope)insertPoint).source = element.getLink();
        }
    }

    public static void main(String[] args) {
        OutputStream out = Utils.getOutput(new File("/home/thomas/Schreibtisch/out.lines"));
        int n = 0;
        while (true) {
            try {
                while (true) {
                    out.write(("lines written" + n + "\n").getBytes());
                    try {
                        Thread.sleep(100L);
                    }
                    catch (InterruptedException interruptedException) {}
                }
            }
            catch (IOException iOException) {
                continue;
            }
            break;
        }
    }
}

