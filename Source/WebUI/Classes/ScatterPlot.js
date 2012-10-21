function ScatterPlot(p)
{
	this.inited = false;
	this.graph = new Graph(p, p.module+'.'+p.source);
	this.module = p.module;
	this.source = p.source;
	this.width = p.width;
	this.height = p.height;
    
	this.min = (p.min ? p.min : 0);
	this.max = (p.max ? p.max : 1);
	this.scale = 1/(this.max == this.min ? 1 : this.max-this.min);
    
	this.min_x = (p.min_x ? p.min_x : this.min);
	this.max_x = (p.max_x ? p.max_x : this.max);
	this.scale_x = 1/(this.max_x == this.min_x ? 1 : this.max_x-this.min_x);
    
	this.min_y = (p.min_y ? p.min_y : this.min);
	this.max_y = (p.max_y ? p.max_y : this.max);
	this.scale_y = 1/(this.max_y == this.min_y ? 1 : this.max_y-this.min_y);
        
    this.flip_y_axis = p.flip_y_axis;

	this.LUT = makeLUTArray(p.color, ['yellow']);
	this.dot = null;

	usesData(this.module, this.source);
}



ScatterPlot.prototype.Init = function(data)
{
    var d = data[this.module][this.source];

	this.inited = true;
	this.dot = new Array(0);
	this.sizex = d[0].length;
	this.sizey = d.length;

    if(this.flip_y_axis)
    {
        for(var i=0; i<this.sizey; i++)
            this.dot.push(this.graph.AddCircle((d[i][0]-this.min_x)*this.scale_x * this.width, this.height-(d[i][1]-this.min_y)*this.scale_y * this.height, 2, this.LUT[i % this.LUT.length]));
    }
    else
    {
        for(var i=0; i<this.sizey; i++)
            this.dot.push(this.graph.AddCircle((d[i][0]-this.min_x)*this.scale_x * this.width, (d[i][1]-this.min_y)*this.scale_y * this.height, 2, this.LUT[i % this.LUT.length]));
    }
}



ScatterPlot.prototype.Update = function(data)
{
    var d = data[this.module];
    if(!d) return;
    d = d[this.source]
    if(!d) return;
    
    if(!this.inited) this.Init(data);

    if(this.flip_y_axis)
    {
        for(var i=0; i<this.sizey; i++)
            {
                this.dot[i].setAttribute("cx", (d[i][0]-this.min_x)*this.scale_x * this.width);
                this.dot[i].setAttribute("cy", this.height-(d[i][1]-this.min_y)*this.scale_y * this.height);
            }
    }
    else
    {
        for(var i=0; i<this.sizey; i++)
            {
                this.dot[i].setAttribute("cx", (d[i][0]-this.min_x)*this.scale_x * this.width);
                this.dot[i].setAttribute("cy", (d[i][1]-this.min_y)*this.scale_y * this.height);
            }
    }
}

