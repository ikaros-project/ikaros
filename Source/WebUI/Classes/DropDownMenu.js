function DropDownMenu(p)
{
	var that = this;

    this.module = p.module;
    this.parameter = p.parameter;
    p.opaque = (p.opaque!=undefined ? p.opaque : 'no');
	this.graph = new Graph(p, (p.name ? p.name : ''));    
    popup = this.graph.AddHTMLPopUp(2, 4, p.title, p.list, p.width-2, p.height-4);

    function none()
    {
    }
    
    popup.onmouseup = function (e)
    {
        if(that.module && that.parameter)
            get("/control/"+that.module+"/"+that.parameter+"/0/0/"+e.target.value, none);
    };

    usesData(this.module, this.parameter);
}


DropDownMenu.prototype.Update = function(data)
{
    var d = data[this.module];
    if(!d) return;
    d = d[this.parameter]
    if(!d) return;
    
    popup.value = d[0][0];
}

