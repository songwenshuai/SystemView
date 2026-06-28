/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.flux;

import de.toem.impulse.flux.FluxNative;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.toolkits.core.Platform;
import de.toem.toolkits.pattern.bundles.Bundles;
import de.toem.toolkits.pattern.ide.Ide;
import de.toem.toolkits.pattern.systemLog.SystemLog;
import de.toem.toolkits.ui.controller.base.ButtonController;
import de.toem.toolkits.ui.controller.base.ComboController;
import de.toem.toolkits.ui.controller.base.TextController;
import de.toem.toolkits.ui.part.dialog.AbstractElementDialog;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;
import de.toem.toolkits.ui.tlk.controls.ITlkComposite;
import java.io.File;

public class FluxNativeDialog
extends AbstractElementDialog {
    protected String BUNDLE_ID = "";
    protected String TARGET = "?";
    protected String FLUX_PATH;
    protected String FLUX_APPLICATION;
    protected String FLUX_MAKEFILE;
    protected String INCPATH_DESCR;
    protected String LIBPATH_DESCR;
    protected boolean REQUIRES_INCPATH;
    protected boolean REQUIRES_LIBPATH;

    public FluxNativeDialog(ITlkPartContainer parent, int style) {
        super(parent, style);
    }

    public FluxNativeDialog() {
    }

    @Override
    protected void createControls(ITlkComposite container) {
        super.createControls(container);
        try {
            if (this.REQUIRES_INCPATH) {
                this.tlk().addText(container, new TextController(this.editor(), FluxNative.class.getField("libPath")), this.cols() - 1, 0x100001, I18n.Natives_IncludePath_);
                this.tlk().addButton(container, new ButtonController(this.editor(), null){

                    @Override
                    public void execute(String id, Object data) {
                        Ide.openFolder(I18n.Natives_IncludePath_, new File(this.getValueAsString()));
                        super.execute(id, data);
                    }
                }, 1, 4096, "", "de-toem-toolkits-tlk-css-general-edit-image");
            }
            if (this.REQUIRES_LIBPATH) {
                this.tlk().addText(container, new TextController(this.editor(), FluxNative.class.getField("libPath")), this.cols() - 1, 0x100001, I18n.Natives_LibraryPath_);
                this.tlk().addButton(container, new ButtonController(this.editor(), null){

                    @Override
                    public void execute(String id, Object data) {
                        Ide.openFolder(I18n.Natives_LibraryPath_, new File(this.getValueAsString()));
                        super.execute(id, data);
                    }
                }, 1, 4096, "", "de-toem-toolkits-tlk-css-general-edit-image");
            }
            this.tlk().addText(container, new TextController(this.editor(), FluxNative.class.getField("makeCmd")).setNullText(Platform.isWin32() ? "mingw32-make" : "make"), this.cols(), 0x100001, I18n.Natives_FluxMakeCmd_);
            this.tlk().addCombo(container, new ComboController(this.editor(), FluxNative.class.getField("makeTarget"), FluxNative.getPlatformLabels(this.TARGET), FluxNative.getPlatformOptions(this.TARGET)), this.cols(), 0x100001, I18n.Natives_MakeTarget_);
            this.tlk().addText(container, new TextController(this.editor(), FluxNative.class.getField("addFlags")), this.cols(), 0x100001, I18n.Natives_AddFlags_);
            this.tlk().addText(container, new TextController(this.editor(), FluxNative.class.getField("addLibs")), this.cols(), 0x100001, I18n.Natives_AddLibs_);
            this.tlk().addButton(container, new ButtonController(this.editor(), null){

                @Override
                public void execute(String id, Object data) {
                    if (this.getCell() instanceof FluxNative) {
                        ((FluxNative)this.getCell()).performMake(FluxNativeDialog.this.BUNDLE_ID, FluxNativeDialog.this.TARGET, FluxNativeDialog.this.FLUX_PATH, FluxNativeDialog.this.FLUX_MAKEFILE);
                    }
                    super.execute(id, data);
                }
            }, 1, 0, I18n.Natives_PerformMake, null);
            this.tlk().addButton(container, new ButtonController(this.editor(), null){

                @Override
                public void execute(String id, Object data) {
                    try {
                        Ide.openEditor(Bundles.getBundleEntryAsFile(FluxNativeDialog.this.BUNDLE_ID, String.valueOf(FluxNativeDialog.this.FLUX_PATH) + "/" + FluxNativeDialog.this.FLUX_APPLICATION));
                    }
                    catch (Throwable e) {
                        e.printStackTrace();
                    }
                    super.execute(id, data);
                }
            }, 1, 0, I18n.Natives_EditExtension, null);
            this.tlk().addButton(container, new ButtonController(this.editor(), null){

                @Override
                public void execute(String id, Object data) {
                    try {
                        Ide.openEditor(Bundles.getBundleEntryAsFile(FluxNativeDialog.this.BUNDLE_ID, String.valueOf(FluxNativeDialog.this.FLUX_PATH) + "/" + FluxNativeDialog.this.FLUX_MAKEFILE));
                    }
                    catch (Throwable throwable) {}
                    super.execute(id, data);
                }
            }, 1, 0, I18n.Natives_EditMakefile, null);
        }
        catch (SecurityException securityException) {
        }
        catch (NoSuchFieldException e) {
            SystemLog.log(e);
        }
    }
}

