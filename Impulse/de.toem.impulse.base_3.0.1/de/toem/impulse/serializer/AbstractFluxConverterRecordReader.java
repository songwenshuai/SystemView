/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.serializer;

import de.toem.impulse.serializer.base.FluxReader;
import de.toem.toolkits.core.Platform;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.bundles.Bundles;
import de.toem.toolkits.pattern.element.serializer.IUriStream;
import de.toem.toolkits.pattern.element.serializer.ParseException;
import de.toem.toolkits.pattern.threading.Actives;
import de.toem.toolkits.pattern.threading.IProgress;
import de.toem.toolkits.storage.TemporaryStorage;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.URI;

public abstract class AbstractFluxConverterRecordReader
extends FluxReader {
    protected URI inputLocation;
    protected URI fluxLocation;
    protected Process p;
    protected InputStream pin;
    protected OutputStream pout;
    protected InputStream perr;
    protected boolean killProcessAfterParse;

    public AbstractFluxConverterRecordReader() {
    }

    public AbstractFluxConverterRecordReader(String id, InputStream in) {
        super(id, in);
    }

    protected void finalize() throws Throwable {
        if (this.p != null) {
            this.p.destroyForcibly();
        }
        super.finalize();
    }

    protected abstract String getBundleId();

    protected abstract String getFluxLocation();

    protected abstract String getFluxName();

    protected abstract String getSharedLocation();

    protected void createProcess(IProgress progress, InputStream in) throws ParseException {
        try {
            if (in instanceof IUriStream) {
                this.inputLocation = ((IUriStream)((Object)in)).getLocation();
            } else {
                OutputStream output = TemporaryStorage.getOutput(4, null, this.toString(), false);
                Utils.write(output, in);
                File file = TemporaryStorage.getFile(4, null, this.toString());
                this.inputLocation = file.toURI();
            }
        }
        catch (Throwable e) {
            throw new ParseException("Could not determine the source input location: " + this.inputLocation, e);
        }
        String loc = "";
        try {
            loc = String.valueOf(this.getFluxLocation()) + Platform.getExecutableSubEntryPath(this.getFluxName());
            this.fluxLocation = Bundles.getBundleEntryAsFileUri(this.getBundleId(), loc);
        }
        catch (Throwable e) {
            throw new ParseException("Could not determine the native executable location: " + loc + ". Please check if the native part is installed and configured correctly (Preferences->impulse->Native Extensions->" + this.getFluxName().toUpperCase() + "Native)", e);
        }
        if (this.fluxLocation == null) {
            throw new ParseException("Could not determine the native executable location: " + loc + ". Please check if the native part is installed and configured correctly (Preferences->impulse->Native Extensions->" + this.getFluxName().toUpperCase() + "Native)");
        }
        String[] command = new String[]{this.fluxLocation.getPath(), this.inputLocation != null ? new File(this.inputLocation).getAbsolutePath() : ""};
        try {
            ProcessBuilder builder = new ProcessBuilder(command);
            if (!Utils.isEmpty(this.getSharedLocation())) {
                String ld = System.getenv().get("LD_LIBRARY_PATH");
                builder.environment().put("LD_LIBRARY_PATH", String.valueOf(ld != null ? String.valueOf(ld) + ":" : "") + this.getSharedLocation());
                ld = System.getenv().get("PATH");
                builder.environment().put("PATH", String.valueOf(ld != null ? String.valueOf(ld) + ";" : "") + this.getSharedLocation());
            }
            this.p = builder.start();
            this.pin = this.p.getInputStream();
            this.pout = this.p.getOutputStream();
            this.perr = this.p.getErrorStream();
        }
        catch (Throwable e) {
            throw new ParseException("Could not start the native executable process :" + command[0] + " - " + command[1], e);
        }
    }

    @Override
    protected boolean prepare(IProgress progress, InputStream in) throws ParseException {
        this.createProcess(progress, in);
        return true;
    }

    @Override
    protected void parse(IProgress progress, InputStream in) throws ParseException {
        super.parse(progress, this.pin);
        try {
            if (this.perr != null && this.perr.available() > 0) {
                String err;
                Actives.sleep(200);
                if (!this.p.isAlive() && this.p.exitValue() >= 1 && !Utils.isEmpty(err = Utils.readStringFromInputStream(this.perr, null, false))) {
                    throw new ParseException("Converter reported error: " + err + "; ExitValue=" + this.p.exitValue());
                }
            }
        }
        catch (IOException iOException) {}
    }

    @Override
    protected void postpare(IProgress progress) throws ParseException {
        try {
            if (this.killProcessAfterParse && this.p != null && this.p != null) {
                this.p.destroyForcibly();
            }
        }
        catch (Throwable throwable) {}
    }
}

