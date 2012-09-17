function Image(p)
{
	this.graph = new Graph(p, p.module+'.'+p.source);	
	this.module = p.module;
	this.source = p.source;
	this.file = p.file;
	this.width = p.width;
	this.height = p.height;
	this.type = (p.type ? p.type : "gray");
	this.c = 0;
	
	if(this.module != undefined)
		this.image = this.graph.AddImage(-0.5, -0.5, p.width, p.height, '/module/'+p.module+'/'+p.source+'/'+this.type+'.jpg');
	else if(this.file)
		this.image = this.graph.AddImage(-0.5, -0.5, p.width, p.height, "/"+this.file);
    else
        this.graph.frame.setAttribute("fill", "black");
}



Image.prototype.Update = function(data)
{
	if(this.image && !this.file)
		this.image.setAttributeNS(xlinkns, 'href', '/module/'+this.module+'/'+this.source+'/'+this.type+'.jpg?'+this.c++);
}
