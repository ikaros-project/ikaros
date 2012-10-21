function Path(p)
{
    this.obj = 	new WebUICanvas(this, p);
    
    this.length_module = (p.length_module ? p.length_module : this.module);
    this.length_source = p.length_source;
 
    this.arrow = (p.arrow ? p.arrow == "yes" : false);
    this.close = (p.close ? p.close == "yes" : false);
    this.row_order = (p.order ? p.order == "row" : true);
    this.select = (p.select ? p.select: 0);
    this.count = (p.count ? p.count : 0);
    
	usesData(this.module, this.source);
    
    if(this.length_source)
        usesData(this.length_module, this.length_source);
}



Path.prototype.DrawRows = function(d, rows)
{
    this.context.clearRect(0, 0, this.width, this.height);
    this.context.lineWidth = this.stroke_width;
    
    var xx = (this.count ? this.select+2*this.count : d[0].length);
    
    for(var i=0; i<rows; i++)
    {
        this.context.strokeStyle = this.stroke_LUT[i % this.stroke_LUT.length];
        this.context.fillStyle = this.fill_LUT[i % this.fill_LUT.length];
        
        this.context.beginPath();
        
        var lx = 0;
        var ly = 0;
        var x = (d[i][this.select+0]-this.min_x)*this.scale_x * this.width;
        var y = (d[i][this.select+1]-this.min_y)*this.scale_y * this.height;
        this.context.moveTo(x, y);
        
        for(var j=this.select+2; j<xx;)
        {
            lx = x;
            ly = y;
            x = (d[i][j++]-this.min_x)*this.scale_x * this.width;
            y = (d[i][j++]-this.min_y)*this.scale_y * this.height;
            
            this.context.lineTo(x, y);
        }
        
        this.context.fill();
        if(this.close)
            this.context.closePath();
        this.context.stroke();
        
        if(this.arrow)
            this.context.drawArrowHead(lx, ly, x, y);
    }
}



Path.prototype.DrawCols = function(d, rows)
{
    this.context.clearRect(0, 0, this.width, this.height);
    this.context.lineWidth = this.stroke_width;
    
    var xx = (this.count ? this.select+2*this.count : d[0].length);
    
    for(var i=this.select; i<xx; i+=2)
    {
        this.context.strokeStyle = this.stroke_LUT[i/2 % this.stroke_LUT.length];
        this.context.fillStyle = this.fill_LUT[i/2 % this.fill_LUT.length];
        
        this.context.beginPath();
        
        var lx = 0;
        var ly = 0;
        var x = (d[0][i+0]-this.min_x)*this.scale_x * this.width;
        var y = (d[0][i+1]-this.min_y)*this.scale_y * this.height;
        this.context.moveTo(x, y);
        
        for(var j=1; j<rows;j++)
        {
            lx = x;
            ly = y;
            x = (d[j][i+0]-this.min_x)*this.scale_x * this.width;
            y = (d[j][i+1]-this.min_y)*this.scale_y * this.height;
            
            this.context.lineTo(x, y);
        }
        
        this.context.fill();
        if(this.close)
            this.context.closePath();
        this.context.stroke();
        
        if(this.arrow)
            this.context.drawArrowHead(lx, ly, x, y);
    }
}



Path.prototype.Update = function(data)
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

    if(this.row_order)
        this.DrawRows(d, rows);
    else
        this.DrawCols(d, rows);
}

