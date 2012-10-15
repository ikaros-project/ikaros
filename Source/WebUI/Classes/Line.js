function Line(p)
{
	this.inited = false;
    p.frame = (p.frame ? p.frame : 'none');
	this.graph = new Graph(p, p.module+'.'+p.source);
	this.color = (p.color ? p.color : 'yellow');
	this.stroke_width = (p.stroke_width ? p.stroke_width : 1);		
	this.module = p.module;
	this.source = p.source;
	this.width = p.width;
	this.height = p.height;
	this.point = null;

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

	usesData(this.module, this.source);
}



Line.prototype.Init = function(data)
{
    var d = data[this.module][this.source];

    this.inited = true;

    this.point = new Array(0);
    this.sizex = d[0].length;
    this.sizey = d.length;

    if(this.flip_y_axis)
    {
        for(var i=0; i<this.sizey-1; i++)
            this.point.push(this.graph.AddLine(
                (d[i][0]-this.min_x)*this.scale_x * this.width, 
                this.height-(d[i][1]-this.min_y)*this.scale_y * this.height, 
                (d[i+1][0]-this.min_x)*this.scale_x * this.width, 
                this.height-(d[i+1][1]-this.min_y)*this.scale_y * this.height, 
                this.color, this.stroke_width));
    }
    else
    {
        for(var i=0; i<this.sizey-1; i++)
            this.point.push(this.graph.AddLine(
                (d[i][0]-this.min_x)*this.scale_x * this.width, 
                (d[i][1]-this.min_y)*this.scale_y * this.height, 
                (d[i+1][0]-this.min_x)*this.scale_x * this.width, 
                (d[i+1][1]-this.min_y)*this.scale_y * this.height, 
                this.color, this.stroke_width));
    }
}



Line.prototype.Update = function(data)
{
    var d = data[this.module];
    if(!d) return;
    d = d[this.source];
    if(!d) return;
    
    if(!this.inited) this.Init(data);

    if(this.flip_y_axis)
    {
        for(var i=0; i<this.sizey-1; i++)
        {
            this.point[i].setAttribute("x1", (d[i][0]-this.min_x)*this.scale_x * this.width);
            this.point[i].setAttribute("y1", this.height-(d[i][1]-this.min_y)*this.scale_y * this.height);
            this.point[i].setAttribute("x2", (d[i+1][0]-this.min_x)*this.scale_x * this.width);
            this.point[i].setAttribute("y2", this.height-(d[i+1][1]-this.min_y)*this.scale_y * this.height);
        }
    }
    else
    {
        for(var i=0; i<this.sizey-1; i++)
        {
            this.point[i].setAttribute("x1", (d[i][0]-this.min_x)*this.scale_x * this.width);
            this.point[i].setAttribute("y1", (d[i][1]-this.min_y)*this.scale_y * this.height);
            this.point[i].setAttribute("x2", (d[i+1][0]-this.min_x)*this.scale_x * this.width);
            this.point[i].setAttribute("y2", (d[i+1][1]-this.min_y)*this.scale_y * this.height);
        }
    }
}
