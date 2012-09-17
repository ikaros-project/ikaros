var animationInterval = 0; // ms
var arrayLines = [];

function ArrayLine(p)
{
	ArrayLine.prototype.Init = function(data)
	{
		this.inited = true;
		this.circle = [];
		d = data[this.module][this.source];
		this.sizex = d[0].length;
		this.sizey = d.length;
		this.path_end = false;
		this.last = [];
		
		for(var i=0; i<this.sizey-1; i++)
		{
			m1x = (d[i][0]-this.min)*this.scale * this.width;
			m1y = (d[i][1]-this.min)*this.scale * this.height;

			if(this.circle)
				this.circle.push(this.graph.AddCircle(m1x, m1y, (0.035353535/2*this.scale* this.width), 'none',this.circle_color,1));	
		}
        
		arrayLines.push(this);
	}

	ArrayLine.prototype.Update = function(data)
	{
        var d = data[this.module];
        if(!d) return;
        d = d[this.source]
        if(!d) return;
        
        if(!this.inited) this.Init(data);

		for(var i=0; i<this.sizey-1; i++)
		{
			if(d[i+1][0] == -1)
			{
				this.path_end = true;	
	
			}
			if(this.path_end)
			{
				m1x = this.last[0];
				m1y = this.last[1];
			}
			else
			{
				m1x = (d[i][0]-this.min)*this.scale * this.width;
				m1y = (d[i][1]-this.min)*this.scale * this.height;
				this.last[0] = m1x;
				this.last[1] = m1y;
			}
			if(this.circle)
			{
				this.circle[i].setAttribute("cx", m1x);
				this.circle[i].setAttribute("cy", m1y);
                this.circle[i].setAttribute("opacity", "0");
			}
		}
		this.path_end = false;
        
        this.index=0;
        
        // The first array lines controls the animation for all ArrayLines

        if(arrayLines[0] == this)
            setTimeout("animate();", animationInterval);  // animate every 100 ms
	}

	ArrayLine.prototype.Animate = function()
    {
       this.circle[this.index].setAttribute("opacity", "1");
       this.index++;
    }

    if(p.interval)
        animationInterval = p.interval;

	this.inited = false;
	this.graph = new Graph(p, p.module+'.'+p.source);
	this.module = p.module;
	this.source = p.source;
	this.circle_color = (p.circle_color ? p.circle_color : 'yellow');
	this.width = p.width;
	this.height = p.height;
	this.min = (p.min ? p.min : 0);
	this.max = (p.max ? p.max : 1);
	this.scale = (this.max-this.min);

	this.circle = null;

	usesData(this.module, this.source);
}


function animate()
{
    for(var i=0; i<arrayLines.length; i++)
        arrayLines[i].Animate();
    setTimeout("animate();", animationInterval);
}

