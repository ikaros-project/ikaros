function BarGraph(p)
{
	this.inited = false;
	this.graph = new Graph(p, p.module+'.'+p.source);
	this.graph.group.setAttribute("shape-rendering", "crispEdges");
	this.module = p.module;
	this.source = p.source;
	this.width = p.width;
	this.height = p.height-6;
    this.autoscale = (p.min || p.max ? false : true);
	this.min = (p.min ? p.min : 0);
	this.max = (p.max ? p.max : 1);
	this.scale = (this.height-6-26)/(this.max-this.min);
	this.zero = this.scale*(0-this.min);
	this.bottom = this.height-6;
	
	this.LUTp = makeLUTArray((p.positive_color ? p.positive_color : p.color), ['yellow']);
	this.LUTn = makeLUTArray((p.negative_color ? p.negative_color : p.color), ['yellow']);
	
	this.LUTpf = makeLUTArray((p.positive_fill ? p.positive_fill : p.fill), ['#4b4b17']);
	this.LUTnf = makeLUTArray((p.negative_fill ? p.negative_fill : p.fill), ['#4b4b17']);
	
	this.bar = null;

	this.zeroline = this.graph.AddLine(0, this.bottom-this.zero, this.width+16, this.bottom-this.zero, '#303030', 1);

	usesData(this.module, this.source);
}



BarGraph.prototype.Init = function(data)
{
    var d = data[this.module][this.source]
        
	this.inited = true;
	this.bar = [];
	this.sizey = d.length;
	this.sizex = d[0].length;
	var size = this.sizex*this.sizey;
	var bar_width = (this.width-4-15)/size;
	
	for(var i=0; i<size; i++)
	this.bar.push(this.graph.AddRect(6+bar_width*i, 2, bar_width, this.bottom, this.LUTpf[i % this.LUTpf.length], this.LUTp[i % this.LUTp.length]));

	this.max_value = this.graph.AddText(this.width-2, 10+26, this.max, 40, 12, 'end');
	this.min_value = this.graph.AddText(this.width-2, this.height-3, this.min, 40, 12, 'end');
}



BarGraph.prototype.Update = function(data)
{
    var d = data[this.module];
    if(!d) return;
    d = d[this.source]
    if(!d) return;
    
	if(!this.inited) this.Init(data);
    
	var b=0;
	
    if(this.autoscale)
    {
        var new_min = this.min;
        var new_max = this.max;
        for(var j=0; j<this.sizey; j++)
            for(var i=0; i<this.sizex; i++)
                if(d[j][i] < new_min)
                    new_min = d[j][i];
                else if(d[j][i] > new_max)
                    new_max = d[j][i];

        if(new_min < this.min && new_min < 0)
        {
            this.min = -Math.pow(10, Math.ceil(Math.log(-new_min) / Math.log(10)));
            if(0.5*this.min < new_min) this.min *= 0.5;
            if(0.5*this.min < new_min) this.min *= 0.5;
            this.min_value.firstChild.nodeValue = this.min;
        }
                
        if(new_max > this.max)
        {
            this.max = Math.pow(10, Math.ceil(Math.log(new_max) / Math.log(10)));
            if(0.5*this.max > new_max) this.max *= 0.5;
            if(0.5*this.max > new_max) this.max *= 0.5;
            this.max_value.firstChild.nodeValue = this.max;
        }
            
        this.scale = (this.height-6-26)/(this.max-this.min);
        this.zero = this.scale*(0-this.min);
        this.zeroline.setAttribute("y1",this.bottom-this.zero);
        this.zeroline.setAttribute("y2",this.bottom-this.zero);
    }
    
	for(var j=0; j<this.sizey; j++)
		for(var i=0; i<this.sizex; i++)
		{
			v = this.scale*(d[j][i]-this.min);
			if(v > this.zero)
			{
				this.bar[b].setAttribute("height", v-this.zero);
				this.bar[b].setAttribute("y", this.bottom-v);
				this.bar[b].setAttribute("fill", this.LUTpf[b % this.LUTpf.length]);
				this.bar[b].setAttribute("stroke", this.LUTp[b % this.LUTp.length]);
			}
			else
			{
				this.bar[b].setAttribute("height", this.zero-v);
				this.bar[b].setAttribute("y", this.bottom-this.zero);
				this.bar[b].setAttribute("fill", this.LUTnf[b % this.LUTnf.length]);
				this.bar[b].setAttribute("stroke", this.LUTn[b % this.LUTn.length]);
			}
			b++;
		}
}
