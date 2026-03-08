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

        const cssWidth = Math.max(1, this.offsetWidth);
        const cssHeight = Math.max(1, this.offsetHeight);
        const oversampling = this.oversampling ? this.oversampling : 1;
        const dpr = window.devicePixelRatio || 1;
        const scale = oversampling * dpr;
        this.canvas_scale = scale;

        this.canvasElement.width = Math.max(1, Math.round(cssWidth * scale));
        this.canvasElement.height = Math.max(1, Math.round(cssHeight * scale));
        this.canvasElement.style.width = cssWidth+"px";
        this.canvasElement.style.height = cssHeight+"px";
        if(this.canvas && this.canvas.setTransform)
            this.canvas.setTransform(scale, 0, 0, scale, 0, 0);

        this.width = cssWidth;
        this.height = cssHeight;
        this.format.width = this.width - this.format.marginLeft - this.format.marginRight;
        this.format.height = this.height - this.format.marginTop - this.format.marginBottom;
    }

    resetCanvasTransform(offsetX=0, offsetY=0)
    {
        const s = this.canvas_scale || 1;
        this.canvas.setTransform(s, 0, 0, s, offsetX*s, offsetY*s);
    }

    setCanvasTransform(a, b, c, d, e, f)
    {
        const s = this.canvas_scale || 1;
        this.canvas.setTransform(a*s, b*s, c*s, d*s, e*s, f*s);
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

    drawArrow(arrow)
    {
        this.canvas.beginPath();
        this.canvas.moveTo(arrow[arrow.length-1][0],arrow[arrow.length-1][1]);
        for(var i=0;i<arrow.length;i++){
            this.canvas.lineTo(arrow[i][0],arrow[i][1]);
        }
        this.canvas.closePath();
        this.canvas.fill();
        this.canvas.stroke();
    }

    moveArrow(arrow, x, y)
    {
        var rv = [];
        for(var i=0;i<arrow.length;i++){
            rv.push([arrow[i][0]+x, arrow[i][1]+y]);
        }
        return rv;
    }

    rotateArrow(arrow,angle)
    {
        var rv = [];
        for(var i=0; i<arrow.length;i++){
            rv.push([(arrow[i][0] * Math.cos(angle)) - (arrow[i][1] * Math.sin(angle)),
                     (arrow[i][0] * Math.sin(angle)) + (arrow[i][1] * Math.cos(angle))]);
        }
        return rv;
    }

    drawArrowHead(fromX, fromY, toX, toY)
    {
        if(fromX==toX && fromY==toY)
            return;

        var angle = Math.atan2(toY-fromY, toX-fromX);
        var arrow = [[0,0], [-10,-5], [-10, 5]];
        this.canvas.save();
        this.canvas.lineJoin = "miter";
        this.canvas.fillStyle = this.canvas.strokeStyle;
        this.drawArrow(this.moveArrow(this.rotateArrow(arrow,angle),toX,toY));
        this.canvas.restore();
    }


    drawLayout()
    {
        this.canvas.canvas.beginPath();
        this.canvas.canvas.lineWidth = 1;
        this.canvas.canvas.strokeStyle = "gray";

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
