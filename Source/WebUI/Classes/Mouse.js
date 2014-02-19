function Mouse(p)
{
	var that = this;
    
    function none()
    {
    }
    
    function mouse_down(event)
    {
        event.preventDefault();
        
        var mx = event.clientX-= 8.5;
        var my = event.clientY-= 8.5;
        
        if(event.shiftKey)
        {
            that.value_x2 = that.min_x+that.range_x*((mx-that.x)/that.width);
            
            if(that.flip_y_axis)
                that.value_y2 = that.min_y+that.range_y*(1-(my-that.y)/that.height);
            else
                that.value_y2 = that.min_y+that.range_y*((my-that.y)/that.height);
            
            get("/control/"+that.module2+"/"+that.parameter2+"/0/0/"+that.value_x2, none);
            get("/control/"+that.module2+"/"+that.parameter2+"/1/0/"+that.value_y2, none);

            if(that.flip_y_axis)
            {
                that.circle2.setAttribute("cx", that.width*that.scale_x*(that.value_x2-that.min_x));
                that.circle2.setAttribute("cy", that.height-that.height*that.scale_y*(that.value_y2-that.min_y));
            }
            else
            {
                that.circle2.setAttribute("cx", that.width*that.scale_x*(that.value_x2-that.min_x));
                that.circle2.setAttribute("cy", that.height*that.scale_y*(that.value_y2-that.min_y));
            }
        }
        
        else if(event.altKey)
        {
            that.value_x3 = that.min_x+that.range_x*((mx-that.x)/that.width);
            
            if(that.flip_y_axis)
                that.value_y3 = that.min_y+that.range_y*(1-(my-that.y)/that.height);
            else
                that.value_y3 = that.min_y+that.range_y*((my-that.y)/that.height);
            
            get("/control/"+that.module3+"/"+that.parameter3+"/0/0/"+that.value_x3, none);
            get("/control/"+that.module3+"/"+that.parameter3+"/1/0/"+that.value_y3, none);

            if(that.flip_y_axis)
            {
                that.circle3.setAttribute("cx", that.width*that.scale_x*(that.value_x3-that.min_x));
                that.circle3.setAttribute("cy", that.height-that.height*that.scale_y*(that.value_y3-that.min_y));
            }
            else
            {
                that.circle3.setAttribute("cx", that.width*that.scale_x*(that.value_x3-that.min_x));
                that.circle3.setAttribute("cy", that.height*that.scale_y*(that.value_y2-that.min_y));
            }
        }
        
        else
        {
            that.value_x = that.min_x+that.range_x*((mx-that.x)/that.width);
            
            if(that.flip_y_axis)
                that.value_y = that.min_y+that.range_y*(1-(my-that.y)/that.height);
            else
                that.value_y = that.min_y+that.range_y*((my-that.y)/that.height);
            
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
        }
        
        document.onmouseup = mouse_up;
    }

    function mouse_up(event)
    {
        document.onmouseup = null;
    }

	this.inited = false;
	this.module = p.module;
	this.module2 = p.module2;
	this.module3 = p.module3;
	this.parameter = p.parameter;
    this.parameter2 = p.parameter2;
    this.parameter3 = p.parameter3;
    this.x = p.x;
    this.y = p.y;	
	this.width = p.width;
	this.height = p.height;
	this.LUT = makeLUTArray(p.color, ['yellow','red','green']);
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
    
    this.value_x2 = 0;
    this.value_y2 = 0;

    this.value_x3 = 0;
    this.value_y3 = 0;

    this.circle = this.graph.AddCircle(this.value_x, this.value_y , this.size*this.width, 'none', this.LUT[0], this.stroke_width);
    
    if(this.parameter2)
        this.circle2 = this.graph.AddCircle(this.value_x2, this.value_y2 , this.size*this.width, 'none', this.LUT[1 % this.LUT.length], this.stroke_width);
    
    if(this.parameter3)
        this.circle3 = this.graph.AddCircle(this.value_x3, this.value_y3, this.size*this.width, 'none', this.LUT[2 % this.LUT.length], this.stroke_width);
    
    this.graph.obj.bg.onmousedown = mouse_down;
        
    usesData(this.module, this.parameter);
    if(this.module2)
        usesData(this.module2, this.parameter2);
    if(this.module3)
        usesData(this.module3, this.parameter3);
}



Mouse.prototype.Update = function(data)
{
    var d = data[this.module];
    if(!d) return;
    d = d[this.parameter]
    if(!d) return;

    var d2 = d[this.module2];
    if(d2) d2 = d[this.parameter2];
    
    var d3 = d[this.module3];
    if(d3) d3 = d[this.parameter3];

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
    
    if(d2)
    {
        this.value_x = d2[0][0];
        this.value_y = d2[0][1];
    
        if(this.flip_y_axis)
        {
            this.circle2.setAttribute("cx", this.width*this.scale_x*(this.value_x2-this.min_x));
            this.circle2.setAttribute("cy", this.height-this.height*this.scale_y*(this.value_y2-this.min_y));
        }
        else
        {
            this.circle2.setAttribute("cx", this.width*this.scale_x*(this.value_x2-this.min_x));
            this.circle2.setAttribute("cy", this.height*this.scale_y*(this.value_y2-this.min_y));
        }
    }

    if(d3)
    {
        this.value_x = d3[0][0];
        this.value_y = d3[0][1];
    
        if(this.flip_y_axis)
        {
            this.circle3.setAttribute("cx", this.width*this.scale_x*(this.value_x3-this.min_x));
            this.circle3.setAttribute("cy", this.height-this.height*this.scale_y*(this.value_y3-this.min_y));
        }
        else
        {
            this.circle3.setAttribute("cx", this.width*this.scale_x*(this.value_x3-this.min_x));
            this.circle3.setAttribute("cy", this.height*this.scale_y*(this.value_y3-this.min_y));
        }
    }
}
