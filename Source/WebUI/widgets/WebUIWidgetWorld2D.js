class WebUIWidgetWorld2D extends WebUIWidgetCanvas
{
    static template()
    {
        return [
            {'name': "WORLD2D", 'control':'header'},
            {'name':'title', 'default':"World2D", 'type':'string', 'control': 'textedit'},
            {'name':'creature_source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'objects_source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'walls_source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'whiskers_source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'whisker_length_parameter', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'whisker_angle_parameter', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'world_width', 'default':300, 'type':'float', 'control': 'textedit'},
            {'name':'world_height', 'default':300, 'type':'float', 'control': 'textedit'},
            {'name':'whisker_length', 'default':35, 'type':'float', 'control': 'textedit'},
            {'name':'whisker_angle', 'default':0.55, 'type':'float', 'control': 'textedit'},
            {'name':'show_grid', 'default':true, 'type':'bool', 'control': 'checkbox'},
            {'name':'grid_step', 'default':50, 'type':'float', 'control': 'textedit'},
            {'name':'background', 'default':"#f7f5ef", 'type':'string', 'control': 'textedit'},
            {'name':'wall_color', 'default':"#31343a", 'type':'string', 'control': 'textedit'}
        ];
    }

    init()
    {
        super.init();
        this.creature = [];
        this.objects = [];
        this.walls = [];
        this.whiskers = [0, 0];
    }

    worldToCanvas(x, y)
    {
        const worldWidth = Math.max(1, Number(this.parameters.world_width));
        const worldHeight = Math.max(1, Number(this.parameters.world_height));
        const margin = 14;
        const scale = Math.min((this.width - 2 * margin) / worldWidth, (this.height - 2 * margin) / worldHeight);
        const ox = (this.width - worldWidth * scale) / 2;
        const oy = (this.height - worldHeight * scale) / 2;
        return {
            x: ox + x * scale,
            y: oy + y * scale,
            r: scale,
            ox,
            oy,
            width: worldWidth * scale,
            height: worldHeight * scale
        };
    }

    drawGrid(bounds)
    {
        if(!this.parameters.show_grid)
            return;

        const step = Math.max(1, Number(this.parameters.grid_step));
        const worldWidth = Math.max(1, Number(this.parameters.world_width));
        const worldHeight = Math.max(1, Number(this.parameters.world_height));

        this.canvas.save();
        this.canvas.strokeStyle = "rgba(49, 52, 58, 0.12)";
        this.canvas.lineWidth = 1;

        for(let x = step; x < worldWidth; x += step)
        {
            const p = this.worldToCanvas(x, 0);
            this.canvas.beginPath();
            this.canvas.moveTo(p.x, bounds.oy);
            this.canvas.lineTo(p.x, bounds.oy + bounds.height);
            this.canvas.stroke();
        }

        for(let y = step; y < worldHeight; y += step)
        {
            const p = this.worldToCanvas(0, y);
            this.canvas.beginPath();
            this.canvas.moveTo(bounds.ox, p.y);
            this.canvas.lineTo(bounds.ox + bounds.width, p.y);
            this.canvas.stroke();
        }

        this.canvas.restore();
    }

    drawCreature(row)
    {
        const p = this.worldToCanvas(Number(row[0]), Number(row[1]));
        const radius = Math.max(1, Number(row[2]) * p.r);
        const heading = Number(row[3]) || 0;
        const color = this.rowColor(row, 4, "#215fc7");

        this.canvas.save();
        this.canvas.translate(p.x, p.y);
        this.canvas.rotate(heading);

        this.canvas.beginPath();
        this.canvas.fillStyle = color;
        this.canvas.strokeStyle = "#14233a";
        this.canvas.lineWidth = 1.5;
        this.canvas.arc(0, 0, radius, 0, 2 * Math.PI);
        this.canvas.fill();
        this.canvas.stroke();

        this.canvas.beginPath();
        this.canvas.strokeStyle = "#f8fbff";
        this.canvas.lineWidth = Math.max(2, radius * 0.22);
        this.canvas.moveTo(0, 0);
        this.canvas.lineTo(radius * 1.35, 0);
        this.canvas.stroke();

        const whiskerLength = this.sourceNumber('whisker_length_parameter', this.parameters.whisker_length);
        const whiskerAngle = this.sourceNumber('whisker_angle_parameter', this.parameters.whisker_angle);
        this.drawWhisker(-whiskerAngle, whiskerLength, Number(this.whiskers[0]) || 0, p.r);
        this.drawWhisker(whiskerAngle, whiskerLength, Number(this.whiskers[1]) || 0, p.r);

        this.canvas.restore();
    }

    drawWhisker(angle, length, value, scale)
    {
        const signal = Math.max(0, Math.min(1, value));
        const scaledLength = Math.max(0, Number(length)) * scale;
        const x = Math.cos(angle) * scaledLength;
        const y = Math.sin(angle) * scaledLength;

        this.canvas.beginPath();
        this.canvas.strokeStyle = signal > 0 ? `rgba(216, 61, 56, ${0.35 + 0.65 * signal})` : "rgba(20,35,58,0.72)";
        this.canvas.lineWidth = 1.5 + 2.5 * signal;
        this.canvas.moveTo(0, 0);
        this.canvas.lineTo(x, y);
        this.canvas.stroke();
    }

    rowColor(row, offset, fallback)
    {
        if(row.length > offset + 2)
        {
            const r = Math.round(Math.max(0, Math.min(1, Number(row[offset]))) * 255);
            const g = Math.round(Math.max(0, Math.min(1, Number(row[offset + 1]))) * 255);
            const b = Math.round(Math.max(0, Math.min(1, Number(row[offset + 2]))) * 255);
            return `rgb(${r}, ${g}, ${b})`;
        }
        return fallback;
    }

    sourceNumber(sourceName, fallback)
    {
        const value = this.getSource(sourceName, undefined);
        if(Array.isArray(value))
        {
            const first = Array.isArray(value[0]) ? value[0][0] : value[0];
            const number = Number(first);
            return Number.isFinite(number) ? number : Number(fallback);
        }

        const number = Number(value);
        return Number.isFinite(number) ? number : Number(fallback);
    }

    drawObject(row)
    {
        const type = Number(row[0]);
        const p = this.worldToCanvas(Number(row[2]), Number(row[3]));
        const radius = Math.max(1, Number(row[4]) * p.r);
        const fallback = type === 2 ? "#38a84f" : "#d83d38";

        this.canvas.save();
        this.canvas.beginPath();
        this.canvas.fillStyle = this.rowColor(row, 6, fallback);
        this.canvas.strokeStyle = type === 2 ? "#276b35" : "#7d2422";
        this.canvas.lineWidth = 1.5;
        this.canvas.arc(p.x, p.y, radius, 0, 2 * Math.PI);
        this.canvas.fill();
        this.canvas.stroke();

        if(type === 3)
        {
            this.canvas.beginPath();
            this.canvas.strokeStyle = "rgba(255,255,255,0.7)";
            this.canvas.moveTo(p.x - radius * 0.45, p.y - radius * 0.45);
            this.canvas.lineTo(p.x + radius * 0.45, p.y + radius * 0.45);
            this.canvas.moveTo(p.x + radius * 0.45, p.y - radius * 0.45);
            this.canvas.lineTo(p.x - radius * 0.45, p.y + radius * 0.45);
            this.canvas.stroke();
        }

        this.canvas.restore();
    }

    drawWall(row)
    {
        const from = this.worldToCanvas(Number(row[1]), Number(row[2]));
        const to = this.worldToCanvas(Number(row[3]), Number(row[4]));
        const lineWidth = Math.max(1, (Number(row[9]) || 2) * from.r);

        this.canvas.save();
        this.canvas.beginPath();
        this.canvas.strokeStyle = this.rowColor(row, 5, this.parameters.wall_color || "#31343a");
        this.canvas.lineWidth = lineWidth;
        this.canvas.lineCap = "round";
        this.canvas.moveTo(from.x, from.y);
        this.canvas.lineTo(to.x, to.y);
        this.canvas.stroke();
        this.canvas.restore();
    }

    drawScene()
    {
        this.canvas.clearRect(0, 0, this.width, this.height);
        const bounds = this.worldToCanvas(0, 0);

        this.canvas.save();
        this.canvas.fillStyle = this.parameters.background || "#f7f5ef";
        this.canvas.fillRect(bounds.ox, bounds.oy, bounds.width, bounds.height);
        this.drawGrid(bounds);
        this.canvas.strokeStyle = this.parameters.wall_color || "#31343a";
        this.canvas.lineWidth = 2;
        this.canvas.strokeRect(bounds.ox, bounds.oy, bounds.width, bounds.height);
        this.canvas.restore();

        for(const row of this.walls)
        {
            if(Array.isArray(row) && row.length >= 5)
                this.drawWall(row);
        }

        for(const row of this.objects)
        {
            if(Array.isArray(row) && row.length >= 5)
                this.drawObject(row);
        }

        if(Array.isArray(this.creature) && this.creature.length >= 4)
            this.drawCreature(this.creature);
    }

    matrixRows(value)
    {
        if(this.getMatrixRank(value) == 1)
            return [value];
        return Array.isArray(value) ? value : [];
    }

    update(d)
    {
        if(!d)
            return;

        const creature = this.getSource('creature_source', []);
        this.creature = this.getMatrixRank(creature) == 1 ? creature : [];
        this.objects = this.matrixRows(this.getSource('objects_source', []));
        this.walls = this.matrixRows(this.getSource('walls_source', []));
        const whiskers = this.getSource('whiskers_source', []);
        this.whiskers = this.getMatrixRank(whiskers) == 1 ? whiskers : [0, 0];

        this.resetCanvasTransform();
        this.drawScene();
    }
}

if(!webui_widgets.constructors['webui-widget-world2d'])
    webui_widgets.add('webui-widget-world2d', WebUIWidgetWorld2D);
