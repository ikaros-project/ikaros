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

    getColorMap(name, customColors="")
    {
        const custom = String(customColors ?? "").split(',').map((color) => color.trim()).filter((color) => color !== "");
        if(custom.length > 0)
            return custom;
        if(name === "fire" && typeof LUT_fire !== "undefined")
            return LUT_fire;
        if(name === "spectrum" && typeof LUT_spectrum !== "undefined")
            return LUT_spectrum;
        return typeof LUT_gray !== "undefined" ? LUT_gray : ["black", "white"];
    }

    getColorMapColor(value, minimum, maximum, colorMap)
    {
        const numeric = Number(value);
        const min = Number(minimum);
        const max = Number(maximum);
        if(!Number.isFinite(numeric) || !Number.isFinite(min) || !Number.isFinite(max) || !Array.isArray(colorMap) || colorMap.length === 0)
            return "black";
        const fraction = max !== min ? (numeric - min) / (max - min) : 0;
        const index = Math.max(0, Math.min(colorMap.length - 1, Math.floor(fraction * colorMap.length)));
        return String(colorMap[index] ?? "black").trim();
    }

    hasVerticalColorLegend()
    {
        return Boolean(this.parameters.show_color_legend);
    }

    getVerticalColorLegendSpace()
    {
        if(!this.hasVerticalColorLegend())
            return 0;
        const width = Math.max(4, Number(this.parameters.color_legend_width) || 14);
        return width + 52;
    }

    formatColorLegendValue(value)
    {
        const configuredDecimals = Number(this.parameters.color_legend_decimals);
        const decimals = Number.isFinite(configuredDecimals) ? Math.max(0, Math.min(20, Math.trunc(configuredDecimals))) : 2;
        return Number.isFinite(value) ? value.toFixed(decimals).replace(/\.?0+$/, "") : "";
    }

    drawVerticalColorLegend(x, y, height, minimum, maximum, colorMap)
    {
        if(!this.hasVerticalColorLegend() || !Array.isArray(colorMap) || colorMap.length === 0 || height <= 0)
            return;

        const width = Math.max(4, Number(this.parameters.color_legend_width) || 14);
        const ticks = Math.max(2, Math.trunc(Number(this.parameters.color_legend_ticks) || 5));
        const gradient = this.canvas.createLinearGradient(0, y + height, 0, y);
        if(colorMap.length === 1)
        {
            gradient.addColorStop(0, colorMap[0]);
            gradient.addColorStop(1, colorMap[0]);
        }
        else
            for(let index = 0; index < colorMap.length; index++)
                gradient.addColorStop(index / (colorMap.length - 1), colorMap[index]);

        this.canvas.save();
        this.canvas.fillStyle = gradient;
        this.canvas.fillRect(x, y, width, height);
        this.canvas.strokeStyle = this.format.axis_color || "black";
        this.canvas.lineWidth = 1;
        this.canvas.strokeRect(x, y, width, height);
        this.canvas.fillStyle = this.format.axis_color || "black";
        this.canvas.font = this.format.scale_font || "12px sans-serif";
        this.canvas.textAlign = "left";
        this.canvas.textBaseline = "middle";
        for(let index = 0; index < ticks; index++)
        {
            const fraction = ticks > 1 ? index / (ticks - 1) : 0;
            const tickY = y + height * (1 - fraction);
            const value = minimum + fraction * (maximum - minimum);
            this.canvas.beginPath();
            this.canvas.moveTo(x + width, tickY);
            this.canvas.lineTo(x + width + 4, tickY);
            this.canvas.stroke();
            this.canvas.fillText(this.formatColorLegendValue(value), x + width + 7, tickY);
        }
        this.canvas.restore();
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
