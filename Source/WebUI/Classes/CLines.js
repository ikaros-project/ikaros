function CLines(p)
{
    this.obj = 	new WebUICanvas(this, p);
 
    this.arrow = p.arrow;
    this.polar = p.polar;
    
	this.min = (p.min ? p.min : 0);
	this.max = (p.max ? p.max : 1);
	this.scale = 1/(this.max == this.min ? 1 : this.max-this.min);
    
	this.min_x = (p.min_x ? p.min_x : this.min);
	this.max_x = (p.max_x ? p.max_x : this.max);
	this.scale_x = 1/(this.max_x == this.min_x ? 1 : this.max_x-this.min_x);
    
	this.min_y = (p.min_y ? p.min_y : this.min);
	this.max_y = (p.max_y ? p.max_y : this.max);
	this.scale_y = 1/(this.max_y == this.min_y ? 1 : this.max_y-this.min_y);
    
    this.length = (p.length ? p.length : 10);
    
    this.flip_y_axis = p.flip_y_axis;
    
    console.log(this.flip_y_axis);

	usesData(this.module, this.source);

    this.context.clearRect(0, 0, this.width, this.height);
    this.context.fillStyle="none";
    this.context.fillRect(0, 0, this.width, this.height);
    this.context.translate(0.5, 0.5);
}



CLines.prototype.Update = function(data)
{
    var d = data[this.module];
    if(!d) return;
    d = d[this.source]
    if(!d) return;
    
    this.sizex = d[0].length;
    this.sizey = d.length;

    this.context.clearRect(0, 0, this.width, this.height);
    
    if(this.flip_y_axis)
    {
        for(var i=0; i<this.sizey; i++)
        {
            var x2, y2;
            if(this.polar)
            {
                var a = d[i][2];
                x2 = d[i][0] + this.length*Math.cos(a);
                y2 = d[i][1] + this.length*Math.sin(a);
            }
            else
            {
                x2 = d[i][2];
                y2 = d[i][3];
            }
            
            if(this.arrow)
            {
                this.context.strokeStyle = this.LUT[i % this.LUT.length];
                this.context.fillStyle = this.LUT[i % this.LUT.length];
                this.context.lineWidth = this.stroke_width;
                this.context.drawLineArrow(
                                           (d[i][0]-this.min_x)*this.scale_x * this.width, this.height-(d[i][1]-this.min_y)*this.scale_y * this.height,
                                           (x2-this.min_x)*this.scale_x * this.width, this.height-(y2-this.min_y)*this.scale_y * this.height);
            }
            else
            {
                this.context.beginPath();
                this.context.strokeStyle = this.LUT[i % this.LUT.length];
                this.context.lineWidth = this.stroke_width;
                this.context.moveTo((d[i][0]-this.min_x)*this.scale_x * this.width, this.height-(d[i][1]-this.min_y)*this.scale_y * this.height);
                this.context.lineTo((x2-this.min_x)*this.scale_x * this.width, this.height-(y2-this.min_y)*this.scale_y * this.height);
                this.context.stroke();
            }
        }
    }
    else
    {
        for(var i=0; i<this.sizey; i++)
        {
            var x2, y2;
            if(this.polar)
            {
                var a = d[i][2];
                x2 = d[i][0] + this.length*Math.cos(a);
                y2 = d[i][1] + this.length*Math.sin(a);
            }
            else
            {
                x2 = d[i][2];
                y2 = d[i][3];
            }
            
            if(this.arrow)
            {
                this.context.strokeStyle = this.LUT[i % this.LUT.length];
                this.context.fillStyle = this.LUT[i % this.LUT.length];
                this.context.lineWidth = this.stroke_width;
                this.context.drawLineArrow(
                              (d[i][0]-this.min_x)*this.scale_x * this.width, (d[i][1]-this.min_y)*this.scale_y * this.height,
                              (x2-this.min_x)*this.scale_x * this.width, (y2-this.min_y)*this.scale_y * this.height);
            }
            else
            {
                this.context.beginPath();
                this.context.moveTo((d[i][0]-this.min_x)*this.scale_x * this.width, (d[i][1]-this.min_y)*this.scale_y * this.height);
                this.context.lineTo((x2-this.min_x)*this.scale_x * this.width, (y2-this.min_y)*this.scale_y * this.height);
                this.context.lineWidth = this.stroke_width;
                this.context.strokeStyle = this.LUT[i % this.LUT.length];
                this.context.stroke();
            }
        }
    }
}
