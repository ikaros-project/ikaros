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
    
    this.angle_module = (p.angle_module ? p.angle_module : this.module);
    this.angle_source = p.angle_source;
    
    this.size_module = (p.size_module ? p.size_module : this.module);
    this.size_source = p.size_source;

    this.select = (p.select ? p.select: 0);
    this.flip_y_axis = p.flip_y_axis;

	usesData(this.module, this.source);
    
    if(this.angle_source)
        usesData(this.length_module, this.angle_source);

    if(this.size_source)
        usesData(this.size_module, this.size_source);
    
    if(this.length_source)
        usesData(this.length_module, this.length_source);
}



Marker.prototype.DrawRows = function(d, s, a, rows)
{
    var date = new Date();
    var t = (date.getTime() % 1000)/1000;
    var t2 = -(date.getTime() % 3000)/3000;
    var t3 = (date.getTime() % 7000)/7000;
    

    this.context.clearRect(0, 0, this.width, this.height);
    
    for(var i=0; i<rows; i++)
    {
        this.context.lineWidth =this.line_width_LUT[i % this.line_width_LUT.length];
        this.context.setLineDash(this.line_dash_LUT[i % this.line_dash_LUT.length]);
        this.context.strokeStyle = this.stroke_LUT[i % this.stroke_LUT.length];
        this.context.fillStyle = this.fill_LUT[i % this.fill_LUT.length];
        
        var x = (d[i][this.select+0]-this.min_x)*this.scale_x * this.width;
        var y = (d[i][this.select+1]-this.min_y)*this.scale_y * this.height;
        var r = 80;
        if(s)
        {
            if(s[i][1])
                r = 0.5*Math.hypot(s[i][0], s[i][1]) * this.width;
            else
                r = 0.7 * s[i][0] * this.width;
        }
        
        // Draw circle
        
        this.context.beginPath();
        this.context.arc(x, y, r, 0, 2*Math.PI);
        if(this.fill_LUT[i % this.fill_LUT.length]!='none')
            this.context.fill();
        this.context.stroke();
        
        // Draw percentage segments
        
        this.context.lineWidth = 10; //this.line_width_LUT[i % this.line_width_LUT.length];
        this.context.setLineDash([]);
        // this.stroke_LUT[i % this.stroke_LUT.length];
        var cc = 0;
        for(var j=0; j<0.5*Math.PI; j+=Math.PI/32)
        {
            this.context.strokeStyle = (cc < 13*t3 ? 'rgba(128,128,128,0.45)' : 'rgba(255,0,0,0.6)');
            this.context.beginPath();
            this.context.arc(x, y, r, j, j+Math.PI/48);
            this.context.stroke();
            cc++;
        }
        
        // Draw trig
        
        // Draw animation
        
        this.context.lineWidth = 3;
        this.context.setLineDash([]);
        this.context.strokeStyle = 'rgba(128,128,128,0.45)';
        this.context.beginPath();
        this.context.arc(x, y, r+10, t*2*Math.PI, t*2*Math.PI+Math.PI);
        this.context.stroke();
        this.context.beginPath();
        this.context.arc(x, y, r+15, t2*2*Math.PI, t2*2*Math.PI+Math.PI/3);
        this.context.stroke();
        
        // Angle
        
        if(a)
        {
            this.context.lineWidth = 10;
            this.context.setLineDash([]);
            this.context.strokeStyle = 'red';
            
            var angle = Math.PI*a[i][0]/180;
        
            this.context.beginPath();
            this.context.arc(x, y, r+20, angle, angle+Math.PI/256);
            this.context.stroke();

            angle += Math.PI;
            this.context.beginPath();
            this.context.arc(x, y, r+20, angle, angle+Math.PI/256);
            this.context.stroke();
        }
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

    var a = data[this.angle_module];
    if(a)
    {
        a = a[this.angle_source];
    }
    
    var s = data[this.size_module];
    if(s)
    {
        s = s[this.size_source];
    }
    
    this.DrawRows(d, s, a, rows);
}

