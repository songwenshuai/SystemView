/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.extension.embedded.serial;

import de.toem.impulse.ImpulseBase;
import de.toem.impulse.cells.ports.AbstractStreamSerializerAdapter;
import de.toem.impulse.extension.embedded.i18n.I18n;
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
import jssc.SerialPort;
import jssc.SerialPortException;

@CellAnnotation(type="port.record.serial", dynamicChildOf={"port.records", "preferences.impulse.ports"})
public class SerialAdapter
extends AbstractStreamSerializerAdapter {
    public static final String TYPE = "port.record.serial";
    public String port;
    public int baudRate = 9600;
    public int dataBits = 8;
    public int stopBits = 0;
    public int parity = 0;
    public boolean enableStimulation;
    public String stimulationScript;
    public String stimulationScriptLanguage;
    public String logPath;

    public String definition() {
        return String.valueOf(String.valueOf(this.baudRate)) + "/" + String.valueOf(this.dataBits) + "/" + String.valueOf(this.stopBits);
    }

    @Override
    public void provideToScriptContext(IScriptContextProvider.IScriptContextInterface context) {
        super.provideToScriptContext(context);
        if (Utils.equals(context.getContextId(), "stimulationScript")) {
            DefaultScriptContextProvider.provideDefaultScriptContext(context, false, false, true, false, false, true);
            context.addSymbol("serialPort", SerialPort.class);
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
        if (Utils.isEmpty(this.port)) {
            ImpulseBase.defaultConsoleStream().println(String.valueOf(new String(this.name)) + ": Port definition empty");
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
    public Closeable getInput(IProgress iProgress) {
        SerialPort serialPort = new SerialPort(this.port);
        try {
            if (serialPort.openPort()) {
                serialPort.setParams(this.baudRate, this.dataBits, this.stopBits + 1, this.parity);
                return SerialAdapter.getSerialInput(iProgress, this.name, serialPort, this.enableStimulation, this.stimulationScript, this.logPath, this);
            }
        }
        catch (SerialPortException ex) {
            ImpulseBase.defaultConsoleStream().println(String.valueOf(new String(this.name)) + ": Could not open serial port:" + ex.getLocalizedMessage());
        }
        return null;
    }

    public static InputStream getSerialInput(final IProgress iProgress, final String name, final SerialPort serialPort, final boolean enableStimulation, String stimulationScript, String logPath, final IScriptContextProvider scriptContextProvider) {
        return new LateInputStream(logPath != null ? new File(logPath) : null){

            @Override
            protected InputStream prepareStream() {
                if (serialPort != null) {
                    try {
                        SerialInput in = new SerialInput(serialPort);
                        if (enableStimulation) {
                            IExecutable stimulation = new IExecutable(){

                                @Override
                                public void execute(IProgress p) {
                                    IScripting scripting = Scripting.create(scriptContextProvider, "stimulationScript", s -> {
                                        s.setSymbol("serialPort", serialPort);
                                        s.setSymbol("out", new SerialOutput(serialPort));
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

    static class SerialInput
    extends InputStream {
        SerialPort serialPort;
        byte[] bytes;
        int pos;
        boolean closed = false;

        public SerialInput(SerialPort serialPort) {
            this.serialPort = serialPort;
        }

        @Override
        public int read() throws IOException {
            if (this.bytes == null) {
                this.pos = 0;
                try {
                    while (!this.closed && this.bytes == null) {
                        this.bytes = this.serialPort.readBytes();
                        Actives.sleep(100);
                    }
                }
                catch (Throwable throwable) {
                    return -1;
                }
            }
            if (this.bytes == null || this.pos >= this.bytes.length) {
                return -1;
            }
            int b = this.bytes[this.pos++] & 0xFF;
            if (this.pos >= this.bytes.length) {
                this.bytes = null;
            }
            return b;
        }

        @Override
        public int available() throws IOException {
            if (this.bytes == null) {
                this.pos = 0;
                try {
                    this.bytes = this.serialPort.readBytes();
                }
                catch (Throwable throwable) {}
            }
            return this.bytes == null ? 0 : this.bytes.length - this.pos;
        }

        @Override
        public int read(byte[] b, int off, int len) throws IOException {
            if (b == null) {
                throw new NullPointerException();
            }
            if (off < 0 || len < 0 || len > b.length - off) {
                throw new IndexOutOfBoundsException();
            }
            if (len == 0) {
                return 0;
            }
            if (this.bytes == null) {
                this.pos = 0;
                try {
                    while (!this.closed && this.bytes == null) {
                        this.bytes = this.serialPort.readBytes();
                        Actives.sleep(100);
                    }
                }
                catch (Throwable throwable) {
                    return -1;
                }
            }
            if (this.bytes == null) {
                return -1;
            }
            int i = Math.min(len, this.bytes.length - this.pos);
            System.arraycopy(this.bytes, this.pos, b, off, i);
            this.pos += i;
            if (this.pos >= this.bytes.length) {
                this.bytes = null;
            }
            return i;
        }

        @Override
        public void close() {
            try {
                if (this.closed) {
                    return;
                }
                this.closed = true;
                this.serialPort.closePort();
            }
            catch (SerialPortException e) {
                SystemLog.log(e);
            }
        }
    }

    static class SerialOutput
    extends OutputStream {
        SerialPort serialPort;
        byte[] bytes;
        int pos;
        boolean closed = false;

        public SerialOutput(SerialPort serialPort) {
            this.serialPort = serialPort;
        }

        @Override
        public void write(int arg0) throws IOException {
            try {
                this.serialPort.writeByte((byte)arg0);
            }
            catch (SerialPortException serialPortException) {}
        }
    }
}

