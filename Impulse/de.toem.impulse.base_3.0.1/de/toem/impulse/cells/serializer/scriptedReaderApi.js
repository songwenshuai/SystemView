
function initRecord(name, domainBase) { reader.initRecord(name, domainBase); }

function addScope(container, name) { return reader.addScope(container, name); }

function addScope(container, name, description) { return reader.addScope(container, name, description); }

function addSignal(container, name, description, processType, signalType, signalDescriptor,
    domainBase) {
    return reader.addSignal(container, name, description, processType, signalType, signalDescriptor,
        domainBase);
}

function addSignal(container, name, description, processType, signalType, signalDescriptor,
    domainBase, createWriter) {
    return reader.addSignal(container, name, description, processType, signalType, signalDescriptor,
        domainBase, createWriter);
}
function addSignal(container, name, description, processType, signalType, signalDescriptor) {
    return reader.addSignal(container, name, description, processType, signalType, signalDescriptor);
}
function getWriter(signal) { return reader.getWriter(signal); }
function open(units) { reader.open(units); }
function close(units) { reader.close(units); }
