class WebUIWidgetVectorField extends WebUIWidgetGraph
{
    static template()
    {
        return [
            {'name': "VECTOR FIELD", 'control':'header'},
            {'name':'title', 'default':"Vector Field", 'type':'string', 'control':'textedit'},
            {'name':'source', 'default':"", 'type':'source', 'control':'textedit'},
            {'name':'x_source', 'default':"", 'type':'source', 'control':'textedit'},
            {'name':'y_source', 'default':"", 'type':'source', 'control':'textedit'},

            {'name': "VECTORS", 'control':'header'},
            {'name':'length_mode', 'default':"relative", 'type':'string', 'control':'menu', 'options':"relative,coordinate,normalized"},
            {'name':'vector_scale', 'default':0.8, 'type':'float', 'control':'textedit'},
            {'name':'anchor', 'default':"center", 'type':'string', 'control':'menu', 'options':"center,start"},
            {'name':'stride', 'default':1, 'type':'int', 'control':'textedit'},
            {'name':'arrow', 'default':"yes", 'type':'bool', 'control':'checkbox'},

            {'name': "STYLE", 'control':'header'},
            {'name':'stroke_color', 'default':'black', 'type':'string', 'control':'textedit'},
            {'name':'line_width', 'default':1, 'type':'float', 'control':'textedit'},
            {'name':'line_cap', 'default':"round", 'type':'string', 'control':'menu', 'options':"butt,round,square"},
            {'name':'origin_marker', 'default':"none", 'type':'string', 'control':'menu', 'options':"none,dot,circle"},
            {'name':'origin_marker_size', 'default':2, 'type':'float', 'control':'textedit'},

            {'name': "COORDINATE SYSTEM", 'control':'header'},
            {'name':'scale_visibility', 'default':"no", 'type':'string', 'control':'menu', 'options':"yes,no,invisible", 'class':'true'},
            {'name':'x_min', 'default':0, 'type':'float', 'control':'textedit'},
            {'name':'x_max', 'default':1, 'type':'float', 'control':'textedit'},
            {'name':'y_min', 'default':0, 'type':'float', 'control':'textedit'},
            {'name':'y_max', 'default':1, 'type':'float', 'control':'textedit'},
            {'name':'flip_x_axis', 'default':"no", 'type':'string', 'control':'menu', 'options':"yes,no"},
            {'name':'flip_y_axis', 'default':"no", 'type':'string', 'control':'menu', 'options':"yes,no"},
            {'name':'flip_x_canvas', 'default':"no", 'type':'string', 'control':'menu', 'options':"yes,no"},
            {'name':'flip_y_canvas', 'default':"no", 'type':'string', 'control':'menu', 'options':"yes,no"},
        ];
    }

    init()
    {
        super.init();
        this.xData = [];
        this.yData = [];
    }

    isMatrix(data)
    {
        return Array.isArray(data) && data.length > 0 && Array.isArray(data[0]) && data[0].length > 0;
    }

    getFieldData()
    {
        const packed = this.getSource('source');
        if(Array.isArray(packed) && packed.length >= 2 && this.isMatrix(packed[0]) && this.isMatrix(packed[1]))
            return {x: packed[0], y: packed[1]};

        const x = this.getSource('x_source');
        const y = this.getSource('y_source');
        if(this.isMatrix(x) && this.isMatrix(y))
            return {x, y};
        return null;
    }

    getFieldShape(xData, yData)
    {
        if(!this.isMatrix(xData) || !this.isMatrix(yData) || xData.length !== yData.length)
            return null;

        const rows = xData.length;
        const columns = xData[0].length;
        if(columns === 0 || yData[0].length !== columns)
            return null;
        for(let row = 0; row < rows; row++)
            if(!Array.isArray(xData[row]) || !Array.isArray(yData[row]) ||
               xData[row].length !== columns || yData[row].length !== columns)
                return null;
        return {rows, columns};
    }

