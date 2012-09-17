function Rectangle(p)
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

        this.markerTop.push(this.graph.AddLine(mx-this.size, my-this.size, mx+this.size, my-this.size, this.LUT[i % this.LUT.length], this.stroke_width));
        this.markerLeft.push(this.graph.AddLine(mx+this.size, my-this.size, mx+this.size, my+this.size, this.LUT[i % this.LUT.length], this.stroke_width));
        this.markerBottom.push(this.graph.AddLine(mx+this.size, my+this.size, mx-this.size, my+this.size, this.LUT[i % this.LUT.length], this.stroke_width));
        this.markerRight.push(this.graph.AddLine(mx-this.size, my+this.size, mx-this.size, my-this.size, this.LUT[i % this.LUT.length], this.stroke_width));
	}
	else
		usesData(this.module, this.source);
}



Rectangle.prototype.Init = function(data)
{
    var d = data[this.module][this.source];

    this.inited = true;

    this.markerTop      = new Array(0);
    this.markerLeft     = new Array(0);
    this.markerBottom   = new Array(0);
    this.markerRight    = new Array(0);

    this.sizex = d[0].length;
    this.sizey = d.length;
    
    if(this.flip_y_axis)
    {
        for(var i=0; i<this.sizey; i++)
        {
            var mx = (d[i][0]-this.min_x)*this.scale_x * this.width;
            var my = this.height-(d[i][1]-this.min_y)*this.scale_y * this.height;
            
            var kc = 1;
            var ks = 0;
            
            if(this.sizex > 2)
            {
                kc = Math.cos(-d[i][2]);
                ks = Math.sin(-d[i][2]);
            }

            var sx = this.size;
            var sy = this.size;
            
            if(d[i][4])
            {
                sx = sx * d[i][4];
                sy = sy * d[i][4];
            }
            
            var mx0 = mx + kc*sx - ks*sy;
            var my0 = my + ks*sx + kc*sy;

            var mx1 = mx - kc*sx - ks*sy;
            var my1 = my - ks*sx + kc*sy;

            var mx2 = mx - kc*sx + ks*sy;
            var my2 = my - ks*sx - kc*sy;

            var mx3 = mx + kc*sx + ks*sy;
            var my3 = my + ks*sx - kc*sy;

            
            this.markerTop.push(this.graph.AddLine(mx0, my0, mx1, my1, this.LUT[i % this.LUT.length], this.stroke_width));
            this.markerLeft.push(this.graph.AddLine(mx1, my1, mx2, my2, this.LUT[i % this.LUT.length], this.stroke_width));
            this.markerBottom.push(this.graph.AddLine(mx2, my2, mx3, my3, this.LUT[i % this.LUT.length], this.stroke_width));
            this.markerRight.push(this.graph.AddLine(mx3, my3, mx0, my0, this.LUT[i % this.LUT.length], this.stroke_width));
        }
    }
    else
    {
        for(var i=0; i<this.sizey; i++)
        {
            var mx = (d[i][0]-this.min_x)*this.scale_x * this.width;
            var mx = (d[i][0]-this.min_x)*this.scale_x * this.width;

            var kc = 1;
            var ks = 0;
            
            if(this.sizex > 2)
            {
                kc = Math.cos(d[i][2]);
                ks = Math.sin(d[i][2]);
            }
            
            var sx = this.size;
            var sy = this.size;
            
            if(d[i][4])
            {
                sx = sx * d[i][4];
                sy = sy * d[i][4];
            }
            
            var mx0 = mx + kc*sx - ks*sy;
            var my0 = my + ks*sx + kc*sy;

            var mx1 = mx - kc*sx - ks*sy;
            var my1 = my - ks*sx + kc*sy;

            var mx2 = mx - kc*sx + ks*sy;
            var my2 = my - ks*sx - kc*sy;

            var mx3 = mx + kc*sx + ks*sy;
            var my3 = my + ks*sx - kc*sy;

            
            this.markerTop.push(this.graph.AddLine(mx0, my0, mx1, my1, this.LUT[i % this.LUT.length], this.stroke_width));
            this.markerLeft.push(this.graph.AddLine(mx1, my1, mx2, my2, this.LUT[i % this.LUT.length], this.stroke_width));
            this.markerBottom.push(this.graph.AddLine(mx2, my2, mx3, my3, this.LUT[i % this.LUT.length], this.stroke_width));
            this.markerRight.push(this.graph.AddLine(mx3, my3, mx0, my0, this.LUT[i % this.LUT.length], this.stroke_width));
        }
    }
}



