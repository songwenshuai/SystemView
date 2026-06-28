/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.charts;

import de.toem.impulse.axis.IDomainAxis;
import de.toem.impulse.cells.charts.AbstractChartCell;
import de.toem.impulse.paint.IPlotItem;
import de.toem.impulse.paint.ITheme;
import de.toem.impulse.paint.ITreeItem;
import de.toem.impulse.paint.plan.IPlan;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.scripting.DefaultScriptContextProvider;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.element.CellAnnotation;
import de.toem.toolkits.pattern.properties.IPropertyModel;
import de.toem.toolkits.pattern.registry.Registration;
import de.toem.toolkits.pattern.scripting.IScriptContextProvider;
import de.toem.toolkits.pattern.scripting.IScripting;
import de.toem.toolkits.pattern.scripting.Scripting;
import de.toem.toolkits.pattern.threading.IProgress;
import de.toem.toolkits.ui.tlk.ITlkPainter;
import de.toem.toolkits.ui.tlk.SerialPainter;
import org.eclipse.swt.graphics.Rectangle;

@CellAnnotation(type="chart.script", dynamicChildOf={"preferences.impulse.charts"})
public class ScriptChart
extends AbstractChartCell
implements IPlan.IExternalPlanner<IPlotItem>,
IScriptContextProvider {
    public static final String TYPE = "chart.script";
    public String script;
    public String scriptLanguage;
    public String frontendScript;

    @Override
    public void plan(IPlan.ISinglePaintPlanner<IPlotItem> planner, IProgress progress, IPlan.IPaintPlanGenerator generator, IPropertyModel parameters, IDomainAxis axis, Rectangle childArea, boolean extend) {
        if (extend) {
            return;
        }
        generator.setScheme(8384);
        ITreeItem item = generator.getItem();
        ITheme theme = generator.getTheme();
        Rectangle area = childArea != null ? childArea : generator.getArea();
        Object color = item.getColor();
        Object font = theme.getFont();
        if (font == null) {
            font = Registration.fonts.getFont("de.toem.impulse.font");
        }
        Object background = theme.getColor(0);
        SerialPainter painter = new SerialPainter(area.width, area.height);
        painter.setForeground(color);
        painter.setBackground(background);
        painter.fillRectangle(0, 0, area.width, area.height);
        IScripting scripting = Scripting.create(this, "script", s -> {
            s.setSymbol("generator", generator);
            s.setSymbol("gc", painter);
            s.setSymbol("painter", painter);
            s.setSymbol("x", 0);
            s.setSymbol("y", 0);
            s.setSymbol("width", rectangle.width);
            s.setSymbol("height", rectangle.height);
            s.setSymbol("color", color);
            s.setSymbol("background", background);
            s.setSymbol("plannable", item);
            s.setSymbol("axis", axis);
            s.setSymbol("theme", theme);
            s.setSymbol("readable", item instanceof IPlotItem ? ((IPlotItem)item).getSamples() : null);
        });
        scripting.run(null);
        byte[] image = painter.getBytes();
        if (image != null) {
            generator.add(32768, generator.getArea().x, 0, image);
        }
        if (!Utils.isEmpty(this.frontendScript)) {
            generator.setScript(this.frontendScript);
        }
    }

    @Override
    public int getPriority() {
        return 2;
    }

    public void create(IProgress progress, IPlotItem plannable, IDomainAxis axis, ITheme theme, IPropertyModel parameters, Rectangle area) {
    }

    @Override
    public void provideToScriptContext(IScriptContextProvider.IScriptContextInterface context) {
        DefaultScriptContextProvider.provideDefaultScriptContext(context, true, false, false, false, false, true);
        context.addSymbol("painter", ITlkPainter.class);
        context.addSymbol("plannable", IPlotItem.class);
        context.addSymbol("axis", IDomainAxis.class);
        context.addSymbol("theme", ITheme.class);
        context.addSymbol("readable", IReadableSamples.class);
        context.setScript(this.script, this.scriptLanguage);
    }
}

