function HUDDirection(p)
{
    this.obj = 	new WebUICanvas(this, p);
    this.scale = 1.0;
    
 	usesData(this.module, this.source);
}



HUDDirection.prototype.Update = function(data)
{
    var d = data[this.module];
    if(!d) return;
    d = d[this.source]
    if(!d) return;

    this.context.clearRect(0, 0, this.width, this.height);
    this.context.lineWidth = 2;
    this.context.strokeStyle = '#00ff00'; //this.stroke_LUT[i % this.stroke_LUT.length];

    this.context.font = '8pt Arial';
    this.context.textAlign = 'center';
    this.context.fillStyle = '#00ff00';
    
    var heading = 180*(d[0][3]/Math.PI);
    var first = 10*Math.round((heading - 100)/10);
    var last = 10*Math.round((heading + 100)/10);
    var center = this.width/2;

    this.context.beginPath();
    
    for(var i=first; i<=last; i+= 5)
    {
        this.context.moveTo(center-heading+i, this.height-30);
        this.context.lineTo(center-heading+i, this.height-25+(i%30==0?5:0)+(i%90==0?5:0));
        
        if(i%90==0)
        {
            this.context.fillText(i, center-heading+i, this.height-5);
        }
    }

    this.context.stroke();

    this.context.beginPath();
    this.context.lineWidth = 1;
    this.context.moveTo(center, this.height-15);
    this.context.lineTo(center, this.height-37);
    this.context.rect(center-20, this.height-52, 40, 15);
    this.context.stroke();

    this.context.font = '10pt Arial';
    this.context.fillText(Math.round(heading)+"°", center, this.height-40);

    this.context.textAlign = 'right';
    this.context.fillText(Math.round(d[0][0]), center-120, this.height-22);
    this.context.fillText(Math.round(d[0][1]), center+150, this.height-22);
    
}

