// painter : Painter object (de.toem.eclipse.toolkits.tlk.ITlkPainter)
// x,y,width,height: Geometry (int)
// color, background: Colors (Object)
// readable: Signal input (de.toem.impulse.samples.IReadableSamples)
// console: Console output (de.toem.impulse.scripting.IScriptConsole)

painter.setForeground(color);
var count = readable.getCount();
var first = readable.positionAt(0);
var last = readable.positionAt(count-2);
var value0 = readable.valueAt(0);
var valueN = readable.valueAt(count-2);
painter.drawText("Count: " +count,x+20,y+20,true);
painter.drawText("From: " +first,x+20,y+40,true);
painter.drawText("To: " +last,x+20,y+60,true);
painter.drawText("Value[0]: " +value0,x+20,y+80,true);
painter.drawText("Value[N]: " +valueN,x+20,y+100,true);

painter.setBackground(color);
painter.fillRectangle(x+20,y+120,width-40,20);
painter.setBackground(painter.getDevice().getSystemColor(7));
painter.fillRectangle(x+20,y+150,width-40,20);
painter.setBackground(painter.getDevice().getSystemColor(15));
painter.fillRectangle(x+width - 50,y+20,20,height - 40);

painter.setForeground(color);
painter.setBackground(background);
painter.drawText("Impulse Script Chart ",x+20,y+180);