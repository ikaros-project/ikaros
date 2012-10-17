function Image(p)
{
    this.oversampling = (p.file ? 4 : 1);
    this.obj = 	new WebUICanvas(this, p);
    this.file = p.file;

    /*
    this.canvas = document.createElement("canvas");
    this.canvas.style.width = p.width;
    this.canvas.style.height = p.height;
    this.canvas.width = this.oversampling*p.width;
    this.canvas.height = this.oversampling*p.height;
    this.canvas.style.borderRadius = "11px";
    
    this.obj.bg.appendChild(this.canvas);

    this.context = this.canvas.getContext("2d");
*/
    
    this.imageObj = document.createElement("image");

    if(this.module != undefined)
        usesBase64Data(this.module, this.source, this.type);

    else if(this.file)
    {
        this.imageObj.src = "/"+this.file;
        this.context.drawImage(this.imageObj, 0, 0, this.width, this.height);
    }
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
    this.context.drawImage(this.imageObj, 0, 0, this.width, this.height);
}

