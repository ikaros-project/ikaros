function Plot3D(p)
{
	this.inited = false;
	this.graph = new Graph(p, p.module+'.'+p.source);
	this.module = p.module;
	this.source = p.source;
	this.width = p.width+4;
	this.height = p.height+4;
	this.max = (p.max ? p.max : 1);
	this.LUT = makeLUTArray(p.color, LUT_fire);
	this.scale = (this.LUT.length-1)/this.max;
	this.stroke = (p.stroke ? p.stroke :  'none');
	this.stroke_width = (p.stroke_width ? p.stroke_width :  (p.stroke != 'none' ? 0.5 : 0));

	usesData(this.module, this.source);
}



Plot3D.prototype.trans = function (x, y, v)
{
	return {
		x: (2/3)*this.box_width*x + (1/3)*this.width-(1/3)*this.width*y/this.sizey,
		y: (1/3)*this.box_height*y+(3/6)*this.height - v*(1/3)*this.height/this.max
	};
};



Plot3D.prototype.Init = function(data)
{
    var d = data[this.module][this.source];
    
    this.inited = true;
    
    this.sizey = d.length;
    this.sizex = d[0].length;
    this.box_width = (this.width-4)/this.sizex;
    this.box_height = (this.height-4)/this.sizey;
    this.box = new Array(this.sizey);
	
    for(var j=0; j<this.sizey-1; j++)
    {
        this.box[j] = new Array(this.sizex);
        for(var i=0; i<this.sizex-1; i++)
        {
            var p0 = this.trans(i, j, 0);
            var p1 = this.trans(i+1, j, 0);
            var p2 = this.trans(i+1, j+1, 0);
            var p3 = this.trans(i, j+1, 0);
            this.box[j][i] = this.graph.AddPolygon(p0.x+','+p0.y+' '+p1.x+','+p1.y+' '+p2.x+','+p2.y+' '+p3.x+','+p3.y, 'none', this.stroke, this.stroke_width);
        }
    }
};



Plot3D.prototype.Update = function(data)
{
    var d = data[this.module];
    if(!d) return;
    d = d[this.source]
    if(!d) return;
    
    if(!this.inited) this.Init(data);

    var round = Math.round;
    
    for(var j=this.sizey-1; j--;)
    {
        var bj = this.box[j];
        for(var i=this.sizex-1; i--;)
        {	
            var p0 = this.trans(i, j, d[j][i]);
            var p1 = this.trans(i+1, j, d[j][i+1]);
            var p2 = this.trans(i+1, j+1, d[j+1][i+1]);
            var p3 = this.trans(i, j+1, d[j+1][i]);
            var mean = 0.25*(d[j][i] + d[j+1][i] + d[j][i+1] + d[j+1][i+1]);
            bj[i].setAttribute("points", p0.x+','+p0.y+' '+p1.x+','+p1.y+' '+p2.x+','+p2.y+' '+p3.x+','+p3.y);
            bj[i].setAttribute("fill", this.LUT[round(this.scale*mean)]);
        }
    }
};

