function PolarPlot(p)
{
	this.inited = false;
	this.graph = new Graph(p, p.module+'.'+p.source);
	this.module = p.module;
	this.source = p.source;
	this.width = p.width;
	this.height = p.height;
	this.cx = this.width/2;
	this.cy = this.height/2;
	this.r = this.cx-6;

	this.margin = (p.margin ? p.margin : 10);
	this.offset = (p.offset ? p.offset : Math.PI);
	this.axes = (p.axes != undefined ? p.axes : 5);
	
	this.grid_color = (p.grid_color ? p.grid_color : 'yellow');
	this.grid_fill = (p.grid_fill ? p.grid_fill : 'none');
	this.grid_stroke_width = (p.grid_stroke_width ? p.grid_stroke_width : 1);

	this.color = (p.color ? p.color : 'yellow');
	this.fill = (p.fill ? p.fill : '#440');
	this.stroke_width = (p.stroke_width ? p.stroke_width : 1);

	this.line_color = (p.line_color ? p.line_color : 'none');
	this.line_stroke_width = (p.line_stroke_width ? p.line_stroke_width : 1);

	this.c = this.graph.AddCircle(this.cx,  this.cy,  this.r, this.grid_fill, this.grid_color, this.grid_stroke_width);
	this.poly = this.graph.AddPolygon("0,0 1,1", this.fill, this.color, this.stroke_width);

	if(this.module)
		usesData(this.module, this.source);
}



PolarPlot.prototype.Init = function(data)
{
    var d = data[this.module][this.source];

	this.inited = true;
	this.line = new Array(0);
	this.x = new Array(0);
	this.y = new Array(0);

	this.sizex = d[0].length;
	this.sizey = d.length;

	var v=0;
	var dv = 2*Math.PI/(this.sizex*this.sizey);
	var w = this.width/2;
	var h = this.height/2;
	for(var j=0; j<this.sizey; j++)
	for(var i=0; i<this.sizex; i++)
	{
		var xx = Math.sin(v+this.offset);
		var yy = Math.cos(v+this.offset);
		this.x.push(xx);
		this.y.push(yy);
		var l = this.graph.AddLine(w, h, w+w*0*xx, h+w*0*yy, this.line_color, this.line_stroke_width);
			this.line.push(l);
		v += dv;
	}

	this.c = this.graph.AddCircle(this.cx,  this.cy,  0.25*this.r, 'none', this.grid_color, this.grid_stroke_width);
	this.c = this.graph.AddCircle(this.cx,  this.cy,  0.50*this.r, 'none', this.grid_color, this.grid_stroke_width);
	this.c = this.graph.AddCircle(this.cx,  this.cy,  0.75*this.r, 'none', this.grid_color, this.grid_stroke_width);

	for(var i=0; i<this.axes; i++)
		this.graph.AddLine(this.cx, this.cy, this.cx+this.r*Math.sin(i*2*Math.PI/this.axes+this.offset), this.cy+this.r*Math.cos(i*2*Math.PI/this.axes+this.offset), this.grid_color, this.grid_stroke_width);
}



PolarPlot.prototype.Update = function(data)
{
    var d = data[this.module];
    if(!d) return;
    d = d[this.source]
    if(!d) return;
    
    if(!this.inited) this.Init(data);

	var b = 0;
	var p = "";
	for(var j=0; j<this.sizey; j++)
	for(var i=0; i<this.sizex; i++)
	{
		var v = d[j][i];
		this.line[i].setAttribute('x2', this.cx+(this.r-this.margin)*this.x[b]*v);
		this.line[i].setAttribute('y2', this.cy+(this.r-this.margin)*this.y[b]*v);
		var xxx =  this.cx+(this.r-this.margin)*this.x[b]*v;
		var yyy = this.cy+(this.r-this.margin)*this.y[b]*v;
		p += " "+xxx+","+yyy+" ";
		b++;
	}
	
	this.poly.setAttribute("points", p);
}
