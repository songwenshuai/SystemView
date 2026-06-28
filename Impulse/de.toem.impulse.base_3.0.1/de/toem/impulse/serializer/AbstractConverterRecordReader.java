/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.serializer;

import de.toem.impulse.ImpulseLicense;
import de.toem.impulse.cells.record.Record;
import de.toem.impulse.serializer.AbstractRecordReader;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.Cover;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.ICover;
import de.toem.toolkits.pattern.element.serializer.ICellReader;
import de.toem.toolkits.pattern.element.serializer.IUriStream;
import de.toem.toolkits.pattern.element.serializer.Message;
import de.toem.toolkits.pattern.ide.Ide;
import de.toem.toolkits.pattern.threading.Actives;
import de.toem.toolkits.pattern.threading.IExecutable;
import de.toem.toolkits.pattern.threading.IProgress;
import de.toem.toolkits.storage.TemporaryStorage;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.URI;

public abstract class AbstractConverterRecordReader
extends AbstractRecordReader {
    protected ICellReader innerReader;
    protected URI location;

    public AbstractConverterRecordReader() {
    }

    public AbstractConverterRecordReader(String id, InputStream in) {
        super(id, in);
    }

    protected abstract ICellReader createReader(String var1, InputStream var2);

    protected String getCommand() {
        return this.properties.get("command");
    }

    protected String getPath() {
        return this.properties.get("path");
    }

    protected String[] createCommand(URI location) {
        String[] command = null;
        if (!Utils.isEmpty(this.getCommand())) {
            command = this.getCommand().split(" ");
            if (!Utils.isEmpty(this.getPath())) {
                command[0] = String.valueOf(this.getPath()) + File.separator + command[0];
            }
            int n = 0;
            while (n < command.length) {
                command[n] = command[n].replace("%f", new File(location).getAbsolutePath());
                ++n;
            }
        }
        return command;
    }

    protected Process createProcess(IProgress progress, InputStream in) throws IOException {
        URI location = null;
        if (in instanceof IUriStream) {
            location = ((IUriStream)((Object)in)).getLocation();
        } else {
            OutputStream output = TemporaryStorage.getOutput(4, null, this.toString(), false);
            Utils.write(output, in);
            File file = TemporaryStorage.getFile(4, null, this.toString());
            location = file.toURI();
        }
        final String[] command = this.createCommand(location);
        try {
            return Runtime.getRuntime().exec(command);
        }
        catch (IOException e) {
            Actives.runInMain(new IExecutable(){

                @Override
                public void execute(IProgress p) {
                    Ide.openError("Converter process creation failed", "The converter process \"" + command[0] + "\" could not be started!\nPlease check preferences and converter installation!\n[" + e.getLocalizedMessage() + "]");
                }
            }, 100);
            return null;
        }
    }

    @Override
    public ICover read(IProgress progress, String configurationName, ICell base, int insert) {
        InputStream input = null;
        Record record = new Record();
        Cover cover = new Cover(record);
        cover.setSerializer(this.id);
        if (ImpulseLicense.isFeatureLocked("de.toem.impulse.featureDomain.serializer", "de.toem.impulse.feature.default", this.id)) {
            base.insertChild(new Message("Locked serializer", 3, "The selected serializer is locked!"), 0);
            return cover;
        }
        try {
            Process process;
            if (this.configuration == null) {
                this.configuration = this.findConfiguration(configurationName);
            }
            if (this.configuration != null) {
                cover.setConfiguration(this.configuration.getName());
                this.configurationProperties = this.readConfigurationProperties(this.configuration, this.properties);
            }
            if ((process = this.createProcess(progress, this.in)) != null) {
                input = process.getInputStream();
                this.innerReader = this.createReader(this.id, input);
                if (input != null && this.innerReader != null) {
                    return this.innerReader.read(progress, configurationName, base, insert);
                }
            }
        }
        catch (Throwable e) {
            AbstractConverterRecordReader.addParseErrorMessage(this.id, e, record);
        }
        return cover;
    }
}

