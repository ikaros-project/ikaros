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
        var h = that.height-40;
        var val = (event.clientY-40-that.y)/h;
        if(val < 0)
            val = 0;
        if(val > 1)
            val = 1;
        if(that.steps > 0)
            val = lookupStepValue(val, that.steptable)
        that.button.setAttribute("y", 20+val*h+4);
        var value = that.min+(that.max-that.min)*(that.invert ? val : 1-val);
        get("/control/"+that.module+"/"+that.parameter+"/"+that.xindex+"/"+that.yindex+"/"+value, none);
    }

    function makeStepTable(numsteps)
    {
        stepsize = 1.0/numsteps;
        radix = stepsize/2.0;
        retval = [];
        retval.push([0, radix, 0]);
        min = radix;
        while(min < 1.0-radix)
        {
            retval.push([min, min+stepsize, min+radix]);
            min += stepsize;
        }
        retval.push([1.0-radix, 1.0, 1.0]);
        return retval;
    }

    function lookupStepValue(val, table)
    {
        for(var i=0; i<table.length; i++)
            if(val >= table[i][0] && val<table[i][1])
                return table[i][2];
        return 1.0;
    }

    
    var cx = p.width/2;
    
    this.module = p.module;
    this.parameter = p.parameter;
    
    p.opaque = true;
    this.graph = new Graph(p, 'Slider');
    this.x = p.x;
    this.y = p.y;	
    this.width = p.width;
    this.height = p.height;
    this.knobradius = 8;
    this.min = (p.min ? p.min : 0);
    this.max = (p.max ? p.max : 1);
    this.select = makeSelectionArray(p.select);
    this.invert = (p.invert ? p.invert : false);
    this.xindex = (p.xindex ? p.xindex : this.select[0][0]);
    this.yindex = (p.yindex ? p.yindex : this.select[0][1]);
    this.slide = this.graph.AddRect(cx-2.5, 30, 5, this.height-40, 'black', '#444444', 2); // local coordinates
    this.steps = (p.steps ? (p.steps>2 ? p.steps-1 : 2) : 0);
    this.steptable = makeStepTable(this.steps);
    
    if(this.steps > 0)
    {
        var s = (this.height-45)/this.steps;
        for(var i=0; i<=this.steps; i++)
            this.graph.AddLine(cx+4, 32.5+i*s, cx+10, 32.5+i*s, '#111', 1);
    }

    this.button = document.createElementNS(svgns,"image");
    this.button.setAttribute('x', cx-this.knobradius-0.5);
    this.button.setAttribute('y', +this.knobradius+11);   // global coordinates
    this.button.setAttribute('width', 2*this.knobradius+1);
    this.button.setAttribute('height', 2*this.knobradius+1);
    this.button.setAttribute('preserveAspectRatio', 'none');
    this.button.setAttributeNS(xlinkns, 'href', "/Classes/knob.png");
    this.button.setAttribute('style', 'cursor: pointer');
    this.graph.group.appendChild(this.button);

    this.title = document.createElementNS(svgns,"text");	
    this.title.setAttribute('x', this.width/2);
    this.title.setAttribute('y', 14);
    this.title.setAttribute('class', 'object_title');        
    this.title.setAttribute('text-rendering','optimizeLegibility');
    this.title.setAttribute('text-anchor', 'middle');
    var tn = document.createTextNode(p.title);	
    this.title.appendChild(tn);
    this.graph.group.appendChild(this.title);

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
    d = d[this.parameter]
    if(!d) return;
    
    this.value = d[this.yindex][this.xindex];
    var h = this.height-40;

    var val = (this.value-this.min)/(this.max-this.min)

    var pos = 4+20+h*(1-val);

    this.button.setAttribute("y", pos);

    if(this.invert)
    {
        var pos = 4+20+h*((this.value-this.min)/(this.max-this.min));
        this.button.setAttribute("y", pos);
    }
    else
    {
        var pos = 4+20+h*(1-val);
        this.button.setAttribute("y", pos);
    }
}
