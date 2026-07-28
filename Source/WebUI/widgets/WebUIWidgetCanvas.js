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
        this.format.width = this.width - this.format.margin_left - this.format.margin_right;
        this.format.height = this.height - this.format.margin_top - this.format.margin_bottom;
    }

    resetCanvasTransform(offsetX=0, offsetY=0)
    {
        const s = this.canvas_scale || 1;
        this.canvas.setTransform(s, 0, 0, s, offsetX*s, offsetY*s);
    }

    clearCanvas(offsetX=-0.5, offsetY=-0.5)
    {
        this.resetCanvasTransform(offsetX, offsetY);
        this.canvas.clearRect(0, 0, this.width, this.height);
    }

    beginCanvasDraw(offsetX=-0.5, offsetY=-0.5)
    {
        this.clearCanvas(offsetX, offsetY);
        this.canvas.translate(this.format.margin_left, this.format.margin_top);
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
        var l = this.format.stroke_color.split(",");
        var n = l.length;
        this.canvas.strokeStyle = l[i % n].trim();

        l = this.format.fill_color.split(",");
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
        this.canvas.beginPath();
        this.canvas.lineWidth = 1;
        this.canvas.strokeStyle = "gray";

        this.canvas.moveTo(0, this.format.margin_top);
        this.canvas.lineTo(this.width, this.format.margin_top);

        this.canvas.moveTo(0, this.height-this.format.margin_bottom);
        this.canvas.lineTo(this.width, this.height-this.format.margin_bottom);

        this.canvas.moveTo(this.format.margin_left, 0);
        this.canvas.lineTo(this.format.margin_left, this.height);

        this.canvas.moveTo(this.width-this.format.margin_right, 0);
        this.canvas.lineTo(this.width-this.format.margin_right, this.height);

        this.canvas.stroke();
    }

};
