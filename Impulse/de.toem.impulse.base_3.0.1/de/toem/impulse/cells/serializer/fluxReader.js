// trace       : Trace object (de.toem.impulse.flux.IFluxInitialisation)
// progress    : Progress control (de.toem.impulse.cells.ports.IPortProgress)
// console     : Console output (de.toem.impulse.scripting.IScriptConsole)

// handler

function handleCreated(trace){
	
	// add content
	flxAddSignal(trace, 1, 0, "integer", "an integer", FLX_TYPE_INTEGER, 0);
	flxAddSignal(trace, 2, 0, "float", "a float", FLX_TYPE_FLOAT, 0);
	flxAddSignal(trace, 3, 0, "event", "an event", FLX_TYPE_EVENT, 0);
}

function handleOpened(trace,openId){

	// write enums for signal 3 (event)
	flxWriteEnumDef(trace, 3, FLX_ENUM_GLOBAL, "Yes", 1);
	flxWriteEnumDef(trace, 3, FLX_ENUM_GLOBAL, "No", 0);
}

function handleClosed(trace,openId){


}

// c/js compatibility helper - do not change


var FLX_OK = 0;
var FLX_ERROR =-1;

var FLX_TYPE_UNKNOWN=  0;
var FLX_TYPE_EVENT = 1;
var FLX_TYPE_INTEGER  =2;
var FLX_TYPE_LOGIC=  3;
var FLX_TYPE_FLOAT  =4;
var FLX_TYPE_TEXT = 5;
var FLX_TYPE_BINARY = 6;
var FLX_TYPE_STRUCT  =7;
var FLX_TYPE_EVENT_ARRAY = 8;
var FLX_TYPE_INTEGER_ARRAY=  9;
var FLX_TYPE_FLOAT_ARRAY=  10;
var FLX_TYPE_TEXT_ARRAY = 11;

var FLX_STRUCTTYPE_UNKNOWN = 0;
var FLX_STRUCTTYPE_TEXT = 1;
var FLX_STRUCTTYPE_ENUM = 2;
var FLX_STRUCTTYPE_INTEGER = 3;
var FLX_STRUCTTYPE_FLOAT=  4;
var FLX_STRUCTTYPE_BINARY = 6;
var FLX_STRUCTTYPE_LOCAL_ENUM = 7;
var FLX_STRUCTTYPE_MERGE_ENUM  =8;
var FLX_STRUCTTYPE_MASK_BASE = 0x0f;
var FLX_STRUCTTYPE_MOD_HIDDEN = 0x80;

var FLX_ENUM_GLOBAL = 0
var FLX_ENUM_ASSOC_TARGET = 1
var FLX_ENUM_ASSOC_STYLE = 2
var FLX_ENUM_LABEL_STYLE = 3
var FLX_ENUM_MEMBER_0 = 8

function flxAddSignal(trace, itemId, parentId,
    name, description, signalType,
    signalDescriptor) {
    trace.flxAddSignal(itemId, parentId,
        name, description, signalType,
        signalDescriptor);
}

function flxAddScatteredSignal(trace, itemId, parentId,
    name, description, signalType,
    signalDescriptor, scatteredFrom, scatteredTo) {
    trace.flxAddScatteredSignal(itemId, parentId,
        name, description, signalType,
        signalDescriptor, scatteredFrom, scatteredTo);
}

function flxAddSignals(trace, itemIdFrom, itemIdTo,
    parentId, name, description, signalType,
    signalDescriptor) {
    trace.flxAddSignals(itemIdFrom, itemIdTo,
        parentId, name, description, signalType,
        signalDescriptor);
}

function flxAddSignalReference(trace, referenceId,
    parentId, name, description) {
    trace.flxAddSignalReference(referenceId,
        parentId, name, description);
}

function flxAddScatteredSignalReference(trace, referenceId,
    parentId, name, description,
    scatteredFrom, scatteredTo) {
    trace.flxAddScatteredSignalReference(referenceId,
        parentId, name, description,
        scatteredFrom, scatteredTo);
}

function flxAddScope(trace, itemId, parentId,
    name, description) {
    trace.flxAddScope(itemId, parentId,
        name, description);
}

function flxOpen(trace, itemId, domainBase, start, rate) {
    trace.flxOpen(itemId, domainBase, start, rate);
}

function flxClose(trace, itemId, end) {
    trace.flxClose(itemId, end);
}

function flxWriteEnumDef(trace, itemId, enumeration,
    label, value) {
    trace.flxWriteEnumDef(itemId, enumeration,
        label, value);
}

function flxWriteArrayDef(trace, itemId, index,
    label) {
    trace.flxWriteArrayDef(itemId, index,
        label);
}

function flxWriteMemberDef(trace, itemId,
    memberId, label,
    memberType, memberDescriptor) {
    trace.flxWriteMemberDef(itemId,
        memberId, label,
        memberType, memberDescriptor);
}
