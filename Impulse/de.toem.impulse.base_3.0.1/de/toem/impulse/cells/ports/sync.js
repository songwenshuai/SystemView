// base        : Root cell (de.toem.impulse.cells.record.RecordContent) 
// insertPoint : rRoot cell of this port (de.toem.impulse.cells.record.PortScope) 
// isync       : Sync interface (de.toem.impulse.cells.ports.IPortSync) 
// console     : Console output (de.toem.impulse.scripting.IScriptConsole)


var reset/*:ISamplePointer:*/ = isync.getPointer( 'wavetest\\reset_s');

if (reset != null){
	if (reset.goNextEdge( 0, null)){
		isync.setSynced(reset.getPosition());
		console.println( "Synced: "+reset.getPosition());
	}else
		console.println( "Sync not found!");
}else
		console.println( "Signal not found!");