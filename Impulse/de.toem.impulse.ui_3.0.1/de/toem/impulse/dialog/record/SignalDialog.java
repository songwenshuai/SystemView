/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.dialog.record;

import de.toem.impulse.cells.record.Signal;
import de.toem.impulse.domain.DomainBase;
import de.toem.impulse.parts.viewer.RecordViewer;
import de.toem.impulse.samples.ISamples;
import de.toem.impulse.ui.i18n.I18n;
import de.toem.toolkits.pattern.controls.IControlProvider;
import de.toem.toolkits.pattern.scan.TextScanResult;
import de.toem.toolkits.ui.controller.abstrac.IController;
import de.toem.toolkits.ui.controller.base.ComboController;
import de.toem.toolkits.ui.controller.base.TextController;
import de.toem.toolkits.ui.part.ITlkPart;
import de.toem.toolkits.ui.part.dialog.ControlProviderElementDialog;
import de.toem.toolkits.ui.proposal.ContentProposal;
import de.toem.toolkits.ui.proposal.ContentProposalExtension;
import de.toem.toolkits.ui.tlk.AbstractControlProvider;
import de.toem.toolkits.ui.tlk.ITlkPartContainer;

public class SignalDialog
extends ControlProviderElementDialog {
    public SignalDialog(ITlkPartContainer parent, int style) {
        super(parent, SignalDialog.getControls(), style);
    }

    public SignalDialog() {
    }

    public static IControlProvider getControls() {
        AbstractControlProvider provider = new AbstractControlProvider(){
            Object domainRange;
            IController processType;
            IController signalType;
            IController signalDescriptor;
            IController domainBase;
            IController domainClass;
            IController rate;

            @Override
            public String getHelpContext() {
                return "de.toem.impulse.ui.signal_dialog";
            }

            @Override
            public boolean fillThis() {
                try {
                    this.tlk().addText(this.container(), new TextController(this.editor(), Signal.class.getField("name")), this.tlk().ld(this.cols() - 1, 4, 200), 0x100001, I18n.General_Name_);
                    this.tlk().addText(this.container(), new TextController(this.editor(), Signal.class.getField("description")), this.cols(), 0x100001, I18n.General_Description_);
                    this.tlk().addText(this.container(), new TextController(this.editor(), null){

                        @Override
                        public Object value() {
                            if (this.getCell() != null) {
                                return this.getCell().getPath();
                            }
                            return null;
                        }
                    }, this.cols(), 0x100001, I18n.General_Location_);
                    this.processType = this.tlk().addCombo(this.container(), new ComboController(this.editor(), Signal.class.getField("processType"), ISamples.ProcessType.getOptions(true), ISamples.ProcessType.getOptions(true)){

                        @Override
                        protected void doUpdateExternal() {
                        }
                    }.setNullItem(""), this.tlk().ld(this.cols() - 1, 524288, -1), 8193, I18n.Samples_ProcessType_);
                    this.signalType = this.tlk().addCombo(this.container(), new ComboController(this.editor(), Signal.class.getField("signalType"), ISamples.SignalType.getOptions(true), ISamples.SignalType.getOptions(true)){

                        @Override
                        public boolean needsUpdate() {
                            return true;
                        }
                    }.setNullItem(""), this.cols(), 8193, I18n.Samples_SignalType_);
                    this.signalDescriptor = this.tlk().addText(this.container(), new TextController(this.editor(), Signal.class.getField("signalDescriptor")){

                        @Override
                        protected Object revert(Object value) {
                            ISamples.SignalType type = ISamples.SignalType.parse(signalType.getValueAsString());
                            return ISamples.SignalDescriptor.parseUser(type, String.valueOf(value)).toString();
                        }

                        @Override
                        protected Object convert(Object value) {
                            ISamples.SignalType type = ISamples.SignalType.parse(signalType.getValueAsString());
                            return ISamples.SignalDescriptor.parse(String.valueOf(value)).toUserString(type);
                        }

                        @Override
                        public boolean needsUpdate() {
                            return true;
                        }

                        @Override
                        protected TextScanResult doCheck(String formatted, int options) {
                            ISamples.SignalType type = ISamples.SignalType.parse(signalType.getValueAsString());
                            return ISamples.SignalDescriptor.checkUser(type, formatted) ? TextScanResult.SCAN_OK : TextScanResult.SCAN_ERROR;
                        }
                    }.add(new ContentProposalExtension(true){

                        @Override
                        public ContentProposal[] getProposals(String contents, int position) {
                            this.clear();
                            ISamples.SignalType type = ISamples.SignalType.parse(signalType.getValueAsString());
                            this.add("default<>", null, null);
                            switch (type) {
                                case Logic: {
                                    this.add("default<bits=16>", null, null);
                                    this.add("default<bits=16,df=Hex>", null, null);
                                    this.add("default<bits=16,df=Binary>", null, null);
                                    this.add("default<bits=16,df=Octal>", null, null);
                                    this.add("default<bits=16,df=ASCII>", null, null);
                                    break;
                                }
                                case Event: {
                                    this.add("default<df=Event>", null, null);
                                    this.add("default<df=Text>", null, null);
                                    break;
                                }
                                case EventArray: {
                                    this.add("default<dim=2>", null, null);
                                    this.add("default<dim=2,df=Index>", null, null);
                                    this.add("default<dim=2,df=Event>", null, null);
                                    this.add("default<dim=2,df=Text>", null, null);
                                    break;
                                }
                                case Integer: 
                                case Float: {
                                    this.add("default<df=Decimal>", null, null);
                                    this.add("default<df=UserDec0>", null, null);
                                    break;
                                }
                                case IntegerArray: 
                                case FloatArray: {
                                    this.add("default<dim=8>", null, null);
                                    this.add("default<dim=8,df=Decimal>", null, null);
                                    this.add("default<dim=8,df=UserDec0>", null, null);
                                    break;
                                }
                                case Text: {
                                    this.add("default<df=Text>", null, null);
                                    break;
                                }
                                case TextArray: {
                                    this.add("default<dim=2>", null, null);
                                    this.add("default<dim=2,df=Text>", null, null);
                                    break;
                                }
                                case Struct: {
                                    this.add("transaction<>", null, null);
                                    break;
                                }
                                case Binary: {
                                    this.add("image<>", null, null);
                                    break;
                                }
                            }
                            this.add("default<df=None>", null, null);
                            this.add("default<df=Index>", null, null);
                            this.add("default<df=\u0394Domain>", null, null);
                            this.add("default<df=\u0394Value>", null, null);
                            return super.getProposals(contents, position);
                        }
                    }), this.cols(), 0x100001, I18n.Samples_SignalDescriptor_);
                    this.domainClass = this.tlk().addCombo(this.container(), new ComboController(this.editor(), "domainClass", DomainBase.CLASS_LABELS, DomainBase.CLASSES){

                        @Override
                        protected void doUpdateHints() {
                            if (this.value == null && domainBase.getValue() instanceof String) {
                                this.source.changeValue(DomainBase.parse((String)domainBase.getValue()).getClass(), true);
                                this.update(true);
                                domainBase.updateControl(true);
                            }
                        }

                        @Override
                        protected void doUpdateExternal() {
                            domainBase.updateControl(true);
                        }
                    }.setNullItem(""), this.cols() - 1, 8193, I18n.Samples_DomainClass_);
                    this.domainBase = this.tlk().addCombo(this.container(), new ComboController(this.editor(), Signal.class.getField("domainBase"), DomainBase.ALL_LABELS, DomainBase.ALL_OPTIONS){

                        @Override
                        protected boolean filterItem(String label, Object value) {
                            return value != null && !DomainBase.parse((String)value).getClass().equals(domainClass.getValue());
                        }

                        @Override
                        protected void doUpdateExternal() {
                        }
                    }.setNullItem(""), 1, 8192, null);
                    this.domainRange = this.tlk().addComposite(this.container(), null, 6, this.cols(), 1, I18n.Samples_DomainRange_, null);
                    this.tlk().addText(this.domainRange, new TextController(this.editor(), Signal.class.getField("start")), 2, 0x100000, I18n.Samples_DomainStart_);
                    this.tlk().addText(this.domainRange, new TextController(this.editor(), Signal.class.getField("end")), 2, 0x100001, I18n.General_Dotdotdot);
                    this.rate = this.tlk().addText(this.domainRange, new TextController(this.editor(), Signal.class.getField("rate")), 2, 0x100001, I18n.Samples_DomainRate_);
                    ITlkPart par = this.editor();
                    while (par.getContainerPart() != null) {
                        par = par.getContainerPart();
                    }
                    if (par instanceof RecordViewer) {
                        this.tlk().setEnabled(false);
                    }
                }
                catch (SecurityException securityException) {
                }
                catch (NoSuchFieldException noSuchFieldException) {}
                return true;
            }
        };
        return provider;
    }
}

