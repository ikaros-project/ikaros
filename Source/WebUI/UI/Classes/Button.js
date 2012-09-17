function Button(p)
{
    var that = this;
    
    function none()
    {
    }
    
	function down(e)
	{
        e.preventDefault();
        that.clicked = true;
        that.button.setAttribute("opacity", 0);
        that.label.setAttribute('y', that.y+that.height/2+6);
        if(that.module && that.parameter)
            get("/control/"+that.module+"/"+that.parameter+"/"+that.xindex+"/"+that.yindex+"/"+that.max, none);
        document.onmouseup = up;
	}
    
	function up()
	{
        that.button.setAttribute("opacity", 1);
        that.label.setAttribute('y', that.y+that.height/2+4);
        document.onmouseup = null;
        if(that.module && that.parameter)
            get("/control/"+that.module+"/"+that.parameter+"/"+that.xindex+"/"+that.yindex+"/"+that.min, none);
        that.clicked = false;
	}
    
    this.title = (p.title ? p.title : p.module+'.'+p.parameter);
    this.module = p.module;
    this.parameter = p.parameter;
    this.x = p.x;
    this.y = p.y;	
    this.width = p.width;
    this.height = p.height;
    this.min = (p.min ? p.min : 0);
    this.max = (p.max ? p.max : 1);
    this.select = makeSelectionArray(p.select);
    this.xindex = (p.xindex ? p.xindex : this.select[0][0]);
    this.yindex = (p.yindex ? p.yindex : this.select[0][1]);

    this.button_pressed = document.createElementNS(svgns,"image");	
    this.button_pressed.setAttribute('x', this.x);
    this.button_pressed.setAttribute('y', this.y);
    this.button_pressed.setAttribute('width', this.width);
    this.button_pressed.setAttribute('height', this.height);
    this.button_pressed.setAttribute('preserveAspectRatio', 'none');
    this.button_pressed.setAttributeNS(xlinkns, 'href', "/Classes/button_pressed.svg");
    this.button_pressed.setAttribute('style', 'cursor: pointer');
    document.documentElement.appendChild(this.button_pressed);

    this.button = document.createElementNS(svgns,"image");	
    this.button.setAttribute('x', this.x);
    this.button.setAttribute('y', this.y);
    this.button.setAttribute('width', this.width);
    this.button.setAttribute('height', this.height);
    this.button.setAttribute('preserveAspectRatio', 'none');
    this.button.setAttributeNS(xlinkns, 'href', "/Classes/button.svg");
    this.button.setAttribute('style', 'cursor: pointer');
    document.documentElement.appendChild(this.button);

    if(this.title)
    {
        this.label = document.createElementNS(svgns,"text");	
        this.label.setAttribute('x', this.x+this.width/2);
        this.label.setAttribute('y', this.y+this.height/2+4);
        this.label.setAttribute('style', 'font-size:12px;font-family:Tahoma,sans-serif;fill:#CCCCCC;-webkit-user-select: none');
        this.label.setAttribute('text-anchor', 'middle');
        this.label.setAttribute('text-rendering','optimizeLegibility');
        this.label.setAttribute('pointer-events','none');
        var tn = document.createTextNode(this.title);	
        this.label.appendChild(tn);
        document.documentElement.appendChild(this.label);
    }
    
    this.button.onmousedown = down;
    this.button.onmouseup = up;
    this.value = 0;
    this.clicked = false;
}



Button.prototype.Update = function(data)
{
}


