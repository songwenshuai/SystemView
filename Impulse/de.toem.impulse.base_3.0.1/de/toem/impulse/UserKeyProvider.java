/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse;

import de.toem.impulse.IKeyListener;
import de.toem.impulse.IKeyProvider;
import de.toem.impulse.ImpulsePreferences;
import de.toem.impulse.cells.preferences.LicensePreferences;
import de.toem.toolkits.pattern.element.ElementListener;
import de.toem.toolkits.pattern.element.ElementModifierEvent;
import de.toem.toolkits.pattern.element.ICell;
import de.toem.toolkits.pattern.element.IElement;
import de.toem.toolkits.pattern.registry.AbstractRegistryObject;
import de.toem.toolkits.pattern.registry.IRegistry;
import de.toem.toolkits.text.MultilineText;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class UserKeyProvider
extends AbstractRegistryObject
implements IKeyProvider {
    List<IKeyListener> listeners = new ArrayList<IKeyListener>();

    public UserKeyProvider() {
        ImpulsePreferences.licensePreferences.addListener(new ElementListener(){

            @Override
            public void elementModified(ElementModifierEvent event) {
                if (event != null && event.isTypeDone()) {
                    UserKeyProvider.this.fireEvent();
                }
            }

            @Override
            public void elementResetted(IElement element) {
                UserKeyProvider.this.fireEvent();
            }
        });
    }

    public UserKeyProvider(AbstractRegistryObject base) {
        super(base);
    }

    public UserKeyProvider(IRegistry registry) {
        super(registry);
    }

    public UserKeyProvider(String id) {
        super(id);
    }

    private void fireEvent() {
        IKeyListener[] iKeyListenerArray = this.listeners.toArray(new IKeyListener[this.listeners.size()]);
        int n = iKeyListenerArray.length;
        int n2 = 0;
        while (n2 < n) {
            IKeyListener listener = iKeyListenerArray[n2];
            listener.keyChanged(this);
            ++n2;
        }
    }

    @Override
    public String getKey() {
        ICell cell = ImpulsePreferences.licensePreferences.getCell();
        if (cell instanceof LicensePreferences && ((LicensePreferences)cell).key != null) {
            return MultilineText.toAscii(((LicensePreferences)cell).key).replaceAll("\\s", "");
        }
        return "";
    }

    @Override
    public int getPriority() {
        return 64;
    }

    @Override
    public void addListener(IKeyListener listener) {
        this.listeners.add(listener);
    }

    @Override
    public void removeListener(IKeyListener listener) {
        this.listeners.remove(listener);
    }

    @Override
    public Map<String, String> getPreset() {
        return null;
    }
}

