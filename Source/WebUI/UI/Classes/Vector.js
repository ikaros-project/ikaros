function Vector(p)
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
	this.LUT = makeLUTArray(p.color, ['black']);
	this.arrow = (p.arrow ? p.arrow : 'yes');
	this.margin = (p.margin ? p.margin : 10);
	this.normalize = (p.normalize ? p.normalize : 'yes');
	this.circle_color = (p.circle_color ? p.circle_color : 'gray');
	this.stroke_width = (p.stroke_width ? p.stroke_width : 3);
    
	this.c = this.graph.AddCircle(this.cx,  this.cy,  this.r, this.circle_color, 'none');

	if(this.module)
		usesData(this.module, this.source);
}



Vector.prototype.Init = function(data)
{
    var d = data[this.module][this.source];

    this.inited = true;
    this.line = new Array(0);
    this.sizex = d[0].length;
    this.sizey = d.length;

    for(var i=0; i<this.sizey; i++)
    {
	var l = this.graph.AddLine(this.width/2, this.height/2, this.width/2, this.height/2, this.LUT[i % this.LUT.length], this.stroke_width);
	if(this.arrow =="yes")
		l.setAttribute("marker-end", "url(#Triangle)");
        this.line.push(l);
    }
}



Vector.prototype.Update = function(data)
{
    var d = data[this.module];
    if(!d) return;
    d = d[this.source]
    if(!d) return;
    
    if(!this.inited) this.Init(data);

	for(var i=0; i<this.sizey; i++)
	{
		var a = d[i][0];
		var b = d[i][1];
		if(this.normalize == 'yes')
		{
			var L = Math.sqrt(a*a+b*b);
			if(L != 0) {
				a /= L;
				b /= L;
			}
		}
		this.line[i].setAttribute('x2', this.cx+(this.r-this.margin)*a);
		this.line[i].setAttribute('y2', this.cy+(this.r-this.margin)*b);
	}
}
