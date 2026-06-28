/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.paint.controller;

import de.toem.impulse.paint.IPlotItem;
import de.toem.impulse.paint.ITreeItem;
import de.toem.impulse.paint.plan.IPlan;
import de.toem.impulse.paint.plan.PaintPlanner;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISamplesProducer;
import de.toem.toolkits.core.Assert;
import de.toem.toolkits.core.Utils;
import de.toem.toolkits.pattern.ide.Ide;
import de.toem.toolkits.pattern.systemLog.SystemLog;
import de.toem.toolkits.pattern.threading.Actives;
import de.toem.toolkits.pattern.threading.INamedExecutable;
import de.toem.toolkits.pattern.threading.IProgress;
import de.toem.toolkits.pattern.threading.Progress;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class PlanFactory {
    public static final int TIMEOUT_MAIN_THREAD = 1000;
    public static final int TIMEOUT_PROGRESS = 3000;
    public static final int MAX_THREADS = 8;
    private Set<ITreeItem> filter = new HashSet<ITreeItem>();
    private Map<Object, IPlan.IPaintPlanner<IPlotItem>> requested = new HashMap<Object, IPlan.IPaintPlanner<IPlotItem>>();
    private Map<Object, Integer> provided = new HashMap<Object, Integer>();
    private LinkedList<IPlan.IPaintPlanner<IPlotItem>> scheduled = new LinkedList();
    private LinkedList<IPlan.IPaintPlanner<IPlotItem>> running = new LinkedList();
    private boolean disposed;
    private int threadIdx = 0;

    public PlanFactory() {
        Actives.run(new INamedExecutable(){

            @Override
            public void execute(IProgress p) {
                PlanFactory.this.controlThread();
                PlanFactory.this.filter.clear();
                PlanFactory.this.requested.clear();
                PlanFactory.this.provided.clear();
                PlanFactory.this.scheduled.clear();
                PlanFactory.this.running.clear();
            }

            @Override
            public String getLabel() {
                return "PlanFactoryControl";
            }

            @Override
            public int getPriority() {
                return 0;
            }
        });
    }

    public void dispose() {
        this.disposed = true;
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    public void limitTo(List<? extends ITreeItem> filter) {
        List<? extends ITreeItem> list = filter;
        synchronized (list) {
            this.filter.clear();
            this.filter.addAll(filter);
        }
    }

    public void request(Object client, int counter, IPlan.IPlanRequest<ITreeItem> request) {
        IPlan.IPaintPlanner<IPlotItem> actual = this.requested.get(client);
        IPlan.IPaintPlanner<IPlotItem> next = null;
        int checked = PaintPlanner.checkAndUpdate(counter, actual, request);
        if ((checked & 1) == 0) {
            next = PaintPlanner.create(client, counter, request);
        }
        if (next != null && next != actual) {
            this.requested.put(client, next);
            this.schedule(next);
        } else if ((checked & 2) != 0) {
            this.schedule(actual);
        } else if ((checked & 4) == 0 && (checked & 8) == 0) {
            this.notify(client, counter, 0, null);
        }
    }

    public void invalidate() {
        this.requested.clear();
    }

    protected void provide(Object handle, int requestIdx, IPlan.IPaintPlanProvision provision, boolean samplesUpdated) {
    }

    protected void notify(Object handle, int requestIdx, int reason, String detail) {
    }

    protected void update() {
    }

    private final synchronized void onPlanCreated(IPlan.IPaintPlanner<IPlotItem> planner, IPlan.IPaintPlanProvision provision, boolean samplesUpdated) {
        Object client = planner.getClient();
        if (client != null) {
            int counter = planner.getCounter();
            if (!this.provided.containsKey(client) || this.provided.get(client) <= counter) {
                this.provide(client, counter, provision, samplesUpdated);
                this.provided.put(client, counter);
            } else {
                this.notify(client, counter, 2, null);
            }
        }
    }

    private final synchronized void onPlanFiltered(IPlan.IPaintPlanner<IPlotItem> planner) {
        Object client = planner.getClient();
        if (client != null) {
            this.notify(client, planner.getCounter(), 1, null);
            planner.dispose();
        }
    }

    private final synchronized void onPlanDestroyed(IPlan.IPaintPlanner<IPlotItem> planner) {
        Object client = planner.getClient();
        if (client != null) {
            int counter = planner.getCounter();
            if (!this.provided.containsKey(client) || this.provided.get(client) <= counter) {
                this.provide(client, counter, planner.createTimeoutProvision(true), false);
                this.provided.put(client, counter);
            } else {
                this.notify(client, counter, 2, null);
            }
            planner.dispose();
        }
    }

    private final synchronized void onPlanFailed(IPlan.IPaintPlanner<IPlotItem> planner, Throwable e) {
        Object client = planner.getClient();
        if (client != null) {
            int counter = planner.getCounter();
            if (!this.provided.containsKey(client) || this.provided.get(client) <= counter) {
                this.notify(client, counter, 4, e.toString());
            }
            planner.dispose();
        }
    }

    private final synchronized void onPlanSequenceDone() {
        this.update();
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    private void schedule(IPlan.IPaintPlanner<IPlotItem> planner) {
        LinkedList<IPlan.IPaintPlanner<IPlotItem>> linkedList = this.scheduled;
        synchronized (linkedList) {
            this.scheduled.addLast(planner);
            this.scheduled.notify();
        }
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    private IPlan.IPaintPlanner<IPlotItem> selectNextSchedule() {
        LinkedList<IPlan.IPaintPlanner<IPlotItem>> linkedList = this.scheduled;
        synchronized (linkedList) {
            this.scheduled.sort(new Comparator<IPlan.IPaintPlanner<IPlotItem>>(){

                @Override
                public int compare(IPlan.IPaintPlanner<IPlotItem> arg0, IPlan.IPaintPlanner<IPlotItem> arg1) {
                    return Utils.compare(arg0.getPriority(), arg1.getPriority(), 1);
                }
            });
            Iterator iter = this.scheduled.iterator();
            while (iter.hasNext()) {
                IPlan.IPaintPlanner planner = (IPlan.IPaintPlanner)iter.next();
                if (planner == null) {
                    iter.remove();
                    continue;
                }
                if (planner.isDisposed()) {
                    iter.remove();
                    this.onPlanFiltered(planner);
                    continue;
                }
                if (planner.getPriority() < 0) continue;
                boolean fiter = true;
                Set<ITreeItem> set = this.filter;
                synchronized (set) {
                    for (ITreeItem plannable : planner.getItems()) {
                        if (!this.filter.contains(plannable)) continue;
                        fiter = false;
                        break;
                    }
                }
                if (fiter) {
                    iter.remove();
                    this.onPlanFiltered(planner);
                    continue;
                }
                boolean settling = false;
                for (IPlotItem item : planner.getItems()) {
                    IReadableSamples readable = item.getSamples();
                    if (readable == null) continue;
                    settling |= readable.isSettling();
                    if (!(readable instanceof ISamplesProducer)) continue;
                    settling |= ((ISamplesProducer)readable).areSourcesSettling();
                }
                if (settling) continue;
                iter.remove();
                LinkedList<IPlan.IPaintPlanner<IPlotItem>> linkedList2 = this.running;
                synchronized (linkedList2) {
                    this.running.addLast(planner);
                }
                return planner;
            }
        }
        return null;
    }

    /*
     * WARNING - Removed try catching itself - possible behaviour change.
     */
    private void planThread(ThreadObserver observer) {
        IPlan.IPaintPlanner<IPlotItem> planner = null;
        while (!this.disposed && !observer.isBackground()) {
            if (planner == null) {
                planner = this.selectNextSchedule();
            }
            if (planner != null) {
                long time = Utils.millies();
                do {
                    Progress progress = observer.start(planner);
                    try {
                        IPlan.IPaintPlanProvision provision = null;
                        boolean hasUpdatedRelease = false;
                        if (planner instanceof IPlan.ISinglePaintPlanner) {
                            provision = ((IPlan.ISinglePaintPlanner)planner).plan(progress);
                            hasUpdatedRelease = this.hasUpdatedRelease((IPlan.ISinglePaintPlanner)planner);
                        } else if (planner instanceof IPlan.IMultiPaintPlanner) {
                            provision = ((IPlan.IMultiPaintPlanner)planner).plan(progress);
                            hasUpdatedRelease = this.hasUpdatedRelease((IPlan.IMultiPaintPlanner)planner);
                        }
                        if (provision != null) {
                            this.onPlanCreated(planner, provision, hasUpdatedRelease);
                        } else {
                            this.onPlanFailed(planner, null);
                        }
                    }
                    catch (Throwable e) {
                        this.onPlanFailed(planner, e);
                    }
                    observer.end();
                    LinkedList<IPlan.IPaintPlanner<IPlotItem>> linkedList = this.running;
                    synchronized (linkedList) {
                        this.running.remove(planner);
                    }
                    planner = null;
                } while (!this.disposed && !observer.isBackground() && Utils.millies() - time < 250L && (planner = this.selectNextSchedule()) != null);
                this.onPlanSequenceDone();
                continue;
            }
            LinkedList<IPlan.IPaintPlanner<IPlotItem>> linkedList = this.scheduled;
            synchronized (linkedList) {
                try {
                    this.scheduled.wait(100L);
                }
                catch (InterruptedException interruptedException) {}
            }
        }
    }

    private boolean hasUpdatedRelease(IPlan.ISinglePaintPlanner<IPlotItem> planner) {
        IPlotItem item = planner.getItem();
        return item != null && item.getSamples() != null && item.getSamplesRelease() != item.getSamples().getRelease();
    }

    private boolean hasUpdatedRelease(IPlan.IMultiPaintPlanner<IPlotItem> planner) {
        List items = planner.getItems();
        if (items != null) {
            for (IPlotItem item : items) {
                if (item == null || item.getSamples() == null || item.getSamplesRelease() == item.getSamples().getRelease()) continue;
                return true;
            }
        }
        return false;
    }

    private void controlThread() {
        ArrayList<ThreadObserver> background = new ArrayList<ThreadObserver>();
        block2: while (!this.disposed) {
            try {
                this.waitToCreatePlanThread();
                ThreadObserver observer = this.createPlanThread();
                while (!this.disposed) {
                    if (observer.isFinalized() || observer.getDuration() > 1000L && background.size() < 8) {
                        observer.toBackground();
                        background.add(observer);
                        Ide.openProgress(observer.getLabel(), observer.progress, (int)observer.getTimeout(), kill -> {
                            if (kill) {
                                observer.destroy();
                            }
                        });
                        continue block2;
                    }
                    Iterator iter = background.iterator();
                    while (iter.hasNext()) {
                        ThreadObserver bo = (ThreadObserver)iter.next();
                        if (!bo.isFinalized()) continue;
                        iter.remove();
                    }
                    Actives.sleep(250);
                }
            }
            catch (Throwable e) {
                SystemLog.log(e);
            }
        }
    }

    private void waitToCreatePlanThread() {
    }

    private ThreadObserver createPlanThread() {
        final ThreadObserver observer = new ThreadObserver();
        Actives.run(new INamedExecutable(){
            int idx;
            {
                PlanFactory planFactory2 = PlanFactory.this;
                int n = planFactory2.threadIdx;
                planFactory2.threadIdx = n + 1;
                this.idx = n;
            }

            @Override
            public void execute(IProgress p) {
                try {
                    observer.init(Thread.currentThread());
                    PlanFactory.this.planThread(observer);
                }
                catch (Throwable e) {
                    SystemLog.log(e);
                }
                observer.finalized = true;
            }

            @Override
            public String getLabel() {
                return "PlanFactoryPlanning" + this.idx;
            }

            @Override
            public int getPriority() {
                return 0;
            }
        });
        return observer;
    }

    private class ThreadObserver {
        private Thread thread;
        private boolean background;
        private boolean finalized;
        private Progress progress;
        private IPlan.IPaintPlanner<IPlotItem> planner;
        private long started;

        private ThreadObserver() {
        }

        void init(Thread thread) {
            this.thread = thread;
        }

        synchronized void toBackground() {
            this.background = true;
        }

        synchronized boolean isBackground() {
            return this.background;
        }

        synchronized boolean isFinalized() {
            return this.finalized;
        }

        synchronized String getLabel() {
            List items;
            List list = items = this.planner != null ? this.planner.getItems() : null;
            if (items != null) {
                String label = "";
                for (IPlotItem item : items) {
                    label = String.valueOf(label) + (label.isEmpty() ? "" : ", ") + item.getText();
                }
                return "Calculating plots " + label;
            }
            return "";
        }

        synchronized Progress start(IPlan.IPaintPlanner<IPlotItem> planner) {
            Assert.isNotNull(planner);
            this.started = Utils.millies();
            this.planner = planner;
            this.progress = new Progress(this.getLabel());
            this.progress.setTimeout(3000);
            this.progress.start();
            return this.progress;
        }

        synchronized long getDuration() {
            IPlan.IPaintPlanner<IPlotItem> planner = this.planner;
            if (planner != null) {
                return Utils.millies() - this.started;
            }
            return 0L;
        }

        synchronized void end() {
            this.progress.end();
            this.planner = null;
            this.progress = null;
        }

        public synchronized void timeout() {
            if (this.planner != null && this.progress != null && this.thread != null) {
                this.progress.cancel();
                this.progress.timeoutOccured();
                this.thread.interrupt();
                this.thread.setPriority(1);
            }
        }

        synchronized boolean isCanceled() {
            return this.progress != null ? this.progress.isCanceled() : true;
        }

        synchronized long getTimeout() {
            return this.progress != null ? this.progress.getTimeout() : 0;
        }

        synchronized long getDestruction() {
            return this.progress != null ? this.progress.getDestruction() : 0;
        }

        void destroy() {
            if (this.planner != null && this.progress != null && this.thread != null) {
                this.progress.destructionOccured();
                PlanFactory.this.onPlanDestroyed(this.planner);
                Actives.destroy(this.thread);
                this.finalized = true;
            }
        }
    }
}

