/*
 * Decompiled with CFR 0.152.
 */
package de.toem.impulse.samples.base;

import de.toem.impulse.domain.DomainValue;
import de.toem.impulse.samples.IMemberDescriptor;
import de.toem.impulse.samples.IReadableSamples;
import de.toem.impulse.samples.ISamplesLegend;
import de.toem.impulse.samples.ISamplesProducer;
import de.toem.impulse.samples.ISamplesReader;
import de.toem.impulse.samples.base.Samples;
import de.toem.impulse.samples.base.SamplesStat;
import de.toem.impulse.values.CompoundPack;
import de.toem.impulse.values.CompoundValue;
import de.toem.impulse.values.Enumeration;
import de.toem.impulse.values.GroupedValue;
import de.toem.impulse.values.IAttachment;
import de.toem.toolkits.pattern.threading.IProgress;
import java.util.List;

public class ReadableSamples
extends Samples
implements IReadableSamples {
    @Override
    public ISamplesLegend getLegend() {
        return null;
    }

    @Override
    public ISamplesReader getReader() {
        return null;
    }

    @Override
    public ISamplesProducer getProducer() {
        return null;
    }

    @Override
    public boolean isEmpty() {
        return false;
    }

    @Override
    public int getGroups() {
        return 0;
    }

    @Override
    public boolean isSettled() {
        return false;
    }

    @Override
    public boolean ensureSettled(IProgress p) {
        return false;
    }

    @Override
    public boolean isSettling() {
        return false;
    }

    @Override
    public int indexAt(long units) {
        return 0;
    }

    @Override
    public int indexAt(DomainValue position) {
        return 0;
    }

    @Override
    public DomainValue positionAt(int idx) {
        return null;
    }

    @Override
    public boolean isNoneAt(int idx) {
        return false;
    }

    @Override
    public boolean isTaggedAt(int idx) {
        return false;
    }

    @Override
    public boolean isConflictAt(int idx) {
        return false;
    }

    @Override
    public int getTagAt(int idx) {
        return 0;
    }

    @Override
    public CompoundValue compoundAt(int idx) {
        return null;
    }

    @Override
    public CompoundValue compoundAt(int idx, int attachments) {
        return null;
    }

    @Override
    public CompoundPack packedAt(int idx) {
        return null;
    }

    @Override
    public List<IAttachment> attachmentsAt(int idx, int type) {
        return null;
    }

    @Override
    public GroupedValue valuesAtGroup(int group) {
        return null;
    }

    @Override
    public GroupedValue valuesAtGroup(int group, int attachments) {
        return null;
    }

    @Override
    public List<IAttachment> attachmentsAtGroup(int group, int type) {
        return null;
    }

    @Override
    public int indexAtGroup(int group) {
        return 0;
    }

    @Override
    public List<IMemberDescriptor> getMemberDescriptors() {
        return null;
    }

    @Override
    public List<Enumeration> getEnums(int enumerationType) {
        return null;
    }

    @Override
    public IMemberDescriptor getMemberDescriptor(Object memberIdentifier) {
        return null;
    }

    @Override
    public List<Enumeration> getMemberEnums(Object memberIdentifier) {
        return null;
    }

    @Override
    public Enumeration getMemberEnum(Object memberIdentifier, String label) {
        return null;
    }

    @Override
    public Enumeration getMemberEnum(Object memberIdentifier, int value) {
        return null;
    }

    @Override
    public List<Object> membersWithContent(String content) {
        return null;
    }

    @Override
    public SamplesStat getStat(int idx0, int idxN, int content) {
        return null;
    }

    @Override
    public int getCount() {
        return 0;
    }
}

