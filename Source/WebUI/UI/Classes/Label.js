function Label(p)
{
	this.inited = false;
	this.module = p.module;
	this.source = p.source;
	this.width = p.width;
	this.height = p.height;
	this.LUT = makeLUTArray(p.color, ['yellow']);
	this.size =	 (p.size ? p.size : 12);
	this.offset_x =	 (p.offset_x ? p.offset_x*this.width : 0);
	this.offset_y =	 (p.offset_y ? p.offset_y*this.height : 0);
    
    this.type = (p.type ? p.type : 'labels' );
    this.select = (p.select ? p.select : 0);
    this.prefix = (p.prefix ? p.prefix : '' );
    this.postfix = (p.postfix ? p.postfix : '' );
    this.decimals = (p.decimals != undefined ? p.decimals : 2);
    
    if(this.type == 'labels')
        this.text = (p.text ? p.text.split(",") : [""] );

    this.text_anchor = (p.text_anchor ? p.text_anchor : 'middle');
	this.graph = new Graph(p, p.module+'.'+p.source);

	this.min = (p.min ? p.min : 0);
	this.max = (p.max ? p.max : 1);
	this.scale = 1/(this.max == this.min ? 1 : this.max-this.min);
    
	this.min_x = (p.min_x ? p.min_x : this.min);
	this.max_x = (p.max_x ? p.max_x : this.max);
	this.scale_x = 1/(this.max_x == this.min_x ? 1 : this.max_x-this.min_x);
    
	this.min_y = (p.min_y ? p.min_y : this.min);
	this.max_y = (p.max_y ? p.max_y : this.max);
	this.scale_y = 1/(this.max_y == this.min_y ? 1 : this.max_y-this.min_y);
    
    this.flip_y_axis = p.flip_y_axis;
	
	if(!this.module)
	{
		var x = p.width/2;
		var y = p.height/2;
		this.label = this.graph.AddText(x+this.offset_x, y+this.offset_y, this.prefix+this.text[0]+this.postfix, this.width, this.height, this.text_anchor, this.size, this.LUT[0]);
	}
	else
		usesData(this.module, this.source);
}



Label.prototype.Init = function(data)
{
    var d = data[this.module][this.source];

    this.inited = true;

    this.label = new Array(0);
    this.sizex = d[0].length;
    this.sizey = d.length;

    if(this.type == 'alphabetical')
    {
        this.text = [];
        for(i=0; i<this.sizey; i++)
            this.text.push(String.fromCharCode(65+i));
    }
    
    else if(this.type == 'numbered')
    {
        this.text = [];
        for(i=0; i<this.sizey; i++)
            this.text.push(i);
    }
    
    else if(this.type == 'x_value')
    {
        this.text = [];
        for(i=0; i<this.sizey; i++)
            this.text.push(d[i][0].toFixed(this.decimals));
    }
    
    else if(this.type == 'y_value')
    {
        this.text = [];
        for(i=0; i<this.sizey; i++)
            this.text.push(d[i][1].toFixed(this.decimals));
    }
    
    else if(this.type == 'xy_value')
    {
        this.text = [];
        for(i=0; i<this.sizey; i++)
            this.text.push(d[i][0].toFixed(this.decimals)+", "+d[i][1].toFixed(this.decimals));
    }
    
    else if(this.type == 'z_value')
    {
        this.text = [];
        for(i=0; i<this.sizey; i++)
            this.text.push(d[i][2].toFixed(this.decimals));
    }
    
    else if(this.type == 'value')
    {
        this.text = [];
        for(i=0; i<this.sizey; i++)
            this.text.push(d[i][this.select].toFixed(this.decimals));
    }
    
    if(this.flip_y_axis)
    {
        for(var i=0; i<this.sizey; i++)
            this.label.push(this.graph.AddText((d[i][0]-this.min_x)*this.scale_x * this.width+this.offset_x, this.height-(d[i][1]-this.min_y)*this.scale_y * this.height+this.offset_y, this.prefix+this.text[i % this.text.length]+this.postfix, this.width, this.height, this.text_anchor, this.size, this.LUT[i % this.LUT.length]));
    }
    else
    {
        for(var i=0; i<this.sizey; i++)
            this.label.push(this.graph.AddText((d[i][0]-this.min_x)*this.scale_x * this.width+this.offset_x, (d[i][1]-this.min_y)*this.scale_y * this.height+this.offset_y, this.prefix+this.text[i % this.text.length]+this.postfix, this.width, this.height, this.text_anchor, this.size,  this.LUT[i % this.LUT.length]));
    }
}



Label.prototype.Update = function(data)
{
    var d = data[this.module];
    if(!d) return;
    d = d[this.source]
    if(!d) return;
    
    if(!this.inited) this.Init(data);

    var change = false;

    if(this.type == 'x_value')
    {
        this.text = [];
        for(i=0; i<this.sizey; i++)
            this.text.push(d[i][0].toFixed(this.decimals));
        change = true;
    }
    
    else if(this.type == 'y_value')
    {
        this.text = [];
        for(i=0; i<this.sizey; i++)
            this.text.push(d[i][1].toFixed(this.decimals));
        change = true;
    }
    
    else if(this.type == 'xy_value')
    {
        this.text = [];
        for(i=0; i<this.sizey; i++)
            this.text.push(d[i][0].toFixed(this.decimals)+", "+d[i][1].toFixed(this.decimals));
        change = true;
    }

    else if(this.type == 'z_value')
    {
        this.text = [];
        for(i=0; i<this.sizey; i++)
            this.text.push(d[i][2].toFixed(this.decimals));
        change = true;
    }
    
    else if(this.type == 'value')
    {
        this.text = [];
        for(i=0; i<this.sizey; i++)
            this.text.push(d[i][this.select].toFixed(this.decimals));
        change = true;
    }
    
    if(change)
    {
        for(var i in this.text)
            this.label[i].textContent = this.prefix+this.text[i]+this.postfix;
    }

    for(var i=0; i<this.sizey; i++)
    {
        if(this.flip_y_axis)
        {
            for(var i=0; i<this.sizey; i++)
            {
                this.label[i].setAttribute("x", (d[i][0]-this.min_x)*this.scale_x * this.width+this.offset_x);
                this.label[i].setAttribute("y", this.height-(d[i][1]-this.min_y)*this.scale_y * this.height+this.offset_y);
            }
        }
        else
        {
            for(var i=0; i<this.sizey; i++)
            {
                this.label[i].setAttribute("x", (d[i][0]-this.min_x)*this.scale_x * this.width+this.offset_x);
                this.label[i].setAttribute("y", (d[i][1]-this.min_y)*this.scale_y * this.height+this.offset_y);
            }
        }
    }
}


