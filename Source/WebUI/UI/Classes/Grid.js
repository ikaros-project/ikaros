function Grid(p)
{
    this.exit = false;
	this.inited = false;
	this.graph = new Graph(p, p.module+'.'+p.source);
	this.graph.group.setAttribute("shape-rendering", "crispEdges");
	this.module = p.module;
	this.source = p.source;
	this.width = p.width+4;
	this.height = p.height+4;
	this.max = (p.max ? p.max : 1);
	this.LUT = makeLUTArray(p.color, LUT_fire);
	this.scale = (this.LUT.length-1)/this.max;
    this.drawonce = (p.drawonce ? p.drawonce=='yes' : false);
	
	usesData(this.module, this.source);
}



Grid.prototype.Init = function(data)
{
    var d = data[this.module][this.source];

    this.inited = true;
    this.sizey = d.length;
    this.sizex = d[0].length;
    
    var box_width = (this.width-4)/this.sizex;
    var box_height = (this.height-4)/this.sizey;

    this.box = new Array(this.sizey);

    for(var j=0; j<this.sizey; j++)
    {
        this.box[j] = new Array(this.sizex);
        for(var i=0; i<this.sizex; i++)
            this.box[j][i] = this.graph.AddRect(box_width*i, box_height*j, box_width, box_height, 'gray', '#1d1d1d');
    }
}



Grid.prototype.Update = function(data)
{
    if(this.exit)
        return;
    
    var d = data[this.module];
    if(!d) return;
    d = d[this.source]
    if(!d) return;
    
    if(!this.inited) this.Init(data);
    

    for(var j=this.sizey; j--;)
    {
        var bj = this.box[j];
        var dj = data[this.module][this.source][j];
        for(var i=this.sizex; i--;)
            bj[i].setAttribute("fill", this.LUT[Math.round(this.scale*dj[i])]);
    }
    
    if(this.drawonce)
        this.exit = true;
}

