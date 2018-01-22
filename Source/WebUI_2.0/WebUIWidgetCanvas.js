class WebUIWidgetCanvas extends WebUIWidget
{
/*
    constructor()
    {
        super();
    }
*/
    static template()
    {
        return [
            {'name':'source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'title', 'default':"", 'type':'string', 'control': 'textedit'}
        ]};
    
    static html()
    {
        return `
            <style>
                canvas { background-color: rgba(0,0,0,0); }
                main[data-mode="run"] canvas { cursor: default; }
            </style>
            <canvas style="height: 100%; width:100%" height="100" width="100"></canvas>
        `;
    }

    connectedCallback()
    {
        this.innerHTML = this.constructor.html();
        this.canvasElement = this.querySelector('canvas');
        this.canvas = this.canvasElement.getContext("2d");

        // read CSS variables
        
        this.format = {}
        
        this.format.marginTop = parseInt(getComputedStyle(this.canvasElement).getPropertyValue('--margin-top'));
        this.format.marginBottom = parseInt(getComputedStyle(this.canvasElement).getPropertyValue('--margin-bottom'));
        this.format.marginLeft = parseInt(getComputedStyle(this.canvasElement).getPropertyValue('--margin-left'));
        this.format.marginRight = parseInt(getComputedStyle(this.canvasElement).getPropertyValue('--margin-right'));

        this.format.titleFont = getComputedStyle(this.canvasElement).getPropertyValue('--title-font').trim();
        this.format.titleColor = getComputedStyle(this.canvasElement).getPropertyValue('--title-color').trim();
        this.format.titleBackground = getComputedStyle(this.canvasElement).getPropertyValue('--title-background').trim();
        this.format.titleMargins = getComputedStyle(this.canvasElement).getPropertyValue('--title-magins').trim() == "yes";
        this.format.titleAlign = getComputedStyle(this.canvasElement).getPropertyValue('--title-align').trim();
        this.format.titleOffset = getComputedStyle(this.canvasElement).getPropertyValue('--title-offset');
        this.format.titleOffsetX = parseInt(this.format.titleOffset.split(",")[0].trim());
        this.format.titleOffsetY = parseInt(this.format.titleOffset.split(",")[1].trim());
        this.format.color = getComputedStyle(this.canvasElement).getPropertyValue('--color').trim();
        this.format.lineWidth = getComputedStyle(this.canvasElement).getPropertyValue('--line-width').trim();
        this.format.fill = getComputedStyle(this.canvasElement).getPropertyValue('--fill').trim();

        // read parameters
        
        // set colors etc from parameters if set
    }


    update()
    {
        let width = parseInt(getComputedStyle(this.canvasElement).width);
        let height = parseInt(getComputedStyle(this.canvasElement).height);
        if(width != this.canvasElement.width || height != this.canvasElement.height)
        {
            this.canvasElement.width = parseInt(width);
            this.canvasElement.height = parseInt(height);
        }

        draw();
    }


    drawTitle()
    {
        this.canvas.beginPath();
        this.canvas.fillStyle = this.format.titleBackground;
        if(this.format.titleBackgroundMargins)
            this.canvas.rect(+this.format.marginLeft-0.5, 0, this.width - this.format.marginRight-0.5, +this.format.marginTop-0.5);
        else
            this.canvas.rect(-0.5, -0.5, this.width, +this.format.marginTop);
        this.canvas.fill();

        this.canvas.font = this.format.titleFont;
        this.canvas.fillStyle = this.format.titleColor;
        this.canvas.textAlign = this.format.titleAlign;

        if(this.format.titleMargins)
        {
            if(this.canvas.textAlign == 'left')
                this.canvas.fillText("Title", this.format.marginLeft+this.format.titleOffsetX, this.format.marginTop+this.format.titleOffsetY-1);
            else if(this.canvas.textAlign == 'right')
                this.canvas.fillText("Title", this.width - this.format.marginRight-this.format.titleOf. this.format.marginTop+this.format.titleOffsetY-1);
            else if(this.canvas.textAlign == 'center')
                this.canvas.fillText("Title", +this.format.marginLeft+this.format.width/2, +this.format.marginTop+this.format.titleOffsetY-1);
        }
        else
        {
             if(this.canvas.textAlign == 'left')
                this.canvas.fillText("OTitleq", this.format.titleOffsetX-1, this.format.marginTop+this.format.titleOffsetY-1);
            else if(this.canvas.textAlign == 'right')
                this.canvas.fillText("Title", this.width-this.format.titleOffsetX, this.format.marginTop+this.format.titleOffsetY-1);
            else if(this.canvas.textAlign == 'center')
                this.canvas.fillText("Title", this.width/2, this.format.marginTop+this.format.titleOffsetY-1);
        }
    }


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

        this.canvas.moveTo(-0.5, this.format.marginTop-1);
        this.canvas.lineTo(this.width, this.format.marginTop-1);

        this.canvas.moveTo(-0.5, this.height-this.format.marginBottom);
        this.canvas.lineTo(this.width, this.height-this.format.marginBottom);

        this.canvas.moveTo(this.format.marginLeft-1, -0.5);
        this.canvas.lineTo(this.format.marginLeft-1, this.height);

        this.canvas.moveTo(this.width-this.format.marginRight, -0.5);
        this.canvas.lineTo(this.width-this.format.marginRight, this.height);

        this.canvas.stroke();
    }
    
    draw()
    {
        this.canvas.beginPath();
        this.canvas.lineWidth = lineWidth;
        this.canvas.strokeStyle = c;

        this.canvas.moveTo(width*Math.random(), height*Math.random());
        this.canvas.lineTo(width*Math.random(), height*Math.random());
        
        this.canvas.stroke();
        
        var w = this;
        setTimeout(function () { w.draw()}, 100)
    }
};



webui_widgets.add('webui-widget-canvas', WebUIWidgetCanvas);
