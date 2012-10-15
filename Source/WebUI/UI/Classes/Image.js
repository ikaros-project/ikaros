function Image(p)
{
    this.obj = 	new WebUIObject(p, p.module+'.'+p.source);
    
    this.module = p.module;
	this.source = p.source;
	this.type = p.type;
	this.width = p.width;
	this.height = p.height;
    this.file = p.file;
    this.oversampling = (this.file ? 4 : 1);
    
    this.canvas = document.createElement("canvas");
    this.canvas.style.width = p.width;
    this.canvas.style.height = p.height;
    this.canvas.width = this.oversampling*p.width;
    this.canvas.height = this.oversampling*p.height;
    this.canvas.style.borderRadius = "11px";
    
    this.obj.bg.appendChild(this.canvas);

    this.context = this.canvas.getContext("2d");
    this.imageObj = document.createElement("image");
    
    var that = this;
    this.imageObj.onload = function()
    {
        that.context.drawImage(this, 0, 0, that.oversampling*that.width, that.oversampling*that.height);
    }
    
    if(this.module != undefined)
        usesBase64Data(this.module, this.source, this.type);
    else if(this.file)
        this.imageObj.src = "/"+this.file;
    else
    {
        this.context.fillStyle="black";
        this.context.fillRect(0, 0, this.width, this.height);
    }
}



Image.prototype.Update = function(data)
{
    if(!this.module || this.file)
        return;

    var d = data[this.module];
    if(!d) return;
    d = d[this.source+':'+this.type]
    if(!d) return;
    this.imageObj.src = d;
}

