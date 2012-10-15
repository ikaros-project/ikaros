function Cross(p)
{
	this.inited = false;
	this.module = p.module;
	this.source = p.source;
	this.width = p.width;
	this.height = p.height;
	this.LUT = makeLUTArray(p.color, ['yellow']);
	this.stroke_width = (p.stroke_width ? p.stroke_width : 3);
	this.size = (p.size ? p.size*p.width : 0.1*p.width);
	this.graph = new Graph(p, p.module+'.'+p.source);
	

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

	if(!this.module)
	{
		var mx = p.width/2;
		var my = p.height/2;
		this.markerV = this.graph.AddLine(mx-this.size, my, mx+this.size, my, this.LUT[0], this.stroke_width);
		this.markerH = this.graph.AddLine(mx, my-this.size, mx, my+this.size, this.LUT[0], this.stroke_width);
	}
	else
		usesData(this.module, this.source);
}



Cross.prototype.Init = function(data)
{
    var d = data[this.module][this.source];

    this.inited = true;

    this.markerV = new Array(0);
    this.markerH = new Array(0);
    this.sizex = d[0].length;
    this.sizey = d.length;
    
    if(this.flip_y_axis)
    {
        for(var i=0; i<this.sizey; i++)
        {
            var mx = (d[i][0]-this.min_x)*this.scale_x * this.width;
            var my = this.height-(d[i][1]-this.min_y)*this.scale_y * this.height;
            this.markerV.push(this.graph.AddLine(mx-this.size, my, mx+this.size, my, this.LUT[i % this.LUT.length], this.stroke_width));
            this.markerH.push(this.graph.AddLine(mx, my-this.size, mx, my+this.size, this.LUT[i % this.LUT.length], this.stroke_width));
        }
    }
    else
    {
        for(var i=0; i<this.sizey; i++)
        {
            var mx = (d[i][0]-this.min_x)*this.scale_x * this.width;
            var my = (d[i][1]-this.min_y)*this.scale_y * this.height;
            this.markerV.push(this.graph.AddLine(mx-this.size, my, mx+this.size, my, this.LUT[i % this.LUT.length], this.stroke_width));
            this.markerH.push(this.graph.AddLine(mx, my-this.size, mx, my+this.size, this.LUT[i % this.LUT.length], this.stroke_width));
        }
    }
}



Cross.prototype.Update = function(data)
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
            var mx = (d[i][0]-this.min_x)*this.scale_x * this.width;
            var my = this.height-(d[i][1]-this.min_y)*this.scale_y * this.height;

            this.markerV[i].setAttribute("x1", mx);
            this.markerV[i].setAttribute("x2", mx);
            this.markerV[i].setAttribute("y1", my-this.size);
            this.markerV[i].setAttribute("y2", my+this.size);

            this.markerH[i].setAttribute("x1", mx-this.size);
            this.markerH[i].setAttribute("x2", mx+this.size);
            this.markerH[i].setAttribute("y1", my);
            this.markerH[i].setAttribute("y2", my);
        }
    }
    else
    {
        for(var i=0; i<this.sizey; i++)
        {
            var mx = (d[i][0]-this.min_x)*this.scale_x * this.width;
            var my = (d[i][1]-this.min_y)*this.scale_y * this.height;

            this.markerV[i].setAttribute("x1", mx);
            this.markerV[i].setAttribute("x2", mx);
            this.markerV[i].setAttribute("y1", my-this.size);
            this.markerV[i].setAttribute("y2", my+this.size);

            this.markerH[i].setAttribute("x1", mx-this.size);
            this.markerH[i].setAttribute("x2", mx+this.size);
            this.markerH[i].setAttribute("y1", my);
            this.markerH[i].setAttribute("y2", my);
        }
    }
}


