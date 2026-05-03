class WebUIWidgetFaceBoxes extends WebUIWidgetCanvas
{
    static template()
    {
        return [
            {'name': "FACE BOXES", 'control':'header'},
            {'name':'title', 'default':"Face Boxes", 'type':'string', 'control': 'textedit'},
            {'name':'source', 'default':"", 'type':'source', 'control': 'textedit'},

            {'name': "STYLE", 'control':'header'},
            {'name':'color', 'default':"#00ff66", 'type':'string', 'control': 'textedit'},
            {'name':'line_width', 'default':3, 'type':'float', 'control': 'textedit'},
            {'name':'show_score', 'default':"no", 'type':'bool', 'control': 'checkbox'},
            {'name':'font_size', 'default':13, 'type':'int', 'control': 'textedit'},
        ];
    }

    init()
    {
        super.init();
        this.style.pointerEvents = "none";
    }

    requestData(data_set)
    {
        this.addSource(data_set, this.parameters.source);
    }

    updateFrame()
    {
        super.updateFrame();
        this.canvasElement.style.background = "transparent";
    }

    normalizeBoxes(data)
    {
        if(!data)
            return [];

        if(Array.isArray(data) && data.length === 0)
            return [];

        if(Array.isArray(data) && data.length >= 4 && !Array.isArray(data[0]))
            return [data];

        if(Array.isArray(data) && data.length === 1 && Array.isArray(data[0]) && data[0].length >= 4 && !Array.isArray(data[0][0]))
            return [data[0]];

        if(Array.isArray(data))
            return data.filter(row => Array.isArray(row) && row.length >= 4);

        return [];
    }

    drawScore(box, x, y)
    {
        if(!this.parameters.show_score || box.length < 5)
            return;

        const score = Number.parseFloat(box[4]);
        if(!Number.isFinite(score))
            return;

        const label = score.toFixed(2);
        const paddingX = 4;
        const paddingY = 2;
        this.canvas.font = `${this.parameters.font_size}px sans-serif`;
        const metrics = this.canvas.measureText(label);
        const textWidth = Math.ceil(metrics.width);
        const textHeight = this.parameters.font_size + paddingY * 2;
        const labelY = Math.max(0, y - textHeight);

        this.canvas.fillStyle = "rgba(0, 0, 0, 0.62)";
        this.canvas.fillRect(x, labelY, textWidth + paddingX * 2, textHeight);
        this.canvas.fillStyle = this.parameters.color;
        this.canvas.textBaseline = "middle";
        this.canvas.fillText(label, x + paddingX, labelY + textHeight / 2);
    }

    update()
    {
        this.resetCanvasTransform();
        this.canvas.clearRect(0, 0, this.width, this.height);

        const boxes = this.normalizeBoxes(this.getSource('source'));
        if(boxes.length === 0)
            return;

        this.canvas.save();
        this.canvas.strokeStyle = this.parameters.color;
        this.canvas.lineWidth = Math.max(1, Number.parseFloat(this.parameters.line_width) || 1);
        this.canvas.lineJoin = "round";

        for(const box of boxes)
        {
            const bx = Number.parseFloat(box[0]);
            const by = Number.parseFloat(box[1]);
            const bw = Number.parseFloat(box[2]);
            const bh = Number.parseFloat(box[3]);
            if(!Number.isFinite(bx) || !Number.isFinite(by) || !Number.isFinite(bw) || !Number.isFinite(bh))
                continue;
            if(bw <= 0 || bh <= 0)
                continue;

            const x = (bx + 1) * 0.5 * this.width;
            const y = (by + 1) * 0.5 * this.height;
            const w = bw * 0.5 * this.width;
            const h = bh * 0.5 * this.height;
            this.canvas.strokeRect(x, y, w, h);
            this.drawScore(box, x, y);
        }

        this.canvas.restore();
    }
}

webui_widgets.add('webui-widget-faceboxes', WebUIWidgetFaceBoxes);
