function Slider(p)
{
	var that = this;
    
    function none()
    {
    }
    
    function drag_start(event)
    {
        event.preventDefault();
        that.button.setAttribute("fill", "#666");
        that.dragged = true;
        document.onmouseup = drag_end;
        document.onmousemove = drag;
    }
    
    function drag_end(event)
    {
        that.button.setAttribute("fill", "gray");
        that.dragged = false;
        document.onmouseup = null;
        document.onmousemove = null;    
    }
    
    function drag(event)
    {
        event.preventDefault();
        var pos = event.clientY-that.y-that.knobradius-1;
        if(pos < 32.5-that.knobradius)
            pos = 32.5-that.knobradius;
        if(pos > that.height-12.5-that.knobradius)
            pos = that.height-12.5-that.knobradius;
        that.button.setAttribute("y", that.y+pos);
        var value = that.min+(that.max-that.min)*(1-(pos-32.5+that.knobradius)/(that.height-45));
        get("/control/"+that.module+"/"+that.parameter+"/"+that.xindex+"/"+that.yindex+"/"+value, none);
    }
    
    cx = p.width/2;
    
    this.module = p.module;
    this.parameter = p.parameter;
    
    p.opaque = true;
    this.graph = new Graph(p, 'Slider');
    this.y = p.y;
    this.x = p.x;
    this.y = p.y;	
    this.width = p.width;
    this.height = p.height;
    this.knobradius = 8;
    this.min = (p.min ? p.min : 0);
    this.max = (p.max ? p.max : 1);
    this.select = makeSelectionArray(p.select);
    this.xindex = (p.xindex ? p.xindex : this.select[0][0]);
    this.yindex = (p.yindex ? p.yindex : this.select[0][1]);
    this.slide = this.graph.AddRect(cx-2.5, 30, 5, this.height-40, 'black', '#444444', 2); // local coordinates
    
    this.button = document.createElementNS(svgns,"image");	
    this.button.setAttribute('x', this.x+cx-this.knobradius-0.5);
    this.button.setAttribute('y', this.y+this.knobradius+11);   // global coordinates
    this.button.setAttribute('width', 2*this.knobradius+1);
    this.button.setAttribute('height', 2*this.knobradius+1);
    this.button.setAttribute('preserveAspectRatio', 'none');
    this.button.setAttributeNS(xlinkns, 'href', "/Classes/knob.png");
    this.button.setAttribute('style', 'cursor: pointer');
    document.documentElement.appendChild(this.button);

    this.title = document.createElementNS(svgns,"text");	
    this.title.setAttribute('x', this.x+this.width/2);
    this.title.setAttribute('y', this.y+14);
    this.title.setAttribute('class', 'object_title');        
    this.title.setAttribute('text-rendering','optimizeLegibility');
    this.title.setAttribute('text-anchor', 'middle');
    var tn = document.createTextNode(p.title);	
    this.title.appendChild(tn);
    document.documentElement.appendChild(this.title);


    this.button.onmousedown = drag_start;
    this.button.owner = this;
    this.value = 0;
    this.dragged = false;
    
    usesData(this.module, this.parameter);
}



Slider.prototype.Update = function(data)
{
    if(this.dragged)
        return;

    var d = data[this.module];
    if(!d) return;
    d = d[this.source]
    if(!d) return;
    
    var b = this.button;
    this.value = d[this.yindex][this.xindex];
    var pos = this.y-this.knobradius+32.5+(this.height-45)*(1-((this.value-this.min)/(this.max-this.min)));
    this.button.setAttribute("y", pos);
}


