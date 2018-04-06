class WebUIWidgetCanvas extends WebUIWidget
{
    static html()
    {
         return `
            <canvas></canvas>
        `;
    }

/*
    connectedCallback()
    {
        super.connectedCallback();
    }
*/

    updateFrame()
    {
        super.updateFrame();
        
        this.canvasElement.width = this.offsetWidth * (this.oversampling ? this.oversampling : 1);
        this.canvasElement.height = this.offsetHeight * (this.oversampling ? this.oversampling : 1);
        this.canvasElement.style.width = this.offsetWidth+"px";
        this.canvasElement.style.height = this.offsetHeight+"px";

        this.width = this.offsetWidth;
        this.height = this.offsetHeight;
        this.format.width = this.width - this.format.marginLeft - this.format.marginRight;
        this.format.height = this.height - this.format.marginTop - this.format.marginBottom;
    }

    init()
    {
        this.canvasElement = this.querySelector('canvas');
        this.canvas = this.canvasElement.getContext("2d");
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
