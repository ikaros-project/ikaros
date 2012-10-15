function Trace(p)
{
	this.inited = false;
	this.graph = new Graph(p, p.module+'.'+p.source);
	
	this.module = p.module;
	this.source = p.source;
	this.width = p.width;
	this.height = p.height;

	this.length = (p.length ? p.length : 10);
	this.color = (p.color ? p.color : 'yellow');
	this.stroke_width = (p.trace_width ? p.trace_width : (p.stroke_width ? p.stroke_width : 1));

	var tx = p.width/2;
	var ty = p.height/2;

	this.trace = new Array(0);
	for(var i=0; i<this.length; i++)
		this.trace.push(this.graph.AddLine(tx, ty, tx+0.01, ty, this.color, this.stroke_width));
		
	this.trace_cur = 0;
	this.trace_last_x = tx;
	this.trace_last_y = ty;
	
	usesData(this.module, this.source);
}



Trace.prototype.Update = function(data)
{	
    var d = data[this.module];
    if(!d) return;
    d = d[this.source]
    if(!d) return;
    
    var tx = this.width*d[0][0];
    var ty = this.height*d[0][1];
        
    this.trace[this.trace_cur].setAttribute("x2", tx); 
    this.trace[this.trace_cur].setAttribute("y2", ty);
        
    this.trace[this.trace_cur].setAttribute("x1", this.trace_last_x);
    this.trace[this.trace_cur].setAttribute("y1", this.trace_last_y);

    this.trace_last_x = tx;
    this.trace_last_y = ty;
        
    this.trace_cur++;
    if(this.trace_cur >= this.length)
        this.trace_cur = 0;

}
