/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.values;

import de.toem.impulse.domain.IDomainBase;
import de.toem.toolkits.pattern.element.Link;
import de.toem.toolkits.pattern.json.IJsonBase;

public interface IAttachment
extends IJsonBase {
    public String getStyle();

    public boolean showMessage();

    public String getMessage();

    public int[] getRgb();

    public String getSymbol();

    public String format(int var1);

    public long getSourceUnits();

    public int getSourceIdx();

    public int getSourceGroup();

    public int getSourceLayer();

    public static interface IAttachedLabel
    extends IAttachment {
    }

    public static interface IAttachedRelation
    extends IAttachment {
        public static final int FORMAT_DEFAULT = -1;
        public static final int FORMAT_TARGET = 1;
        public static final int FORMAT_MESSAGE_TARGET = 2;
        public static final int FORMAT_SOURCE_MESSAGE_TARGET = 3;

        public int getType();

        public boolean isReverse();

        public boolean isDelta();

        public boolean hasTargetIdx();

        public boolean hasTargetLayer();

        public String getLineStyle();

        public String getArrowStyle();

        public String getTargetId();

        public long getTargetPosition();

        public IDomainBase getTargetBase();

        public int getTargetIdx();

        public int getTargetLayer();

        public Link getLink();

        public long getAbsoluteTargetUnits(IDomainBase var1);
    }
}

