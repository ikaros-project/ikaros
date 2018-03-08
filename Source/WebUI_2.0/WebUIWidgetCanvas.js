class WebUIWidgetCanvas extends WebUIWidget
{
    static html()
    {
         return `
            <canvas></canvas>
        `;
    }


    connectedCallback()
    {
        super.connectedCallback();

//        this.style.display = "block";


//        this.innerHTML = this.constructor.html();
        
        // update style
        
 //       this.updateStyle(this, this.parameters['style']);
 //       this.updateStyle(this.parentNode, this.parameters['frame-style']);
 //       this.readCSSvariables();
        
    }
    
    init()
    {
        this.canvasElement = this.querySelector('canvas');
        this.canvas = this.canvasElement.getContext("2d");
    }
/*
    drawTitle()
    {
        this.canvas.beginPath();
        this.canvas.lineWidth = 1;
//        this.canvas.strokeStyle = "purple";
        this.canvas.fillStyle = this.format.titleBackground;

        if(this.format.titleMargins)
            this.canvas.rect(this.format.marginLeft, 0, this.width - this.format.marginLeft - this.format.marginRight, this.format.titleHeight);
        else
            this.canvas.rect(0, 0, this.width, this.format.titleHeight);
        this.canvas.fill();
        this.canvas.stroke();
        
        this.canvas.font = this.format.titleFont;
        this.canvas.fillStyle = this.format.titleColor;
        this.canvas.textAlign = this.format.titleAlign;
        this.canvas.textBaseline="bottom";

        if(this.format.titleMargins)
        {
            if(this.canvas.textAlign == 'left')
                this.canvas.fillText(this.parameters.title, this.format.marginLeft+this.format.titleOffsetX, this.format.titleHeight+this.format.titleOffsetY-1);
            else if(this.canvas.textAlign == 'right')
                this.canvas.fillText(this.parameters.title, this.width - this.format.marginRight-this.format.titleOf. this.format.titleHeight+this.format.titleOffsetY-1);
            else if(this.canvas.textAlign == 'center')
                this.canvas.fillText(this.parameters.title, +this.format.marginLeft+this.format.width/2, this.format.titleHeight+this.format.titleOffsetY-1);
        }
        else
        {
             if(this.canvas.textAlign == 'left')
                this.canvas.fillText(this.parameters.title, this.format.titleOffsetX-1, this.format.titleHeight+this.format.titleOffsetY-1);
            else if(this.canvas.textAlign == 'right')
                this.canvas.fillText(this.parameters.title, this.width-this.format.titleOffsetX, this.format.titleHeight+this.format.titleOffsetY-1);
            else if(this.canvas.textAlign == 'center')
                this.canvas.fillText(this.parameters.title, this.width/2, this.format.titleHeight+this.format.titleOffsetY-1);
        }
    }
*/
    setColor(i)
    {
        var l = this.format.color.split(",");
        var n = l.length;
        this.canvas.strokeStyle = l[i % n].trim();

        l = this.format.fill.split(",");
        n = l.length;
        this.canvas.fillStyle = l[i % n].trim();
    }

    drawLayout()
    {
        this.canvas.beginPath();
        this.canvas.lineWidth = 1;
        this.canvas.strokeStyle = "gray";

        this.canvas.moveTo(0, this.format.marginTop);
        this.canvas.lineTo(this.width, this.format.marginTop);

        this.canvas.moveTo(0, this.format.height);
        this.canvas.lineTo(this.width, this.format.height);

        this.canvas.moveTo(this.format.marginLeft, 0);
        this.canvas.lineTo(this.format.marginLeft, this.height);

        this.canvas.moveTo(this.width-this.format.marginRight, 0);
        this.canvas.lineTo(this.width-this.format.marginRight, this.height);

        this.canvas.stroke();
    }

};



webui_widgets.add('webui-widget-canvas', WebUIWidgetCanvas);
