function Table(p)
{
	this.inited = false;
	this.graph = new Graph(p, p.module+'.'+p.source);
	this.module = p.module;
	this.source = p.source;
	this.width = p.width;
	this.height = p.height;
	this.decimals = (p.decimals == null) ? 2 : p.decimals;
	this.LUTp = makeLUTArray((p.positive_color ? p.positive_color : p.color), ['#BABABA']);
	this.LUTn = makeLUTArray((p.negative_color ? p.negative_color : p.color), ['#BABABA']);
    this.font_size = p.font_size;
    
    usesData(this.module, this.source);
}



Table.prototype.Init = function(data)
{
    var d = data[this.module][this.source];

	this.inited = true;    
    this.sizey = d.length;
	this.sizex = d[0].length;	
	var box_width = (this.width-4)/this.sizex;
	var box_height = (this.height-4-23)/this.sizey;
	this.box = new Array(this.sizey);
    var cc = 0;
	for(var j=0; j<this.sizey; j++)
	{
		this.box[j] = new Array(this.sizex);
		for(var i=0; i<this.sizex; i++)
			this.box[j][i] = this.graph.AddText(box_width*(i+1)-2, box_height*(j+1)-14+23, '0', box_width, box_height, 'end', this.font_size, this.LUTp[cc++ % this.LUTp.length]);
	}
}



Table.prototype.Update = function(data)
{
    var d = data[this.module];
    if(!d) return;
    d = d[this.source]
    if(!d) return;
    
    if(!this.inited) this.Init(data);

    var cc = 0;

    for(var j=0; j<this.sizey; j++)
		for(var i=0; i<this.sizex; i++)
        {
			this.box[j][i].firstChild.nodeValue = d[j][i].toFixed(this.decimals);
            this.box[j][i].setAttribute('fill', (d[j][i] >= 0 ? this.LUTp[cc++ % this.LUTp.length] : this.LUTn[cc++ % this.LUTp.length]));
        }
}