Rectangle.prototype.Update = function(data)
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

            var kc = 1;
            var ks = 0;
            
            if(this.sizex > 2)
            {
                kc = Math.cos(-d[i][2]);
                ks = Math.sin(-d[i][2]);
            }

            var sx = this.size;
            var sy = this.size;
            
            if(d[i][4])
            {
                sx = sx * d[i][4];
                sy = sy * d[i][4];
            }
            
            var mx0 = mx + kc*sx - ks*sy;
            var my0 = my + ks*sx + kc*sy;

            var mx1 = mx - kc*sx - ks*sy;
            var my1 = my - ks*sx + kc*sy;

            var mx2 = mx - kc*sx + ks*sy;
            var my2 = my - ks*sx - kc*sy;

            var mx3 = mx + kc*sx + ks*sy;
            var my3 = my + ks*sx - kc*sy;

            this.markerTop[i].setAttribute("x1", mx0);
            this.markerTop[i].setAttribute("y1", my0);
            this.markerTop[i].setAttribute("x2", mx1);
            this.markerTop[i].setAttribute("y2", my1);

            this.markerRight[i].setAttribute("x1", mx1);
            this.markerRight[i].setAttribute("y1", my1);
            this.markerRight[i].setAttribute("x2", mx2);
            this.markerRight[i].setAttribute("y2", my2);
            
            this.markerBottom[i].setAttribute("x1", mx2);
            this.markerBottom[i].setAttribute("y1", my2);
            this.markerBottom[i].setAttribute("x2", mx3);
            this.markerBottom[i].setAttribute("y2", my3);
            
            this.markerLeft[i].setAttribute("x1", mx3);
            this.markerLeft[i].setAttribute("y1", my3);
            this.markerLeft[i].setAttribute("x2", mx0);
            this.markerLeft[i].setAttribute("y2", my0);
        }
    }
    else
    {
        for(var i=0; i<this.sizey; i++)
        {
            var mx = (d[i][0]-this.min_x)*this.scale_x * this.width;
            var my = (d[i][1]-this.min_y)*this.scale_y * this.height;

            var kc = 1;
            var ks = 0;
            
            if(this.sizex > 2)
            {
                kc = Math.cos(d[i][2]);
                ks = Math.sin(d[i][2]);
            }

            var sx = this.size;
            var sy = this.size;
            
             if(d[i][4])
            {
                sx = sx * d[i][4];
                sy = sy * d[i][4];
            }
            
           var mx0 = mx + kc*sx - ks*sy;
            var my0 = my + ks*sx + kc*sy;

            var mx1 = mx - kc*sx - ks*sy;
            var my1 = my - ks*sx + kc*sy;

            var mx2 = mx - kc*sx + ks*sy;
            var my2 = my - ks*sx - kc*sy;

            var mx3 = mx + kc*sx + ks*sy;
            var my3 = my + ks*sx - kc*sy;

            this.markerTop[i].setAttribute("x1", mx0);
            this.markerTop[i].setAttribute("y1", my0);
            this.markerTop[i].setAttribute("x2", mx1);
            this.markerTop[i].setAttribute("y2", my1);

            this.markerRight[i].setAttribute("x1", mx1);
            this.markerRight[i].setAttribute("y1", my1);
            this.markerRight[i].setAttribute("x2", mx2);
            this.markerRight[i].setAttribute("y2", my2);
            
            this.markerBottom[i].setAttribute("x1", mx2);
            this.markerBottom[i].setAttribute("y1", my2);
            this.markerBottom[i].setAttribute("x2", mx3);
            this.markerBottom[i].setAttribute("y2", my3);
            
            this.markerLeft[i].setAttribute("x1", mx3);
            this.markerLeft[i].setAttribute("y1", my3);
            this.markerLeft[i].setAttribute("x2", mx0);
            this.markerLeft[i].setAttribute("y2", my0);
        }
    }
}



