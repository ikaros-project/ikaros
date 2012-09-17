function VectorField(p)
{
	this.inited = false;
	this.graph = new Graph(p, p.x_module+'+'+p.y_module);
	
	this.x_module = p.x_module;
	this.x_source = p.x_source;
	
	this.y_module = p.y_module;
	this.y_source = p.y_source;
	
	this.width = p.width;
	this.height = p.height;
	this.scale = (p.scale ? p.scale : 1);
    		
	this.stroke_width = (p.stroke_width? p.stroke_width : 2);
	this.circle_color = (p.circle_color ? p.circle_color : 'none');
	
	usesData(this.x_module, this.x_source);
	usesData(this.y_module, this.y_source);
}



VectorField.prototype.Init = function(data)
{
    var d = data[this.x_module][this.x_source];
    
    this.inited = true;
    
    this.sizey = d.length;
    this.sizex = d[0].length;
    
    this.box_width = (this.width-4)/this.sizex;
    this.box_height = (this.height-4)/this.sizey;
    this.r = Math.min(this.box_width, this.box_height)/2-1;
    this.r1 = this.r;
    this.r2 = this.r - 3*this.stroke_width;
    
    this.w2 = this.box_width/2+2;
    this.h2 = this.box_height/2+2;
    this.box = new Array(this.sizey);

    for(var j=0; j<this.sizey; j++)
    {
        this.box[j] = new Array(this.sizex);
        for(var i=0; i<this.sizex; i++)
        {
            x = this.w2+this.box_width*i;
            y = this.h2+this.box_height*j;
    
            if(this.circle_color != 'none')
                this.graph.AddCircle(x, y, this.r, this.circle_color);
            this.box[j][i] = this.graph.AddLine(x, y, x+10, y, 'black', this.stroke_width);
            this.box[j][i].setAttribute("marker-end", "url(#Triangle)");
        }
    }
}



VectorField.prototype.Update = function(data)
{
    var dx = data[this.x_module];
    if(!dx) return;
    dx = dx[this.x_source]
    if(!dx) return;
    
    var dy = data[this.y_module];
    if(!dy) return;
    dy = dy[this.y_source]
    if(!dy) return;
    
    if(!this.inited) this.Init(data);

    var dx = data[this.x_module][this.x_source];
    var dy = data[this.y_module][this.y_source];
    
    if(!dx) return;
    
    for(var j=0; j<this.sizey; j++)
        for(var i=0; i<this.sizex; i++)
        {	
            var x = this.w2+this.box_width*i;
            var y = this.h2+this.box_height*j;

            var a = dx[j][i];
            var b = dy[j][i];
            var L = this.scale/Math.sqrt(a*a+b*b);
            if(L != 0) {
                a *= L;
                b *= L;
            }
            this.box[j][i].setAttribute('x1', x-this.r1*a);
            this.box[j][i].setAttribute('y1', y-this.r1*b);
            this.box[j][i].setAttribute('x2', x+this.r2*a);
            this.box[j][i].setAttribute('y2', y+this.r2*b);
        }
}
