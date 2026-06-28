/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cells.view;

import de.toem.impulse.cells.view.AbstractTreeConfiguration;
import de.toem.impulse.i18n.I18n;

public class AxisConfiguration
extends AbstractTreeConfiguration {
    public static final int AXIS_NONE = 0;
    public static final int AXIS_DEDICATED = 1;
    public static final int AXIS_DEDICATED_BASE = 2;
    public static final String[] AXIS_ROOT_OPTIONS = new String[]{I18n.Axis_RootDefault, I18n.Axis_RootOverridingDomain};
    public static final String[] AXIS_OPTIONS = new String[]{I18n.Axis_No, I18n.Axis_Dedicated, I18n.Axis_DedicatedOverridingDomain};
    public int axisMode;
    public String domainBase;
    public long min = Long.MIN_VALUE;
    public long max = Long.MAX_VALUE;
    public static final int AXIS_TYPE_LINEAR = 0;
    public static final int AXIS_TYPE_LOG10 = 1;
    public static final String[] AXIS_TYPE_OPTIONS = new String[]{I18n.Axis_Linear, I18n.Axis_Log10};
    public int axisType;
}

