function CCircle(p)
{
    this.obj = 	new WebUICanvas(this, p);
    
    this.length_module = (p.length_module ? p.length_module : this.module);
    this.length_source = p.length_source;
 
	this.size =	 (p.size ? p.size : 0.1);
	this.scale_size =	 (p.scale_size ? p.scale_size : 1.0);
    this.order = (p.order ? p.order == "row" : true);
    this.select = (p.select ? p.select: 0);
    this.select_size = (p.select_size ? p.select_size: null);
    this.count = (p.count ? p.count : 0);
    this.select = (p.select ? p.select: 0);
    
	usesData(this.module, this.source);
    
    if(this.length_source)
        usesData(this.length_module, this.length_source);
}



CCircle.prototype.DrawRows = function(d, rows)
{
    this.context.clearRect(0, 0, this.width, this.height);
    
    var xx = (this.count ? this.select+2*this.count : d[0].length);
    
    for(var i=0; i<rows; i++)
    {
        this.context.lineWidth = this.line_width_LUT[i % this.line_width_LUT.length];
        this.context.setLineDash(this.line_dash_LUT[i % this.line_dash_LUT.length]);
        this.context.strokeStyle = this.stroke_LUT[i % this.stroke_LUT.length];
        this.context.fillStyle = this.fill_LUT[i % this.fill_LUT.length];
        
        this.context.beginPath();
        
        var lx = 0;
        var ly = 0;
        var x = (d[i][this.select+0]-this.min_x)*this.scale_x * this.width;
        var y = (d[i][this.select+1]-this.min_y)*this.scale_y * this.height;
        var s = this.select_size ? this.width * d[i][this.select_size] * this.scale_size : this.width * this.size;
        
        this.context.arc(x, y, s, 0, 2*Math.PI);
        
        if(this.fill_LUT[i % this.fill_LUT.length]!= 'none')
            this.context.fill();

        this.context.stroke();
    }
}



CCircle.prototype.DrawCols = function(d, rows)
{
    this.context.clearRect(0, 0, this.width, this.height);
    this.context.lineWidth = this.stroke_width;
    
    var xx = (this.count ? this.select+2*this.count : d[0].length);
    
    for(var i=this.select; i<xx; i+=2)
    {
        this.context.lineWidth = this.line_width_LUT[i % this.line_width_LUT.length];
        this.context.setLineDash(this.line_dash_LUT[i % this.line_dash_LUT.length]);
        this.context.strokeStyle = this.stroke_LUT[i % this.stroke_LUT.length];
        this.context.fillStyle = this.fill_LUT[i % this.fill_LUT.length];
        
        this.context.beginPath();
        
        var lx = 0;
        var ly = 0;
        var x = (d[0][i+0]-this.min_x)*this.scale_x * this.width;
        var y = (d[0][i+1]-this.min_y)*this.scale_y * this.height;
        var s = this.select_size ? this.width * d[i][this.select_size] * this.scale_size : this.width * this.size;

        this.context.arc(x, y, s, 0, 2*Math.PI);

        if(this.fill_LUT[i % this.fill_LUT.length]!= 'none')
            this.context.fill();

        this.context.stroke();
    }
}



CCircle.prototype.Update = function(data)
{
    var d = data[this.module];
    if(!d) return;
    d = d[this.source]
    if(!d) return;
    
    var rows = d.length;
    if(this.length_source)
    {
        var r = data[this.length_module];
        if(r)
        {
            r = r[this.length_source];
            if(r)
                rows = r;
        }
    }

    if(this.order)
        this.DrawRows(d, rows);
    else
        this.DrawCols(d, rows);
}

