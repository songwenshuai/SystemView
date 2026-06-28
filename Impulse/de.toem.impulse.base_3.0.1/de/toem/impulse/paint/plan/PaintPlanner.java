/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint.plan;

import de.toem.impulse.ImpulseLicense;
import de.toem.impulse.axis.IDomainAxis;
import de.toem.impulse.paint.IPlotItem;
import de.toem.impulse.paint.ITheme;
import de.toem.impulse.paint.ITreeItem;
import de.toem.impulse.paint.plan.AreaPaintPlanner;
import de.toem.impulse.paint.plan.ChartPaintPlanner;
import de.toem.impulse.paint.plan.EventPaintPlanner;
import de.toem.impulse.paint.plan.GanttPaintPlanner;
import de.toem.impulse.paint.plan.IPlan;
import de.toem.impulse.paint.plan.ImagePaintPlanner;
import de.toem.impulse.paint.plan.LinePaintPlanner;
import de.toem.impulse.paint.plan.LogPaintPlanner;
import de.toem.impulse.paint.plan.LogicPaintPlanner;
import de.toem.impulse.paint.plan.MultiAreaPaintPlanner;
import de.toem.impulse.paint.plan.MultiChartPaintPlanner;
import de.toem.impulse.paint.plan.MultiLinePaintPlanner;
import de.toem.impulse.paint.plan.MultiPaintPlanner;
import de.toem.impulse.paint.plan.NonePaintPlanner;
import de.toem.impulse.paint.plan.PaintStyle;
import de.toem.impulse.paint.plan.SinglePaintPlanner;
import de.toem.impulse.paint.plan.TransactionPaintPlanner;
import de.toem.impulse.paint.plan.VectorPaintPlanner;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISamples;
import java.util.List;
import org.eclipse.swt.graphics.Rectangle;

public class PaintPlanner {
    public static IPlan.IPaintPlanner<IPlotItem> create(Object client, int counter, IPlan.IPlanRequest<ITreeItem> request) {
        List<ITreeItem> items = request.getItems();
        IPlan.IPaintPlanner planner = null;
        if (items.size() == 1) {
            if (items.get(0) instanceof IPlotItem) {
                planner = PaintPlanner.create(null, request.getTheme(), (IPlotItem)items.get(0), request.getAxis(), request.getArea(), request.getStyle());
            }
        } else if (items.size() > 1) {
            int diagramType = -1;
            for (ITreeItem item : items) {
                if (!(item instanceof IPlotItem)) {
                    return null;
                }
                PaintStyle paintStyle = (PaintStyle)item.getPaintStyle();
                if (paintStyle == null) {
                    return null;
                }
                if (diagramType == -1) {
                    diagramType = paintStyle.diagramType;
                }
                if (diagramType == -1 || diagramType == paintStyle.diagramType) continue;
                return null;
            }
            planner = PaintPlanner.create(diagramType, request.getTheme(), items, request.getAxis(), request.getArea(), request.getStyle());
        }
        if (planner != null) {
            planner.setClient(client, counter);
        }
        return planner;
    }

    public static IPlan.IPaintPlanner<IPlotItem> create(Object client, int counter, IPlan.IPaintPlanner previous, IPlan.IPlanRequest request) {
        IPlan.IPaintPlanner<IPlotItem> planner = PaintPlanner.create(client, counter, request);
        return planner;
    }

    public static int checkAndUpdate(int counter, IPlan.IPaintPlanner previous, IPlan.IPlanRequest request) {
        if (previous instanceof IPlan.IPaintPlanner) {
            int checked = previous.checkAndUpdate(request.getAxis(), request.getArea(), request.getStyle());
            if ((checked & 1) != 0) {
                previous.setCounter(counter);
            }
            return checked;
        }
        return 0;
    }

    protected static SinglePaintPlanner create(MultiPaintPlanner multi, ITheme theme, IPlotItem item, IDomainAxis axis, Rectangle area, int style) {
        IReadableSamples readable = item.getSamples();
        PaintStyle paintStyle = item.getPaintStyle();
        if (paintStyle == null) {
            return null;
        }
        if (ImpulseLicense.isFeatureLocked("de.toem.impulse.featureDomain.diagrams", "de.toem.impulse.feature.default", String.valueOf(paintStyle.getType()))) {
            return new NonePaintPlanner(theme, item, axis, area, style, 2);
        }
        switch (paintStyle.getType()) {
            case 1: {
                return new LogicPaintPlanner(theme, item, axis, area, style);
            }
            case 2: {
                return new VectorPaintPlanner(theme, item, axis, area, style);
            }
            case 3: {
                return new EventPaintPlanner(theme, item, axis, area, style);
            }
            case 4: {
                return new LinePaintPlanner(theme, item, axis, area, style);
            }
            case 5: {
                if (readable == null || readable.getProcessType() == ISamples.ProcessType.Discrete) {
                    return new TransactionPaintPlanner(theme, item, axis, area, style);
                }
            }
            case 6: {
                return new LogPaintPlanner(theme, item, axis, area, style);
            }
            case 7: {
                return new ImagePaintPlanner(theme, item, axis, area, style);
            }
            case 8: {
                return new ChartPaintPlanner(theme, item, axis, area, style);
            }
            case 9: {
                return new AreaPaintPlanner(theme, item, axis, area, style);
            }
            case 10: {
                return new GanttPaintPlanner(theme, item, axis, area, style);
            }
        }
        return new NonePaintPlanner(theme, item, axis, area, style, 1);
    }

    protected static SinglePaintPlanner create(MultiPaintPlanner multi, IPlotItem item, Rectangle area) {
        item.getSamples();
        PaintStyle paintStyle = item.getPaintStyle();
        if (paintStyle == null) {
            return null;
        }
        if (ImpulseLicense.isFeatureLocked("de.toem.impulse.featureDomain.diagrams", "de.toem.impulse.feature.default", String.valueOf(paintStyle.getType()))) {
            return new NonePaintPlanner(multi, item, 2);
        }
        switch (paintStyle.getType()) {
            case 4: {
                return new LinePaintPlanner(multi, item);
            }
            case 8: {
                return new ChartPaintPlanner(multi, item, area);
            }
            case 9: {
                return new AreaPaintPlanner(multi, item);
            }
        }
        return null;
    }

    private static IPlan.IPaintPlanner create(int diagramType, ITheme theme, List<? extends IPlotItem> items, IDomainAxis axis, Rectangle area, int style) {
        switch (diagramType) {
            case 4: {
                return new MultiLinePaintPlanner(theme, items, axis, area, style);
            }
            case 8: {
                return new MultiChartPaintPlanner(theme, items, axis, area, style);
            }
            case 9: {
                return new MultiAreaPaintPlanner(theme, items, axis, area, style);
            }
        }
        return null;
    }
}

