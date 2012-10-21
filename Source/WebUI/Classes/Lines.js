function Lines(p)
{
    this.obj = 	new WebUICanvas(this, p);
 
    this.arrow = p.arrow;
    this.polar = p.polar;
    this.length = (p.length ? p.length : 10);
    
	usesData(this.module, this.source);
}



Lines.prototype.Update = function(data)
{
    var d = data[this.module];
    if(!d) return;
    d = d[this.source]
    if(!d) return;
    
    this.sizex = d[0].length;
    this.sizey = d.length;

    this.context.clearRect(0, 0, this.width, this.height);
    
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
