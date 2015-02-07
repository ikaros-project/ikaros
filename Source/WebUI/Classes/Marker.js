//
// Percentage
// Trig
// Size
//

function Marker(p)
{
    this.obj = 	new WebUICanvas(this, p);
    
    this.length_module = (p.length_module ? p.length_module : this.module);
    this.length_source = p.length_source;
    this.select = (p.select ? p.select: 0);
    
    this.flip_y_axis = p.flip_y_axis;

	usesData(this.module, this.source);
    
    if(this.length_source)
        usesData(this.length_module, this.length_source);
}



Marker.prototype.DrawRows = function(d, rows)
{
    this.context.clearRect(0, 0, this.width, this.height);
    
    for(var i=0; i<rows; i++)
    {
        this.context.lineWidth = this.line_width_LUT[i % this.line_width_LUT.length];
        this.context.setLineDash(this.line_dash_LUT[i % this.line_dash_LUT.length]);
        this.context.strokeStyle = this.stroke_LUT[i % this.stroke_LUT.length];
        this.context.fillStyle = this.fill_LUT[i % this.fill_LUT.length];
        
        var x = (d[i][this.select+0]-this.min_x)*this.scale_x * this.width;
        var y = (d[i][this.select+1]-this.min_y)*this.scale_y * this.height;
        var r = 20;

        // Draw circle
        
        this.context.beginPath();
        this.arc(x, y, r, 0, 2*Math.PI);
        if(this.fill_LUT[i % this.fill_LUT.length]!= 'none')
            this.context.fill();
        if(this.close)
            this.context.closePath();
        this.context.stroke();
        
        // Draw percentage segments
        
        for(var j=0; j<Math.PI; j+=Math.PI/8)
        {
        
        }
        
        // Draw trig
        
    }
}



Marker.prototype.Update = function(data)
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

    this.DrawRows(d, rows);
}

