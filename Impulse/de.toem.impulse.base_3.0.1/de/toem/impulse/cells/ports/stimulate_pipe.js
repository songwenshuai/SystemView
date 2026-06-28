// file        : Input file (java.io.File)
// progress    : Progress control (de.toem.impulse.cells.ports.IPortProgress)
// console     : Console output (de.toem.impulse.scripting.IScriptConsole)

var Runtime /*:@java.lang.Runtime:*/ = Java.type("java.lang.Runtime");

console.log("Started");
Runtime.getRuntime().exec("ls | ~/pipe");
console.log("Ended");