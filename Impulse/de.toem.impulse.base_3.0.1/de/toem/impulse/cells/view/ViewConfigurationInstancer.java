/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.view;

import de.toem.impulse.cells.view.CursorConfigurationInstancer;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.element.instancer.IBaseContextInstancer;
import de.toem.toolkits.ui.controller.abstrac.IController;
import de.toem.toolkits.ui.instancer.AbstractDefaultInstancer;
import de.toem.toolkits.ui.part.ITlkPart;

public class ViewConfigurationInstancer
extends AbstractDefaultInstancer
implements IBaseContextInstancer {
    private IController controller;

    public ViewConfigurationInstancer() {
    }

    protected ViewConfigurationInstancer(ViewConfigurationInstancer base, IController controller) {
        super(base);
        this.controller = controller;
    }

    @Override
    public String getCellType() {
        return "configuration.record";
    }

    public static String configurationNameForRecord(IElement element) {
        if (element.isBound()) {
            String name = element.getName();
            if (name.indexOf(46) >= 0) {
                name = name.substring(0, name.indexOf(46));
            }
            return name;
        }
        return null;
    }

    @Override
    public ICell createOne(String id, ICell container, IElement preferences) {
        String name = null;
        if (this.controller != null && this.controller instanceof ITlkPart && this.controller.getEditor() != null && this.controller.getEditor().getElement().isBound()) {
            name = ViewConfigurationInstancer.configurationNameForRecord(this.controller.getEditor().getElement());
        }
        ICell cell = super.createOne(id, container, preferences);
        if (name != null) {
            cell.setName(name);
        }
        cell.addChild(CursorConfigurationInstancer.createCell(cell, IElement.NONE));
        return cell;
    }

    public ViewConfigurationInstancer forContext(Object context) {
        if (context instanceof IController) {
            return new ViewConfigurationInstancer(this, (IController)context);
        }
        return null;
    }
}

