/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.scripting;

import de.toem.impulse.i18n.I18n;
import de.toem.impulse.scripting.ScriptContentProposalExtension;
import de.toem.toolkits.pattern.ide.Ide;
import de.toem.toolkits.ui.colorizer.JavaScriptColorizer;
import de.toem.toolkits.ui.controller.abstrac.IController;
import de.toem.toolkits.ui.controller.base.ButtonController;
import de.toem.toolkits.ui.controller.base.ComboController;
import de.toem.toolkits.ui.controller.base.TextBoxController;
import de.toem.toolkits.ui.part.ITlkPart;
import de.toem.toolkits.ui.tlk.TLK;
import de.toem.toolkits.ui.tlk.controls.ITlkComposite;
import java.lang.reflect.Field;

public class ScriptControls {
    public static Object fillScriptControls(TLK tlk, Object container, ITlkPart editor, Field field, Field language, Object ld) {
        return ScriptControls.fillScriptControls(tlk, container, editor, field, language, ld, null);
    }

    public static Object fillScriptControls(TLK tlk, Object container, ITlkPart editor, final Field field, Field language, Object ld, String group) {
        ITlkComposite composite = null;
        composite = group == null ? tlk.addComposite(container, null, 1, ld, 0, null, null) : tlk.addGroup(container, null, 1, ld, 0, group, null);
        IController test = tlk.addTextBox(composite, new TextBoxController(editor, field).add(new JavaScriptColorizer()).add(new ScriptContentProposalExtension()), tlk.ld(1, 4, 1, 524288, 1), 0x100018, null);
        test.setFont("org.eclipse.jface.textfont");
        ITlkComposite buttons = tlk.addComposite(composite, null, 3, tlk.ld(1, 131072, -1), 0, null, null);
        if (language != null) {
            tlk.addCombo(buttons, new ComboController(editor, language, new String[]{"js", "py"}, new String[]{"js", "py"}), tlk.ld(1, 131072, -1), 0, null);
        }
        ButtonController button = tlk.addButton(buttons, new ButtonController(editor, null){

            @Override
            public void execute(String id, Object data) {
                this.tlk.openTextEditor(this.getCell().getElement(), field != null ? field.getName() : null, null);
            }
        }, tlk.ld(1, 131072, -1), 4096, "", "de.toem.impulse.images.part.script");
        button.setTooltip(I18n.Scripting_EditInScriptEditor);
        button = tlk.addButton(buttons, new ButtonController(editor, null){

            @Override
            public void execute(String id, Object data) {
                this.tlk.showConsole(Ide.DEFAULT_CONSOLE);
            }
        }, tlk.ld(1, 131072, -1), 4096, "", "de.toem.impulse.images.part.terminal");
        button.setTooltip(I18n.Scripting_ShowConsole);
        return composite;
    }
}

