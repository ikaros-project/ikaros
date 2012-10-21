function Plot(p)
{
	this.inited = false;
	this.graph = new Graph(p, p.module+'.'+p.source);
	this.module = p.module;
	this.source = p.source;
    this.type = (p.type ? p.type : 'normal');
	this.width = p.width-16;
	this.height = p.height-16;
	this.min = (p.min ? p.min : 0);
	this.max = (p.max ? p.max : 1);
	this.scale = (this.height-6-26)/(this.max-this.min);
	this.zero = this.scale*(0-this.min);
	this.bottom = this.height-3;
	this.position = 1;
	this.LUT = makeLUTArray(p.color);
    this.stroke_width = (p.stroke_width ? p.stroke_width : 1);
	this.select = makeSelectionArray(p.select);
    this.stack = (this.type == "stack" ? 1 : 0);
    if(this.stack)
        this.scale /= this.select.length;
 	usesData(this.module, this.source);
}



Plot.prototype.Init = function(data)
{
    var d = data[this.module][this.source];

	var i, k;
	this.inited = true;
    this.mark = this.graph.AddLine(0, 2, 0, this.height+16, 'black', 1);
    this.buffer = [];
    this.offset = [];
    var h = (this.height-6-26)/this.select.length;
    for(i in this.select)
    {
        this.offset[i] = this.stack*h*i;
        this.graph.AddLine(0, this.bottom-this.zero+8-this.offset[i], this.width+16, this.bottom-this.zero+8-this.offset[i], '#303030', 1);
        var c = this.LUT[i % this.LUT.length];
        var n = [];
        this.buffer.push(n);
        for(k=0; k<this.width; k++)
            n.push(this.graph.AddLine(k+8, this.bottom-this.zero+8-this.offset[i], k+1+8, this.bottom-this.zero+8-this.offset[i], c, this.stroke_width));
    }
    
    if(!this.stack)
    {
        this.graph.AddText(this.width-1+8, 10+8+26, this.max, 40, 12, 'end');
        this.graph.AddText(this.width-1+8, this.height-3+8, this.min, 40, 12, 'end');
    }
};



Plot.prototype.Update = function(data)
{
    var d = data[this.module];
    if(!d) return;
    d = d[this.source]
    if(!d) return;
    
    if(!this.inited) this.Init(data);

	var n;
    
    for(n in this.buffer)
    {
        var v = this.scale*(d[this.select[n][1]][this.select[n][0]]-this.min)+this.offset[n];
        this.buffer[n][this.position].setAttribute('y2', this.bottom - v+8);
        if(this.position>0)
            this.buffer[n][this.position].setAttribute('y1', this.buffer[n][this.position-1].getAttribute('y2'));
        else
            this.buffer[n][this.position].setAttribute('y1', this.buffer[n][this.width-1].getAttribute('y2'));
    }
    
    this.position++;
    if(this.position >= this.width)
        this.position = 0;

    this.mark.setAttribute('x1', this.position+8);
    this.mark.setAttribute('x2', this.position+8);
};