    getVectorDisplacement(vx, vy, magnitude, cellWidth, cellHeight, plotWidth, plotHeight)
    {
        const configuredScale = Number(this.parameters.vector_scale);
        const scale = Number.isFinite(configuredScale) ? configuredScale : 0.8;
        if(this.parameters.length_mode === "normalized")
        {
            if(magnitude === 0)
                return {x:0, y:0};
            return {x:scale * cellWidth * vx / magnitude, y:scale * cellHeight * vy / magnitude};
        }
        if(this.parameters.length_mode === "coordinate")
        {
            const xRange = Number(this.parameters.x_max) - Number(this.parameters.x_min);
            const yRange = Number(this.parameters.y_max) - Number(this.parameters.y_min);
            return {
                x:Number.isFinite(xRange) && xRange !== 0 ? scale * plotWidth * vx / xRange : 0,
                y:Number.isFinite(yRange) && yRange !== 0 ? scale * plotHeight * vy / yRange : 0
            };
        }
        return {x:scale * cellWidth * vx, y:scale * cellHeight * vy};
    }

    drawOriginMarker(x, y)
    {
        if(this.parameters.origin_marker === "none")
            return;

        const configuredSize = Number(this.parameters.origin_marker_size);
        const size = Number.isFinite(configuredSize) ? Math.max(0, configuredSize) : 2;
        this.canvas.beginPath();
        this.canvas.arc(x, y, size, 0, 2 * Math.PI);
        if(this.parameters.origin_marker === "dot")
        {
            this.canvas.fillStyle = this.canvas.strokeStyle;
            this.canvas.fill();
        }
        else
            this.canvas.stroke();
    }

    drawPlotHorizontal(width, height, index, transform)
    {
        const shape = this.getFieldShape(this.xData, this.yData);
        if(!shape)
            return;

        const {rows, columns} = shape;
        const cellWidth = width / columns;
        const cellHeight = height / rows;
        const stride = Math.max(1, Math.trunc(Number(this.parameters.stride) || 1));
        const centerAnchor = this.parameters.anchor !== "start";
        this.canvas.strokeStyle = String(this.parameters.stroke_color || "black").split(',')[0].trim();
        this.canvas.lineWidth = Math.max(0, Number(this.parameters.line_width) || 0);
        this.canvas.lineCap = this.parameters.line_cap;

        for(let row = 0; row < rows; row += stride)
        {
            for(let column = 0; column < columns; column += stride)
            {
                const vx = Number(this.xData[row][column]);
                const vy = Number(this.yData[row][column]);
                if(!Number.isFinite(vx) || !Number.isFinite(vy))
                    continue;

                const magnitude = Math.hypot(vx, vy);
                const originX = (column + 0.5) * cellWidth;
                const originY = (row + 0.5) * cellHeight;
                const displacement = this.getVectorDisplacement(vx, vy, magnitude, cellWidth, cellHeight, width, height);
                const startX = centerAnchor ? originX - displacement.x / 2 : originX;
                const startY = centerAnchor ? originY - displacement.y / 2 : originY;
                const endX = centerAnchor ? originX + displacement.x / 2 : originX + displacement.x;
                const endY = centerAnchor ? originY + displacement.y / 2 : originY + displacement.y;
                const start = transform(startX, startY);
                const end = transform(endX, endY);
                const origin = transform(originX, originY);

                this.drawOriginMarker(origin[0], origin[1]);
                if(magnitude === 0)
                    continue;

                this.canvas.beginPath();
                this.canvas.moveTo(start[0], start[1]);
                this.canvas.lineTo(end[0], end[1]);
                this.canvas.stroke();
                if(this.parameters.arrow)
                    this.drawArrowHead(start[0], start[1], end[0], end[1]);
            }
        }
    }

    update()
    {
        const field = this.getFieldData();
        if(!field || !this.getFieldShape(field.x, field.y))
        {
            this.xData = [];
            this.yData = [];
            this.clearCanvas();
            return;
        }

        this.xData = field.x;
        this.yData = field.y;
        this.beginCanvasDraw();
        this.drawHorizontal(1, 1);
    }
}


webui_widgets.add('webui-widget-vector-field', WebUIWidgetVectorField);
