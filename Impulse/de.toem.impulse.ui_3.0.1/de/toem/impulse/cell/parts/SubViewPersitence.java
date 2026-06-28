/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.cell.parts;

import de.toem.impulse.i18n.I18n;
import de.toem.toolkits.pattern.element.CellAnnotation;
import de.toem.toolkits.pattern.element.Link;
import de.toem.toolkits.ui.eclipse.part.session.view.AbstractInputViewPersitence;

@CellAnnotation(type="persitence.impulse.subview", dynamicChildOf={"preferences.impulse.parts"})
public class SubViewPersitence
extends AbstractInputViewPersitence {
    public static final String TYPE = "persitence.impulse.subview";
    public boolean enableGroups = true;
    public Link partPreferences = Link.fromPath(I18n.General_Default);
}

