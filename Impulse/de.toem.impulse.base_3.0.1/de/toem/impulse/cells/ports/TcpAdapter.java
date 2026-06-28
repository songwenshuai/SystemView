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
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;

@CellAnnotation(type="port.record.socket", dynamicChildOf={"port.records", "preferences.impulse.ports"})
public class TcpAdapter
extends AbstractStreamSerializerAdapter {
    public static final String TYPE = "port.record.socket";
    public static final int MODE_CLIENT = 0;
    public static final int MODE_CLIENT_WAIT_FOR_SERVER = 1;
    public static final int MODE_SERVER = 2;
    public int mode = 0;
    public String server = "localhost";
    public int socket = 5000;
    public boolean enableStimulation;
    public String stimulationScript;
    public String stimulationScriptLanguage;
    public String logPath;

    public String definition() {
        return String.valueOf(String.valueOf(this.server)) + ":" + String.valueOf(this.socket);
    }

    @Override
    public void provideToScriptContext(IScriptContextProvider.IScriptContextInterface context) {
        super.provideToScriptContext(context);
        if (Utils.equals(context.getContextId(), "stimulationScript")) {
            DefaultScriptContextProvider.provideDefaultScriptContext(context, false, false, true, false, false, true);
            context.addSymbol("socket", Socket.class);
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
        if (this.server == null) {
            ImpulseBase.defaultConsoleStream().println(String.valueOf(new String(this.name)) + ": " + I18n.Adapter_ServerIsEmtpy);
            return false;
        }
        if (this.socket <= 0) {
            ImpulseBase.defaultConsoleStream().println(String.valueOf(new String(this.name)) + ": " + I18n.Adapter_SocketIsInvalid);
            return false;
        }
        return true;
    }

    @Override
    public InputStream getInput(IProgress iProgress) {
        return TcpAdapter.getSocketInput(iProgress, this.name, this.server, this.socket, this.mode, this.enableStimulation, this.stimulationScript, this.logPath, this);
    }

    @Override
    public int getNature() {
        SerializerDescriptor descr = (SerializerDescriptor)Elements.serializers.get(this.serializer);
        int nature = 0;
        if (descr != null) {
            if (descr.supports(4096, null)) {
                nature |= 3;
            }
            if (descr.supports(8192, null)) {
                nature |= 0x20;
            }
        }
        return nature;
    }

    public static InputStream getSocketInput(final IProgress iProgress, final String name, final String server, final int port, final int mode, final boolean enableStimulation, String stimulationScript, String logPath, final IScriptContextProvider scriptContextProvider) {
        return new LateInputStream(logPath != null ? new File(logPath) : null){
            ServerSocket serverSocket;
            Socket socket;
            InputStream in;
            OutputStream out;
            {
                super($anonymous0);
                this.serverSocket = null;
                this.socket = null;
                this.in = null;
                this.out = null;
            }

            @Override
            public void close() throws IOException {
                super.close();
                if (this.serverSocket != null) {
                    this.serverSocket.close();
                }
                this.serverSocket = null;
            }

            @Override
            protected InputStream prepareStream() {
                if (mode == 0 || mode == 1) {
                    while (this.socket == null && !this.closed) {
                        try {
                            this.socket = new Socket(server, port);
                            this.in = this.socket.getInputStream();
                            this.out = this.socket.getOutputStream();
                        }
                        catch (Throwable e) {
                            if (this.socket == null && mode == 1) {
                                Actives.sleep(500);
                                continue;
                            }
                            this.closed = true;
                            Actives.runInMain(new IExecutable(){

                                @Override
                                public void execute(IProgress p) {
                                    ImpulseBase.defaultConsoleStream().println(String.valueOf(new String(name)) + ": " + I18n.Adapter_CouldNotCreateSocket + " -> " + e.getLocalizedMessage());
                                }
                            });
                        }
                    }
                } else if (mode == 2) {
                    try {
                        this.serverSocket = new ServerSocket(port);
                        this.socket = this.serverSocket.accept();
                        this.in = this.socket.getInputStream();
                        this.out = this.socket.getOutputStream();
                    }
                    catch (Throwable throwable) {}
                }
                if (this.socket != null) {
                    try {
                        if (enableStimulation) {
                            IExecutable stimulation = new IExecutable(){

                                @Override
                                public void execute(IProgress p) {
                                    IScripting scripting = Scripting.create(scriptContextProvider, "stimulationScript", s -> {
                                        s.setSymbol("socket", socket);
                                        s.setSymbol("out", out);
                                        s.setSymbol("log", log);
                                    });
                                    scripting.run(iProgress);
                                }
                            };
                            Actives.run(stimulation);
                        }
                        return this.in;
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

