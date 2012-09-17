function Switch(p)
{
    var that = this;
    
    function none()
    {
    }
    
	function down()
	{
        document.onmouseup = up;
	}
    
	function up()
	{
        document.onmouseup = null;
        that.value = (that.value > 0.5*(that.max+that.min) ? that.min : that.max);
        
        if(that.value > 0.5*(that.max+that.min))
        {
            that.button_on.setAttribute('opacity', 1);
            that.button_off.setAttribute('opacity', 0);
        }
        else    
         {
            that.button_on.setAttribute('opacity', 0);
            that.button_off.setAttribute('opacity', 1);
        }
            
        get("/control/"+that.module+"/"+that.parameter+"/"+that.xindex+"/"+that.yindex+"/"+that.value, none);
        that.clicked = false;
	}
    
    cx = p.width/2;
    cy = p.height/2;
    
    this.module = p.module;
    this.parameter = p.parameter;
    p.opaque = true;
    this.graph = new Graph(p, 'Switch');
    this.x = p.x;
    this.y = p.y;	
    this.width = p.width;
    this.height = p.height;	
    this.min = (p.min ? p.min : 0);
    this.max = (p.max ? p.max : 1);
    this.value = this.min;
    this.select = makeSelectionArray(p.select);
    this.xindex = (p.xindex ? p.xindex : this.select[0][0]);
    this.yindex = (p.yindex ? p.yindex : this.select[0][1]);

    this.clicked = false;
    
    this.button_on = document.createElementNS(svgns,"image");	
    this.button_on.setAttribute('x', this.width/2-35);
    this.button_on.setAttribute('y', 30.5);
    this.button_on.setAttribute('width', 70);
    this.button_on.setAttribute('height', 18);
    this.button_on.setAttribute('preserveAspectRatio', 'none');
    this.button_on.setAttributeNS(xlinkns, 'href', "/Classes/switch_on.png");
    this.button_on.setAttribute('style', 'cursor: pointer');
    that.button_on.setAttribute('opacity', 0);
    this.graph.group.appendChild(this.button_on);

    this.button_off = document.createElementNS(svgns,"image");	
    this.button_off.setAttribute('x', this.width/2-35);
    this.button_off.setAttribute('y', 30.5);
    this.button_off.setAttribute('width', 70);
    this.button_off.setAttribute('height', 18);
    this.button_off.setAttribute('preserveAspectRatio', 'none');
    this.button_off.setAttributeNS(xlinkns, 'href', "/Classes/switch_off.png");
    this.button_off.setAttribute('style', 'cursor: pointer');
    that.button_off.setAttribute('opacity', 1);
    this.graph.group.appendChild(this.button_off);

    this.title = document.createElementNS(svgns,"text");	
    this.title.setAttribute('x', this.x+this.width/2);
    this.title.setAttribute('y', this.y+14);
    this.title.setAttribute('class', 'object_title');        
    this.title.setAttribute('text-rendering','optimizeLegibility');
    this.title.setAttribute('text-anchor', 'middle');

    var tn = document.createTextNode(p.title);	
    this.title.appendChild(tn);
    document.documentElement.appendChild(this.title);

    this.button_on.onmousedown = down;
    this.button_off.onmousedown = down;

    if(this.parameter)
        usesData(this.module, this.parameter);
}



Switch.prototype.Update = function(data)
{
    if(!this.parameter)
        return;

    if(this.clicked) return;
    
    var d = data[this.module];
    if(!d) return;
    d = d[this.source]
    if(!d) return;
    
    this.value = d[this.yindex][this.xindex];

    if(this.value > 0.5*(this.max+this.min))
    {
        this.button_on.setAttribute('opacity', 1);
        this.button_off.setAttribute('opacity', 0);
    }
    else    
     {
        this.button_on.setAttribute('opacity', 0);
        this.button_off.setAttribute('opacity', 1);
    }
}

