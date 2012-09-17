function Text(p)
{
    p.opaque = (p.opaque!=undefined ? p.opaque : 'no');
	this.graph = new Graph(p, (p.name ? p.name : ''));    
    this.graph.AddHTMLText(2, 4, p.text, p.width-2, p.height-4);
}


Text.prototype.Update = function(data)
{
}
