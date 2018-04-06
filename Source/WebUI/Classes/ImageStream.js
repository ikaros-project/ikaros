function ImageStream(p)
{
    this.oversampling = (p.oversampling ? p.oversampling : (p.file ? 4 : 1));
    this.obj = 	new WebUICanvas(this, p);
    this.file = p.file;
    this.type = (p.type ? p.type : "gray");

    this.imageObj = new Image();
 
    if(this.file)
         this.imageObj.src = "/"+this.file;
    else
    {
        this.context.fillStyle="black";
        this.context.fillRect(0, 0, this.width, this.height);
        if(this.module != undefined)
            usesBase64Data(this.module, this.source, this.type);
    }
}



ImageStream.prototype.LoadData = function(data)
{
    if(this.module)
    {
        var d = data[this.module];
        if(!d) return;
        d = d[this.source+':'+this.type]
        if(!d) return;
        
        this.imageObj.onload = function ()
        {
            load_count--;
        };
        
        this.imageObj.src = d;
        return 1;
    }
    
    return 0;
}



ImageStream.prototype.Update = function(data)
{
    this.context.drawImage(this.imageObj, 0, 0, this.oversampling*this.width, this.oversampling*this.height);
}

