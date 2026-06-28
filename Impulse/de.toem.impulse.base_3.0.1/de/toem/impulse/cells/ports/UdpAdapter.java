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
import de.toem.toolkits.pattern.systemLog.SystemLog;
import de.toem.toolkits.pattern.threading.Actives;
import de.toem.toolkits.pattern.threading.IExecutable;
import de.toem.toolkits.pattern.threading.IProgress;
import de.toem.toolkits.storage.LateInputStream;
import java.io.Closeable;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.PipedInputStream;
import java.io.PipedOutputStream;
import java.net.DatagramPacket;
import java.net.DatagramSocket;

@CellAnnotation(type="port.record.udp", dynamicChildOf={"port.records", "preferences.impulse.ports"})
public class UdpAdapter
extends AbstractStreamSerializerAdapter {
    public static final String TYPE = "port.record.udp";
    public static final int MODE_NORMAL = 0;
    public static final int MODE_MANUAL_FEED = 1;
    public int mode = 0;
    public int socket = 5000;
    public boolean enableStimulation;
    public String stimulationScript;
    public String stimulationScriptLanguage;
    public String logPath;

    public String definition() {
        return String.valueOf(this.socket);
    }

    @Override
    public void provideToScriptContext(IScriptContextProvider.IScriptContextInterface context) {
        super.provideToScriptContext(context);
        if (Utils.equals(context.getContextId(), "stimulationScript")) {
            DefaultScriptContextProvider.provideDefaultScriptContext(context, false, false, true, false, false, true);
            context.addClasses(DatagramSocket.class, DatagramPacket.class);
            context.addSymbol("socket", DatagramSocket.class);
            context.addSymbol("feed", OutputStream.class);
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
        if (this.socket <= 0) {
            ImpulseBase.defaultConsoleStream().println(String.valueOf(new String(this.name)) + ": " + I18n.Adapter_SocketIsInvalid);
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
                nature |= 3;
            }
            if (descr.supports(8192, null)) {
                nature |= 0x20;
            }
        }
        return nature;
    }

    @Override
    public Closeable getInput(IProgress iProgress) {
        try {
            DatagramSocket datagramSocket = new DatagramSocket(this.socket);
            return UdpAdapter.getSocketInput(iProgress, this.name, datagramSocket, this.mode, this.enableStimulation, this.stimulationScript, this.logPath, this);
        }
        catch (Throwable ex) {
            ImpulseBase.defaultConsoleStream().println(String.valueOf(new String(this.name)) + ": Could not create datagram socket:" + ex.getLocalizedMessage());
            return null;
        }
    }

    public static InputStream getSocketInput(final IProgress iProgress, final String name, final DatagramSocket datagramSocket, final int mode, final boolean enableStimulation, String stimulationScript, String logPath, final IScriptContextProvider scriptContextProvider) {
        return new LateInputStream(logPath != null ? new File(logPath) : null){

            @Override
            protected InputStream prepareStream() {
                if (datagramSocket != null) {
                    try {
                        final UdpInput in = new UdpInput(datagramSocket);
                        if (in != null && mode == 0) {
                            in.listen();
                        }
                        if (enableStimulation) {
                            IExecutable stimulation = new IExecutable(){

                                @Override
                                public void execute(IProgress p) {
                                    IScripting scripting = Scripting.create(scriptContextProvider, "stimulationScript", s -> {
                                        s.setSymbol("socket", datagramSocket);
                                        s.setSymbol("feed", in.getOutput());
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

    static class UdpInput
    extends PipedInputStream {
        DatagramSocket datagramSocket;
        boolean closed = false;
        PipedOutputStream output = new PipedOutputStream();

        public UdpInput(DatagramSocket datagramSocket) {
            this.datagramSocket = datagramSocket;
            this.output = new PipedOutputStream();
            try {
                this.output.connect(this);
            }
            catch (IOException iOException) {}
        }

        public OutputStream getOutput() {
            return this.output;
        }

        public void listen() {
            Actives.run(new IExecutable(){

                @Override
                public void execute(IProgress p) {
                    try {
                        DatagramPacket packet = new DatagramPacket(new byte[65536], 65536);
                        while (true) {
                            datagramSocket.receive(packet);
                            if (packet.getLength() <= 0) continue;
                            output.write(packet.getData(), 0, packet.getLength());
                        }
                    }
                    catch (IOException iOException) {
                        return;
                    }
                }
            });
        }

        @Override
        public void close() {
            try {
                if (this.closed) {
                    return;
                }
                this.closed = true;
                this.datagramSocket.close();
                this.output.close();
            }
            catch (Throwable e) {
                SystemLog.log(e);
            }
            try {
                super.close();
            }
            catch (Throwable throwable) {}
        }
    }
}

