/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.ports;

import de.toem.impulse.ImpulseBase;
import de.toem.impulse.cells.ports.AbstractStreamSerializerAdapter;
import de.toem.impulse.i18n.I18n;
import de.toem.impulse.scripting.DefaultScriptContextProvider;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.CellAnnotation;
import de.toem.toolkits.pattern.element.Elements;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.serializer.SerializerDescriptor;
import de.toem.toolkits.pattern.scripting.IScriptContextProvider;
import de.toem.toolkits.pattern.scripting.IScripting;
import de.toem.toolkits.pattern.scripting.Scripting;
import de.toem.toolkits.pattern.threading.Actives;
import de.toem.toolkits.pattern.threading.IExecutable;
import de.toem.toolkits.pattern.threading.IProgress;
import de.toem.toolkits.storage.LateInputStream;
import java.io.File;
import java.io.InputStream;
import java.io.OutputStream;

@CellAnnotation(type="port.record.process", dynamicChildOf={"port.records", "preferences.impulse.ports"})
public class ProcessAdapter
extends AbstractStreamSerializerAdapter {
    public static final String TYPE = "port.record.process";
    public String path;
    public String command = "";
    public String parameter;
    public boolean enableStimulation;
    public String stimulationScript;
    public String stimulationScriptLanguage;
    public String logPath;

    public String definition() {
        return this.command != null ? this.command : "";
    }

    @Override
    public void provideToScriptContext(IScriptContextProvider.IScriptContextInterface context) {
        super.provideToScriptContext(context);
        if (Utils.equals(context.getContextId(), "stimulationScript")) {
            DefaultScriptContextProvider.provideDefaultScriptContext(context, false, false, true, false, false, true);
            context.addSymbol("process", Process.class);
            context.addSymbol("out", OutputStream.class);
            context.addSymbol("log", OutputStream.class);
            context.setScript(this.stimulationScript, this.stimulationScriptLanguage);
        }
    }

    @Override
    public boolean validate(ICell insertPoint) {
        if (!super.validate(insertPoint)) {
            return false;
        }
        if (Utils.isEmpty(this.command)) {
            ImpulseBase.defaultConsoleStream().println(String.valueOf(new String(this.name)) + ": " + I18n.Adapter_CommandIsEmpty);
            return false;
        }
        return true;
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

    @Override
    public InputStream getInput(IProgress iProgress) {
        try {
            ProcessBuilder builder = new ProcessBuilder(this.command, this.parameter != null ? this.parameter : "");
            if (this.path != null) {
                builder.directory(new File(this.path));
            }
            Process process = builder.start();
            return ProcessAdapter.getProcessInput(iProgress, this.name, process, this.enableStimulation, this.stimulationScript, this.logPath, this);
        }
        catch (Throwable e) {
            ImpulseBase.defaultConsoleStream().println(String.valueOf(new String(this.name)) + ": " + I18n.Adapter_CouldNotCreateProcess + "->" + e.getLocalizedMessage());
            return null;
        }
    }

    public static InputStream getProcessInput(final IProgress iProgress, final String name, final Process process, final boolean enableStimulation, String stimulationScript, String logPath, final IScriptContextProvider scriptContextProvider) {
        return new LateInputStream(logPath != null ? new File(logPath) : null){

            @Override
            protected InputStream prepareStream() {
                if (process != null) {
                    try {
                        InputStream in = process.getInputStream();
                        if (enableStimulation) {
                            IExecutable stimulation = new IExecutable(){

                                @Override
                                public void execute(IProgress p) {
                                    IScripting scripting = Scripting.create(scriptContextProvider, "stimulationScript", s -> {
                                        s.setSymbol("process", process);
                                        s.setSymbol("out", process.getOutputStream());
                                        s.setSymbol("log", log);
                                    });
                                    scripting.run(iProgress);
                                }
                            };
                            Actives.run(stimulation);
                        }
                        return in;
                    }
                    catch (Throwable e) {
                        Actives.runInMain(new IExecutable(){

                            @Override
                            public void execute(IProgress p) {
                                ImpulseBase.defaultConsoleStream().println(String.valueOf(new String(name)) + ": " + I18n.Adapter_CouldNotCreateInput + " -> " + e.getLocalizedMessage());
                            }
                        });
                    }
                }
                return null;
            }
        };
    }
}

