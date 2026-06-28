/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.ui;

import de.toem.impulse.ui.IImpulsePart;
import de.toem.impulse.ui.IImpulsePartListener;
import de.toem.impulse.ui.IRecordViewer;
import de.toem.impulse.ui.IRecordViewerListener;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.element.Link;
import java.util.ArrayList;
import java.util.List;

public class ImpulseNotifications {
    private static List<IImpulsePartListener> listeners = new ArrayList<IImpulsePartListener>();

    public static synchronized void addListener(IImpulsePartListener listener) {
        listeners.add(listener);
    }

    public static synchronized void removeListener(IImpulsePartListener listener) {
        listeners.remove(listener);
    }

    public static synchronized void firePartCreated(IImpulsePart viewer) {
        for (IImpulsePartListener l : listeners) {
            l.created(viewer);
        }
    }

    public static synchronized void firePartDisposed(IImpulsePart viewer) {
        for (IImpulsePartListener l : listeners) {
            l.disposed(viewer);
        }
    }

    public static synchronized void fireViewerInputSet(IRecordViewer viewer, IElement inputElement, IElement editorElement, IElement baseElement) {
        for (IImpulsePartListener l : listeners) {
            if (!(l instanceof IRecordViewerListener)) continue;
            ((IRecordViewerListener)l).inputSet(viewer, inputElement, editorElement, baseElement);
        }
    }

    public static synchronized void fireViewerViewSet(IRecordViewer viewer, Link view, IElement viewElement) {
        for (IImpulsePartListener l : listeners) {
            if (!(l instanceof IRecordViewerListener)) continue;
            ((IRecordViewerListener)l).viewSet(viewer, view, viewElement);
        }
    }

    public static synchronized void fireViewerAboutToRefresh(IRecordViewer viewer) {
        for (IImpulsePartListener l : listeners) {
            if (!(l instanceof IRecordViewerListener)) continue;
            ((IRecordViewerListener)l).aboutToRefresh(viewer);
        }
    }
}

