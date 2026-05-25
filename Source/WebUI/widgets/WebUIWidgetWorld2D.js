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
            {'name':'tool_parameter', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'whisker_length_parameter', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'whisker_angle_parameter', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'world_width', 'default':300, 'type':'float', 'control': 'textedit'},
            {'name':'world_height', 'default':300, 'type':'float', 'control': 'textedit'},
            {'name':'whisker_length', 'default':35, 'type':'float', 'control': 'textedit'},
            {'name':'whisker_angle', 'default':0.55, 'type':'float', 'control': 'textedit'},
            {'name':'show_grid', 'default':true, 'type':'bool', 'control': 'checkbox'},
            {'name':'grid_step', 'default':10, 'type':'float', 'control': 'textedit'},
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
        this.selectedItem = null;
        this.dragState = null;
        this.snapPreview = null;
        this.suppressNextClick = false;
        this.canvasElement.tabIndex = 0;
        this.canvasElement.style.touchAction = "none";
        this.updateCursor();
        this.canvasElement.addEventListener('pointerdown', (event) => this.handlePointerDown(event));
        this.canvasElement.addEventListener('pointermove', (event) => this.handlePointerMove(event));
        this.canvasElement.addEventListener('pointerup', (event) => this.handlePointerUp(event));
        this.canvasElement.addEventListener('pointercancel', (event) => this.handlePointerUp(event));
        this.canvasElement.addEventListener('pointerleave', () => this.clearSnapPreview());
        this.canvasElement.addEventListener('click', (event) => this.handleCanvasClick(event));
        this.canvasElement.addEventListener('keydown', (event) => this.handleKeyDown(event));
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

    canvasToWorld(x, y)
    {
        const bounds = this.worldToCanvas(0, 0);
        const worldWidth = Math.max(1, Number(this.parameters.world_width));
        const worldHeight = Math.max(1, Number(this.parameters.world_height));
        return {
            x: Math.max(0, Math.min(worldWidth, (x - bounds.ox) / bounds.r)),
            y: Math.max(0, Math.min(worldHeight, (y - bounds.oy) / bounds.r))
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
        this.canvas.setLineDash([]);
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
        this.canvas.setLineDash([]);
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

        if(this.isSelected('creature', row))
        {
            this.canvas.beginPath();
            this.canvas.strokeStyle = "#f6c945";
            this.canvas.lineWidth = 3;
            this.canvas.arc(0, 0, radius + 5, 0, 2 * Math.PI);
            this.canvas.stroke();
        }

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

    currentTool()
    {
        return Math.trunc(this.sourceNumber('tool_parameter', 0));
    }

    updateCursor()
    {
        const tool = this.currentTool();
        const cross = "url(\"data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='17' height='17' viewBox='0 0 17 17'%3E%3Cpath d='M8 1v15M1 8h15' stroke='black' stroke-width='1.5' stroke-linecap='square'/%3E%3C/svg%3E\") 8 8, crosshair";
        const eraser = "url(\"data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='24' height='24' viewBox='0 0 24 24'%3E%3Cpath d='M4.5 15.3 L13.6 6.2 C14.4 5.4 15.7 5.4 16.5 6.2 L20 9.7 C20.8 10.5 20.8 11.8 20 12.6 L13.1 19.5 H7.3 L4.5 16.7 C4.1 16.3 4.1 15.7 4.5 15.3 Z' fill='white' stroke='black' stroke-width='2' stroke-linejoin='round'/%3E%3Cpath d='M10.2 9.6 L16.6 16' fill='none' stroke='black' stroke-width='2' stroke-linecap='round'/%3E%3Cpath d='M13.1 19.5 H21' fill='none' stroke='black' stroke-width='2' stroke-linecap='round'/%3E%3C/svg%3E\") 7 17, default";
        this.canvasElement.style.cursor = tool === 3 ? eraser : ((tool === 1 || tool === 2) ? cross : "");
    }

    isFiniteNumber(value)
    {
        return Number.isFinite(Number(value));
    }

    objectId(row) { return Number(row[0]); }
    objectType(row) { return Number(row[1]); }
    objectX(row) { return Number(row[2]); }
    objectY(row) { return Number(row[3]); }
    objectRadius(row) { return Number(row[4]); }
    creatureX(row) { return Number(row[0]); }
    creatureY(row) { return Number(row[1]); }
    creatureRadius(row) { return Number(row[2]); }
    wallId(row) { return Number(row[0]); }
    wallOpaque(row) { return Number(row[1]) > 0.5; }
    wallX1(row) { return Number(row[2]); }
    wallY1(row) { return Number(row[3]); }
    wallX2(row) { return Number(row[4]); }
    wallY2(row) { return Number(row[5]); }
    wallLineWidth(row) { return Number(row[10]) || 2; }

    wallCornerSnapRadius()
    {
        const bounds = this.worldToCanvas(0, 0);
        return 10 / Math.max(0.000001, bounds.r);
    }

    snapToWallCorner(world, altKey=false)
    {
        if(altKey)
            return world;

        let best = null;
        let bestDistanceSquared = Infinity;
        const radius = this.wallCornerSnapRadius();
        const maxDistanceSquared = radius * radius;
        for(const row of this.walls)
        {
            if(!Array.isArray(row) || row.length < 6)
                continue;

            const corners = [
                {x: this.wallX1(row), y: this.wallY1(row)},
                {x: this.wallX2(row), y: this.wallY2(row)}
            ];
            for(const corner of corners)
            {
                const dx = world.x - corner.x;
                const dy = world.y - corner.y;
                const distanceSquared = dx * dx + dy * dy;
                if(distanceSquared <= maxDistanceSquared && distanceSquared < bestDistanceSquared)
                {
                    best = corner;
                    bestDistanceSquared = distanceSquared;
                }
            }
        }

        return best ? {...best, snapped: true} : world;
    }

    snapToGrid(world)
    {
        const step = Math.max(0.000001, Number(this.parameters.grid_step) || 10);
        return {
            x: Math.max(0, Math.min(Number(this.parameters.world_width) || 300, Math.round(world.x / step) * step)),
            y: Math.max(0, Math.min(Number(this.parameters.world_height) || 300, Math.round(world.y / step) * step)),
            gridSnapped: true
        };
    }

    snapWallPoint(world, event)
    {
        if(event.altKey)
            return world;

        if(event.shiftKey)
            return this.snapToGrid(world);

        return this.snapToWallCorner(world, false);
    }

    updateSnapPreview(world, event)
    {
        if(this.currentTool() !== 2 || event.altKey || event.shiftKey)
        {
            this.snapPreview = null;
            return;
        }

        const snapped = this.snapToWallCorner(world, false);
        this.snapPreview = snapped.snapped ? snapped : null;
    }

    clearSnapPreview()
    {
        if(!this.snapPreview)
            return;

        this.snapPreview = null;
        this.resetCanvasTransform();
        this.drawScene();
    }

    isSelected(kind, row)
    {
        if(!this.selectedItem)
            return false;

        const id = kind === 'creature' ? 0 : (kind === 'wall' ? this.wallId(row) : this.objectId(row));
        const type = kind === 'object' ? this.objectType(row) : undefined;
        return this.selectedItem.kind === kind &&
            this.selectedItem.id === id &&
            (kind !== 'object' || this.selectedItem.type === type);
    }

    canvasPoint(event)
    {
        const rect = this.canvasElement.getBoundingClientRect();
        return {
            x: event.clientX - rect.left,
            y: event.clientY - rect.top
        };
    }

    distanceToCanvasSegment(px, py, ax, ay, bx, by)
    {
        const vx = bx - ax;
        const vy = by - ay;
        const wx = px - ax;
        const wy = py - ay;
        const lengthSquared = vx * vx + vy * vy;
        const t = lengthSquared > 0 ? Math.max(0, Math.min(1, (wx * vx + wy * vy) / lengthSquared)) : 0;
        const cx = ax + t * vx;
        const cy = ay + t * vy;
        const dx = px - cx;
        const dy = py - cy;
        return Math.sqrt(dx * dx + dy * dy);
    }

    hitTestObject(row, x, y)
    {
        if(!Array.isArray(row) || row.length < 5)
            return null;

        const p = this.worldToCanvas(this.objectX(row), this.objectY(row));
        const radius = Math.max(1, this.objectRadius(row) * p.r);
        const dx = x - p.x;
        const dy = y - p.y;
        const distance = Math.sqrt(dx * dx + dy * dy);
        const tolerance = 5;
        if(distance > radius + tolerance)
            return null;

        return {kind: 'object', type: this.objectType(row), id: this.objectId(row), score: distance - radius};
    }

    hitTestCreature(row, x, y)
    {
        if(!Array.isArray(row) || row.length < 3)
            return null;

        const p = this.worldToCanvas(this.creatureX(row), this.creatureY(row));
        const radius = Math.max(1, this.creatureRadius(row) * p.r);
        const dx = x - p.x;
        const dy = y - p.y;
        const distance = Math.sqrt(dx * dx + dy * dy);
        const tolerance = 5;
        if(distance > radius + tolerance)
            return null;

        return {kind: 'creature', id: 0, score: distance - radius};
    }

    hitTestWall(row, x, y)
    {
        if(!Array.isArray(row) || row.length < 6)
            return null;

        if(!this.isFiniteNumber(this.wallX1(row)) || !this.isFiniteNumber(this.wallY1(row)) ||
            !this.isFiniteNumber(this.wallX2(row)) || !this.isFiniteNumber(this.wallY2(row)))
            return null;

        const from = this.worldToCanvas(this.wallX1(row), this.wallY1(row));
        const to = this.worldToCanvas(this.wallX2(row), this.wallY2(row));
        const lineWidth = Math.max(1, this.wallLineWidth(row) * from.r);
        const distance = this.distanceToCanvasSegment(x, y, from.x, from.y, to.x, to.y);
        const tolerance = Math.max(6, lineWidth * 0.5 + 4);
        if(distance > tolerance)
            return null;

        return {kind: 'wall', id: this.wallId(row), score: distance - tolerance};
    }

    hitTest(x, y)
    {
        let best = null;
        const creatureHit = this.hitTestCreature(this.creature, x, y);
        if(creatureHit)
            best = creatureHit;

        for(let i = this.objects.length - 1; i >= 0; --i)
        {
            const hit = this.hitTestObject(this.objects[i], x, y);
            if(hit)
                hit.rowIndex = i;
            if(hit && (!best || hit.score < best.score))
                best = hit;
        }

        for(let i = this.walls.length - 1; i >= 0; --i)
        {
            const hit = this.hitTestWall(this.walls[i], x, y);
            if(hit)
                hit.rowIndex = i;
            if(hit && (!best || hit.score < best.score))
                best = hit;
        }

        return best;
    }

    handleCanvasClick(event)
    {
        if(this.suppressNextClick)
        {
            this.suppressNextClick = false;
            return;
        }

        if(this.currentTool() !== 0)
            return;

        const p = this.canvasPoint(event);
        this.canvasElement.focus({preventScroll: true});
        this.selectedItem = this.hitTest(p.x, p.y);
        this.sendSelectionCommand(this.selectedItem);
        this.resetCanvasTransform();
        this.drawScene();
    }

    commandModulePath()
    {
        const source = this.parameters.objects_source || this.parameters.walls_source || this.parameters.creature_source || "";
        const dot = source.lastIndexOf(".");
        return dot > 0 ? source.substring(0, dot) : "";
    }

    sendWorldCommand(dictionary)
    {
        const path = this.commandModulePath();
        if(path === "" || typeof controller === "undefined")
            return;

        controller.queueCommand("command", path, dictionary, true);
    }

    sendSelectionCommand(hit)
    {
        if(hit && hit.kind === 'object')
        {
            this.sendWorldCommand({
                command: "select_object",
                type: hit.type,
                id: hit.id
            });
        }
        else if(hit && hit.kind === 'wall')
        {
            this.sendWorldCommand({
                command: "select_wall",
                id: hit.id
            });
        }
        else
        {
            this.sendWorldCommand({
                command: "clear_selection"
            });
        }
    }

    isEditableKeyTarget(target)
    {
        if(!target)
            return false;

        const tagName = target.tagName ? target.tagName.toLowerCase() : "";
        return tagName === "input" ||
            tagName === "textarea" ||
            tagName === "select" ||
            target.isContentEditable;
    }

    handleKeyDown(event)
    {
        if((event.key !== "Delete" && event.key !== "Backspace") || this.isEditableKeyTarget(event.target))
            return;

        if(!this.deleteSelectedItem())
            return;

        event.preventDefault();
        event.stopPropagation();
    }

    deleteSelectedItem()
    {
        if(!this.selectedItem || (this.selectedItem.kind !== 'object' && this.selectedItem.kind !== 'wall'))
            return false;

        this.deleteItem(this.selectedItem);
        return true;
    }

    deleteItem(item)
    {
        if(item.kind === 'object')
        {
            this.sendWorldCommand({
                command: "delete_object",
                type: item.type,
                id: item.id
            });
            this.objects = this.objects.filter((row) => !(this.objectId(row) === item.id && this.objectType(row) === item.type));
        }
        else if(item.kind === 'wall')
        {
            this.sendWorldCommand({
                command: "delete_wall",
                id: item.id
            });
            this.walls = this.walls.filter((row) => this.wallId(row) !== item.id);
        }

        this.selectedItem = null;
        this.dragState = null;
        this.resetCanvasTransform();
        this.drawScene();
    }

    nextSceneId()
    {
        let maxId = 0;
        for(const row of this.objects)
            if(Array.isArray(row))
                maxId = Math.max(maxId, this.objectId(row) || 0);
        for(const row of this.walls)
            if(Array.isArray(row))
                maxId = Math.max(maxId, this.wallId(row) || 0);
        return maxId + 1;
    }

    isDrawingWall()
    {
        return this.dragState && this.dragState.item && this.dragState.item.kind === 'add_wall';
    }

    updateWallDraftEnd(world, event)
    {
        if(!this.isDrawingWall())
            return;

        const end = this.snapWallPoint(world, event);
        this.dragState.endWorldX = end.x;
        this.dragState.endWorldY = end.y;
        this.dragState.endSnapped = !!end.snapped;
        this.dragState.endGridSnapped = !!end.gridSnapped;
    }

    commitWallDraft()
    {
        if(!this.isDrawingWall())
            return null;

        const endX = this.dragState.endWorldX;
        const endY = this.dragState.endWorldY;
        const dx = this.dragState.endWorldX - this.dragState.startWorldX;
        const dy = this.dragState.endWorldY - this.dragState.startWorldY;
        const created = Math.sqrt(dx * dx + dy * dy) > 1;
        if(created)
        {
            this.sendWorldCommand({
                command: "add_wall",
                x1: this.dragState.startWorldX,
                y1: this.dragState.startWorldY,
                x2: this.dragState.endWorldX,
                y2: this.dragState.endWorldY
            });
        }

        this.dragState = null;
        this.snapPreview = null;
        return created ? {
            x: endX,
            y: endY,
            snapped: true
        } : null;
    }

    handlePointerDown(event)
    {
        const p = this.canvasPoint(event);
        this.canvasElement.focus({preventScroll: true});
        const tool = this.currentTool();
        const world = this.canvasToWorld(p.x, p.y);

        if(tool !== 2 && this.isDrawingWall())
        {
            this.dragState = null;
            this.snapPreview = null;
        }

        if(tool === 1)
        {
            const id = this.nextSceneId();
            this.selectedItem = null;
            this.dragState = null;
            this.snapPreview = null;
            this.sendSelectionCommand(null);
            this.sendWorldCommand({
                command: "add_object",
                x: world.x,
                y: world.y
            });
            this.selectedItem = {kind: 'object', type: 2, id};
            this.suppressNextClick = true;
            event.preventDefault();
            return;
        }

        if(tool === 2)
        {
            this.selectedItem = null;
            this.snapPreview = null;
            this.sendSelectionCommand(null);

            if(this.isDrawingWall())
            {
                this.updateWallDraftEnd(world, event);
                const end = this.commitWallDraft();
                if(end && !event.altKey && !event.shiftKey)
                    this.snapPreview = end;
                this.suppressNextClick = true;
                this.resetCanvasTransform();
                this.drawScene();
                event.preventDefault();
                return;
            }

            const start = this.snapWallPoint(world, event);
            this.dragState = {
                item: {kind: 'add_wall'},
                startCanvasX: p.x,
                startCanvasY: p.y,
                startWorldX: start.x,
                startWorldY: start.y,
                endWorldX: start.x,
                endWorldY: start.y,
                startSnapped: !!start.snapped,
                endSnapped: !!start.snapped,
                startGridSnapped: !!start.gridSnapped,
                endGridSnapped: !!start.gridSnapped,
                moved: false
            };

            this.suppressNextClick = true;
            this.resetCanvasTransform();
            this.drawScene();
            event.preventDefault();
            return;
        }

        const hit = this.hitTest(p.x, p.y);
        if(tool === 3)
        {
            this.dragState = null;
            this.snapPreview = null;
            if(hit && (hit.kind === 'object' || hit.kind === 'wall'))
                this.deleteItem(hit);
            else
                this.sendSelectionCommand(null);
            this.suppressNextClick = true;
            event.preventDefault();
            return;
        }

        this.selectedItem = hit;
        this.dragState = null;
        this.snapPreview = null;
        this.sendSelectionCommand(hit);

        if(hit)
        {
            const world = this.canvasToWorld(p.x, p.y);
            this.dragState = {
                item: {...hit},
                startCanvasX: p.x,
                startCanvasY: p.y,
                startWorldX: world.x,
                startWorldY: world.y,
                moved: false
            };

            if(hit.kind === 'object' && this.objects[hit.rowIndex])
            {
                const row = this.objects[hit.rowIndex];
                this.dragState.originalObjectX = this.objectX(row) || 0;
                this.dragState.originalObjectY = this.objectY(row) || 0;
            }
            else if(hit.kind === 'creature' && this.creature)
            {
                this.dragState.originalCreatureX = this.creatureX(this.creature) || 0;
                this.dragState.originalCreatureY = this.creatureY(this.creature) || 0;
            }
            else if(hit.kind === 'wall' && this.walls[hit.rowIndex])
            {
                const row = this.walls[hit.rowIndex];
                this.dragState.originalWall = [
                    this.wallX1(row) || 0,
                    this.wallY1(row) || 0,
                    this.wallX2(row) || 0,
                    this.wallY2(row) || 0
                ];
            }

            if(this.canvasElement.setPointerCapture)
                this.canvasElement.setPointerCapture(event.pointerId);
            event.preventDefault();
        }

        this.resetCanvasTransform();
        this.drawScene();
    }

    handlePointerMove(event)
    {
        if(!this.dragState)
        {
            if(this.currentTool() === 2)
            {
                const p = this.canvasPoint(event);
                const world = this.canvasToWorld(p.x, p.y);
                this.updateSnapPreview(world, event);
                this.resetCanvasTransform();
                this.drawScene();
                event.preventDefault();
            }
            return;
        }

        const p = this.canvasPoint(event);
        if(!this.dragState.moved)
        {
            const dx = p.x - this.dragState.startCanvasX;
            const dy = p.y - this.dragState.startCanvasY;
            this.dragState.moved = Math.sqrt(dx * dx + dy * dy) > 2;
        }

        const world = this.canvasToWorld(p.x, p.y);
        if(this.dragState.item.kind === 'add_wall')
        {
            this.updateWallDraftEnd(world, event);
        }
        else
        {
            this.previewDraggedItem(world);
        }

        if(this.dragState.moved && this.dragState.item.kind !== 'add_wall')
            this.sendDraggedItemCommand();
        this.resetCanvasTransform();
        this.drawScene();
        event.preventDefault();
    }

    handlePointerUp(event)
    {
        if(!this.dragState)
            return;

        const p = this.canvasPoint(event);
        const world = this.canvasToWorld(p.x, p.y);
        if(this.dragState.item.kind === 'add_wall')
        {
            this.updateWallDraftEnd(world, event);
            this.resetCanvasTransform();
            this.drawScene();
            event.preventDefault();
            return;
        }
        else
        {
            this.previewDraggedItem(world);
        }

        if(this.dragState.moved)
        {
            this.sendDraggedItemCommand();
            this.suppressNextClick = true;
        }

        if(this.canvasElement.releasePointerCapture)
            this.canvasElement.releasePointerCapture(event.pointerId);
        this.dragState = null;
        this.resetCanvasTransform();
        this.drawScene();
        event.preventDefault();
    }

    previewDraggedItem(world)
    {
        const drag = this.dragState;
        if(!drag)
            return;

        if(drag.item.kind === 'object' && this.objects[drag.item.rowIndex])
        {
            const row = this.objects[drag.item.rowIndex];
            row[2] = world.x;
            row[3] = world.y;
        }
        else if(drag.item.kind === 'creature' && Array.isArray(this.creature))
        {
            this.creature[0] = world.x;
            this.creature[1] = world.y;
        }
        else if(drag.item.kind === 'wall' && this.walls[drag.item.rowIndex] && drag.originalWall)
        {
            const dx = world.x - drag.startWorldX;
            const dy = world.y - drag.startWorldY;
            const row = this.walls[drag.item.rowIndex];
            row[2] = drag.originalWall[0] + dx;
            row[3] = drag.originalWall[1] + dy;
            row[4] = drag.originalWall[2] + dx;
            row[5] = drag.originalWall[3] + dy;
        }
    }

    sendDraggedItemCommand()
    {
        const drag = this.dragState;
        if(!drag)
            return;

        if(drag.item.kind === 'object' && this.objects[drag.item.rowIndex])
        {
            const row = this.objects[drag.item.rowIndex];
            this.sendWorldCommand({
                command: "move_object",
                type: this.objectType(row),
                id: this.objectId(row),
                x: this.objectX(row),
                y: this.objectY(row)
            });
        }
        else if(drag.item.kind === 'creature' && Array.isArray(this.creature))
        {
            this.sendWorldCommand({
                command: "move_creature",
                x: this.creatureX(this.creature),
                y: this.creatureY(this.creature)
            });
        }
        else if(drag.item.kind === 'wall' && this.walls[drag.item.rowIndex])
        {
            const row = this.walls[drag.item.rowIndex];
            this.sendWorldCommand({
                command: "move_wall",
                id: this.wallId(row),
                x1: this.wallX1(row),
                y1: this.wallY1(row),
                x2: this.wallX2(row),
                y2: this.wallY2(row)
            });
        }
    }

    drawObject(row)
    {
        const type = this.objectType(row);
        const p = this.worldToCanvas(this.objectX(row), this.objectY(row));
        const radius = Math.max(1, this.objectRadius(row) * p.r);
        const fallback = type === 2 ? "#38a84f" : "#d83d38";

        this.canvas.save();
        this.canvas.setLineDash([]);
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

        if(this.isSelected('object', row))
        {
            this.canvas.beginPath();
            this.canvas.strokeStyle = "#f6c945";
            this.canvas.lineWidth = 3;
            this.canvas.arc(p.x, p.y, radius + 5, 0, 2 * Math.PI);
            this.canvas.stroke();
        }

        this.canvas.restore();
    }

    drawWall(row)
    {
        if(!Array.isArray(row) || row.length < 6)
            return;

        if(!this.isFiniteNumber(this.wallX1(row)) || !this.isFiniteNumber(this.wallY1(row)) ||
            !this.isFiniteNumber(this.wallX2(row)) || !this.isFiniteNumber(this.wallY2(row)))
            return;

        const from = this.worldToCanvas(this.wallX1(row), this.wallY1(row));
        const to = this.worldToCanvas(this.wallX2(row), this.wallY2(row));
        const lineWidth = Math.max(1, this.wallLineWidth(row) * from.r);
        const opaque = this.wallOpaque(row);

        this.canvas.save();
        if(this.isSelected('wall', row))
        {
            this.canvas.beginPath();
            this.canvas.strokeStyle = "#f6c945";
            this.canvas.lineWidth = Math.max(3, lineWidth + 5);
            this.canvas.lineCap = "round";
            this.canvas.moveTo(from.x, from.y);
            this.canvas.lineTo(to.x, to.y);
            this.canvas.stroke();
        }

        this.canvas.beginPath();
        this.canvas.strokeStyle = this.rowColor(row, 6, this.parameters.wall_color || "#31343a");
        this.canvas.lineWidth = lineWidth;
        this.canvas.lineCap = "round";
        if(!opaque)
            this.canvas.setLineDash([0.01, Math.max(3, lineWidth * 2.5)]);
        else
            this.canvas.setLineDash([]);
        this.canvas.moveTo(from.x, from.y);
        this.canvas.lineTo(to.x, to.y);
        this.canvas.stroke();
        this.canvas.restore();
    }

    drawWallDraft()
    {
        if(!this.dragState || this.dragState.item.kind !== 'add_wall')
            return;

        if(!this.isFiniteNumber(this.dragState.startWorldX) || !this.isFiniteNumber(this.dragState.startWorldY) ||
            !this.isFiniteNumber(this.dragState.endWorldX) || !this.isFiniteNumber(this.dragState.endWorldY))
            return;

        const from = this.worldToCanvas(this.dragState.startWorldX, this.dragState.startWorldY);
        const to = this.worldToCanvas(this.dragState.endWorldX, this.dragState.endWorldY);
        this.canvas.save();
        this.canvas.beginPath();
        this.canvas.strokeStyle = "#f6c945";
        this.canvas.lineWidth = 3;
        this.canvas.lineCap = "round";
        this.canvas.setLineDash([6, 4]);
        this.canvas.moveTo(from.x, from.y);
        this.canvas.lineTo(to.x, to.y);
        this.canvas.stroke();

        for(const point of [
            {x: from.x, y: from.y, snapped: this.dragState.startSnapped || this.dragState.startGridSnapped},
            {x: to.x, y: to.y, snapped: this.dragState.endSnapped || this.dragState.endGridSnapped}
        ])
        {
            if(!point.snapped)
                continue;

            this.canvas.beginPath();
            this.canvas.setLineDash([]);
            this.canvas.fillStyle = "#f6c945";
            this.canvas.strokeStyle = "#14233a";
            this.canvas.lineWidth = 1.5;
            this.canvas.arc(point.x, point.y, 4, 0, 2 * Math.PI);
            this.canvas.fill();
            this.canvas.stroke();
        }
        this.canvas.restore();
    }

    drawSnapPreview()
    {
        if(!this.snapPreview)
            return;

        const p = this.worldToCanvas(this.snapPreview.x, this.snapPreview.y);
        this.canvas.save();
        this.canvas.setLineDash([]);
        this.canvas.beginPath();
        this.canvas.fillStyle = "#f6c945";
        this.canvas.strokeStyle = "#14233a";
        this.canvas.lineWidth = 1.5;
        this.canvas.arc(p.x, p.y, 4, 0, 2 * Math.PI);
        this.canvas.fill();
        this.canvas.stroke();
        this.canvas.restore();
    }

    drawScene()
    {
        this.canvas.clearRect(0, 0, this.width, this.height);
        this.canvas.setLineDash([]);
        const bounds = this.worldToCanvas(0, 0);

        this.canvas.save();
        this.canvas.setLineDash([]);
        this.canvas.fillStyle = this.parameters.background || "#f7f5ef";
        this.canvas.fillRect(bounds.ox, bounds.oy, bounds.width, bounds.height);
        this.drawGrid(bounds);
        this.canvas.strokeStyle = this.parameters.wall_color || "#31343a";
        this.canvas.lineWidth = 2;
        this.canvas.strokeRect(bounds.ox, bounds.oy, bounds.width, bounds.height);
        this.canvas.restore();

        for(const row of this.walls)
        {
            try
            {
                if(Array.isArray(row) && row.length >= 6)
                    this.drawWall(row);
            }
            catch(err)
            {
                console.warn("World2D skipped a wall row:", err);
            }
        }

        try
        {
            this.drawWallDraft();
            this.drawSnapPreview();
        }
        catch(err)
        {
            console.warn("World2D skipped wall edit overlay:", err);
        }

        for(const row of this.objects)
        {
            try
            {
                if(Array.isArray(row) && row.length >= 5)
                    this.drawObject(row);
            }
            catch(err)
            {
                console.warn("World2D skipped an object row:", err);
            }
        }

        try
        {
            if(Array.isArray(this.creature) && this.creature.length >= 4)
                this.drawCreature(this.creature);
        }
        catch(err)
        {
            console.warn("World2D skipped creature draw:", err);
        }
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

        this.updateCursor();
        this.resetCanvasTransform();
        this.drawScene();
    }
}

webui_widgets.add('webui-widget-world2d', WebUIWidgetWorld2D);
