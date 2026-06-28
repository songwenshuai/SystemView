/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.view;

import de.toem.impulse.cells.view.CursorConfiguration;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.element.instancer.IBaseContextInstancer;
import de.toem.toolkits.ui.controller.abstrac.IController;
import de.toem.toolkits.ui.instancer.AbstractDefaultInstancer;
import de.toem.toolkits.ui.tlk.TLK;
import java.util.List;

public class CursorConfigurationInstancer
extends AbstractDefaultInstancer
implements IBaseContextInstancer {
    private IController controller;

    @Override
    public String getCellType() {
        return "configuration.cursor";
    }

    public CursorConfigurationInstancer() {
    }

    protected CursorConfigurationInstancer(CursorConfigurationInstancer base, IController controller) {
        super(base);
        this.controller = controller;
    }

    @Override
    public ICell createOne(String id, ICell container, IElement preferences) {
        if (this.controller != null && this.controller instanceof IRecordViewerController && ((IRecordViewerController)((Object)this.controller)).getView().isBound() && ((IRecordViewerController)((Object)this.controller)).getView().hasCell()) {
            return CursorConfigurationInstancer.createCell(((IRecordViewerController)((Object)this.controller)).getView().getCell(), preferences);
        }
        return super.createOne(id, container, preferences);
    }

    public static ICell createCell(ICell configuration, IElement preferences) {
        List<ICell> cursors = configuration.getTribe(false, CursorConfiguration.class);
        CursorConfiguration cell = new CursorConfiguration();
        int name = 65;
        while (name < 90) {
            boolean found = false;
            for (ICell cursor : cursors) {
                if (!String.valueOf((char)name).equals(cursor.getName())) continue;
                found = true;
                break;
            }
            if (!found) break;
            ++name;
        }
        cell.color = TLK.getSystemColorRgbInt("de.toem.impulse.color.cursor." + String.valueOf((name - 65) % 10), cell.color);
        cell.setName(String.valueOf((char)name));
        return cell;
    }

    public CursorConfigurationInstancer forContext(Object context) {
        if (context instanceof IController) {
            return new CursorConfigurationInstancer(this, (IController)context);
        }
        return null;
    }

    public static interface IRecordViewerController {
        public IElement getView();
    }
}

