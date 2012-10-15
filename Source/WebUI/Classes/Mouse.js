function Mouse(p)
{
	var that = this;
    
    function none()
    {
    }
    
    function mouse_down(event)
    {
        event.preventDefault();
        
        that.value_x = that.min_x+that.range_x*((event.clientX-that.x)/that.width);
        
        if(that.flip_y_axis)
            that.value_y = that.min_y+that.range_y*(1-(event.clientY-that.y)/that.height);
        else
            that.value_y = that.min_y+that.range_y*((event.clientY-that.y)/that.height);
        
        get("/control/"+that.module+"/"+that.parameter+"/0/0/"+that.value_x, none);
        get("/control/"+that.module+"/"+that.parameter+"/1/0/"+that.value_y, none);

        if(that.flip_y_axis)
        {
            that.circle.setAttribute("cx", that.width*that.scale_x*(that.value_x-that.min_x));
            that.circle.setAttribute("cy", that.height-that.height*that.scale_y*(that.value_y-that.min_y));
        }
        else
        {
            that.circle.setAttribute("cx", that.width*that.scale_x*(that.value_x-that.min_x));
            that.circle.setAttribute("cy", that.height*that.scale_y*(that.value_y-that.min_y));
        }
        document.onmouseup = mouse_up;
    }

    function mouse_up(event)
    {
        document.onmouseup = null;
    }

	this.inited = false;
	this.module = p.module;
	this.parameter = p.parameter;
    this.x = p.x;
    this.y = p.y;	
	this.width = p.width;
	this.height = p.height;
	this.LUT = makeLUTArray(p.color, ['yellow']);
	this.stroke_width = (p.stroke_width ? p.stroke_width : 3);
	this.size =	 (p.size ? p.size : 0.005);
	this.graph = new Graph(p, p.module+'.'+p.parameter);

	this.min = (p.min ? p.min : 0);
	this.max = (p.max ? p.max : 1);
	this.scale = 1/(this.max == this.min ? 1 : this.max-this.min);
    
	this.min_x = (p.min_x ? p.min_x : this.min);
	this.max_x = (p.max_x ? p.max_x : this.max);
	this.range_x = (this.max_x == this.min_x ? 1 : this.max_x-this.min_x);
	this.scale_x = 1/this.range_x;
    
	this.min_y = (p.min_y ? p.min_y : this.min);
	this.max_y = (p.max_y ? p.max_y : this.max);
	this.range_y = (this.max_x == this.min_x ? 1 : this.max_x-this.min_x);
	this.scale_y = 1/this.range_y;
    
    this.flip_y_axis = p.flip_y_axis;
	
    this.value_x = 0;
    this.value_y = 0;
    
    this.circle = this.graph.AddCircle(this.value_x, this.value_y , this.size*this.width, 'none', this.LUT[0], this.stroke_width);
    this.graph.background.onmousedown = mouse_down;
        
    usesData(this.module, this.parameter);
}



Mouse.prototype.Update = function(data)
{
    var d = data[this.module];
    if(!d) return;
    d = d[this.parameter]
    if(!d) return;

    this.value_x = d[0][0];
    this.value_y = d[0][1];
    
    if(this.flip_y_axis)
    {
        this.circle.setAttribute("cx", this.width*this.scale_x*(this.value_x-this.min_x));
        this.circle.setAttribute("cy", this.height-this.height*this.scale_y*(this.value_y-this.min_y));
    }
    else
    {
        this.circle.setAttribute("cx", this.width*this.scale_x*(this.value_x-this.min_x));
        this.circle.setAttribute("cy", this.height*this.scale_y*(this.value_y-this.min_y));
    }
}


