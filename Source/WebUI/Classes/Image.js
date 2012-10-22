function Image(p)
{
    this.oversampling = (p.oversampling ? p.oversampling : (p.file ? 4 : 1));
    this.obj = 	new WebUICanvas(this, p);
    this.file = p.file;
    this.type = (p.type ? p.type : "gray");

    this.imageObj = document.createElement("image");
 
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



Image.prototype.LoadData = function(data)
{
    if(this.module)
    {
        var d = data[this.module];
        if(!d) return;
        d = d[this.source+':'+this.type]
        if(!d) return;
        
        var d1 = new Date();
        this.imageObj.onload = function ()
        {
            var d2 = new Date();
            top.profiling.image_decoding += (d2.getTime() - d1.getTime());
            load_count--;
        };
        
        this.imageObj.src = d;
        return 1;
    }
    
    return 0;
}



Image.prototype.Update = function(data)
{
    var d1 = new Date();
    this.context.drawImage(this.imageObj, 0, 0, this.oversampling*this.width, this.oversampling*this.height);
    var d2 = new Date();
    top.profiling.image_drawing += (d2.getTime() - d1.getTime());
}

